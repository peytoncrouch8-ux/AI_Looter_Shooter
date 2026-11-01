// BargeRules: the boarding stage's numbers as plain math (Main 6, "The Pilot Light").

#include "World/BargeRules.h"

namespace BargeRules
{
	FName Mission()
	{
		static const FName Name(TEXT("Main6"));
		return Name;
	}

	FName DrumJammedFlag()
	{
		static const FName Name(TEXT("Pilot.DrumJammed"));
		return Name;
	}

	FName BargeFellFlag()
	{
		static const FName Name(TEXT("Pilot.BargeFell"));
		return Name;
	}

	bool HallFits(float Radius)
	{
		return Radius > 0.f && Radius + WallThickness + HullWall <= HullHalfBeam;
	}

	FVector HallPoint(float AngleDegrees, float Radius, float Up)
	{
		const float Turn = FMath::DegreesToRadians(AngleDegrees);
		return HallCenter + FVector(FMath::Cos(Turn) * Radius, FMath::Sin(Turn) * Radius, HallFloor + Up);
	}

	FVector DeskTop()
	{
		return HallPoint(OfficeAngle, HallRadius + DeskOut, CatwalkUp + DeskTopUp);
	}

	int32 HotGrates(int32 ValvesShut)
	{
		// Each valve shut takes a wedge's heat; the last one takes the rest: the ring goes cold with the furnace.
		if (ValvesShut >= NumValves)
		{
			return 0;
		}
		return FMath::Clamp(4 - FMath::Max(ValvesShut, 0), 0, 4);
	}

	uint32 HotMask(float Time, int32 ValvesShut)
	{
		const int32 Hot = HotGrates(ValvesShut);
		if (Hot <= 0)
		{
			return 0u;
		}
		// The wheel steps one wedge a turn; the hot ones spread evenly round it (4: every third; fewer: as evenly as they go).
		const int32 Turn = FMath::FloorToInt32(FMath::Max(Time, 0.f) / GrateTurnSeconds);
		uint32 Mask = 0u;
		for (int32 Index = 0; Index < Hot; ++Index)
		{
			const int32 Wedge = (Turn + Index * Grates / Hot) % Grates;
			Mask |= 1u << Wedge;
		}
		return Mask;
	}

	bool IsBurning(float Time)
	{
		return FMath::Fmod(FMath::Max(Time, 0.f), GrateTurnSeconds) >= GrateTellSeconds;
	}

	int32 GrateAt(const FVector2D& FromHallMiddle)
	{
		const double Radius = FromHallMiddle.Size();
		if (Radius < GrateInner || Radius > GrateOuter)
		{
			return INDEX_NONE;
		}
		double Angle = FMath::RadiansToDegrees(FMath::Atan2(FromHallMiddle.Y, FromHallMiddle.X));
		if (Angle < 0.0)
		{
			Angle += 360.0;
		}
		return FMath::Clamp(FMath::FloorToInt32(Angle / (360.0 / Grates)), 0, Grates - 1);
	}

	EVentPhase VentPhase(int32 Index, float Time)
	{
		const float Local = Time - Index * VentStagger;
		if (Local < 0.f)
		{
			return EVentPhase::Quiet;
		}
		const float Into = FMath::Fmod(Local, VentEvery);
		// The glow's tell at the end of its quiet, then the burn: so the last second before a burn always glows.
		const float BurnFrom = VentEvery - VentBurnSeconds;
		if (Into >= BurnFrom)
		{
			return EVentPhase::Burn;
		}
		if (Into >= BurnFrom - VentTellSeconds)
		{
			return EVentPhase::Tell;
		}
		return EVentPhase::Quiet;
	}

	float HoseReach(int32 Step, float SecondsInStep)
	{
		// Before the boarding it hangs halfway; it snakes lower while the Warden fights, and stops short of the roof's ridge.
		if (Step < 3)
		{
			return 0.55f;
		}
		const float Base = Step == 3 ? 0.6f : Step == 4 ? 0.75f : 0.85f;
		const float Cap = Step == 3 ? 0.75f : Step == 4 ? 0.85f : 0.94f;
		return FMath::Min(Base + HoseLowerPerSecond * FMath::Max(SecondsInStep, 0.f), Cap);
	}

	float DriftAt(float DyingSeconds)
	{
		// Easing in (the wind takes a dead ship slowly), then a steady drift, never past DriftMost.
		const float T = FMath::Max(DyingSeconds, 0.f);
		const float Eased = T < DriftEaseSeconds ? 0.5f * DriftSpeed * T * T / DriftEaseSeconds : DriftSpeed * (T - 0.5f * DriftEaseSeconds);
		return FMath::Min(Eased, DriftMost);
	}

	float ListAt(float DyingSeconds)
	{
		return ListDegrees * FMath::SmoothStep(0.f, ListSeconds, FMath::Max(DyingSeconds, 0.f));
	}

	FTransform FallPose(const FTransform& From, const FVector& To, float Seconds)
	{
		const float T = FMath::Clamp(Seconds, 0.f, FallSeconds);
		const FVector Start = From.GetLocation();
		// The glide: most of the way across, sinking a little, eased; then the plunge: the rest of the way, falling hard.
		const FVector Brink = FMath::Lerp(Start, To, 0.8) + FVector(0.0, 0.0, (Start.Z - To.Z) * 0.6);
		FVector Where;
		float Nose = 0.f;
		if (T < PlungeAt)
		{
			const float A = FMath::SmoothStep(0.f, 1.f, T / PlungeAt);
			Where = FMath::Lerp(Start, Brink, static_cast<double>(A));
			Nose = 6.f * A;
		}
		else
		{
			const float A = (T - PlungeAt) / (FallSeconds - PlungeAt);
			// Gravity's curve: slow off the brink, fast at the end.
			Where = FMath::Lerp(Brink, To, static_cast<double>(A * A));
			Nose = 6.f + 30.f * A;
		}
		// Nosing down (pitch, bow first) and rolling further onto her berth side as the dead weight goes (From already has
		// the dying list in it). A turn about +X lifts +Y: her berth side (-Y) goes down with a positive turn.
		const FQuat Turn = From.GetRotation() * FQuat(FVector::YAxisVector, FMath::DegreesToRadians(Nose))
			* FQuat(FVector::XAxisVector, FMath::DegreesToRadians(-BerthSide * 1.5f * T));
		return FTransform(Turn, Where, From.GetScale3D());
	}

	bool IsAboard(const FVector& BargeLocal)
	{
		return BargeLocal.X >= DeckAft - 100.f && BargeLocal.X <= DeckFore + 100.f && FMath::Abs(BargeLocal.Y) <= DeckHalfBeam + 60.f
			&& BargeLocal.Z >= HallFloor - 150.f && BargeLocal.Z <= DeckTop + 900.f;
	}
}
