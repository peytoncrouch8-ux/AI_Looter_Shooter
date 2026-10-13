#include "Creatures/SpitterRules.h"

namespace
{
	/** A lob's flight grows by a second for every this many centimetres of throw, from ShortestFlight's start. */
	constexpr float FlightBase = 0.55f;
	constexpr float FlightPerCm = 1.f / 2800.f;

	/** A puddle spreads out over this long (s) and dries up over the last DryingSeconds of its life. */
	constexpr float SpreadingSeconds = 0.2f;
	constexpr float DryingSeconds = 0.8f;
}

float SpitterRules::FlightSeconds(float Distance)
{
	return FMath::Clamp(FlightBase + FMath::Max(Distance, 0.f) * FlightPerCm, ShortestFlight, LongestFlight);
}

FVector SpitterRules::LobVelocity(const FVector& Start, const FVector& Target, float Seconds, float Gravity)
{
	// Start + V t - g t^2/2 (up) = Target at t = Seconds.
	const float Time = FMath::Max(Seconds, 0.05f);
	FVector Velocity = (Target - Start) / Time;
	Velocity.Z += 0.5f * Gravity * Time;
	return Velocity;
}

FVector SpitterRules::LobPoint(const FVector& Start, const FVector& Velocity, float After, float Gravity)
{
	return Start + Velocity * After - FVector(0.0, 0.0, 0.5 * Gravity * After * After);
}

FVector SpitterRules::AimPoint(const FVector& Feet, const FVector& Velocity, float Seconds, float LeadShare)
{
	const FVector Flat(Velocity.X, Velocity.Y, 0.0);
	return Feet + Flat * (FMath::Max(Seconds, 0.f) * FMath::Clamp(LeadShare, 0.f, 1.f));
}

ESpitterKeep SpitterRules::ChooseKeep(float Distance, float BackOffWithin, float CloseBeyond, bool bWayBack, bool bStrafing)
{
	if (Distance > CloseBeyond)
	{
		return ESpitterKeep::Close;
	}
	if (Distance < BackOffWithin)
	{
		return bWayBack ? ESpitterKeep::BackOff : ESpitterKeep::Cornered;
	}
	return bStrafing ? ESpitterKeep::Strafe : ESpitterKeep::Hold;
}

bool SpitterRules::CanSpit(float Distance, float MinDistance, float MaxDistance, bool bReady)
{
	return bReady && Distance >= MinDistance && Distance <= MaxDistance;
}

bool SpitterRules::WantsBite(float Distance, float BiteFrom, float CaughtWithin, bool bCornered, bool bReady)
{
	return bReady && Distance <= BiteFrom && (bCornered || Distance <= CaughtWithin);
}

int32 SpitterRules::GlobsFor(ECreatureRank Rank)
{
	switch (Rank)
	{
	case ECreatureRank::Epic:
	case ECreatureRank::Legendary:
	case ECreatureRank::Boss:
		return 3;
	default:
		return 1;
	}
}

float SpitterRules::IntervalShareFor(ECreatureRank Rank)
{
	return Rank == ECreatureRank::Basic ? 1.f : 0.8f;
}

bool SpitterRules::PuddleTouches(const FVector& Center, float Radius, const FVector& Feet, float BodyRadius)
{
	const float Rise = static_cast<float>(Feet.Z - Center.Z);
	if (Rise < -PuddleBelow || Rise > PuddleAbove)
	{
		return false;
	}
	const float Reach = FMath::Max(Radius, 0.f) + FMath::Max(BodyRadius, 0.f) * 0.5f;
	return FVector::DistSquared2D(Center, Feet) <= FMath::Square(Reach);
}

float SpitterRules::PuddleSpread(float Age, float Seconds)
{
	if (Age < 0.f || Age >= Seconds)
	{
		return 0.f;
	}
	const float In = FMath::Clamp(Age / SpreadingSeconds, 0.f, 1.f);
	const float Out = FMath::Clamp((Seconds - Age) / DryingSeconds, 0.f, 1.f);
	// It splashes out fast and settles (ease out), and shrinks away as it dries (ease in).
	return (1.f - FMath::Square(1.f - In)) * FMath::Sqrt(Out);
}
