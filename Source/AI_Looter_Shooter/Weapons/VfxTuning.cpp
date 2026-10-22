#include "Weapons/VfxTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Vfx.* cvars (the VFX pass, Saved/Vfx/PLAN.md). Their defaults are FVfxTuning's own field values, so the rules'
// tests (a default-made tuning) and a fresh game agree.

namespace
{
	const FVfxTuning VfxDefaults;

	TAutoConsoleVariable<float> CVarVfxHitScale(TEXT("Looter.Vfx.Hit.Scale"), VfxDefaults.HitScale,
		TEXT("The creature hits' new layers (pop, chunks, spray, crit ring), times (0: today's spurts alone)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxHitPop(TEXT("Looter.Vfx.Hit.Pop"), VfxDefaults.HitPop,
		TEXT("The starburst every hit flashes, times its strength (0: none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxImpactScale(TEXT("Looter.Vfx.Impact.Scale"), VfxDefaults.ImpactScale,
		TEXT("The world hits' new layers (dirt geysers, stone jets, metal pings), times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxKillScale(TEXT("Looter.Vfx.Kill.Scale"), VfxDefaults.KillScale,
		TEXT("The kill layers on top of each kind's death burst, times (0: the burst alone)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxKillRing(TEXT("Looter.Vfx.Kill.Ring"), VfxDefaults.KillRing,
		TEXT("A kill's shock rings (soul-light, tiers), times their strength (0: none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleScale(TEXT("Looter.Vfx.Muzzle.Scale"), VfxDefaults.MuzzleScale,
		TEXT("The smoke, sparks, side jets and rings round a shot's flash, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxPowerScale(TEXT("Looter.Vfx.Power.Scale"), VfxDefaults.PowerScale,
		TEXT("The ember powers' new punch (activation rings, body hits, blasts, strikes), times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxBlastScale(TEXT("Looter.Vfx.Blast.Scale"), VfxDefaults.BlastScale,
		TEXT("Explosions (a rocket's, Slag Bomb's, Last Rites'), times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxGlow(TEXT("Looter.Vfx.Glow"), VfxDefaults.Glow,
		TEXT("Every new glow's strength (rings, fireballs, starbursts), times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxSmoke(TEXT("Looter.Vfx.Smoke"), VfxDefaults.Smoke,
		TEXT("Every new puff's opacity (mist, geysers, columns), times."), ECVF_Default);

	TAutoConsoleVariable<float> CVarVfxDetailNear(TEXT("Looter.Vfx.Detail.Near"), VfxDefaults.DetailNear,
		TEXT("Full detail within this far of the camera (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxDetailFar(TEXT("Looter.Vfx.Detail.Far"), VfxDefaults.DetailFar,
		TEXT("The least detail (Detail.Min) from this far on (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxDetailMin(TEXT("Looter.Vfx.Detail.Min"), VfxDefaults.DetailMin,
		TEXT("The share of a look's pieces still thrown far off (a hit's pop is never thinned)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxStreamWindow(TEXT("Looter.Vfx.Stream.Window"), VfxDefaults.StreamWindow,
		TEXT("Hits this close in time (s) and Stream.Reach in space are one stream of fire, its look thinned."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxStreamReach(TEXT("Looter.Vfx.Stream.Reach"), VfxDefaults.StreamReach,
		TEXT("...and this close (cm)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarVfxMaxParticles(TEXT("Looter.Vfx.MaxParticles"), VfxDefaults.MaxParticles,
		TEXT("The code-drawn effects' particle pool (the oldest gives way past it)."), ECVF_Default);

	TAutoConsoleVariable<bool> CVarVfxShapes(TEXT("Looter.Vfx.Shapes"), VfxDefaults.bShapes,
		TEXT("Rings, fireballs and starbursts on M_FX_Shape when it's in the game (read as a level's effects start)."), ECVF_Default);
	TAutoConsoleVariable<bool> CVarVfxPuffs(TEXT("Looter.Vfx.Puffs"), VfxDefaults.bPuffs,
		TEXT("Smoke on M_FX_Puff (noise-broken, eroding) when it's in the game (read as a level's effects start)."), ECVF_Default);
	TAutoConsoleVariable<bool> CVarVfxFire(TEXT("Looter.Vfx.Fire"), VfxDefaults.bFire,
		TEXT("Blasts' fire and soot on M_FX_Fire (alpha-blended) when it's in the game (read as a level's effects start)."), ECVF_Default);
	TAutoConsoleVariable<bool> CVarVfxBurnTint(TEXT("Looter.Vfx.BurnTint"), VfxDefaults.bBurnTint,
		TEXT("A power's burn tint on the bodies it burns (Slag Bomb's pool; an overlay: the body drawn again)."), ECVF_Default);
	TAutoConsoleVariable<bool> CVarVfxSplats(TEXT("Looter.Vfx.Splats"), VfxDefaults.bSplats,
		TEXT("Ground splats under kills and blasts."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarVfxSplatMax(TEXT("Looter.Vfx.Splat.Max"), VfxDefaults.SplatMax,
		TEXT("Ground splats at once (the oldest fades out quickly past it)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxSplatHold(TEXT("Looter.Vfx.Splat.Hold"), VfxDefaults.SplatHold,
		TEXT("How long a ground splat holds (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxSplatFade(TEXT("Looter.Vfx.Splat.Fade"), VfxDefaults.SplatFade,
		TEXT("...and fades (s)."), ECVF_Default);

	float VfxNotBelowZero(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), 0.f);
	}
}

FVfxTuning FVfxTuning::Live()
{
	FVfxTuning Tuning;
	Tuning.HitScale = VfxNotBelowZero(CVarVfxHitScale);
	Tuning.HitPop = VfxNotBelowZero(CVarVfxHitPop);
	Tuning.ImpactScale = VfxNotBelowZero(CVarVfxImpactScale);
	Tuning.KillScale = VfxNotBelowZero(CVarVfxKillScale);
	Tuning.KillRing = VfxNotBelowZero(CVarVfxKillRing);
	Tuning.MuzzleScale = VfxNotBelowZero(CVarVfxMuzzleScale);
	Tuning.PowerScale = VfxNotBelowZero(CVarVfxPowerScale);
	Tuning.BlastScale = VfxNotBelowZero(CVarVfxBlastScale);
	Tuning.Glow = VfxNotBelowZero(CVarVfxGlow);
	Tuning.Smoke = VfxNotBelowZero(CVarVfxSmoke);
	// Far is never nearer than near, and the far share stays a share.
	Tuning.DetailNear = VfxNotBelowZero(CVarVfxDetailNear);
	Tuning.DetailFar = FMath::Max(CVarVfxDetailFar.GetValueOnGameThread(), Tuning.DetailNear + 1.f);
	Tuning.DetailMin = FMath::Clamp(CVarVfxDetailMin.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.StreamWindow = VfxNotBelowZero(CVarVfxStreamWindow);
	Tuning.StreamReach = VfxNotBelowZero(CVarVfxStreamReach);
	Tuning.MaxParticles = FMath::Clamp(CVarVfxMaxParticles.GetValueOnGameThread(), 100, 4000);
	Tuning.bShapes = CVarVfxShapes.GetValueOnGameThread();
	Tuning.bPuffs = CVarVfxPuffs.GetValueOnGameThread();
	Tuning.bFire = CVarVfxFire.GetValueOnGameThread();
	Tuning.bBurnTint = CVarVfxBurnTint.GetValueOnGameThread();
	Tuning.bSplats = CVarVfxSplats.GetValueOnGameThread();
	Tuning.SplatMax = FMath::Clamp(CVarVfxSplatMax.GetValueOnGameThread(), 0, 64);
	Tuning.SplatHold = VfxNotBelowZero(CVarVfxSplatHold);
	Tuning.SplatFade = FMath::Max(CVarVfxSplatFade.GetValueOnGameThread(), 0.1f);
	return Tuning;
}
