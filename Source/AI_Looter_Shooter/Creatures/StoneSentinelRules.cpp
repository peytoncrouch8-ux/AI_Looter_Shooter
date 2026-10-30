#include "Creatures/StoneSentinelRules.h"

namespace
{
	/** The poses the slam passes through (StoneSentinelRules::SlamPoseAt): standing, fists overhead, fists in the dirt. */
	const FStoneSentinelPose Idle{ 8.f, 12.f, 4.f };
	const FStoneSentinelPose Raised{ 172.f, 35.f, -10.f };
	const FStoneSentinelPose Down{ 62.f, 6.f, 34.f };

	FStoneSentinelPose Blend(const FStoneSentinelPose& A, const FStoneSentinelPose& B, float Alpha)
	{
		const float T = FMath::Clamp(Alpha, 0.f, 1.f);
		return FStoneSentinelPose{ FMath::Lerp(A.ArmSwing, B.ArmSwing, T), FMath::Lerp(A.ElbowBend, B.ElbowBend, T), FMath::Lerp(A.Bow, B.Bow, T) };
	}
}

namespace StoneSentinelRules
{
	float MaxHealth()
	{
		return DeadMinerHealth * HealthShare;
	}

	float ChaseSpeedFor(float Felt, float ChaseScale)
	{
		return Felt / FMath::Max(ChaseScale, 0.1f);
	}

	bool ShouldWake(float FlatDistance, float Rise, const FStoneSentinelTuning& Tuning)
	{
		return FlatDistance <= Tuning.WakeDistance && FMath::Abs(Rise) <= Tuning.WakeRise;
	}

	bool IsSlamHit(const FVector& Forward, const FVector& ToTarget, const FStoneSentinelTuning& Tuning, float Size)
	{
		const float Scale = FMath::Max(Size, 0.05f);
		if (ToTarget.Size2D() > Tuning.SlamReach * Scale || FMath::Abs(ToTarget.Z) > Tuning.SlamMaxRise * FMath::Max(Scale, 1.f))
		{
			return false;
		}
		if (ToTarget.SizeSquared2D() < 1.0)
		{
			return true;
		}
		return FVector::DotProduct(Forward.GetSafeNormal2D(), ToTarget.GetSafeNormal2D()) > Tuning.SlamFront;
	}

	float CombinedDefense(float Stone, float Tier)
	{
		// Multiplied through, never added: a Restless sentinel (0.1) keeps 0.75 x 0.9 of each hit, not 0.65.
		const float Left = (1.f - FMath::Clamp(Stone, 0.f, 0.9f)) * (1.f - FMath::Clamp(Tier, 0.f, 0.9f));
		return FMath::Clamp(1.f - Left, 0.f, 0.9f);
	}

	bool TracksAim(float StateTime, float Windup, float AimShare)
	{
		return StateTime < Windup * FMath::Clamp(AimShare, 0.f, 1.f);
	}

	FStoneSentinelPose IdlePose()
	{
		return Idle;
	}

	FStoneSentinelPose SlamPoseAt(float AttackTime, const FStoneSentinelTuning& Tuning)
	{
		const float Windup = FMath::Max(Tuning.SlamWindup, 0.05f);
		const float Fall = FMath::Clamp(Tuning.SlamFallSeconds, 0.01f, Windup * 0.5f);
		const float Heave = Windup - Fall;
		if (AttackTime < Heave)
		{
			// Up slowly, faster at the top: the tell is the fists climbing.
			return Blend(Idle, Raised, FMath::SmoothStep(0.f, 1.f, AttackTime / Heave));
		}
		if (AttackTime < Windup)
		{
			// Down hard: it gathers speed all the way to the ground.
			return Blend(Raised, Down, FMath::Square((AttackTime - Heave) / Fall));
		}
		const float Recovery = FMath::Max(Tuning.SlamRecovery, 0.05f);
		const float Hold = Recovery * FMath::Clamp(Tuning.SlamHoldShare, 0.f, 1.f);
		const float After = AttackTime - Windup;
		if (After < Hold)
		{
			return Down;
		}
		return Blend(Down, Idle, FMath::SmoothStep(0.f, 1.f, (After - Hold) / FMath::Max(Recovery - Hold, 0.01f)));
	}

	float WakeShare(float WakeTime, float WakeSeconds)
	{
		const float T = FMath::Clamp(WakeTime / FMath::Max(WakeSeconds, 0.01f), 0.f, 1.f);
		// Most of the waking is the stone grinding loose (a slow start); the last of it, the body comes free.
		return FMath::Pow(T, 1.8f);
	}

	bool StepBetween(float FromPhase, float ToPhase)
	{
		if (ToPhase <= FromPhase)
		{
			return false;
		}
		// Footfalls at every half stride: the first half-mark past FromPhase, if ToPhase reaches it.
		const float Next = (FMath::FloorToFloat(FromPhase * 2.f) + 1.f) * 0.5f;
		return ToPhase >= Next;
	}

	int32 ChipsFor(bool bCritical, float BurstShare, const FStoneSentinelTuning& Tuning)
	{
		const int32 Base = bCritical ? Tuning.CritChips : Tuning.HitChips;
		// A heavy burst (a point-blank blast: a tenth of its health at once) knocks more off.
		return FMath::Max(0, Base + (BurstShare >= 0.1f ? 2 : 0));
	}

	TArray<ECreatureTrait> WithoutFleet(TConstArrayView<ECreatureTrait> Rolled, int32 Count)
	{
		TArray<ECreatureTrait> Kept;
		for (const ECreatureTrait Trait : Rolled)
		{
			if (Kept.Num() >= Count)
			{
				break;
			}
			if (Trait != ECreatureTrait::Fleet)
			{
				Kept.Add(Trait);
			}
		}
		return Kept;
	}
}
