#include "Creatures/DredgerTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Dredger.* cvars. Their defaults are FDredgerTuning's own field values, so the rules' tests (which use a
// default-made tuning) and a fresh game agree.

namespace
{
	const FDredgerTuning DredgerDefaults;

	// The rifleman's gun.
	TAutoConsoleVariable<float> CVarDredgerBulletScale(TEXT("Looter.Dredger.Rifle.BulletScale"), DredgerDefaults.BulletScale,
		TEXT("A rifleman's bullet's damage against his AttackDamage (before Looter.Fight.DamageScale)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDredgerBurstCount(TEXT("Looter.Dredger.Rifle.Burst"), DredgerDefaults.BurstCount,
		TEXT("Rounds in a rifleman's burst."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerBurstGap(TEXT("Looter.Dredger.Rifle.BurstGap"), DredgerDefaults.BurstGap,
		TEXT("Seconds between a burst's rounds."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerBurstPeriod(TEXT("Looter.Dredger.Rifle.Period"), DredgerDefaults.BurstPeriod,
		TEXT("Seconds from one burst's start to the next (at the soonest; the fight director's turns may make it longer)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerRifleWindup(TEXT("Looter.Dredger.Rifle.Windup"), DredgerDefaults.RifleWindup,
		TEXT("The tell before a burst (s): rising, shouldering, the lenses' flare and the muzzle's glint."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDredgerMagazine(TEXT("Looter.Dredger.Rifle.Magazine"), DredgerDefaults.Magazine,
		TEXT("Rounds in a rifleman's magazine."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerReload(TEXT("Looter.Dredger.Rifle.Reload"), DredgerDefaults.ReloadSeconds,
		TEXT("A rifleman's reload (s): crouched in cover, his call heard."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerAccuracyNear(TEXT("Looter.Dredger.Rifle.AccuracyNear"), DredgerDefaults.AccuracyNear,
		TEXT("Share of a rifleman's bullets aimed to hit at Looter.Dredger.Rifle.NearRange or nearer."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerAccuracy(TEXT("Looter.Dredger.Rifle.Accuracy"), DredgerDefaults.Accuracy,
		TEXT("Share of a rifleman's bullets aimed to hit at 15 m."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerAccuracyFar(TEXT("Looter.Dredger.Rifle.AccuracyFar"), DredgerDefaults.AccuracyFar,
		TEXT("Share of a rifleman's bullets aimed to hit at Looter.Dredger.Rifle.FarRange or farther."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerNearRange(TEXT("Looter.Dredger.Rifle.NearRange"), DredgerDefaults.NearRange,
		TEXT("Within this (cm) a rifleman's accuracy is its near share."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerFarRange(TEXT("Looter.Dredger.Rifle.FarRange"), DredgerDefaults.FarRange,
		TEXT("Past this (cm) a rifleman's accuracy is its far share."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerMovingPenalty(TEXT("Looter.Dredger.Rifle.MovingPenalty"), DredgerDefaults.MovingPenalty,
		TEXT("Accuracy lost against a target crossing his line at sprint speed."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerBulletSpeed(TEXT("Looter.Dredger.Rifle.BulletSpeed"), DredgerDefaults.BulletSpeed,
		TEXT("How fast a rifleman's bullets fly (cm/s)."), ECVF_Default);

	// The rifleman's distance and cover.
	TAutoConsoleVariable<float> CVarDredgerBandNear(TEXT("Looter.Dredger.Rifle.Near"), DredgerDefaults.BandNear,
		TEXT("A rifleman backs away from a target nearer than this (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerBandFar(TEXT("Looter.Dredger.Rifle.Far"), DredgerDefaults.BandFar,
		TEXT("A rifleman comes in from farther than this (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerPreferred(TEXT("Looter.Dredger.Rifle.Preferred"), DredgerDefaults.Preferred,
		TEXT("The distance a rifleman's cover search likes best (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerCoverRecheck(TEXT("Looter.Dredger.Rifle.CoverRecheck"), DredgerDefaults.CoverRecheck,
		TEXT("How often (s) a rifleman looks at his cover again."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDredgerRelocateAfter(TEXT("Looter.Dredger.Rifle.RelocateAfter"), DredgerDefaults.RelocateAfter,
		TEXT("After this many bursts from one spot a rifleman moves on (0: never)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerFlankAngle(TEXT("Looter.Dredger.Rifle.FlankAngle"), DredgerDefaults.FlankAngle,
		TEXT("Riflemen on one target spread round it this far apart (degrees)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerButtReach(TEXT("Looter.Dredger.Rifle.ButtReach"), DredgerDefaults.ButtReach,
		TEXT("Closer than this (cm) a rifleman clubs with the butt."), ECVF_Default);

	// The soul-hose.
	TAutoConsoleVariable<float> CVarDredgerLatchRange(TEXT("Looter.Dredger.Hose.LatchRange"), DredgerDefaults.LatchRange,
		TEXT("A hose-man throws his line within this (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerBreakRange(TEXT("Looter.Dredger.Hose.BreakRange"), DredgerDefaults.BreakRange,
		TEXT("The soul-hose breaks once its victim is this far (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerLineSpeed(TEXT("Looter.Dredger.Hose.LineSpeed"), DredgerDefaults.LineSpeed,
		TEXT("How fast the thrown line flies (cm/s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerDrain(TEXT("Looter.Dredger.Hose.Drain"), DredgerDefaults.DrainPerSecond,
		TEXT("Health drained a second at level 1."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerPull(TEXT("Looter.Dredger.Hose.Pull"), DredgerDefaults.PullShare,
		TEXT("How hard the line pulls (a share of the victim's own walking input)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerLatchWindup(TEXT("Looter.Dredger.Hose.Windup"), DredgerDefaults.LatchWindup,
		TEXT("The tell before the throw (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerLatchMax(TEXT("Looter.Dredger.Hose.MaxSeconds"), DredgerDefaults.LatchMaxSeconds,
		TEXT("A hose-man lets go after this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerLatchRest(TEXT("Looter.Dredger.Hose.Rest"), DredgerDefaults.LatchRest,
		TEXT("Seconds after the line lets go before the next throw."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerBlockedBreak(TEXT("Looter.Dredger.Hose.BlockedBreak"), DredgerDefaults.BlockedBreak,
		TEXT("The line breaks once the world has cut it this long (s)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDredgerFrostEdge(TEXT("Looter.Dredger.Hose.FrostEdge"), DredgerDefaults.FrostEdge,
		TEXT("1: the drain keeps the rattle's frost on the screen's edges; 0: none (the HUD shows its own)."), ECVF_Default);

	// The tank.
	TAutoConsoleVariable<float> CVarDredgerTankShare(TEXT("Looter.Dredger.Tank.Share"), DredgerDefaults.TankShare,
		TEXT("The tank bursts once hits on it have taken this share of the hose-man's health."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerTankStagger(TEXT("Looter.Dredger.Tank.Stagger"), DredgerDefaults.TankStagger,
		TEXT("The burst staggers him this long (s at Regular)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerTankRadius(TEXT("Looter.Dredger.Tank.Radius"), DredgerDefaults.TankRadius,
		TEXT("The burst's reach (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerTankSplash(TEXT("Looter.Dredger.Tank.Splash"), DredgerDefaults.TankSplashShare,
		TEXT("Share of their health the burst takes from other creatures in it."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerTankSelf(TEXT("Looter.Dredger.Tank.Self"), DredgerDefaults.TankSelfShare,
		TEXT("Share of his health the burst takes from him."), ECVF_Default);

	// The body.
	TAutoConsoleVariable<float> CVarDredgerWalkClip(TEXT("Looter.Dredger.Body.WalkSpeed"), DredgerDefaults.WalkClipSpeed,
		TEXT("The speed (cm/s) the mannequin's walk clips were made at."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerJogClip(TEXT("Looter.Dredger.Body.JogSpeed"), DredgerDefaults.JogClipSpeed,
		TEXT("The speed (cm/s) the mannequin's jog clips were made at."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDredgerRagdoll(TEXT("Looter.Dredger.Body.Ragdoll"), DredgerDefaults.Ragdoll,
		TEXT("1: a killed Dredger goes limp (ragdoll); 0: the code fall."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDredgerKit(TEXT("Looter.Dredger.Body.Kit"), DredgerDefaults.Kit,
		TEXT("1: the stand-in kit (mask, lenses, cap, tank) shows on Dredgers spawned after it is set."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDredgerBarkGap(TEXT("Looter.Dredger.BarkGap"), DredgerDefaults.BarkGap,
		TEXT("Two Dredger barks at least this far apart (s)."), ECVF_Default);

	float AtLeast(const TAutoConsoleVariable<float>& Variable, float Floor)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), Floor);
	}
}

FDredgerTuning FDredgerTuning::Live()
{
	FDredgerTuning Tuning;
	Tuning.BulletScale = AtLeast(CVarDredgerBulletScale, 0.f);
	Tuning.BurstCount = FMath::Clamp(CVarDredgerBurstCount.GetValueOnGameThread(), 1, 12);
	Tuning.BurstGap = AtLeast(CVarDredgerBurstGap, 0.03f);
	Tuning.BurstPeriod = AtLeast(CVarDredgerBurstPeriod, 0.3f);
	Tuning.RifleWindup = AtLeast(CVarDredgerRifleWindup, 0.1f);
	Tuning.Magazine = FMath::Max(CVarDredgerMagazine.GetValueOnGameThread(), 1);
	Tuning.ReloadSeconds = AtLeast(CVarDredgerReload, 0.2f);
	Tuning.AccuracyNear = FMath::Clamp(CVarDredgerAccuracyNear.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.Accuracy = FMath::Clamp(CVarDredgerAccuracy.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.AccuracyFar = FMath::Clamp(CVarDredgerAccuracyFar.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.NearRange = AtLeast(CVarDredgerNearRange, 100.f);
	Tuning.FarRange = FMath::Max(CVarDredgerFarRange.GetValueOnGameThread(), Tuning.NearRange + 100.f);
	Tuning.MovingPenalty = FMath::Clamp(CVarDredgerMovingPenalty.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.BulletSpeed = AtLeast(CVarDredgerBulletSpeed, 500.f);

	Tuning.BandNear = AtLeast(CVarDredgerBandNear, 200.f);
	Tuning.BandFar = FMath::Max(CVarDredgerBandFar.GetValueOnGameThread(), Tuning.BandNear + 200.f);
	Tuning.Preferred = FMath::Clamp(CVarDredgerPreferred.GetValueOnGameThread(), Tuning.BandNear, Tuning.BandFar);
	Tuning.CoverRecheck = AtLeast(CVarDredgerCoverRecheck, 0.3f);
	Tuning.RelocateAfter = FMath::Max(CVarDredgerRelocateAfter.GetValueOnGameThread(), 0);
	Tuning.FlankAngle = FMath::Clamp(CVarDredgerFlankAngle.GetValueOnGameThread(), 0.f, 90.f);
	Tuning.ButtReach = AtLeast(CVarDredgerButtReach, 50.f);

	Tuning.LatchRange = AtLeast(CVarDredgerLatchRange, 100.f);
	Tuning.BreakRange = FMath::Max(CVarDredgerBreakRange.GetValueOnGameThread(), Tuning.LatchRange + 100.f);
	Tuning.LineSpeed = AtLeast(CVarDredgerLineSpeed, 200.f);
	Tuning.DrainPerSecond = AtLeast(CVarDredgerDrain, 0.f);
	Tuning.PullShare = FMath::Clamp(CVarDredgerPull.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.LatchWindup = AtLeast(CVarDredgerLatchWindup, 0.1f);
	Tuning.LatchMaxSeconds = AtLeast(CVarDredgerLatchMax, 0.5f);
	Tuning.LatchRest = AtLeast(CVarDredgerLatchRest, 0.f);
	Tuning.BlockedBreak = AtLeast(CVarDredgerBlockedBreak, 0.f);
	Tuning.FrostEdge = CVarDredgerFrostEdge.GetValueOnGameThread();

	Tuning.TankShare = FMath::Clamp(CVarDredgerTankShare.GetValueOnGameThread(), 0.01f, 1.f);
	Tuning.TankStagger = AtLeast(CVarDredgerTankStagger, 0.f);
	Tuning.TankRadius = AtLeast(CVarDredgerTankRadius, 0.f);
	Tuning.TankSplashShare = FMath::Clamp(CVarDredgerTankSplash.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.TankSelfShare = FMath::Clamp(CVarDredgerTankSelf.GetValueOnGameThread(), 0.f, 1.f);

	Tuning.WalkClipSpeed = AtLeast(CVarDredgerWalkClip, 10.f);
	Tuning.JogClipSpeed = FMath::Max(CVarDredgerJogClip.GetValueOnGameThread(), Tuning.WalkClipSpeed + 10.f);
	Tuning.Ragdoll = CVarDredgerRagdoll.GetValueOnGameThread();
	Tuning.Kit = CVarDredgerKit.GetValueOnGameThread();
	Tuning.BarkGap = AtLeast(CVarDredgerBarkGap, 0.f);
	return Tuning;
}
