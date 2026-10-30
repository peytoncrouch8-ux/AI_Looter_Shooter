#include "Elements/ElementTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Element.* cvars. Their defaults are FElementTuning's own field values, so the rules' tests (a default-made
// tuning) and a fresh game agree.

namespace
{
	const FElementTuning ElementDefaults;

	TAutoConsoleVariable<float> CVarWeak(TEXT("Looter.Element.Weak"), ElementDefaults.Weak,
		TEXT("An element on a creature weak to it: its effect's damage and chance, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarResist(TEXT("Looter.Element.Resist"), ElementDefaults.Resist,
		TEXT("An element on a creature that resists it: its effect's damage and chance, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBulletWeak(TEXT("Looter.Element.BulletWeak"), ElementDefaults.BulletWeak,
		TEXT("An element gun's hit on a creature weak to its element, times (its status keeps Looter.Element.Weak)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBulletResist(TEXT("Looter.Element.BulletResist"), ElementDefaults.BulletResist,
		TEXT("An element gun's hit on a creature that resists its element, times (its status keeps Looter.Element.Resist)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarImmuneHit(TEXT("Looter.Element.ImmuneHit"), ElementDefaults.ImmuneHit,
		TEXT("An element gun's hit on a creature immune to its element, times (no status sets)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWardMatch(TEXT("Looter.Element.Ward.Match"), ElementDefaults.WardMatch,
		TEXT("A hit of a ward's own element into it, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWardBreakAny(TEXT("Looter.Element.Ward.Wardbreak"), ElementDefaults.WardBreakAny,
		TEXT("A Wardbreak hit into any other ward, times (the larger of this and Ward.Match, never both)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarChanceRifle(TEXT("Looter.Element.Chance.Rifle"), ElementDefaults.ChanceRifle,
		TEXT("A rifle round's chance to set its element's status."), ECVF_Default);
	TAutoConsoleVariable<float> CVarChanceShotgun(TEXT("Looter.Element.Chance.Shotgun"), ElementDefaults.ChanceShotgun,
		TEXT("A shotgun pellet's chance to set its element's status (a blast's nine roll apart)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarChanceRevolver(TEXT("Looter.Element.Chance.Revolver"), ElementDefaults.ChanceRevolver,
		TEXT("A revolver round's chance to set its element's status."), ECVF_Default);
	TAutoConsoleVariable<float> CVarChanceSMG(TEXT("Looter.Element.Chance.SMG"), ElementDefaults.ChanceSMG,
		TEXT("An SMG round's chance to set its element's status."), ECVF_Default);
	TAutoConsoleVariable<float> CVarChanceSniper(TEXT("Looter.Element.Chance.Sniper"), ElementDefaults.ChanceSniper,
		TEXT("A sniper round's chance to set its element's status."), ECVF_Default);
	TAutoConsoleVariable<float> CVarChanceLMG(TEXT("Looter.Element.Chance.LMG"), ElementDefaults.ChanceLMG,
		TEXT("An LMG round's chance to set its element's status."), ECVF_Default);
	TAutoConsoleVariable<float> CVarChanceLauncher(TEXT("Looter.Element.Chance.Launcher"), ElementDefaults.ChanceLauncher,
		TEXT("A rocket's chance to set its element's status on each creature in its blast."), ECVF_Default);
	TAutoConsoleVariable<float> CVarChanceRarityStep(TEXT("Looter.Element.Chance.RarityStep"), ElementDefaults.ChanceRarityStep,
		TEXT("Added to the chance for each rarity step above Uncommon."), ECVF_Default);
	TAutoConsoleVariable<float> CVarChanceMax(TEXT("Looter.Element.Chance.Max"), ElementDefaults.ChanceMax,
		TEXT("The most any hit's chance can be, after the weakness."), ECVF_Default);

	TAutoConsoleVariable<float> CVarEmberShare(TEXT("Looter.Element.Ember.Share"), ElementDefaults.EmberShare,
		TEXT("A burn deals this share of the hit that set it, over Ember.Seconds."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEmberSeconds(TEXT("Looter.Element.Ember.Seconds"), ElementDefaults.EmberSeconds,
		TEXT("How long a burn lasts (s)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarEmberMaxStacks(TEXT("Looter.Element.Ember.MaxStacks"), ElementDefaults.EmberMaxStacks,
		TEXT("Burns at once on one creature."), ECVF_Default);

	TAutoConsoleVariable<float> CVarSoulLightShare(TEXT("Looter.Element.SoulLight.Share"), ElementDefaults.SoulLightShare,
		TEXT("A smite's burst: this share of the hit that set it."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSoulLightDelay(TEXT("Looter.Element.SoulLight.Delay"), ElementDefaults.SoulLightDelay,
		TEXT("How long after the hit the smite bursts (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSoulLightChainRadius(TEXT("Looter.Element.SoulLight.ChainRadius"), ElementDefaults.SoulLightChainRadius,
		TEXT("A smite leaps to the nearest of the dead within this (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSoulLightChainShare(TEXT("Looter.Element.SoulLight.ChainShare"), ElementDefaults.SoulLightChainShare,
		TEXT("The leap's share of the smite's burst."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarSoulLightMaxPending(TEXT("Looter.Element.SoulLight.MaxPending"), ElementDefaults.SoulLightMaxPending,
		TEXT("Smites waiting on one creature at once."), ECVF_Default);

	TAutoConsoleVariable<float> CVarGraveSaltShare(TEXT("Looter.Element.GraveSalt.Share"), ElementDefaults.GraveSaltShare,
		TEXT("Salt deals this share of the hit that set it, over GraveSalt.Seconds."), ECVF_Default);
	TAutoConsoleVariable<float> CVarGraveSaltSeconds(TEXT("Looter.Element.GraveSalt.Seconds"), ElementDefaults.GraveSaltSeconds,
		TEXT("How long a creature stays salted (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarGraveSaltSlow(TEXT("Looter.Element.GraveSalt.Slow"), ElementDefaults.GraveSaltSlow,
		TEXT("A salted creature walks at this share of its speed (never a boss; 1 = no slow)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarThunderShare(TEXT("Looter.Element.Thunder.Share"), ElementDefaults.ThunderShare,
		TEXT("A jolt's burst: this share of the hit that set it."), ECVF_Default);
	TAutoConsoleVariable<float> CVarThunderDelay(TEXT("Looter.Element.Thunder.Delay"), ElementDefaults.ThunderDelay,
		TEXT("How long after the hit the jolt bursts (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarThunderStaggerSeconds(TEXT("Looter.Element.Thunder.Stagger"), ElementDefaults.ThunderStaggerSeconds,
		TEXT("The burst staggers a Basic creature this long (s; tougher ranks shorter, never a boss or a Soulfed; 0: none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarThunderChainRadius(TEXT("Looter.Element.Thunder.ChainRadius"), ElementDefaults.ThunderChainRadius,
		TEXT("A jolt arcs to the nearest living creature within this (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarThunderChainShare(TEXT("Looter.Element.Thunder.ChainShare"), ElementDefaults.ThunderChainShare,
		TEXT("The arc's share of the jolt's burst."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarThunderMaxPending(TEXT("Looter.Element.Thunder.MaxPending"), ElementDefaults.ThunderMaxPending,
		TEXT("Jolts waiting on one creature at once."), ECVF_Default);

	TAutoConsoleVariable<float> CVarRotShare(TEXT("Looter.Element.Rot.Share"), ElementDefaults.RotShare,
		TEXT("A rot deals this share of the hit that set it, over Rot.Seconds."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRotSeconds(TEXT("Looter.Element.Rot.Seconds"), ElementDefaults.RotSeconds,
		TEXT("How long a rot lasts (s); a new hit sets it back to this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRotSpreadRadius(TEXT("Looter.Element.Rot.SpreadRadius"), ElementDefaults.RotSpreadRadius,
		TEXT("A rotting creature's death sets every living creature within this (cm) rotting."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRotSpreadShare(TEXT("Looter.Element.Rot.SpreadShare"), ElementDefaults.RotSpreadShare,
		TEXT("The spread rots as a hit of this share of the strongest hit that rotted the dead one."), ECVF_Default);

	TAutoConsoleVariable<float> CVarBossDuration(TEXT("Looter.Element.BossDuration"), ElementDefaults.BossDuration,
		TEXT("Bosses carry a status this share of its time (a smite's burst this share of its damage)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTickSeconds(TEXT("Looter.Element.TickSeconds"), ElementDefaults.TickSeconds,
		TEXT("Damage over time comes off in pieces this often (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTracerTint(TEXT("Looter.Element.TracerTint"), ElementDefaults.TracerTint,
		TEXT("An element gun's tracer is mixed this far toward its element's colour."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLookScale(TEXT("Looter.Element.LookScale"), ElementDefaults.LookScale,
		TEXT("The statuses' and element hits' looks, times (0 hides them)."), ECVF_Default);
}

FElementTuning FElementTuning::Live()
{
	FElementTuning Tuning;
	Tuning.Weak = FMath::Max(CVarWeak.GetValueOnGameThread(), 0.f);
	Tuning.Resist = FMath::Max(CVarResist.GetValueOnGameThread(), 0.f);
	Tuning.BulletWeak = FMath::Max(CVarBulletWeak.GetValueOnGameThread(), 0.f);
	Tuning.BulletResist = FMath::Max(CVarBulletResist.GetValueOnGameThread(), 0.f);
	Tuning.ImmuneHit = FMath::Max(CVarImmuneHit.GetValueOnGameThread(), 0.f);
	Tuning.WardMatch = FMath::Max(CVarWardMatch.GetValueOnGameThread(), 0.f);
	Tuning.WardBreakAny = FMath::Max(CVarWardBreakAny.GetValueOnGameThread(), 0.f);
	Tuning.ChanceRifle = CVarChanceRifle.GetValueOnGameThread();
	Tuning.ChanceShotgun = CVarChanceShotgun.GetValueOnGameThread();
	Tuning.ChanceRevolver = CVarChanceRevolver.GetValueOnGameThread();
	Tuning.ChanceSMG = CVarChanceSMG.GetValueOnGameThread();
	Tuning.ChanceSniper = CVarChanceSniper.GetValueOnGameThread();
	Tuning.ChanceLMG = CVarChanceLMG.GetValueOnGameThread();
	Tuning.ChanceLauncher = CVarChanceLauncher.GetValueOnGameThread();
	Tuning.ChanceRarityStep = CVarChanceRarityStep.GetValueOnGameThread();
	Tuning.ChanceMax = FMath::Clamp(CVarChanceMax.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.EmberShare = FMath::Max(CVarEmberShare.GetValueOnGameThread(), 0.f);
	Tuning.EmberSeconds = FMath::Max(CVarEmberSeconds.GetValueOnGameThread(), 0.1f);
	Tuning.EmberMaxStacks = FMath::Max(CVarEmberMaxStacks.GetValueOnGameThread(), 1);
	Tuning.SoulLightShare = FMath::Max(CVarSoulLightShare.GetValueOnGameThread(), 0.f);
	Tuning.SoulLightDelay = FMath::Max(CVarSoulLightDelay.GetValueOnGameThread(), 0.f);
	Tuning.SoulLightChainRadius = FMath::Max(CVarSoulLightChainRadius.GetValueOnGameThread(), 0.f);
	Tuning.SoulLightChainShare = FMath::Max(CVarSoulLightChainShare.GetValueOnGameThread(), 0.f);
	Tuning.SoulLightMaxPending = FMath::Max(CVarSoulLightMaxPending.GetValueOnGameThread(), 1);
	Tuning.GraveSaltShare = FMath::Max(CVarGraveSaltShare.GetValueOnGameThread(), 0.f);
	Tuning.GraveSaltSeconds = FMath::Max(CVarGraveSaltSeconds.GetValueOnGameThread(), 0.1f);
	Tuning.GraveSaltSlow = FMath::Clamp(CVarGraveSaltSlow.GetValueOnGameThread(), 0.05f, 1.f);
	Tuning.BossDuration = FMath::Clamp(CVarBossDuration.GetValueOnGameThread(), 0.05f, 1.f);
	Tuning.TickSeconds = FMath::Max(CVarTickSeconds.GetValueOnGameThread(), 0.05f);
	Tuning.ThunderShare = FMath::Max(CVarThunderShare.GetValueOnGameThread(), 0.f);
	Tuning.ThunderDelay = FMath::Max(CVarThunderDelay.GetValueOnGameThread(), 0.f);
	Tuning.ThunderStaggerSeconds = FMath::Max(CVarThunderStaggerSeconds.GetValueOnGameThread(), 0.f);
	Tuning.ThunderChainRadius = FMath::Max(CVarThunderChainRadius.GetValueOnGameThread(), 0.f);
	Tuning.ThunderChainShare = FMath::Max(CVarThunderChainShare.GetValueOnGameThread(), 0.f);
	Tuning.ThunderMaxPending = FMath::Max(CVarThunderMaxPending.GetValueOnGameThread(), 1);
	Tuning.RotShare = FMath::Max(CVarRotShare.GetValueOnGameThread(), 0.f);
	Tuning.RotSeconds = FMath::Max(CVarRotSeconds.GetValueOnGameThread(), 0.f);
	Tuning.RotSpreadRadius = FMath::Max(CVarRotSpreadRadius.GetValueOnGameThread(), 0.f);
	Tuning.RotSpreadShare = FMath::Max(CVarRotSpreadShare.GetValueOnGameThread(), 0.f);
	Tuning.TracerTint = FMath::Clamp(CVarTracerTint.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.LookScale = FMath::Max(CVarLookScale.GetValueOnGameThread(), 0.f);
	return Tuning;
}
