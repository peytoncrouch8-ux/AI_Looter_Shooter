#include "Player/PlayerTraversal.h"

// FPlayerTraversal's planning: the body's and the eye's curves for a mantle, a vault or an unstick glide, checked against
// the obstacle and retimed until clear.

bool FPlayerTraversal::Plan(const FTraversalSetup& Setup)
{
	bActive = false;
	Kind = ETraversalKind::None;
	Time = 0.0;
	Origin = FVector(Setup.Origin.X, Setup.Origin.Y, 0.0);
	Direction = FVector(Setup.Direction.X, Setup.Direction.Y, 0.0).GetSafeNormal();
	if (Direction.IsNearlyZero() || Setup.Duration <= 0.f || Setup.Kind == ETraversalKind::None)
	{
		return false;
	}
	Right = FVector::CrossProduct(FVector::UpVector, Direction);
	HalfHeight = Setup.HalfHeight;

	if (Setup.Kind == ETraversalKind::Unstick)
	{
		BuildGlide(Setup, Setup.Duration);
		return Commit(Setup.Kind, Setup.Duration);
	}

	// Retimed until clear: a mantle's body waits longer at the face before coming over the edge and rises sooner; a
	// vault's hop goes higher; the eye rises sooner (a mantle) or higher (a vault); past those, the whole move takes
	// longer. A mantle always comes clear by its last timing: the rise is over before the body comes forward.
	double Planned = Setup.Duration;
	double Hold = 0.45;
	double Rise = 0.75;
	double EyeShare = 1.0;
	double Lift = 0.0;
	double EyeLift = 0.0;
	for (int32 Try = 0; Try < 14; ++Try)
	{
		if (Setup.Kind == ETraversalKind::Vault)
		{
			BuildVault(Setup, Planned, Lift, EyeLift);
		}
		else
		{
			BuildMantle(Setup, Planned, Hold, Rise, EyeShare);
		}
		float Corner = 0.f;
		float EyeOver = 0.f;
		float EyeLow = 0.f;
		float EyeHigh = 0.f;
		float EyeAcceleration = 0.f;
		Measure(Setup, Planned, Corner, EyeOver, EyeLow, EyeHigh, EyeAcceleration);
		const bool bClear = Corner <= 1.f && EyeOver <= 1.f && EyeLow <= 1.f && EyeHigh <= 1.f;
		const bool bEyeHard = EyeAcceleration > MaxEyeAcceleration;
		if (bClear && (!bEyeHard || Planned >= MaxDuration))
		{
			return Commit(Setup.Kind, Planned);
		}
		bool bRetimed = false;
		if (Corner > 1.f)
		{
			if (Setup.Kind == ETraversalKind::Vault && Lift < Setup.Tuck + 2.0)
			{
				Lift = FMath::Min(Lift + Corner + 2.0, Setup.Tuck + 2.0);
				bRetimed = true;
			}
			else if (Setup.Kind == ETraversalKind::Mantle && Hold < 0.62)
			{
				Hold += 0.06;
				Rise = FMath::Max(Rise - 0.05, 0.55);
				bRetimed = true;
			}
		}
		if (EyeOver > 1.f || EyeLow > 1.f)
		{
			if (Setup.Kind == ETraversalKind::Vault)
			{
				EyeLift += FMath::Max(EyeOver, EyeLow) + 2.0;
				bRetimed = true;
			}
			else if (EyeShare > Rise + 0.01)
			{
				EyeShare = FMath::Max(EyeShare - 0.1, Rise);
				bRetimed = true;
			}
		}
		// Past those (or the eye turning too hard), the whole move takes longer.
		if (!bRetimed || (bClear && bEyeHard))
		{
			if (Planned >= MaxDuration)
			{
				break;
			}
			Planned = FMath::Min(Planned * 1.15, MaxDuration);
		}
	}
	return false;
}

double FPlayerTraversal::RequiredFeet(const FTraversalSetup& Setup, double S)
{
	const double Outside = FMath::Max3(Setup.NearFace - S, S - Setup.FarFace, 0.0);
	if (Outside >= Setup.Radius)
	{
		return -UE_BIG_NUMBER;
	}
	return Setup.TopZ - Setup.Tuck - Setup.Radius + FMath::Sqrt(FMath::Square(static_cast<double>(Setup.Radius)) - FMath::Square(Outside));
}

