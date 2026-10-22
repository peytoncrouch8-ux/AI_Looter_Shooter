#include "Weapons/WeaponTrigger.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"

WeaponTrigger::EPress WeaponTrigger::Decide(EWeaponFireMode FireMode, double SecondsUntilReady, bool bShotWaiting)
{
	// Already cycling: one shot is on its way, and a second press adds nothing (this is what keeps clicks from stacking).
	if (bShotWaiting)
	{
		return EPress::Ignore;
	}
	if (SecondsUntilReady <= 0.0)
	{
		return EPress::FireNow;
	}
	// A held trigger on a full-auto gun fires when ready; letting go cancels it, so nothing arrives late.
	if (FireMode == EWeaponFireMode::FullAuto)
	{
		return EPress::Wait;
	}
	return SecondsUntilReady <= BufferSeconds ? EPress::Wait : EPress::Ignore;
}

WeaponTrigger::FTriggerPlan WeaponTrigger::PlanFor(EWeaponFireMode DefinitionMode, int32 DefinitionBurst, EPartEffect Effect,
	const FPartEffectTuning& Tuning)
{
	FTriggerPlan Plan;
	Plan.FireMode = DefinitionMode;
	Plan.BurstCount = DefinitionBurst;
	switch (Effect)
	{
	case EPartEffect::Burst:
		// Whatever it was, it fires bursts now: quick rounds, then a breath before the next pull. A pump or a bolt that
		// was already a burst gun keeps its own count.
		Plan.FireMode = EWeaponFireMode::Burst;
		Plan.BurstCount = DefinitionMode == EWeaponFireMode::Burst && DefinitionBurst > 1 ? DefinitionBurst : Tuning.BurstCount;
		Plan.BurstIntervalScale = Tuning.BurstGap;
		Plan.PullIntervalScale = Tuning.BurstPullGap;
		break;
	case EPartEffect::Overcharge:
		// One wound-up round a pull, at a slower cycle: semi-auto whatever the gun was.
		Plan.FireMode = EWeaponFireMode::SemiAuto;
		Plan.BurstCount = 0;
		Plan.ChargeSeconds = Tuning.OverchargeSeconds;
		Plan.PullIntervalScale = 1.f / FMath::Max(Tuning.OverchargeFireRate, 0.05f);
		break;
	default:
		break;
	}
	return Plan;
}

WeaponTrigger::FTriggerPlan WeaponTrigger::PlanFor(const FWeaponInstanceData& Instance)
{
	if (!Instance.Definition)
	{
		return FTriggerPlan();
	}
	return PlanFor(Instance.Definition->FireMode, Instance.Definition->BurstCount, WeaponParts::EffectOf(Instance), FPartEffectTuning::Live());
}
