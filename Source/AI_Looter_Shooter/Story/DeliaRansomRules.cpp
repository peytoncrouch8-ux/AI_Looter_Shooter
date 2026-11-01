#include "Story/DeliaRansomRules.h"
#include "Combat/CombatRules.h"

namespace
{
	/** A ramp from 0 at From to 1 at To, eased. */
	float DeliaRamp(float Share, float From, float To)
	{
		return FMath::SmoothStep(From, To, Share);
	}
}

bool DeliaFightRules::InReach(const FVector& Porch, const FVector& Where, float Radius)
{
	return FVector::DistSquared2D(Porch, Where) <= FMath::Square(static_cast<double>(Radius));
}

bool DeliaFightRules::WorthABlast(float Health, float MaxHealth, float Leave)
{
	return MaxHealth > 0.f && Health > MaxHealth * (Leave + WorthMargin);
}

int32 DeliaFightRules::Choose(const FVector& From, const TArray<FDeliaTarget>& Targets, bool& bOutWorth, float Leave)
{
	int32 Best = INDEX_NONE;
	int32 Cover = INDEX_NONE;
	double BestDistance = TNumericLimits<double>::Max();
	double CoverDistance = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Targets.Num(); ++Index)
	{
		const FDeliaTarget& Target = Targets[Index];
		if (!Target.bSeen || Target.Health <= 0.f)
		{
			continue;
		}
		const double Distance = FVector::DistSquared(From, Target.Where);
		if (WorthABlast(Target.Health, Target.MaxHealth, Leave) && Distance < BestDistance)
		{
			Best = Index;
			BestDistance = Distance;
		}
		if (Distance < CoverDistance)
		{
			Cover = Index;
			CoverDistance = Distance;
		}
	}
	bOutWorth = Best != INDEX_NONE;
	return Best != INDEX_NONE ? Best : Cover;
}

float DeliaFightRules::PelletDamage(float MaxHealth, float Share, int32 Count)
{
	return FMath::Max(MaxHealth, 0.f) * Share / (FMath::Max(Count, 1) * LandingShare);
}

float DeliaFightRules::LandingScale(float Health, float MaxHealth, float Pellet, float Leave)
{
	const float Allowed = Health - MaxHealth * Leave;
	const float Best = LooterCombat::MaxHitDamage(Pellet);
	if (Allowed <= 0.f || Best <= 0.f)
	{
		return 0.f;
	}
	return FMath::Min(Allowed / Best, 1.f);
}

float DeliaFightRules::SpreadFor(float Distance)
{
	const float Degrees = FMath::RadiansToDegrees(FMath::Atan2(PatternRadius, FMath::Max(Distance, 1.f)));
	return FMath::Clamp(Degrees, MinSpread, MaxSpread);
}

bool DeliaFightRules::PlayerInLine(const FVector& Muzzle, const FVector& Target, const FVector& Player, float Clearance)
{
	const FVector Line = Target - Muzzle;
	const double Length = Line.Size();
	if (Length < 1.0)
	{
		return false;
	}
	const double Along = ((Player - Muzzle) | Line) / Length;
	if (Along <= 0.0 || Along >= Length)
	{
		return false;
	}
	return FMath::PointDistToSegment(Player, Muzzle, Target) <= Clearance;
}

const TArray<int32>& DeliaFightRules::BarkLines(EDeliaBark Moment)
{
	static const TArray<int32> Opening = { 0 };
	static const TArray<int32> Reload = { 3 };
	static const TArray<int32> HoseMan = { 4 };
	static const TArray<int32> Down = { 2, 1 };
	static const TArray<int32> Hurt = { 6, 5 };
	static const TArray<int32> Finish = { 7, 1, 5 };
	static const TArray<int32> None;
	switch (Moment)
	{
	case EDeliaBark::Opening:
		return Opening;
	case EDeliaBark::Reload:
		return Reload;
	case EDeliaBark::HoseMan:
		return HoseMan;
	case EDeliaBark::Down:
		return Down;
	case EDeliaBark::Hurt:
		return Hurt;
	case EDeliaBark::Finish:
		return Finish;
	default:
		return None;
	}
}

void DeliaFightRules::ReloadPose(float Share, float& OutBreak, float& OutToPocket)
{
	const float T = FMath::Clamp(Share, 0.f, 1.f);
	// Broken open at once, held while the shells go in, snapped shut at the end.
	OutBreak = DeliaRamp(T, 0.f, 0.15f) * (1.f - DeliaRamp(T, 0.8f, 0.92f));
	// The hand to the pocket and back to the breech.
	OutToPocket = DeliaRamp(T, 0.18f, 0.38f) * (1.f - DeliaRamp(T, 0.48f, 0.68f));
}