void FPlayerTraversal::Measure(const FTraversalSetup& Setup, double Length, float& OutCorner, float& OutEyeOver, float& OutEyeLow, float& OutEyeHigh,
	float& OutEyeAcceleration) const
{
	OutCorner = 0.f;
	OutEyeOver = 0.f;
	OutEyeLow = 0.f;
	OutEyeHigh = 0.f;
	OutEyeAcceleration = 0.f;
	constexpr int32 Samples = 64;
	for (int32 Index = 0; Index <= Samples; ++Index)
	{
		const double At = Length * Index / Samples;
		const double S = Along.Value(At);
		const double Z = Feet.Value(At);
		const double E = Eye.Value(At);
		OutCorner = FMath::Max(OutCorner, static_cast<float>(RequiredFeet(Setup, S) - Z));
		const double Camera = S + Setup.EyeForward;
		if (Camera > Setup.NearFace - LooterTraversal::EyeClearance && Camera < Setup.FarFace + LooterTraversal::EyeClearance)
		{
			OutEyeOver = FMath::Max(OutEyeOver, static_cast<float>(Setup.TopZ + LooterTraversal::EyeClearance - E));
		}
		const double Above = E - Z;
		OutEyeLow = FMath::Max(OutEyeLow, static_cast<float>(MinEyeOverFeet - Above));
		OutEyeHigh = FMath::Max(OutEyeHigh, static_cast<float>(Above - (2.0 * Setup.HalfHeight - 6.0)));
		OutEyeAcceleration = FMath::Max(OutEyeAcceleration, static_cast<float>(FMath::Abs(Eye.Acceleration(At))));
	}
}

bool FPlayerTraversal::Commit(ETraversalKind InKind, double InDuration)
{
	Kind = InKind;
	Duration = InDuration;
	Time = 0.0;
	bActive = true;
	return true;
}

void FPlayerTraversal::BuildSide(const FTraversalSetup& Setup, double Length)
{
	// Over the time a natural stop at EndSide takes, between a tenth and a fifth of a second.
	const double Drift = Setup.Velocity | Right;
	const double Settle = FMath::Abs(Drift) > 1.0 ? FMath::Clamp(2.0 * FMath::Abs(Setup.EndSide) / FMath::Abs(Drift), 0.1, 0.2) : 0.2;
	Side.Begin(0.0, Drift, 0.0);
	Side.Add(FMath::Min(Settle, Length), Setup.EndSide);
	Side.Add(Length, Setup.EndSide);
}

void FPlayerTraversal::BuildEye(const FTraversalSetup& Setup, double Until, double Length)
{
	Eye.Begin(Setup.EyeZ, Setup.EyeSpeed, Setup.EyeAcceleration);
	const double EndEye = Setup.EndFeetZ + Setup.EyeEndHeight;
	Eye.Add(Until, EndEye);
	Eye.Add(Length, EndEye);
}

