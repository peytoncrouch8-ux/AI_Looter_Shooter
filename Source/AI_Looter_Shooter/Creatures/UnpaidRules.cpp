#include "Creatures/UnpaidRules.h"
#include "Creatures/HuntingGround.h"

namespace
{
	/** Spots fan out round the target from straight at it by these angles (degrees), each side in turn. */
	constexpr float FanDegrees[] = { 0.f, 20.f, 40.f, 60.f };

	/** Shorter steps are tried this much shorter each (cm), down to the shortest. */
	constexpr float StepDecrement = 50.f;

	/** A step to sight lands round the target this often (degrees) from its own side, out to the far side. */
	constexpr float SightFanStep = 30.f;

	/** ...at NearestToTarget, then this much farther (cm). */
	constexpr float SightRingGap = 100.f;

	/** The first of Candidates on its ground (with its reach) where CanStand finds its feet, on Here's level. */
	bool ChooseFrom(const TArray<FVector>& Candidates, const FPhaseStepRules& Rules, const FVector& Here, const FHuntingGround& Ground,
		const FVector& Home, TFunctionRef<bool(const FVector& Candidate, FVector& OutFeet)> CanStand, FVector& OutFeet, float GroundReach)
	{
		for (const FVector& Candidate : Candidates)
		{
			// It keeps to its hunting ground: a step never carries it over a fence it fights inside, or out of the yard.
			if (!Ground.ContainsSpot(Candidate, Home, GroundReach))
			{
				continue;
			}
			FVector Feet;
			if (!CanStand(Candidate, Feet) || FMath::Abs(Feet.Z - Here.Z) > Rules.MaxRise)
			{
				continue;
			}
			OutFeet = Feet;
			return true;
		}
		return false;
	}
}

FUnpaidRankTraits UnpaidRules::RankTraits(ECreatureRank Rank)
{
	FUnpaidRankTraits Traits;
	switch (Rank)
	{
	case ECreatureRank::Basic:
		// Dull red is a dark color: it glows harder to read as an ember at all.
		Traits.CoalGlow = 1.8f;
		break;
	case ECreatureRank::Rare:
		Traits.LungeSpeedScale = 1.35f;
		Traits.WindupScale = 0.75f;
		break;
	case ECreatureRank::Epic:
	case ECreatureRank::Hallowed:
	case ECreatureRank::Ethereal:
	case ECreatureRank::Legendary:
		Traits.LungeSpeedScale = 1.35f;
		Traits.WindupScale = 0.75f;
		Traits.bSlowingShriek = true;
		break;
	default:
		break;
	}
	return Traits;
}

bool UnpaidRules::IsCoalShot(const FVector& Coal, const FVector& CoalFacing, const FVector& ImpactPoint, const FVector& ShotDirection,
	float Radius, float ViewAngleDegrees, float Reach)
{
	const FVector Direction = ShotDirection.GetSafeNormal();
	if (Direction.IsZero())
	{
		return false;
	}
	// From the front only: the shot came toward the way the coal faces. From behind or the side the body is in the way.
	const FVector Facing = CoalFacing.GetSafeNormal();
	if (!Facing.IsZero() && FVector::DotProduct(-Direction, Facing) < FMath::Cos(FMath::DegreesToRadians(ViewAngleDegrees)))
	{
		return false;
	}
	// The shot stops at the chest; its line would have gone on through the coal.
	return FMath::PointDistToSegment(Coal, ImpactPoint, ImpactPoint + Direction * FMath::Max(Reach, 0.f)) <= Radius;
}

float UnpaidRules::StepLength(const FPhaseStepRules& Rules, float DistanceToTarget)
{
	const float Step = FMath::Clamp(DistanceToTarget - Rules.NearestToTarget, 0.f, Rules.MaxStep);
	return Step >= Rules.MinStep ? Step : 0.f;
}

