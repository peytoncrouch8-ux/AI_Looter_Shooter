// AFaunaFlock's flight: taking off, wheeling, coming back to a perch with a flare, hops between perches, leaving over
// the hills, an aerial flock's endless courses, and the wings' bouts of beating and gliding.

#include "World/FaunaFlock.h"
#include "World/FaunaRules.h"

namespace
{
	/** A returning bird's last point before its perch: this far up and back from it (cm, at the bird's own scale). */
	constexpr float ApproachHeight = 110.f;
	constexpr float ApproachBack = 160.f;

	/** The landing's flare: how long it takes (s) and how high the wings go (degrees). */
	constexpr float LandingSeconds = 0.75f;
	constexpr float FlareDegrees = 55.f;

	/** A hop between two perches of the flock: its speed (cm/s) and the shortest and longest it takes (s). */
	constexpr float HopSpeed = 420.f;
	constexpr float HopMinSeconds = 0.7f;
	constexpr float HopMaxSeconds = 2.2f;

	/** Where a leaving flock goes: this far out and up from its perches (cm). */
	constexpr float LeaveDistance = 7000.f;
	constexpr float LeaveHeight = 2500.f;

	/** A wheeling bird's slow rise and fall (cm). */
	constexpr float CircleBob = 120.f;

	/** The steepest a flier pitches up or down (degrees), and its widest bank. */
	constexpr float MaxClimbPitch = 35.f;
	constexpr float MaxDivePitch = 30.f;
	constexpr float MaxBank = 50.f;

	/** Bouts of beating and gliding (s): a beating bout, and a glide (longer for a flock that mostly glides). */
	constexpr float BeatBoutMin = 0.6f;
	constexpr float BeatBoutMax = 1.8f;
	constexpr float GlideBoutMin = 0.5f;
	constexpr float GlideBoutMax = 1.5f;

	/** A perch's facing for a bird settling on it: across a rail or a ridge either way, anything on a post, a limb, the ground. */
	float FacingOn(const FFaunaPerch& Perch, FRandomStream& Random)
	{
		switch (Perch.Kind)
		{
		case EFaunaPerchKind::Rail:
		case EFaunaPerchKind::Roof:
			return Perch.Yaw + (Random.FRand() < 0.5f ? 0.f : 180.f);
		default:
			return Random.FRandRange(-180.f, 180.f);
		}
	}
}

void AFaunaFlock::UpdateBird(FFaunaBird& Bird, int32 Index, float DeltaSeconds)
{
	switch (Bird.State)
	{
	case EFaunaBirdState::Perched:
		UpdatePerched(Bird, DeltaSeconds);
		break;
	case EFaunaBirdState::Startled:
		// Alert for its moment: the head up and still, then gone.
		Bird.HeadPitchGoal = 12.f;
		Bird.HeadYawGoal = 0.f;
		UpdatePerched(Bird, DeltaSeconds);
		Bird.Delay -= DeltaSeconds;
		if (Bird.Delay <= 0.f)
		{
			TakeOff(Bird, Index);
		}
		break;
	case EFaunaBirdState::TakingOff:
		// Frantic beats off the perch.
		Bird.FlapGoal = 1.f;
		Bird.FlapBout = FMath::Max(Bird.FlapBout, 0.2f);
		UpdateWings(Bird, DeltaSeconds, 1.35f);
		if (FollowFlight(Bird, DeltaSeconds))
		{
			Bird.State = EFaunaBirdState::Circling;
		}
		break;
	case EFaunaBirdState::Circling:
		UpdateWings(Bird, DeltaSeconds);
		FlyCircle(Bird, DeltaSeconds);
		break;
	case EFaunaBirdState::Returning:
		UpdateWings(Bird, DeltaSeconds);
		if (FollowFlight(Bird, DeltaSeconds) && Perches.IsValidIndex(Bird.Perch))
		{
			// The last stretch: braking onto the perch with the wings thrown up, turning to its facing.
			FVector Along = Perches[Bird.Perch].Location - Bird.Position;
			Along.Z = 0.0;
			Bird.P0 = Bird.Position;
			Bird.V0 = Bird.Velocity;
			Bird.P1 = Perches[Bird.Perch].Location + FVector(0.0, 0.0, BodyLift * Bird.Scale);
			Bird.V1 = Along.GetSafeNormal() * 30.f;
			Bird.FlightTime = 0.f;
			Bird.FlightDuration = LandingSeconds;
			Bird.State = EFaunaBirdState::Landing;
		}
		break;
	case EFaunaBirdState::Landing:
	case EFaunaBirdState::Hopping:
	{
		const float Alpha = Bird.FlightTime / FMath::Max(Bird.FlightDuration, 0.01f);
		const bool bFlaring = Bird.State == EFaunaBirdState::Landing || Alpha > 0.6f;
		Bird.FlapGoal = bFlaring ? 0.45f : 1.f;
		Bird.FlapBout = FMath::Max(Bird.FlapBout, 0.2f);
		Bird.Flare = FMath::FInterpTo(Bird.Flare, bFlaring ? FlareDegrees : 0.f, DeltaSeconds, 8.f);
		UpdateWings(Bird, DeltaSeconds, 1.15f);
		const bool bArrived = FollowFlight(Bird, DeltaSeconds);
		if (bFlaring)
		{
			// Turning to face the way it will sit as it comes down.
			Bird.Yaw = FaunaRules::StepAngle(Bird.Yaw, Bird.LandYaw, 360.f * DeltaSeconds);
			Bird.Pitch = FMath::FInterpTo(Bird.Pitch, 25.f, DeltaSeconds, 10.f);
		}
		if (bArrived && Perches.IsValidIndex(Bird.Perch))
		{
			Settle(Bird, Bird.Perch);
		}
		break;
	}
	case EFaunaBirdState::Leaving:
		UpdateWings(Bird, DeltaSeconds);
		if (FollowFlight(Bird, DeltaSeconds))
		{
			Bird.State = EFaunaBirdState::Away;
			Bird.Velocity = Bird.V1;
		}
		break;
	case EFaunaBirdState::Away:
		break;
	case EFaunaBirdState::Soaring:
		UpdateWings(Bird, DeltaSeconds);
		FlyAerial(Bird, DeltaSeconds);
		break;
	}
}

