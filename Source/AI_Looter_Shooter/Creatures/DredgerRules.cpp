#include "Creatures/DredgerRules.h"

// The Dredgers' rules (DredgerRules.h): how true a rifleman shoots, where he wants to stand, when the soul-hose holds.

namespace
{
	/** The contract's accuracy point: Accuracy is the share at this distance (cm). */
	constexpr float AccuracyMid = 1500.f;
	/** A target crossing at this speed (cm/s, a sprint) takes the whole moving penalty. */
	constexpr float CrossFull = 600.f;
	/** The pull eases off from this far in (cm) to nothing at PullGone. */
	constexpr float PullFull = 250.f;
	constexpr float PullGone = 150.f;
	/** He never throws at a victim nearer than this (cm): a hose in the face is a club's job. */
	constexpr float LatchNearest = 150.f;

	float LerpBetween(float From, float To, float Value, float A, float B)
	{
		const float Span = FMath::Max(To - From, 1.f);
		return FMath::Lerp(A, B, FMath::Clamp((Value - From) / Span, 0.f, 1.f));
	}
}

float DredgerRules::HitChance(float Distance, float CrossSpeed, const FDredgerTuning& Tuning)
{
	const float Mid = FMath::Clamp(AccuracyMid, Tuning.NearRange + 1.f, Tuning.FarRange - 1.f);
	float Chance = Tuning.AccuracyFar;
	if (Distance <= Tuning.NearRange)
	{
		Chance = Tuning.AccuracyNear;
	}
	else if (Distance <= Mid)
	{
		Chance = LerpBetween(Tuning.NearRange, Mid, Distance, Tuning.AccuracyNear, Tuning.Accuracy);
	}
	else if (Distance <= Tuning.FarRange)
	{
		Chance = LerpBetween(Mid, Tuning.FarRange, Distance, Tuning.Accuracy, Tuning.AccuracyFar);
	}
	const float Crossing = FMath::Clamp(CrossSpeed / CrossFull, 0.f, 1.f);
	return FMath::Clamp(Chance * (1.f - Tuning.MovingPenalty * Crossing), 0.f, 1.f);
}

FVector DredgerRules::MissPoint(const FVector& Chest, const FVector& Muzzle, float Side01, float Spread01)
{
	FVector Along = (Chest - Muzzle).GetSafeNormal();
	if (Along.IsNearlyZero())
	{
		Along = FVector::ForwardVector;
	}
	FVector Side = FVector::CrossProduct(FVector::UpVector, Along).GetSafeNormal();
	if (Side.IsNearlyZero())
	{
		Side = FVector::RightVector;
	}
	const float Off = FMath::Lerp(MissNear, MissFar, FMath::Clamp(Spread01, 0.f, 1.f));
	// Two in five go by on each side, one in five over the head: a burst's misses read as rounds cracking past, not into the floor.
	if (Side01 < 0.4f)
	{
		return Chest - Side * Off;
	}
	if (Side01 < 0.8f)
	{
		return Chest + Side * Off;
	}
	return Chest + FVector::UpVector * Off * 0.8f + Side * (Off * 0.3f * (Side01 - 0.9f) * 10.f);
}

FVector DredgerRules::LeadPoint(const FVector& Muzzle, const FVector& Chest, const FVector& Velocity, float BulletSpeed, float Lead)
{
	const float Flight = static_cast<float>(FVector::Dist(Muzzle, Chest)) / FMath::Max(BulletSpeed, 1.f);
	// A jump's rise and fall is hard to read from far off: only a little of the up and down is led.
	const FVector Moving(Velocity.X, Velocity.Y, Velocity.Z * 0.3f);
	return Chest + Moving * (Flight * Lead);
}

EDredgerKeep DredgerRules::ChooseKeep(float Distance, float Near, float Far, bool bHasSpot, float GapToSpot)
{
	if (Distance > Far + 100.f)
	{
		return EDredgerKeep::None;
	}
	if (Distance < Near)
	{
		return EDredgerKeep::BackOff;
	}
	return bHasSpot && GapToSpot > SpotArrive ? EDredgerKeep::ToSpot : EDredgerKeep::Hold;
}

float DredgerRules::FlankOffset(int32 Index, float Angle)
{
	if (Index <= 0)
	{
		return 0.f;
	}
	const int32 Step = (Index + 1) / 2;
	const float Sign = (Index % 2 == 1) ? 1.f : -1.f;
	return FMath::Clamp(Sign * static_cast<float>(Step) * Angle, -150.f, 150.f);
}

