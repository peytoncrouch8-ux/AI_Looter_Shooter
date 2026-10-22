#include "Weapons/PartEffectTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.PartEffect.* cvars; their defaults are FPartEffectTuning's own field values.

namespace
{
	const FPartEffectTuning EffectDefaults;

	TAutoConsoleVariable<float> CVarRicochetRadius(TEXT("Looter.PartEffect.Ricochet.Radius"), EffectDefaults.RicochetRadius,
		TEXT("A Ricochet round bounces to the nearest creature within this of where it hit (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRicochetShare(TEXT("Looter.PartEffect.Ricochet.Share"), EffectDefaults.RicochetShare,
		TEXT("The bounce's share of the round's damage."), ECVF_Default);

	TAutoConsoleVariable<int32> CVarBurstCount(TEXT("Looter.PartEffect.Burst.Count"), EffectDefaults.BurstCount,
		TEXT("Rounds in a Burst gun's pull."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstGap(TEXT("Looter.PartEffect.Burst.Gap"), EffectDefaults.BurstGap,
		TEXT("A burst's rounds come this many times the gun's interval apart."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstPullGap(TEXT("Looter.PartEffect.Burst.PullGap"), EffectDefaults.BurstPullGap,
		TEXT("Between a Burst gun's pulls: this many times its interval."), ECVF_Default);

	TAutoConsoleVariable<float> CVarExplosiveRadius(TEXT("Looter.PartEffect.Explosive.Radius"), EffectDefaults.ExplosiveRadius,
		TEXT("An Explosive round's splash reaches this far (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarExplosiveDirect(TEXT("Looter.PartEffect.Explosive.Direct"), EffectDefaults.ExplosiveDirect,
		TEXT("An Explosive round's share of its damage to what it hit."), ECVF_Default);
	TAutoConsoleVariable<float> CVarExplosiveSplash(TEXT("Looter.PartEffect.Explosive.Splash"), EffectDefaults.ExplosiveSplash,
		TEXT("An Explosive round's share of its damage splashed round (what it hit too)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarExplosiveEdgeShare(TEXT("Looter.PartEffect.Explosive.EdgeShare"), EffectDefaults.ExplosiveEdgeShare,
		TEXT("The splash at its edge: this share of its middle's."), ECVF_Default);

	TAutoConsoleVariable<int32> CVarPiercingCount(TEXT("Looter.PartEffect.Piercing.Count"), EffectDefaults.PiercingCount,
		TEXT("Creatures a Piercing round passes through."), ECVF_Default);
	TAutoConsoleVariable<float> CVarPiercingShare(TEXT("Looter.PartEffect.Piercing.Share"), EffectDefaults.PiercingShare,
		TEXT("A Piercing round keeps this share of its damage past each creature."), ECVF_Default);

	TAutoConsoleVariable<float> CVarOverchargeSeconds(TEXT("Looter.PartEffect.Overcharge.Seconds"), EffectDefaults.OverchargeSeconds,
		TEXT("An Overcharge pull winds up this long before it fires (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarOverchargeDamage(TEXT("Looter.PartEffect.Overcharge.Damage"), EffectDefaults.OverchargeDamage,
		TEXT("A full charge's damage, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarOverchargeFireRate(TEXT("Looter.PartEffect.Overcharge.FireRate"), EffectDefaults.OverchargeFireRate,
		TEXT("An Overcharge gun cycles at this share of its rate."), ECVF_Default);
	TAutoConsoleVariable<float> CVarOverchargeMinCharge(TEXT("Looter.PartEffect.Overcharge.MinCharge"), EffectDefaults.OverchargeMinCharge,
		TEXT("A pull let go early fires at least this far charged (0: a plain round)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarTwinShare(TEXT("Looter.PartEffect.Twin.Share"), EffectDefaults.TwinShare,
		TEXT("Each of a Twin shot's two bullets: this share of a round's damage."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTwinAngle(TEXT("Looter.PartEffect.Twin.Angle"), EffectDefaults.TwinAngle,
		TEXT("A Twin shot's bullets fly this many degrees apart."), ECVF_Default);
}

FPartEffectTuning FPartEffectTuning::Live()
{
	FPartEffectTuning Tuning;
	Tuning.RicochetRadius = FMath::Max(CVarRicochetRadius.GetValueOnGameThread(), 0.f);
	Tuning.RicochetShare = FMath::Max(CVarRicochetShare.GetValueOnGameThread(), 0.f);
	Tuning.BurstCount = FMath::Clamp(CVarBurstCount.GetValueOnGameThread(), 1, 10);
	Tuning.BurstGap = FMath::Max(CVarBurstGap.GetValueOnGameThread(), 0.05f);
	Tuning.BurstPullGap = FMath::Max(CVarBurstPullGap.GetValueOnGameThread(), 0.05f);
	Tuning.ExplosiveRadius = FMath::Max(CVarExplosiveRadius.GetValueOnGameThread(), 0.f);
	Tuning.ExplosiveDirect = FMath::Clamp(CVarExplosiveDirect.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.ExplosiveSplash = FMath::Max(CVarExplosiveSplash.GetValueOnGameThread(), 0.f);
	Tuning.ExplosiveEdgeShare = FMath::Clamp(CVarExplosiveEdgeShare.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.PiercingCount = FMath::Clamp(CVarPiercingCount.GetValueOnGameThread(), 0, 8);
	Tuning.PiercingShare = FMath::Max(CVarPiercingShare.GetValueOnGameThread(), 0.f);
	Tuning.OverchargeSeconds = FMath::Max(CVarOverchargeSeconds.GetValueOnGameThread(), 0.f);
	Tuning.OverchargeDamage = FMath::Max(CVarOverchargeDamage.GetValueOnGameThread(), 1.f);
	Tuning.OverchargeFireRate = FMath::Clamp(CVarOverchargeFireRate.GetValueOnGameThread(), 0.05f, 1.f);
	Tuning.OverchargeMinCharge = FMath::Clamp(CVarOverchargeMinCharge.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.TwinShare = FMath::Max(CVarTwinShare.GetValueOnGameThread(), 0.f);
	Tuning.TwinAngle = FMath::Max(CVarTwinAngle.GetValueOnGameThread(), 0.f);
	return Tuning;
}