void AFaunaFlock::TakeOff(FFaunaBird& Bird, int32 Index)
{
	// It lets go of its perch: whoever comes back first may take it.
	if (PerchHolder.IsValidIndex(Bird.Perch) && PerchHolder[Bird.Perch] == Index)
	{
		PerchHolder[Bird.Perch] = INDEX_NONE;
	}
	// Into the circle where it's nearest, a little either way, each at its own width and height.
	Bird.CircleAngle = static_cast<float>(FMath::Atan2(Bird.Position.Y - CircleCenter.Y, Bird.Position.X - CircleCenter.X))
		+ Bird.Random.FRandRange(-0.6f, 0.6f);
	Bird.CircleRadius = CircleRadius * Bird.Random.FRandRange(0.75f, 1.25f);
	Bird.CircleHeight = CircleHeight * Bird.Random.FRandRange(-0.25f, 0.25f);
	Bird.BobPhase = Bird.Random.FRandRange(0.f, UE_TWO_PI);
	const float Angle = Bird.CircleAngle;
	const FVector Entry = CircleCenter + FVector(FMath::Cos(Angle) * Bird.CircleRadius, FMath::Sin(Angle) * Bird.CircleRadius,
		Bird.CircleHeight + CircleBob * FMath::Sin(Bird.BobPhase));
	const FVector Tangent = FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0.0) * CircleDirection;

	// Up and away from what startled it (the circle hangs away from it); a bird already flying keeps some of its way.
	FVector Away = CircleCenter - Bird.Position;
	Away.Z = 0.0;
	Away = Away.GetSafeNormal(UE_SMALL_NUMBER, FRotator(0.f, Bird.Yaw, 0.f).Vector());
	const bool bFromPerch = Bird.Velocity.SizeSquared() <= 100.0;
	const FVector Start = !bFromPerch
		? Bird.Velocity.GetSafeNormal() * 0.6 + FVector::UpVector * 0.6 + Away * 0.4
		: FVector::UpVector * 0.9 + Away * 0.6;
	// Off a perch the flying body starts where the perched one's middle was (its pivot is its middle, not its feet).
	Bird.P0 = Bird.Position + (bFromPerch ? FVector(0.0, 0.0, BodyLift * Bird.Scale) : FVector::ZeroVector);
	Bird.V0 = Start.GetSafeNormal() * FlightSpeed;
	Bird.P1 = Entry;
	Bird.V1 = Tangent * FlightSpeed;
	Bird.FlightDuration = FaunaRules::FlightSeconds(static_cast<float>(FVector::Dist(Bird.P0, Bird.P1)), FlightSpeed * 0.8f, 1.2f, 5.f);
	Bird.FlightTime = 0.f;
	Bird.State = EFaunaBirdState::TakingOff;
	Bird.FlapGoal = 1.f;
	Bird.FlapBout = Bird.FlightDuration;
	Bird.Flare = 0.f;
	Bird.Pitch = 30.f;
	Bird.BodyPitch = 0.f;
}