float DredgerRules::ScoreSpot(const FDredgerSpotFacts& Spot, const FDredgerTuning& Tuning)
{
	if (!Spot.bCanShoot)
	{
		return TNumericLimits<float>::Max();
	}
	// A metre off his liking costs as much as two metres' walk or 12 degrees off his flank; no cover costs as much as 25 m
	// off his liking, so he takes cover wherever there is any in his band.
	float Cost = FMath::Abs(Spot.ToTarget - Tuning.Preferred) / 100.f + Spot.Travel / 200.f + Spot.BearingOff / 12.f;
	if (!Spot.bCovered)
	{
		Cost += 25.f;
	}
	if (Spot.bClaimed)
	{
		Cost += 60.f;
	}
	if (Spot.bCrossesTarget)
	{
		Cost += 40.f;
	}
	if (Spot.ToTarget < Tuning.BandNear)
	{
		Cost += 40.f;
	}
	else if (Spot.ToTarget > Tuning.BandFar)
	{
		Cost += 30.f;
	}
	if (Spot.bCurrent)
	{
		Cost -= 3.f;
	}
	return Cost;
}

TArray<FVector2D> DredgerRules::SpotCandidates(float Bearing, const FDredgerTuning& Tuning)
{
	TArray<FVector2D> Candidates;
	const float Low = Tuning.BandNear + 100.f;
	const float High = FMath::Max(Tuning.BandFar - 100.f, Low);
	const float Distances[] = { Tuning.Preferred, Tuning.Preferred - 400.f, Tuning.Preferred + 400.f };
	const float Offsets[] = { 0.f, 30.f, -30.f, 60.f, -60.f };
	for (const float Distance : Distances)
	{
		for (const float Offset : Offsets)
		{
			Candidates.Emplace(FMath::Clamp(Distance, Low, High), FRotator::NormalizeAxis(Bearing + Offset));
		}
	}
	return Candidates;
}

float DredgerRules::BurstRecovery(const FDredgerTuning& Tuning)
{
	return Tuning.BurstGap * static_cast<float>(FMath::Max(Tuning.BurstCount - 1, 0)) + 0.3f;
}

float DredgerRules::BurstCooldown(const FDredgerTuning& Tuning)
{
	return FMath::Max(Tuning.BurstPeriod - Tuning.RifleWindup - BurstRecovery(Tuning), 0.2f);
}

bool DredgerRules::CanLatch(float Distance, bool bLineClear, float RestLeft, const FDredgerTuning& Tuning)
{
	return bLineClear && RestLeft <= 0.f && Distance <= Tuning.LatchRange && Distance >= LatchNearest;
}

EDredgerHoseBreak DredgerRules::HoseBreak(const FDredgerHoseFacts& Facts, const FDredgerTuning& Tuning)
{
	if (Facts.bOwnerDown)
	{
		return EDredgerHoseBreak::OwnerDown;
	}
	if (Facts.bTankBurst)
	{
		return EDredgerHoseBreak::TankBurst;
	}
	if (Facts.bOwnerHeld)
	{
		return EDredgerHoseBreak::Stagger;
	}
	if (Facts.bVictimGone)
	{
		return EDredgerHoseBreak::VictimGone;
	}
	if (Facts.Distance > Tuning.BreakRange)
	{
		return EDredgerHoseBreak::Range;
	}
	if (Facts.BlockedFor >= Tuning.BlockedBreak && Tuning.BlockedBreak >= 0.f && Facts.BlockedFor > 0.f)
	{
		return EDredgerHoseBreak::Blocked;
	}
	if (Facts.LatchedFor >= Tuning.LatchMaxSeconds)
	{
		return EDredgerHoseBreak::TooLong;
	}
	return EDredgerHoseBreak::None;
}

float DredgerRules::DrainPerSecond(float AttackDamage, float ClassAttackDamage, const FDredgerTuning& Tuning)
{
	const float Grown = ClassAttackDamage > UE_KINDA_SMALL_NUMBER ? AttackDamage / ClassAttackDamage : 1.f;
	return Tuning.DrainPerSecond * FMath::Max(Grown, 0.f);
}

float DredgerRules::PullInput(float Distance, const FDredgerTuning& Tuning)
{
	return Tuning.PullShare * FMath::Clamp((Distance - PullGone) / (PullFull - PullGone), 0.f, 1.f);
}

bool DredgerRules::TankBursts(float TankDamage, float MaxHealth, const FDredgerTuning& Tuning)
{
	return MaxHealth > 0.f && TankDamage >= Tuning.TankShare * MaxHealth - 0.01f;
}

FVector DredgerRules::HosePoint(const FVector& A, const FVector& B, float T, float Slack, float Wave, float Clock)
{
	const float Along = FMath::Clamp(T, 0.f, 1.f);
	FVector Point = FMath::Lerp(A, B, Along);
	// Hung under its own weight: deepest at the middle, none at the ends.
	Point.Z -= 4.f * Along * (1.f - Along) * Slack;
	// A ripple running along it from the victim to the nozzle (the soul being drawn in), none at the ends.
	FVector Side = FVector::CrossProduct(FVector::UpVector, B - A).GetSafeNormal();
	if (!Side.IsNearlyZero() && Wave > 0.f)
	{
		const float Ripple = FMath::Sin(Along * PI * 3.f + Clock * 9.f) * FMath::Sin(Along * PI);
		Point += Side * (Wave * Ripple);
	}
	return Point;
}