bool UnpaidRules::WantsPhaseStep(const FPhaseStepRules& Rules, float DistanceToTarget, float StuckSeconds, float SinceLastStep)
{
	if (SinceLastStep < Rules.Cooldown || StepLength(Rules, DistanceToTarget) <= 0.f)
	{
		return false;
	}
	return DistanceToTarget > Rules.FarBehind || StuckSeconds >= Rules.StuckSeconds;
}

TArray<FVector> UnpaidRules::PhaseCandidates(const FPhaseStepRules& Rules, const FVector& Here, const FVector& Target)
{
	TArray<FVector> Candidates;
	const FVector2D Out(Here.X - Target.X, Here.Y - Target.Y);
	const double Distance = Out.Size();
	const float Longest = StepLength(Rules, static_cast<float>(Distance));
	if (Longest <= 0.f || Distance <= UE_KINDA_SMALL_NUMBER)
	{
		return Candidates;
	}
	const FVector2D Away = Out / Distance;
	// The nearest it may come first, straight at the target first: round a wall the same step lands beside it.
	for (float Step = Longest; Step >= Rules.MinStep - 0.5f; Step -= StepDecrement)
	{
		const double Radius = Distance - Step;
		for (const float Fan : FanDegrees)
		{
			for (const float Side : { 1.f, -1.f })
			{
				const FVector2D Way = Away.GetRotated(Fan * Side);
				Candidates.Emplace(Target.X + Way.X * Radius, Target.Y + Way.Y * Radius, Here.Z);
				if (Fan == 0.f)
				{
					break;
				}
			}
		}
	}
	return Candidates;
}

bool UnpaidRules::ChoosePhaseSpot(const FPhaseStepRules& Rules, const FVector& Here, const FVector& Target, const FHuntingGround& Ground,
	const FVector& Home, TFunctionRef<bool(const FVector& Candidate, FVector& OutFeet)> CanStand, FVector& OutFeet, float GroundReach)
{
	return ChooseFrom(PhaseCandidates(Rules, Here, Target), Rules, Here, Ground, Home, CanStand, OutFeet, GroundReach);
}

bool UnpaidRules::WantsSightStep(const FPhaseStepRules& Rules, bool bSeesTarget, float StuckSeconds, float SinceLastStep)
{
	return !bSeesTarget && SinceLastStep >= Rules.Cooldown && StuckSeconds >= Rules.StuckSeconds;
}

TArray<FVector> UnpaidRules::SightCandidates(const FPhaseStepRules& Rules, const FVector& Here, const FVector& Target)
{
	TArray<FVector> Candidates;
	FVector2D Out(Here.X - Target.X, Here.Y - Target.Y);
	Out = Out.IsNearlyZero() ? FVector2D(1.0, 0.0) : Out.GetSafeNormal();
	for (const float Radius : { Rules.NearestToTarget, Rules.NearestToTarget + SightRingGap })
	{
		for (float Fan = 0.f; Fan <= 180.f + 0.5f; Fan += SightFanStep)
		{
			for (const float Side : { 1.f, -1.f })
			{
				const FVector2D Way = Out.GetRotated(Fan * Side) * FMath::Max(Radius, 1.f);
				const FVector Spot(Target.X + Way.X, Target.Y + Way.Y, Here.Z);
				// No blink longer than its longest step.
				if (FVector::Dist2D(Spot, Here) <= Rules.MaxStep)
				{
					Candidates.Add(Spot);
				}
				if (Fan == 0.f || Fan >= 180.f)
				{
					break;
				}
			}
		}
	}
	return Candidates;
}

bool UnpaidRules::ChooseSightSpot(const FPhaseStepRules& Rules, const FVector& Here, const FVector& Target, const FHuntingGround& Ground,
	const FVector& Home, TFunctionRef<bool(const FVector& Candidate, FVector& OutFeet)> CanStand, FVector& OutFeet, float GroundReach)
{
	return ChooseFrom(SightCandidates(Rules, Here, Target), Rules, Here, Ground, Home, CanStand, OutFeet, GroundReach);
}