void AFaunaFlock::FlyCircle(FFaunaBird& Bird, float DeltaSeconds)
{
	const float Radius = FMath::Max(Bird.CircleRadius, 100.f);
	Bird.CircleAngle += CircleDirection * FlightSpeed / Radius * DeltaSeconds;
	Bird.BobPhase += DeltaSeconds * 0.9f;
	const float Angle = Bird.CircleAngle;
	const FVector Position = CircleCenter + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius,
		Bird.CircleHeight + CircleBob * FMath::Sin(Bird.BobPhase));
	const FVector Velocity = FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0.0) * (CircleDirection * FlightSpeed)
		+ FVector(0.0, 0.0, CircleBob * 0.9f * FMath::Cos(Bird.BobPhase));
	SteerBody(Bird, Velocity, DeltaSeconds);
	Bird.Position = Position;
}

void AFaunaFlock::StartReturn(FFaunaBird& Bird, int32 PerchIndex)
{
	const FFaunaPerch& Perch = Perches[PerchIndex];
	if (Bird.State == EFaunaBirdState::Away)
	{
		// Flying in from where it went: it shows again as it comes.
		Bird.Velocity = (Perch.Location - Bird.Position).GetSafeNormal() * FlightSpeed;
	}
	FVector Along = Perch.Location - Bird.Position;
	Along.Z = 0.0;
	Along = Along.GetSafeNormal(UE_SMALL_NUMBER, FRotator(0.f, Bird.Yaw, 0.f).Vector());
	Bird.Perch = PerchIndex;
	Bird.LandYaw = FacingOn(Perch, Bird.Random);
	Bird.P0 = Bird.Position;
	Bird.V0 = Bird.Velocity.SizeSquared() > 100.0 ? Bird.Velocity : Along * FlightSpeed;
	Bird.P1 = Perch.Location + FVector(0.0, 0.0, (ApproachHeight + BodyLift) * Bird.Scale) - Along * (ApproachBack * Bird.Scale);
	Bird.V1 = Along * (FlightSpeed * 0.4f) + FVector(0.0, 0.0, -80.0);
	Bird.FlightDuration = FaunaRules::FlightSeconds(static_cast<float>(FVector::Dist(Bird.P0, Bird.P1)), FlightSpeed * 0.9f, 1.5f, 9.f);
	Bird.FlightTime = 0.f;
	Bird.State = EFaunaBirdState::Returning;
	Bird.Flare = 0.f;
}

void AFaunaFlock::StartHop(FFaunaBird& Bird, int32 PerchIndex)
{
	const FFaunaPerch& Perch = Perches[PerchIndex];
	FVector Along = Perch.Location - Bird.Position;
	Along.Z = 0.0;
	Along = Along.GetSafeNormal(UE_SMALL_NUMBER, FRotator(0.f, Bird.Yaw, 0.f).Vector());
	Bird.Perch = PerchIndex;
	Bird.LandYaw = FacingOn(Perch, Bird.Random);
	// Perch to perch, the flying body's middle where the perched body's middle stands.
	const FVector Lift(0.0, 0.0, BodyLift * Bird.Scale);
	Bird.P0 = Bird.Position + Lift;
	Bird.V0 = (Along * 0.55 + FVector::UpVector * 0.85).GetSafeNormal() * HopSpeed;
	Bird.P1 = Perch.Location + Lift;
	Bird.V1 = (Along * 0.6 - FVector::UpVector * 0.4).GetSafeNormal() * (HopSpeed * 0.4f);
	Bird.FlightDuration = FaunaRules::FlightSeconds(static_cast<float>(FVector::Dist(Bird.P0, Bird.P1)), HopSpeed, HopMinSeconds, HopMaxSeconds);
	Bird.FlightTime = 0.f;
	Bird.State = EFaunaBirdState::Hopping;
	Bird.FlapGoal = 1.f;
	Bird.FlapBout = Bird.FlightDuration;
	Bird.Yaw = static_cast<float>(Along.Rotation().Yaw);
	Bird.PecksLeft = 0;
	Bird.WalkDuration = 0.f;
}