void FPlayerTraversal::BuildMantle(const FTraversalSetup& Setup, double Length, double Hold, double Rise, double EyeShare)
{
	// The body closes on the face while it rises (slowing to a natural stop there if it ran in, or stepping up to it from a
	// stand), waits there until its bottom is clear of the edge, then comes forward onto the top at ExitSpeed. Hold is the
	// share of the move before it comes forward, Rise the share the rise takes, EyeShare the eye's.
	const double Speed = Setup.Velocity | Direction;
	const double Gap = FMath::Max(Setup.NearFace - Setup.Radius - 1.0, 0.0);
	const double HoldUntil = Hold * Length;
	double Close = HoldUntil;
	double Approach = Gap;
	if (Speed > 1.0)
	{
		Close = FMath::Clamp(2.0 * Gap / Speed, 0.05, HoldUntil);
		Approach = FMath::Min(Gap, Speed * Close * 0.5);
	}
	Along.Begin(0.0, Speed, 0.0);
	Along.Add(Close, Approach);
	Along.Add(HoldUntil, Approach);
	Along.Add(Length, Setup.EndAlong, Setup.ExitSpeed);
	BuildSide(Setup, Length);

	// Already moving up (a jump caught on its way up): the rise turns over naturally at the top rather than overshooting.
	const double FeetStart = Setup.Origin.Z - Setup.HalfHeight;
	const double Climb = Setup.EndFeetZ - FeetStart;
	double RiseUntil = Rise * Length;
	if (Setup.Velocity.Z > 1.0 && Climb > 0.0)
	{
		RiseUntil = FMath::Clamp(2.0 * Climb / Setup.Velocity.Z, 0.1, RiseUntil);
	}
	Feet.Begin(FeetStart, Setup.Velocity.Z, Setup.GravityZ);
	Feet.Add(RiseUntil, Setup.EndFeetZ);
	Feet.Add(Length, Setup.EndFeetZ);
	BuildEye(Setup, FMath::Max(EyeShare * Length, RiseUntil), Length);
}

void FPlayerTraversal::BuildVault(const FTraversalSetup& Setup, double Length, double Lift, double EyeLift)
{
	// One sweep along the run from its speed to ExitSpeed; the body hops over the obstacle's middle with its legs tucked
	// (Lift raises the hop), the eye rides a gentler hop over its top (EyeLift raises it), and both come down at rest on
	// the floor beyond.
	Along.Begin(0.0, Setup.Velocity | Direction, 0.0);
	Along.Add(Length, Setup.EndAlong, Setup.ExitSpeed);
	BuildSide(Setup, Length);

	// The hop's top comes as the body is over the obstacle's middle (found on the run's own curve), kept off the ends.
	const double Middle = 0.5 * (Setup.NearFace + Setup.FarFace);
	double Low = 0.0;
	double High = Length;
	for (int32 Step = 0; Step < 24; ++Step)
	{
		const double Mid = 0.5 * (Low + High);
		if (Along.Value(Mid) < Middle)
		{
			Low = Mid;
		}
		else
		{
			High = Mid;
		}
	}
	const double Apex = FMath::Clamp(0.5 * (Low + High), 0.3 * Length, 0.65 * Length);
	const double FeetStart = Setup.Origin.Z - Setup.HalfHeight;
	const double Hop = Setup.TopZ - Setup.Tuck + 3.0 + Lift;
	Feet.Begin(FeetStart, Setup.Velocity.Z, Setup.GravityZ);
	if (FeetStart < Hop || Setup.Velocity.Z > 0.0)
	{
		// Up over it (a rise still under way when caught in the air turns over naturally rather than overshooting). Already
		// over it and coming down, it goes straight on down to the floor beyond.
		Feet.Add(Apex, FMath::Max(Hop, FeetStart + FMath::Max(Setup.Velocity.Z, 0.0) * Apex * 0.5), 0.0, -HopTurn);
	}
	Feet.Add(Length, Setup.EndFeetZ);

	const double EndEye = Setup.EndFeetZ + Setup.EyeEndHeight;
	const double EyeApex = FMath::Max(FMath::Max(static_cast<double>(Setup.EyeZ), EndEye) + LooterTraversal::VaultEyeHop,
		Setup.TopZ + LooterTraversal::EyeClearance + 3.0) + EyeLift;
	Eye.Begin(Setup.EyeZ, Setup.EyeSpeed, Setup.EyeAcceleration);
	Eye.Add(Apex, EyeApex, 0.0, -EyeHopTurn);
	Eye.Add(Length, EndEye);
}

void FPlayerTraversal::BuildGlide(const FTraversalSetup& Setup, double Length)
{
	// Unstuck: a short straight glide to the free spot, from a stand still, the eye going along.
	Along.Begin(0.0, 0.0, 0.0);
	Along.Add(Length, Setup.EndAlong);
	Side.Begin(0.0, 0.0, 0.0);
	Feet.Begin(Setup.Origin.Z - Setup.HalfHeight, 0.0, 0.0);
	Feet.Add(Length, Setup.EndFeetZ);
	BuildEye(Setup, Length, Length);
}
