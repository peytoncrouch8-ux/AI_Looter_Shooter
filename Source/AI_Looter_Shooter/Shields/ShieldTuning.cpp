#include "Shields/ShieldTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Shield.* cvars. Their defaults are FShieldTuning's own field values, so the rules' tests (which use a
// default-made tuning) and a fresh game agree.

namespace
{
	const FShieldTuning ShieldDefaults;

	TAutoConsoleVariable<int32> CVarShieldEnable(TEXT("Looter.Shield.Enable"), ShieldDefaults.Enable,
		TEXT("0: no ward takes anything (every hit goes to health)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCapacityScale(TEXT("Looter.Shield.CapacityScale"), ShieldDefaults.CapacityScale,
		TEXT("Every ward's capacity, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDelayScale(TEXT("Looter.Shield.DelayScale"), ShieldDefaults.DelayScale,
		TEXT("Every ward's wait before it refills, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRateScale(TEXT("Looter.Shield.RateScale"), ShieldDefaults.RateScale,
		TEXT("Every ward's refill rate, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBreakSpill(TEXT("Looter.Shield.BreakSpill"), ShieldDefaults.BreakSpill,
		TEXT("The share of the breaking hit's overflow that reaches health (0-1)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarHealthWaits(TEXT("Looter.Shield.HealthWaits"), ShieldDefaults.HealthWaits,
		TEXT("1: health's regen holds off its ramp until the ward is full again (its wait still counts)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDropShare(TEXT("Looter.Shield.Drop.Share"), ShieldDefaults.DropShare,
		TEXT("The share of a ward-dropping table's guns that come as a ward of the same rarity instead (0-1)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarStarter(TEXT("Looter.Shield.Starter"), ShieldDefaults.Starter,
		TEXT("1: the first gun in the player's hands brings a Common ward."), ECVF_Default);
	TAutoConsoleVariable<float> CVarNovaDamage(TEXT("Looter.Shield.Nova.Damage"), ShieldDefaults.NovaDamage,
		TEXT("Last Breath's burst at its heart, at level 1."), ECVF_Default);
	TAutoConsoleVariable<float> CVarNovaRadius(TEXT("Looter.Shield.Nova.Radius"), ShieldDefaults.NovaRadius,
		TEXT("Last Breath's reach (cm, to a body's nearest side)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarNovaCore(TEXT("Looter.Shield.Nova.Core"), ShieldDefaults.NovaCore,
		TEXT("Last Breath deals its whole damage within this (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarNovaEdge(TEXT("Looter.Shield.Nova.Edge"), ShieldDefaults.NovaEdge,
		TEXT("The share Last Breath still deals at its reach's edge."), ECVF_Default);
	TAutoConsoleVariable<float> CVarNovaBossCap(TEXT("Looter.Shield.Nova.BossCap"), ShieldDefaults.NovaBossCap,
		TEXT("A boss takes at most this share of its health from one Last Breath."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBarbShare(TEXT("Looter.Shield.Barb.Share"), ShieldDefaults.BarbShare,
		TEXT("Barbwire cuts a biter for this share of the ward's capacity, each bite on a charged ward."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLodestoneChance(TEXT("Looter.Shield.Lodestone.Chance"), ShieldDefaults.LodestoneChance,
		TEXT("Lodestone: the chance a hit on a charged ward gives rounds."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLodestoneShare(TEXT("Looter.Shield.Lodestone.Share"), ShieldDefaults.LodestoneShare,
		TEXT("Lodestone: the rounds given, as a share of the gun in hand's magazine (at least 1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHarvestShare(TEXT("Looter.Shield.Harvest.Share"), ShieldDefaults.HarvestShare,
		TEXT("Harvest: a kill puts back this share of the ward's capacity and starts it refilling."), ECVF_Default);
	TAutoConsoleVariable<float> CVarGraceBelow(TEXT("Looter.Shield.Grace.Below"), ShieldDefaults.GraceBelow,
		TEXT("Saint's Grace answers once health falls under this share with the ward broken."), ECVF_Default);
	TAutoConsoleVariable<float> CVarGraceRefill(TEXT("Looter.Shield.Grace.Refill"), ShieldDefaults.GraceRefill,
		TEXT("Saint's Grace flares the ward back to this share of its capacity."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitSoundGap(TEXT("Looter.Shield.HitSoundGap"), ShieldDefaults.HitSoundGap,
		TEXT("Shield.Hit plays at most this often (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHudChipHold(TEXT("Looter.Shield.Hud.ChipHold"), ShieldDefaults.HudChipHold,
		TEXT("What a hit took off the ward bar lingers pale this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHudChipDrain(TEXT("Looter.Shield.Hud.ChipDrain"), ShieldDefaults.HudChipDrain,
		TEXT("...then drains over this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHudFlashSeconds(TEXT("Looter.Shield.Hud.FlashSeconds"), ShieldDefaults.HudFlashSeconds,
		TEXT("The ward bar's white flash as it breaks or fills (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHudShimmerSeconds(TEXT("Looter.Shield.Hud.ShimmerSeconds"), ShieldDefaults.HudShimmerSeconds,
		TEXT("One sweep of the shimmer along the ward bar while it refills (s)."), ECVF_Default);

	float AtLeastOf(const TAutoConsoleVariable<float>& Variable, float Least)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), Least);
	}

	float ShareOf(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Clamp(Variable.GetValueOnGameThread(), 0.f, 1.f);
	}
}

FShieldTuning FShieldTuning::Live()
{
	FShieldTuning Tuning;
	Tuning.Enable = CVarShieldEnable.GetValueOnGameThread();
	Tuning.CapacityScale = AtLeastOf(CVarCapacityScale, 0.f);
	Tuning.DelayScale = AtLeastOf(CVarDelayScale, 0.f);
	Tuning.RateScale = AtLeastOf(CVarRateScale, 0.f);
	Tuning.BreakSpill = ShareOf(CVarBreakSpill);
	Tuning.HealthWaits = CVarHealthWaits.GetValueOnGameThread();
	Tuning.DropShare = ShareOf(CVarDropShare);
	Tuning.Starter = CVarStarter.GetValueOnGameThread();
	Tuning.NovaDamage = AtLeastOf(CVarNovaDamage, 0.f);
	Tuning.NovaRadius = AtLeastOf(CVarNovaRadius, 50.f);
	Tuning.NovaCore = FMath::Clamp(CVarNovaCore.GetValueOnGameThread(), 0.f, Tuning.NovaRadius);
	Tuning.NovaEdge = ShareOf(CVarNovaEdge);
	Tuning.NovaBossCap = ShareOf(CVarNovaBossCap);
	Tuning.BarbShare = AtLeastOf(CVarBarbShare, 0.f);
	Tuning.LodestoneChance = ShareOf(CVarLodestoneChance);
	Tuning.LodestoneShare = AtLeastOf(CVarLodestoneShare, 0.f);
	Tuning.HarvestShare = ShareOf(CVarHarvestShare);
	Tuning.GraceBelow = ShareOf(CVarGraceBelow);
	Tuning.GraceRefill = ShareOf(CVarGraceRefill);
	Tuning.HitSoundGap = AtLeastOf(CVarHitSoundGap, 0.f);
	Tuning.HudChipHold = AtLeastOf(CVarHudChipHold, 0.f);
	Tuning.HudChipDrain = AtLeastOf(CVarHudChipDrain, 0.01f);
	Tuning.HudFlashSeconds = AtLeastOf(CVarHudFlashSeconds, 0.01f);
	Tuning.HudShimmerSeconds = AtLeastOf(CVarHudShimmerSeconds, 0.1f);
	return Tuning;
}
