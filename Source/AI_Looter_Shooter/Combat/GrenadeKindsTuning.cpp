#include "Combat/GrenadeKindsTuning.h"
#include "HAL/IConsoleManager.h"

// The grenade kinds' cvars: their defaults are FGrenadeKindTuning's own field values, so the tests (a default-made
// tuning) and a fresh game agree.

namespace
{
	const FGrenadeKindTuning KindDefaults;

	TAutoConsoleVariable<int32> CVarFromLevel(TEXT("Looter.Grenade.Kinds.FromLevel"), KindDefaults.FromLevel,
		TEXT("Dropped grenade tins are plain grave salt below this player level."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShare(TEXT("Looter.Grenade.Kinds.Share"), KindDefaults.SpecialShare,
		TEXT("The chance a dropped grenade tin is a pitch, scatter or cinder jar rather than grave salt."), ECVF_Default);
	TAutoConsoleVariable<float> CVarPitchWeight(TEXT("Looter.Grenade.Kinds.PitchWeight"), KindDefaults.PitchWeight,
		TEXT("The pitch jar's share of the special drops (0: never)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarScatterWeight(TEXT("Looter.Grenade.Kinds.ScatterWeight"), KindDefaults.ScatterWeight,
		TEXT("The scatter jar's share of the special drops (0: never)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCinderWeight(TEXT("Looter.Grenade.Kinds.CinderWeight"), KindDefaults.CinderWeight,
		TEXT("The cinder jar's share of the special drops (0: never)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarPitchDamage(TEXT("Looter.Grenade.Pitch.Damage"), KindDefaults.PitchDamage,
		TEXT("The pitch jar's burst damage as a share of the salt jar's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarPitchRadius(TEXT("Looter.Grenade.Pitch.Radius"), KindDefaults.PitchRadius,
		TEXT("The pitch jar's reach as a share of the salt jar's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarPitchCore(TEXT("Looter.Grenade.Pitch.Core"), KindDefaults.PitchCore,
		TEXT("The pitch jar's full-damage core as a share of the salt jar's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarPitchStuck(TEXT("Looter.Grenade.Pitch.StuckFuse"), KindDefaults.PitchStuckFuse,
		TEXT("Seconds a pitch jar stuck on a creature waits at most before it bursts."), ECVF_Default);
	TAutoConsoleVariable<float> CVarPitchSlow(TEXT("Looter.Grenade.Pitch.Slow"), KindDefaults.PitchSlow,
		TEXT("What the pitch jar's burst doesn't kill walks at this share of its speed (never a boss)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarPitchSlowTime(TEXT("Looter.Grenade.Pitch.SlowSeconds"), KindDefaults.PitchSlowSeconds,
		TEXT("Seconds the pitch jar's tar slows what it caught."), ECVF_Default);

	TAutoConsoleVariable<float> CVarScatterDamage(TEXT("Looter.Grenade.Scatter.Damage"), KindDefaults.ScatterDamage,
		TEXT("The scatter jar's own burst damage as a share of the salt jar's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarScatterRadius(TEXT("Looter.Grenade.Scatter.Radius"), KindDefaults.ScatterRadius,
		TEXT("The scatter jar's own reach as a share of the salt jar's."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarScatterCount(TEXT("Looter.Grenade.Scatter.Count"), KindDefaults.ScatterCount,
		TEXT("The small jars a scatter jar flings when it bursts."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBroodDamage(TEXT("Looter.Grenade.Scatter.BroodDamage"), KindDefaults.BroodDamage,
		TEXT("Each small jar's burst damage as a share of the salt jar's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBroodRadius(TEXT("Looter.Grenade.Scatter.BroodRadius"), KindDefaults.BroodRadius,
		TEXT("Each small jar's reach as a share of the salt jar's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBroodSpeed(TEXT("Looter.Grenade.Scatter.Speed"), KindDefaults.BroodSpeed,
		TEXT("How hard the small jars are flung out across the ground (cm/s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBroodLift(TEXT("Looter.Grenade.Scatter.Lift"), KindDefaults.BroodLift,
		TEXT("How hard the small jars are flung up (cm/s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBroodFuseMin(TEXT("Looter.Grenade.Scatter.FuseMin"), KindDefaults.BroodFuseMin,
		TEXT("The small jars' shortest fuse (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBroodFuseMax(TEXT("Looter.Grenade.Scatter.FuseMax"), KindDefaults.BroodFuseMax,
		TEXT("The small jars' longest fuse (s)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarCinderDamage(TEXT("Looter.Grenade.Cinder.Damage"), KindDefaults.CinderDamage,
		TEXT("The cinder jar's burst damage as a share of the salt jar's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCinderRadius(TEXT("Looter.Grenade.Cinder.Radius"), KindDefaults.CinderRadius,
		TEXT("The cinder jar's reach as a share of the salt jar's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBedRadius(TEXT("Looter.Grenade.Cinder.BedRadius"), KindDefaults.BedRadius,
		TEXT("The cinder bed's reach (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBedSeconds(TEXT("Looter.Grenade.Cinder.BedSeconds"), KindDefaults.BedSeconds,
		TEXT("Seconds the cinder bed burns."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBedShare(TEXT("Looter.Grenade.Cinder.BedShare"), KindDefaults.BedShare,
		TEXT("What the cinder bed deals a second, as a share of the salt jar's heart."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBedTick(TEXT("Looter.Grenade.Cinder.BedTick"), KindDefaults.BedTick,
		TEXT("Seconds between the cinder bed's burns."), ECVF_Default);
}

FGrenadeKindTuning FGrenadeKindTuning::Current()
{
	FGrenadeKindTuning Made;
	Made.FromLevel = FMath::Max(CVarFromLevel.GetValueOnGameThread(), 1);
	Made.SpecialShare = FMath::Clamp(CVarShare.GetValueOnGameThread(), 0.f, 1.f);
	Made.PitchWeight = FMath::Max(CVarPitchWeight.GetValueOnGameThread(), 0.f);
	Made.ScatterWeight = FMath::Max(CVarScatterWeight.GetValueOnGameThread(), 0.f);
	Made.CinderWeight = FMath::Max(CVarCinderWeight.GetValueOnGameThread(), 0.f);

	Made.PitchDamage = FMath::Max(CVarPitchDamage.GetValueOnGameThread(), 0.f);
	// Shares of the salt jar's reach: never under a quarter (a metre), never over twice.
	Made.PitchRadius = FMath::Clamp(CVarPitchRadius.GetValueOnGameThread(), 0.25f, 2.f);
	Made.PitchCore = FMath::Clamp(CVarPitchCore.GetValueOnGameThread(), 0.f, 4.f);
	Made.PitchStuckFuse = FMath::Clamp(CVarPitchStuck.GetValueOnGameThread(), 0.05f, 5.f);
	Made.PitchSlow = FMath::Clamp(CVarPitchSlow.GetValueOnGameThread(), 0.05f, 1.f);
	Made.PitchSlowSeconds = FMath::Max(CVarPitchSlowTime.GetValueOnGameThread(), 0.f);

	Made.ScatterDamage = FMath::Max(CVarScatterDamage.GetValueOnGameThread(), 0.f);
	Made.ScatterRadius = FMath::Clamp(CVarScatterRadius.GetValueOnGameThread(), 0.25f, 2.f);
	// More than eight small jars is a firework show the effects' budget can't pay for.
	Made.ScatterCount = FMath::Clamp(CVarScatterCount.GetValueOnGameThread(), 0, 8);
	Made.BroodDamage = FMath::Max(CVarBroodDamage.GetValueOnGameThread(), 0.f);
	Made.BroodRadius = FMath::Clamp(CVarBroodRadius.GetValueOnGameThread(), 0.25f, 2.f);
	Made.BroodSpeed = FMath::Max(CVarBroodSpeed.GetValueOnGameThread(), 0.f);
	Made.BroodLift = FMath::Max(CVarBroodLift.GetValueOnGameThread(), 0.f);
	Made.BroodFuseMin = FMath::Max(CVarBroodFuseMin.GetValueOnGameThread(), 0.05f);
	Made.BroodFuseMax = FMath::Max(CVarBroodFuseMax.GetValueOnGameThread(), Made.BroodFuseMin);

	Made.CinderDamage = FMath::Max(CVarCinderDamage.GetValueOnGameThread(), 0.f);
	Made.CinderRadius = FMath::Clamp(CVarCinderRadius.GetValueOnGameThread(), 0.25f, 2.f);
	Made.BedRadius = FMath::Max(CVarBedRadius.GetValueOnGameThread(), 50.f);
	Made.BedSeconds = FMath::Max(CVarBedSeconds.GetValueOnGameThread(), 0.f);
	Made.BedShare = FMath::Max(CVarBedShare.GetValueOnGameThread(), 0.f);
	Made.BedTick = FMath::Clamp(CVarBedTick.GetValueOnGameThread(), 0.1f, 5.f);
	return Made;
}
