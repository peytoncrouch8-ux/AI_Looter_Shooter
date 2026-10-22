#include "Combat/GunfeelReactTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Gunfeel.* cvars of how a creature takes a gun's hit (GUNFEEL-REACT). Their defaults are FGunfeelReactTuning's
// own field values, so the rules' tests (which use a default-made tuning) and a fresh game agree.

namespace
{
	const FGunfeelReactTuning ReactDefaults;

	// The flinch and the slow.
	TAutoConsoleVariable<float> CVarFlinchScale(TEXT("Looter.Gunfeel.Flinch.Scale"), ReactDefaults.FlinchScale,
		TEXT("Every creature's hit flinch, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlinchDamageGain(TEXT("Looter.Gunfeel.Flinch.DamageGain"), ReactDefaults.FlinchDamageGain,
		TEXT("How much a hit's share of health adds to its flinch (flinch = Scale x clamp(0.6 + Gain x share, 0.6, Max))."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlinchMax(TEXT("Looter.Gunfeel.Flinch.Max"), ReactDefaults.FlinchMax,
		TEXT("The most a hit flinches a creature, before a crit's own."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlinchCritScale(TEXT("Looter.Gunfeel.Flinch.CritScale"), ReactDefaults.FlinchCritScale,
		TEXT("A critical hit's flinch, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlinchBossScale(TEXT("Looter.Gunfeel.Flinch.BossScale"), ReactDefaults.FlinchBossScale,
		TEXT("A boss flinches this share of a creature's flinch (its moves are never interrupted)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlinchSlow(TEXT("Looter.Gunfeel.Flinch.Slow"), ReactDefaults.FlinchSlow,
		TEXT("Any hit holds a creature's move speed to this share for SlowSeconds (never a boss; 1 = off)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlinchSlowSeconds(TEXT("Looter.Gunfeel.Flinch.SlowSeconds"), ReactDefaults.FlinchSlowSeconds,
		TEXT("How long a hit's slow lasts (s; slows never stack past one)."), ECVF_Default);

	// Staggers.
	TAutoConsoleVariable<float> CVarStaggerHeavyShare(TEXT("Looter.Gunfeel.Stagger.HeavyShare"), ReactDefaults.StaggerHeavyShare,
		TEXT("A burst worth this share of a creature's health is a heavy hit: a stagger and a short hit-stop."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStaggerCooldown(TEXT("Looter.Gunfeel.Stagger.Cooldown"), ReactDefaults.StaggerCooldown,
		TEXT("After a stagger ends, none again for this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStaggerSeconds(TEXT("Looter.Gunfeel.Stagger.Seconds"), ReactDefaults.StaggerSeconds,
		TEXT("A Basic creature's stagger (s); the tougher ranks' scale from it."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStaggerShove(TEXT("Looter.Gunfeel.Stagger.Shove"), ReactDefaults.StaggerShove,
		TEXT("A stagger shoves a full-size Basic creature back this fast (cm/s)."), ECVF_Default);

	// Hit-stops.
	TAutoConsoleVariable<float> CVarHitStopCrit(TEXT("Looter.Gunfeel.HitStop.Crit"), ReactDefaults.HitStopCrit,
		TEXT("A crit's hit-stop (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitStopHeavy(TEXT("Looter.Gunfeel.HitStop.Heavy"), ReactDefaults.HitStopHeavy,
		TEXT("A heavy body hit's hit-stop (s), cooled down as a crit's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitStopKill(TEXT("Looter.Gunfeel.HitStop.Kill"), ReactDefaults.HitStopKill,
		TEXT("The killing hit's hit-stop (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitStopCritKill(TEXT("Looter.Gunfeel.HitStop.CritKill"), ReactDefaults.HitStopCritKill,
		TEXT("A critical kill's hit-stop (s)."), ECVF_Default);

	// Kills by gun.
	TAutoConsoleVariable<float> CVarKillShotgunKnock(TEXT("Looter.Gunfeel.Kill.ShotgunKnock"), ReactDefaults.KillShotgunKnock,
		TEXT("A shotgun kill throws the corpse along the shot this fast (cm/s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKillShotgunLift(TEXT("Looter.Gunfeel.Kill.ShotgunLift"), ReactDefaults.KillShotgunLift,
		TEXT("...lifting it this high at the arc's top (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKillRifleKnock(TEXT("Looter.Gunfeel.Kill.RifleKnock"), ReactDefaults.KillRifleKnock,
		TEXT("A rifle kill's corpse knock (cm/s): it crumples where it stands."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKillRevolverBurst(TEXT("Looter.Gunfeel.Kill.RevolverBurst"), ReactDefaults.KillRevolverBurst,
		TEXT("A revolver crit kill's death burst at the crit spot, times as big."), ECVF_Default);

	// Spurts.
	TAutoConsoleVariable<float> CVarSpurtScale(TEXT("Looter.Gunfeel.Spurt.Scale"), ReactDefaults.SpurtScale,
		TEXT("What a creature hit spills (blood, gel, ash), times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSpurtExit(TEXT("Looter.Gunfeel.Spurt.Exit"), ReactDefaults.SpurtExit,
		TEXT("The exit spray behind the target, as a share of the entry's particles (0 = off)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSpurtBlastBudget(TEXT("Looter.Gunfeel.Spurt.BlastBudget"), ReactDefaults.SpurtBlastBudget,
		TEXT("A shotgun blast's pellets share this many full hits' spurts."), ECVF_Default);

	// The guns' jobs.
	TAutoConsoleVariable<int32> CVarRiflePressureHits(TEXT("Looter.Gunfeel.Rifle.PressureHits"), ReactDefaults.RiflePressureHits,
		TEXT("This many rifle hits on one creature within PressureWindow stagger it."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRiflePressureWindow(TEXT("Looter.Gunfeel.Rifle.PressureWindow"), ReactDefaults.RiflePressureWindow,
		TEXT("The rifle's pressure window (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShotgunKnockShare(TEXT("Looter.Gunfeel.Shotgun.KnockShare"), ReactDefaults.ShotgunKnockShare,
		TEXT("A shotgun blast worth this share of a creature's health, close enough, knocks it back (or staggers a big one)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShotgunKnockRange(TEXT("Looter.Gunfeel.Shotgun.KnockRange"), ReactDefaults.ShotgunKnockRange,
		TEXT("...within this far of the shooter (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShotgunKnockSize(TEXT("Looter.Gunfeel.Shotgun.KnockSize"), ReactDefaults.ShotgunKnockSize,
		TEXT("A creature this size or smaller is knocked back; a bigger one staggered."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShotgunKnockSpeed(TEXT("Looter.Gunfeel.Shotgun.KnockSpeed"), ReactDefaults.ShotgunKnockSpeed,
		TEXT("The knock-back's launch across the ground (cm/s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShotgunKnockHeight(TEXT("Looter.Gunfeel.Shotgun.KnockHeight"), ReactDefaults.ShotgunKnockHeight,
		TEXT("The knock-back's arc top (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShotgunKnockHold(TEXT("Looter.Gunfeel.Shotgun.KnockHold"), ReactDefaults.ShotgunKnockHold,
		TEXT("Once down from a knock-back it gathers itself this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRevolverCritHitStop(TEXT("Looter.Gunfeel.Revolver.CritHitStop"), ReactDefaults.RevolverCritHitStop,
		TEXT("A revolver crit's hit-stop (s)."), ECVF_Default);

	float NotBelowZero(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), 0.f);
	}
}

FGunfeelReactTuning FGunfeelReactTuning::Live()
{
	FGunfeelReactTuning Tuning;
	Tuning.FlinchScale = NotBelowZero(CVarFlinchScale);
	Tuning.FlinchDamageGain = NotBelowZero(CVarFlinchDamageGain);
	// The cap is never under the floor (a hit's least flinch is 0.6).
	Tuning.FlinchMax = FMath::Max(CVarFlinchMax.GetValueOnGameThread(), 0.6f);
	Tuning.FlinchCritScale = NotBelowZero(CVarFlinchCritScale);
	Tuning.FlinchBossScale = NotBelowZero(CVarFlinchBossScale);
	Tuning.FlinchSlow = FMath::Clamp(CVarFlinchSlow.GetValueOnGameThread(), 0.05f, 1.f);
	Tuning.FlinchSlowSeconds = NotBelowZero(CVarFlinchSlowSeconds);

	Tuning.StaggerHeavyShare = FMath::Max(CVarStaggerHeavyShare.GetValueOnGameThread(), 0.01f);
	Tuning.StaggerCooldown = NotBelowZero(CVarStaggerCooldown);
	Tuning.StaggerSeconds = NotBelowZero(CVarStaggerSeconds);
	Tuning.StaggerShove = NotBelowZero(CVarStaggerShove);

	Tuning.HitStopCrit = NotBelowZero(CVarHitStopCrit);
	Tuning.HitStopHeavy = NotBelowZero(CVarHitStopHeavy);
	Tuning.HitStopKill = NotBelowZero(CVarHitStopKill);
	Tuning.HitStopCritKill = NotBelowZero(CVarHitStopCritKill);

	Tuning.KillShotgunKnock = NotBelowZero(CVarKillShotgunKnock);
	Tuning.KillShotgunLift = NotBelowZero(CVarKillShotgunLift);
	Tuning.KillRifleKnock = NotBelowZero(CVarKillRifleKnock);
	Tuning.KillRevolverBurst = FMath::Max(CVarKillRevolverBurst.GetValueOnGameThread(), 0.1f);

	Tuning.SpurtScale = NotBelowZero(CVarSpurtScale);
	Tuning.SpurtExit = NotBelowZero(CVarSpurtExit);
	Tuning.SpurtBlastBudget = NotBelowZero(CVarSpurtBlastBudget);

	Tuning.RiflePressureHits = FMath::Clamp(CVarRiflePressureHits.GetValueOnGameThread(), 1, 32);
	Tuning.RiflePressureWindow = NotBelowZero(CVarRiflePressureWindow);
	Tuning.ShotgunKnockShare = FMath::Max(CVarShotgunKnockShare.GetValueOnGameThread(), 0.01f);
	Tuning.ShotgunKnockRange = NotBelowZero(CVarShotgunKnockRange);
	Tuning.ShotgunKnockSize = NotBelowZero(CVarShotgunKnockSize);
	Tuning.ShotgunKnockSpeed = NotBelowZero(CVarShotgunKnockSpeed);
	Tuning.ShotgunKnockHeight = FMath::Max(CVarShotgunKnockHeight.GetValueOnGameThread(), 1.f);
	Tuning.ShotgunKnockHold = NotBelowZero(CVarShotgunKnockHold);
	Tuning.RevolverCritHitStop = NotBelowZero(CVarRevolverCritHitStop);
	return Tuning;
}

float GunfeelReact::BlastSpurtShare(float Budget, float Spent, float Want)
{
	return FMath::Clamp(Budget - Spent, 0.f, FMath::Max(Want, 0.f));
}

float GunfeelReact::KnockDistance(float Speed, float Height, float Gravity)
{
	// Up fast enough to top out at Height, so back down at its start's height after twice the climb's time.
	const float G = FMath::Max(Gravity, 1.f);
	const float Air = 2.f * FMath::Sqrt(2.f * G * FMath::Max(Height, 1.f)) / G;
	return FMath::Max(Speed, 0.f) * Air;
}
