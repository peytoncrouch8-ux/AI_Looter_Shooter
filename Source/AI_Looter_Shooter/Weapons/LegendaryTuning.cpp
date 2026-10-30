#include "Weapons/LegendaryTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Legendary.* cvars; their defaults are FLegendaryTuning's own field values.

namespace
{
	const FLegendaryTuning LegendaryDefaults;

	TAutoConsoleVariable<float> CVarGravesilkDropChance(TEXT("Looter.Legendary.Gravesilk.DropChance"), LegendaryDefaults.GravesilkDropChance,
		TEXT("The Gravemother's chance a kill (a vigil stone's refight too) to drop Gravesilk (0-1)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarGravesilkStacks(TEXT("Looter.Legendary.Gravesilk.Stacks"), LegendaryDefaults.GravesilkStacks,
		TEXT("Gravesilk: stacks of silk that root the target."), ECVF_Default);
	TAutoConsoleVariable<float> CVarGravesilkRootSeconds(TEXT("Looter.Legendary.Gravesilk.RootSeconds"), LegendaryDefaults.GravesilkRootSeconds,
		TEXT("Gravesilk: how long a root holds (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarGravesilkRootShare(TEXT("Looter.Legendary.Gravesilk.RootShare"), LegendaryDefaults.GravesilkRootShare,
		TEXT("Gravesilk: a rooted creature walks at this share of its speed (the movement slow's floor is 0.05)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarGravesilkSilkSeconds(TEXT("Looter.Legendary.Gravesilk.SilkSeconds"), LegendaryDefaults.GravesilkSilkSeconds,
		TEXT("Gravesilk: silk falls away this long after the last hit (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarGravesilkRootGap(TEXT("Looter.Legendary.Gravesilk.RootGap"), LegendaryDefaults.GravesilkRootGap,
		TEXT("Gravesilk: after a root ends, no silk takes for this long (s)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarScripDropChance(TEXT("Looter.Legendary.CompanyScrip.DropChance"), LegendaryDefaults.CompanyScripDropChance,
		TEXT("The Foreman's chance a kill (a vigil stone's refight too) to drop Company Scrip (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarScripFireRatePerStack(TEXT("Looter.Legendary.CompanyScrip.FireRatePerStack"),
		LegendaryDefaults.CompanyScripFireRatePerStack, TEXT("Company Scrip: fire rate each kill in one burst adds (0.04: 4 percent faster)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarScripMaxStacks(TEXT("Looter.Legendary.CompanyScrip.MaxStacks"), LegendaryDefaults.CompanyScripMaxStacks,
		TEXT("Company Scrip: the most kills a burst counts."), ECVF_Default);
	TAutoConsoleVariable<float> CVarScripRefundShare(TEXT("Looter.Legendary.CompanyScrip.RefundShare"), LegendaryDefaults.CompanyScripRefundShare,
		TEXT("Company Scrip: share of the magazine a reload pays back to the pool (never more than it took)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarScripBurstGap(TEXT("Looter.Legendary.CompanyScrip.BurstGap"), LegendaryDefaults.CompanyScripBurstGap,
		TEXT("Company Scrip: this long without a shot ends a burst and its stacks (s)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarLamplighterDropChance(TEXT("Looter.Legendary.Lamplighter.DropChance"), LegendaryDefaults.LamplighterDropChance,
		TEXT("Abel's chance a kill (a vigil stone's refight too) to drop Lamplighter (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLamplighterHealShare(TEXT("Looter.Legendary.Lamplighter.HealShare"), LegendaryDefaults.LamplighterHealShare,
		TEXT("Lamplighter: a lantern heals this share of the player's max health a second."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLamplighterRadius(TEXT("Looter.Legendary.Lamplighter.Radius"), LegendaryDefaults.LamplighterRadius,
		TEXT("Lamplighter: a lantern heals the player within this far (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLamplighterSeconds(TEXT("Looter.Legendary.Lamplighter.Seconds"), LegendaryDefaults.LamplighterSeconds,
		TEXT("Lamplighter: a lantern burns this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLamplighterScopedFrom(TEXT("Looter.Legendary.Lamplighter.ScopedFrom"), LegendaryDefaults.LamplighterScopedFrom,
		TEXT("Lamplighter: a shot counts as scoped with the sights at least this far raised (0-1)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarLamplighterMaxLanterns(TEXT("Looter.Legendary.Lamplighter.MaxLanterns"), LegendaryDefaults.LamplighterMaxLanterns,
		TEXT("Lamplighter: lanterns burning at once at most (the oldest goes out)."), ECVF_Default);
}

FLegendaryTuning FLegendaryTuning::Live()
{
	FLegendaryTuning Tuning;
	Tuning.GravesilkDropChance = FMath::Clamp(CVarGravesilkDropChance.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.GravesilkStacks = FMath::Clamp(CVarGravesilkStacks.GetValueOnGameThread(), 1, 100);
	Tuning.GravesilkRootSeconds = FMath::Max(CVarGravesilkRootSeconds.GetValueOnGameThread(), 0.f);
	Tuning.GravesilkRootShare = FMath::Clamp(CVarGravesilkRootShare.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.GravesilkSilkSeconds = FMath::Max(CVarGravesilkSilkSeconds.GetValueOnGameThread(), 0.f);
	Tuning.GravesilkRootGap = FMath::Max(CVarGravesilkRootGap.GetValueOnGameThread(), 0.f);
	Tuning.CompanyScripDropChance = FMath::Clamp(CVarScripDropChance.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.CompanyScripFireRatePerStack = FMath::Max(CVarScripFireRatePerStack.GetValueOnGameThread(), 0.f);
	Tuning.CompanyScripMaxStacks = FMath::Clamp(CVarScripMaxStacks.GetValueOnGameThread(), 0, 100);
	Tuning.CompanyScripRefundShare = FMath::Clamp(CVarScripRefundShare.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.CompanyScripBurstGap = FMath::Max(CVarScripBurstGap.GetValueOnGameThread(), 0.05f);
	Tuning.LamplighterDropChance = FMath::Clamp(CVarLamplighterDropChance.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.LamplighterHealShare = FMath::Max(CVarLamplighterHealShare.GetValueOnGameThread(), 0.f);
	Tuning.LamplighterRadius = FMath::Max(CVarLamplighterRadius.GetValueOnGameThread(), 0.f);
	Tuning.LamplighterSeconds = FMath::Max(CVarLamplighterSeconds.GetValueOnGameThread(), 0.f);
	Tuning.LamplighterScopedFrom = FMath::Clamp(CVarLamplighterScopedFrom.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.LamplighterMaxLanterns = FMath::Clamp(CVarLamplighterMaxLanterns.GetValueOnGameThread(), 1, 20);
	return Tuning;
}
