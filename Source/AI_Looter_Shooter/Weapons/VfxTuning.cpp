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
	TAutoConsoleVariable<float> CVarVfxMuzzleFlash(TEXT("Looter.Vfx.Muzzle.Flash"), VfxDefaults.MuzzleFlash,
		TEXT("The muzzle flash's size, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleGlow(TEXT("Looter.Vfx.Muzzle.Glow"), VfxDefaults.MuzzleGlow,
		TEXT("The muzzle flash's strength, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleTime(TEXT("Looter.Vfx.Muzzle.Time"), VfxDefaults.MuzzleTime,
		TEXT("How long the muzzle flash shows, times (each kind's own, about two frames at 60 fps)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleFadePower(TEXT("Looter.Vfx.Muzzle.FadePower"), VfxDefaults.MuzzleFadePower,
		TEXT("How fast the flash fades over its time ((1 - age share) ^ this)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleJitter(TEXT("Looter.Vfx.Muzzle.Jitter"), VfxDefaults.MuzzleJitter,
		TEXT("Each shot's flash this share bigger or smaller at random."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleStar1P(TEXT("Looter.Vfx.Muzzle.Star1P"), VfxDefaults.MuzzleStar1P,
		TEXT("The flash's star in the player's own view, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleTongue1P(TEXT("Looter.Vfx.Muzzle.Tongue1P"), VfxDefaults.MuzzleTongue1P,
		TEXT("The flash's tongues along the barrel in the player's own view (they run toward the crosshair), times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleBloom1P(TEXT("Looter.Vfx.Muzzle.Bloom1P"), VfxDefaults.MuzzleBloom1P,
		TEXT("The flash's soft bloom in the player's own view, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleJets1P(TEXT("Looter.Vfx.Muzzle.Jets1P"), VfxDefaults.MuzzleJets1P,
		TEXT("A brake's jets and a cylinder's spit in the player's own view, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleAim(TEXT("Looter.Vfx.Muzzle.Aim"), VfxDefaults.MuzzleAim,
		TEXT("The flash's size aimed down the sights, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleAimGlow(TEXT("Looter.Vfx.Muzzle.AimGlow"), VfxDefaults.MuzzleAimGlow,
		TEXT("The flash's strength aimed down the sights, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleClear(TEXT("Looter.Vfx.Muzzle.Clear"), VfxDefaults.MuzzleClear,
		TEXT("Degrees round the crosshair the flash keeps out of, from the hip (0: none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleAimClear(TEXT("Looter.Vfx.Muzzle.AimClear"), VfxDefaults.MuzzleAimClear,
		TEXT("...and aimed down the sights."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleClearMin(TEXT("Looter.Vfx.Muzzle.ClearMin"), VfxDefaults.MuzzleClearMin,
		TEXT("The least share of its size the flash shrinks to keeping clear of the crosshair."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleSparks1P(TEXT("Looter.Vfx.Muzzle.Sparks1P"), VfxDefaults.MuzzleSparks1P,
		TEXT("The share of a shot's sparks thrown in the player's own view (small, flying out away from the crosshair)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleSmoke1P(TEXT("Looter.Vfx.Muzzle.Smoke1P"), VfxDefaults.MuzzleSmoke1P,
		TEXT("A shot's smoke opacity in the player's own view, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVfxMuzzleLight(TEXT("Looter.Vfx.Muzzle.Light"), VfxDefaults.MuzzleLight,
		TEXT("The muzzle flash's point light, times (0: none)."), ECVF_Default);
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
	Tuning.MuzzleFlash = VfxNotBelowZero(CVarVfxMuzzleFlash);
	Tuning.MuzzleGlow = VfxNotBelowZero(CVarVfxMuzzleGlow);
	Tuning.MuzzleTime = FMath::Max(CVarVfxMuzzleTime.GetValueOnGameThread(), 0.1f);
	Tuning.MuzzleFadePower = FMath::Clamp(CVarVfxMuzzleFadePower.GetValueOnGameThread(), 0.25f, 8.f);
	Tuning.MuzzleJitter = FMath::Clamp(CVarVfxMuzzleJitter.GetValueOnGameThread(), 0.f, 0.9f);
	Tuning.MuzzleStar1P = VfxNotBelowZero(CVarVfxMuzzleStar1P);
	Tuning.MuzzleTongue1P = VfxNotBelowZero(CVarVfxMuzzleTongue1P);
	Tuning.MuzzleBloom1P = VfxNotBelowZero(CVarVfxMuzzleBloom1P);
	Tuning.MuzzleJets1P = VfxNotBelowZero(CVarVfxMuzzleJets1P);
	Tuning.MuzzleAim = VfxNotBelowZero(CVarVfxMuzzleAim);
	Tuning.MuzzleAimGlow = VfxNotBelowZero(CVarVfxMuzzleAimGlow);
	Tuning.MuzzleClear = FMath::Clamp(CVarVfxMuzzleClear.GetValueOnGameThread(), 0.f, 30.f);
	Tuning.MuzzleAimClear = FMath::Clamp(CVarVfxMuzzleAimClear.GetValueOnGameThread(), 0.f, 30.f);
	Tuning.MuzzleClearMin = FMath::Clamp(CVarVfxMuzzleClearMin.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.MuzzleSparks1P = VfxNotBelowZero(CVarVfxMuzzleSparks1P);
	Tuning.MuzzleSmoke1P = VfxNotBelowZero(CVarVfxMuzzleSmoke1P);
	Tuning.MuzzleLight = VfxNotBelowZero(CVarVfxMuzzleLight);
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