void AFaunaFlock::StartLeave(FFaunaBird& Bird)
{
	// Away from the perches' middle on its own heading off the circle, a little spread, climbing out of sight.
	FVector Out = Bird.Position - Center;
	Out.Z = 0.0;
	Out = Out.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector).RotateAngleAxis(Bird.Random.FRandRange(-30.f, 30.f), FVector::UpVector);
	Bird.P0 = Bird.Position;
	Bird.V0 = Bird.Velocity.SizeSquared() > 100.0 ? Bird.Velocity : Out * FlightSpeed;
	Bird.P1 = Center + Out * LeaveDistance + FVector(0.0, 0.0, LeaveHeight);
	Bird.V1 = Out * FlightSpeed;
	Bird.FlightDuration = FaunaRules::FlightSeconds(static_cast<float>(FVector::Dist(Bird.P0, Bird.P1)), FlightSpeed, 3.f, 14.f);
	Bird.FlightTime = 0.f;
	Bird.State = EFaunaBirdState::Leaving;
}

void AFaunaFlock::FlyAerial(FFaunaBird& Bird, float DeltaSeconds)
{
	Bird.AerialTime += DeltaSeconds * Bird.AerialRate;
	const float Radius = FMath::Max(Bird.CircleRadius, 100.f);
	// Radians a second round the figure, so the bird keeps about its flying speed.
	const float Rate = FlightSpeed / Radius;
	const float T = Bird.AerialTime * Rate;
	const float Speed = Rate * FMath::Sign(Bird.AerialRate);
	FVector Local;
	FVector LocalVelocity;
	if (AerialFigure <= 1)
	{
		// Circling a thermal that drifts slowly about the sky's middle.
		const float Drift = Bird.AerialTime * 0.05f;
		Local = FVector(FMath::Cos(T + Bird.AerialPhase.X) * Radius + FMath::Sin(Drift + Bird.AerialPhase.Y) * Radius * 0.35f,
			FMath::Sin(T + Bird.AerialPhase.X) * Radius + FMath::Cos(Drift * 0.86f + Bird.AerialPhase.Z) * Radius * 0.35f, 0.0);
		LocalVelocity = FVector(-FMath::Sin(T + Bird.AerialPhase.X), FMath::Cos(T + Bird.AerialPhase.X), 0.0) * (Radius * Speed);
	}
	else
	{
		// Figure-eights (a Lissajous of 1 : AerialFigure), swooping low and turning hard at the ends.
		const float Ratio = static_cast<float>(AerialFigure);
		Local = FVector(FMath::Sin(T + Bird.AerialPhase.X) * Radius,
			FMath::Sin(Ratio * (T + Bird.AerialPhase.X) + Bird.AerialPhase.Y) * Radius * 0.55f, 0.0);
		LocalVelocity = FVector(FMath::Cos(T + Bird.AerialPhase.X) * Radius,
			Ratio * FMath::Cos(Ratio * (T + Bird.AerialPhase.X) + Bird.AerialPhase.Y) * Radius * 0.55f, 0.0) * Speed;
	}
	const FQuat Turn(FVector::UpVector, FMath::DegreesToRadians(Bird.AerialTurn));
	const float Low = AerialArea.MinHeight;
	const float High = FMath::Max(AerialArea.MinHeight, AerialArea.MaxHeight);
	const float Rise = 0.37f * T + Bird.AerialPhase.Z;
	const float Height = Low + (High - Low) * (0.5f + 0.5f * FMath::Sin(Rise));
	const float Climb = (High - Low) * 0.5f * FMath::Cos(Rise) * 0.37f * Speed;
	const FVector Velocity = Turn.RotateVector(LocalVelocity) + FVector(0.0, 0.0, Climb);
	SteerBody(Bird, Velocity, DeltaSeconds);
	Bird.Position = AerialArea.Center + Turn.RotateVector(Local) + FVector(0.0, 0.0, Height);
}

bool AFaunaFlock::FollowFlight(FFaunaBird& Bird, float DeltaSeconds)
{
	Bird.FlightTime += DeltaSeconds;
	const float Alpha = Bird.FlightTime / FMath::Max(Bird.FlightDuration, 0.01f);
	FVector Velocity;
	const FVector Position = FaunaRules::Hermite(Bird.P0, Bird.V0, Bird.P1, Bird.V1, Bird.FlightDuration, Alpha, &Velocity);
	SteerBody(Bird, Velocity, DeltaSeconds);
	Bird.Position = Position;
	return Alpha >= 1.f;
}

