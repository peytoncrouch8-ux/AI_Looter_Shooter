#include "Creatures/BellRingerRules.h"

namespace
{
	/** A hit counts as the bell's within this of its ball's surface (cm): the impact point sits on it, give or take. */
	constexpr float BellHitSlack = 4.f;

	/** Backing off, it tries these turns off straight away from its target (degrees), in order. */
	constexpr float BackOffTurns[] = { 0.f, 35.f, -35.f, 70.f, -70.f };
}

EBellRingerStance BellRingerRules::ChooseStance(const FBellRingerTuning& Tuning, float Distance, bool bBackingOff, float BackOffTime,
	float SinceBackOff)
{
	if (bBackingOff)
	{
		// It backs off only a little: far enough again, or long enough, and it turns to hold its ground.
		return Distance < Tuning.BackOffUntil && BackOffTime < Tuning.BackOffSeconds ? EBellRingerStance::BackOff : EBellRingerStance::Hold;
	}
	if (Distance < Tuning.BackOffBelow && SinceBackOff >= Tuning.BackOffRest)
	{
		return EBellRingerStance::BackOff;
	}
	return Distance > Tuning.HoldDistance ? EBellRingerStance::Approach : EBellRingerStance::Hold;
}

bool BellRingerRules::CanSwipe(const FBellRingerTuning& Tuning, float Distance, float SizeScale)
{
	return Distance <= Tuning.SwipeRange * FMath::Max(SizeScale, 0.1f);
}

float BellRingerRules::TollInterval(const FBellRingerTuning& Tuning, ECreatureRank Rank)
{
	switch (Rank)
	{
	case ECreatureRank::Rare:
		return Tuning.TollInterval * 0.8f;
	case ECreatureRank::Epic:
	case ECreatureRank::Hallowed:
	case ECreatureRank::Ethereal:
	case ECreatureRank::Legendary:
		return Tuning.TollInterval * 0.75f;
	default:
		return Tuning.TollInterval;
	}
}

bool BellRingerRules::IsCrackedBell(ECreatureRank Rank)
{
	// Gravebound and above, never a boss's.
	return LooterRanks::IsAtLeast(Rank, ECreatureRank::Epic) && Rank != ECreatureRank::Boss;
}

EBellToll BellRingerRules::ChooseToll(bool bFallenToRaise, int32 RaisedSoFar, int32 MaxRaises, bool bRoomForMore)
{
	return bFallenToRaise && bRoomForMore && RaisedSoFar < MaxRaises ? EBellToll::Raise : EBellToll::Steady;
}

bool BellRingerRules::IsRaisable(const FBellRingerFallen& Fallen, const FVector& Ringer, double Now, const FVector& Player,
	const FBellRingerTuning& Tuning)
{
	if (Fallen.bRaised || !Fallen.Class.IsValid())
	{
		return false;
	}
	const double Age = Now - Fallen.DiedAt;
	if (Age < 0.0 || Age > Tuning.FallenMaxAge)
	{
		return false;
	}
	if (FVector::DistSquared2D(Fallen.Feet, Ringer) > FMath::Square(static_cast<double>(Tuning.RaiseRadius)))
	{
		return false;
	}
	// The ringer's middle is its capsule's, about a meter over its feet: the fallen's feet are compared against that.
	if (FMath::Abs(Fallen.Feet.Z - Ringer.Z) > Tuning.RaiseMaxRise + 100.f)
	{
		return false;
	}
	return FVector::DistSquared2D(Fallen.Feet, Player) >= FMath::Square(static_cast<double>(Tuning.RaiseClearOfPlayer));
}

int32 BellRingerRules::ChooseFallen(TConstArrayView<FBellRingerFallen> Fallen, const FVector& Ringer, double Now, const FVector& Player,
	const FBellRingerTuning& Tuning)
{
	int32 Best = INDEX_NONE;
	double BestDistance = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Fallen.Num(); ++Index)
	{
		const FBellRingerFallen& Each = Fallen[Index];
		if (!IsRaisable(Each, Ringer, Now, Player, Tuning))
		{
			continue;
		}
		const double Distance = FVector::Dist2D(Each.Feet, Ringer);
		// Within a meter counts as as near: the one fallen last is the one the bell calls.
		const bool bNearer = Distance < BestDistance - 100.0;
		const bool bAsNearAndLater = Best != INDEX_NONE && FMath::Abs(Distance - BestDistance) <= 100.0 && Each.DiedAt > Fallen[Best].DiedAt;
		if (Best == INDEX_NONE || bNearer || bAsNearAndLater)
		{
			Best = Index;
			BestDistance = Distance;
		}
	}
	return Best;
}

bool BellRingerRules::IsBellHit(const FVector& BellCenter, float Radius, const FVector& Point)
{
	return FVector::DistSquared(BellCenter, Point) <= FMath::Square(static_cast<double>(Radius + BellHitSlack));
}

FVector BellRingerRules::GripPoint(const FVector& Wrist, const FVector& Knuckle)
{
	return FMath::Lerp(Wrist, Knuckle, 0.8f);
}

TArray<FVector> BellRingerRules::BackOffDirections(const FVector& Here, const FVector& Target)
{
	FVector Away(Here.X - Target.X, Here.Y - Target.Y, 0.0);
	if (!Away.Normalize())
	{
		Away = FVector::BackwardVector;
	}
	TArray<FVector> Ways;
	for (const float Turn : BackOffTurns)
	{
		Ways.Add(Away.RotateAngleAxis(Turn, FVector::UpVector));
	}
	return Ways;
}
