#include "Creatures/UnpaidFightRules.h"

namespace
{
	/**
	 * Round the player, a flank comes back at these angles off the way they look (degrees), best first: wide at their side,
	 * then behind the shoulder, straight behind, then nearly beside them (still out of the middle of their view).
	 */
	constexpr float FlankOffsets[] = { 105.f, 145.f, 180.f, 75.f };

	float AngleApart(float A, float B)
	{
		return FMath::Abs(FMath::UnwindDegrees(A - B));
	}
}

FVector UnpaidFightRules::LeadPoint(const FVector& From, const FVector& TargetAt, const FVector& TargetVelocity, float Speed,
	float LeadShare, float MaxLeadSeconds)
{
	const FVector Run(TargetVelocity.X, TargetVelocity.Y, 0.f);
	if (Speed <= UE_KINDA_SMALL_NUMBER || Run.IsNearlyZero() || LeadShare <= 0.f)
	{
		return TargetAt;
	}
	// Twice round: the time to where they stand, then to where that takes them, which is near enough at these speeds.
	float Seconds = FMath::Min(static_cast<float>(FVector::Dist2D(From, TargetAt)) / Speed, MaxLeadSeconds);
	const FVector First = TargetAt + Run * Seconds;
	Seconds = FMath::Min(static_cast<float>(FVector::Dist2D(From, First)) / Speed, MaxLeadSeconds);
	return TargetAt + Run * (Seconds * FMath::Clamp(LeadShare, 0.f, 1.f));
}

EUnpaidMove UnpaidFightRules::ChooseMove(float Distance, float RakeRange, float RakeChance, float Roll)
{
	return Distance <= RakeRange && Roll < RakeChance ? EUnpaidMove::Rake : EUnpaidMove::Lunge;
}

bool UnpaidFightRules::InSwipe(const FVector& Here, const FVector& Facing, const FVector& Victim, float Radius, float ArcDegrees,
	float UpDown)
{
	const FVector To = Victim - Here;
	if (FMath::Abs(To.Z) > UpDown || To.SizeSquared2D() > FMath::Square(Radius))
	{
		return false;
	}
	const FVector Flat = To.GetSafeNormal2D();
	const FVector Ahead = Facing.GetSafeNormal2D();
	// Right on top of it, the swipe can't miss.
	if (Flat.IsNearlyZero() || Ahead.IsNearlyZero())
	{
		return true;
	}
	return FVector::DotProduct(Flat, Ahead) >= FMath::Cos(FMath::DegreesToRadians(0.5f * ArcDegrees));
}

FVector UnpaidFightRules::TurnToward(const FVector& Direction, const FVector& Wanted, float MaxDegrees)
{
	const FVector From = Direction.GetSafeNormal2D();
	const FVector To = Wanted.GetSafeNormal2D();
	if (From.IsNearlyZero())
	{
		return To;
	}
	if (To.IsNearlyZero())
	{
		return From;
	}
	const float Have = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(From.Y, From.X)));
	const float Want = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X)));
	const float Delta = FMath::UnwindDegrees(Want - Have);
	const float Turn = FMath::Clamp(Delta, -FMath::Max(MaxDegrees, 0.f), FMath::Max(MaxDegrees, 0.f));
	const float Radians = FMath::DegreesToRadians(Have + Turn);
	return FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.f);
}

TArray<float> UnpaidFightRules::FlankBearings(float ViewYaw, float CreatureBearing)
{
	TArray<float> Bearings;
	for (const float Offset : FlankOffsets)
	{
		const float Left = FMath::UnwindDegrees(ViewYaw + Offset);
		const float Right = FMath::UnwindDegrees(ViewYaw - Offset);
		if (Offset >= 180.f)
		{
			Bearings.Add(Left);
			continue;
		}
		// Its own side first: the shorter way round, never across the player's eyes.
		const bool bLeftNearer = AngleApart(Left, CreatureBearing) <= AngleApart(Right, CreatureBearing);
		Bearings.Add(bLeftNearer ? Left : Right);
		Bearings.Add(bLeftNearer ? Right : Left);
	}
	return Bearings;
}

bool UnpaidFightRules::WantsFlank(float Frontness, float Distance, float Front, float MinRange, float MaxRange)
{
	return Frontness >= Front && Distance >= MinRange && Distance <= MaxRange;
}

float UnpaidFightRules::ChaseBoost(float Distance, bool bWaiting, float ClosingRange, float ClosingScale, bool bFrenzy, float FrenzyChase)
{
	const float Closing = !bWaiting && Distance > ClosingRange ? ClosingScale : 1.f;
	return Closing * (bFrenzy ? FrenzyChase : 1.f);
}

float UnpaidFightRules::JitterStepClock(float SinceLastStep, float Cooldown, float Jitter, float Roll)
{
	return FMath::Min(SinceLastStep, Cooldown - FMath::Clamp(Roll, 0.f, 1.f) * FMath::Max(Jitter, 0.f));
}

bool FUnpaidStepGate::MayStep(uint32 Id, bool bOwed, double Now, float Gap)
{
	if (Gap <= 0.f)
	{
		return true;
	}
	const bool bHeld = OwedId != 0 && Now - OwedAsked <= OwedHold;
	if (bOwed && (!bHeld || OwedId == Id))
	{
		// The next opening is its own: the first owed one to ask holds it while it keeps asking.
		OwedId = Id;
		OwedAsked = Now;
	}
	if (Now - LastStep < Gap)
	{
		return false;
	}
	// Another's owed step goes first.
	return !(OwedId != 0 && OwedId != Id && Now - OwedAsked <= OwedHold);
}

void FUnpaidStepGate::Note(uint32 Id, double Now)
{
	LastStep = Now;
	if (OwedId == Id)
	{
		OwedId = 0;
		OwedAsked = -1.0e9;
	}
}