void AFaunaFlock::SteerBody(FFaunaBird& Bird, const FVector& NewVelocity, float DeltaSeconds)
{
	Bird.Velocity = NewVelocity;
	const float Ground = static_cast<float>(NewVelocity.Size2D());
	if (Ground < 20.f)
	{
		return;
	}
	const float NewYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(NewVelocity.Y, NewVelocity.X)));
	const float Turned = FMath::FindDeltaAngleDegrees(Bird.Yaw, NewYaw);
	const float TurnRate = DeltaSeconds > 0.f ? FMath::DegreesToRadians(Turned) / DeltaSeconds : 0.f;
	Bird.Yaw = NewYaw;
	const float Climb = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(NewVelocity.Z, static_cast<double>(Ground))));
	const float PitchGoal = FMath::Clamp(Climb * 0.7f, -MaxDivePitch, MaxClimbPitch);
	// Turning right (yaw growing) banks right: a positive roll drops the right wing.
	const float RollGoal = FaunaRules::BankDegrees(static_cast<float>(NewVelocity.Size()), TurnRate, MaxBank);
	if (DeltaSeconds <= 0.f)
	{
		Bird.Pitch = PitchGoal;
		Bird.Roll = RollGoal;
		return;
	}
	Bird.Pitch = FMath::FInterpTo(Bird.Pitch, PitchGoal, DeltaSeconds, 6.f);
	Bird.Roll = FMath::FInterpTo(Bird.Roll, RollGoal, DeltaSeconds, 5.f);
}

void AFaunaFlock::UpdateWings(FFaunaBird& Bird, float DeltaSeconds, float BeatScale)
{
	Bird.FlapBout -= DeltaSeconds;
	if (Bird.FlapBout <= 0.f)
	{
		// The next bout: a glide in the flock's share of them (long ones for a flock that mostly glides), else beating.
		const bool bGlide = Bird.Random.FRand() < GlideShare;
		Bird.FlapGoal = bGlide ? 0.f : 1.f;
		Bird.FlapBout = bGlide ? Bird.Random.FRandRange(GlideBoutMin, GlideBoutMax) * (1.f + 4.f * FMath::Square(GlideShare))
			: Bird.Random.FRandRange(BeatBoutMin, BeatBoutMax);
	}
	Bird.FlapBlend = FMath::FInterpTo(Bird.FlapBlend, Bird.FlapGoal, DeltaSeconds, 5.f);
	// The beat slows as the wings settle into a glide, so they come to rest smoothly rather than freeze mid-stroke.
	const float Rate = WingbeatsPerSecond * BeatScale * (0.4f + 0.6f * Bird.FlapBlend);
	Bird.FlapPhase = FMath::Fmod(Bird.FlapPhase + UE_TWO_PI * Rate * DeltaSeconds, UE_TWO_PI);
	if (Bird.State != EFaunaBirdState::Landing && Bird.State != EFaunaBirdState::Hopping)
	{
		Bird.Flare = FMath::FInterpTo(Bird.Flare, 0.f, DeltaSeconds, 6.f);
	}
}

void AFaunaFlock::Settle(FFaunaBird& Bird, int32 PerchIndex)
{
	const FFaunaPerch& Perch = Perches[PerchIndex];
	Bird.State = EFaunaBirdState::Perched;
	Bird.Perch = PerchIndex;
	Bird.Position = Perch.Location;
	Bird.Velocity = FVector::ZeroVector;
	Bird.Yaw = Bird.LandYaw;
	Bird.WalkYaw = Bird.LandYaw;
	Bird.Pitch = 0.f;
	Bird.Roll = 0.f;
	Bird.Flare = 0.f;
	Bird.FlapBlend = 0.f;
	Bird.FlapGoal = 0.f;
	Bird.BodyPitch = 0.f;
	Bird.BodyPitchGoal = 0.f;
	Bird.HeadYaw = 0.f;
	Bird.HeadPitch = 0.f;
	Bird.WalkFrom = FVector2D::ZeroVector;
	Bird.WalkTo = FVector2D::ZeroVector;
	Bird.WalkTime = 0.f;
	Bird.WalkDuration = 0.f;
	Bird.PecksLeft = 0;
	Bird.HeadTimer = Bird.Random.FRandRange(0.2f, 1.f);
	Bird.IdleTimer = Bird.Random.FRandRange(1.f, 4.f);
}
