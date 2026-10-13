#include "Creatures/DeadMinerRules.h"
#include "Session/CampaignRecord.h"

namespace DeadMinerRules
{
	bool ShiftStillWorks(bool bMissionFinished, bool bPayrollKept)
	{
		return !bMissionFinished || bPayrollKept;
	}

	bool ShiftStillWorks(const FCampaignRecord& Campaign, FName Mission, FName Flag)
	{
		// Back Pay's choice is a story flag the turn-in sets (FCampaignRecord::SetFlag): kept, the dead keep working.
		return ShiftStillWorks(Campaign.HasCompleted(Mission), !Flag.IsNone() && Campaign.HasFlag(Flag));
	}

	bool IsSwingHit(const FVector& Forward, const FVector& ToTarget, float Reach, float Front, float MaxRise)
	{
		if (ToTarget.Size2D() > Reach || FMath::Abs(ToTarget.Z) > MaxRise)
		{
			return false;
		}
		// Right on top of it, it can't miss.
		if (ToTarget.SizeSquared2D() < 1.0)
		{
			return true;
		}
		return FVector::DotProduct(Forward.GetSafeNormal2D(), ToTarget.GetSafeNormal2D()) > Front;
	}

	float WindupFlare(float StateTime, float Windup)
	{
		return Windup > 0.f ? FMath::SmoothStep(0.f, 1.f, FMath::Clamp(StateTime / Windup, 0.f, 1.f)) : 1.f;
	}

	bool ShouldLightLamp(float DistanceToPlayer, int32 NearerLit, int32 MaxLit, float Range)
	{
		return DistanceToPlayer <= Range && NearerLit < MaxLit;
	}

	float WorkPhase(float WorkTime, float SwingSeconds)
	{
		const float Length = FMath::Max(SwingSeconds, 0.01f);
		return FMath::Fmod(FMath::Max(WorkTime, 0.f), Length) / Length;
	}

	bool WorkBlowBetween(float FromTime, float ToTime, float SwingSeconds)
	{
		// The blow lands at WorkBlowShare of each swing: once per swing, whichever frame steps over it.
		const float Length = FMath::Max(SwingSeconds, 0.01f);
		if (ToTime <= FromTime)
		{
			return false;
		}
		const float Blow = WorkBlowShare * Length;
		const float FirstSwing = FMath::FloorToFloat((FromTime - Blow) / Length) + 1.f;
		const float NextBlow = FirstSwing * Length + Blow;
		return NextBlow > FromTime && NextBlow <= ToTime;
	}

	FLinearColor LampColor()
	{
		return FLinearColor(1.f, 0.82f, 0.55f);
	}
}
