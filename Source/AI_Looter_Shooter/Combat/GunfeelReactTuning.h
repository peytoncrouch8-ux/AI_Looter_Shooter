#pragma once

#include "CoreMinimal.h"

/**
 * Every number of how a creature takes a gun's hit (Gunfeel, Docs/Plan.md 12.1; GUNFEEL-REACT): the flinch sized by the
 * damage, the brief slow, staggers, hit-stops, kills by gun, the hit spurts and each gun's job on the target. Each is a
 * Looter.Gunfeel.* console variable so the user tunes it live in PIE; the fields' own values are the cvars' defaults.
 * The rules (FHitReaction, FCreatureHit) take one of these, so tests drive them with the defaults (a default-made one),
 * never with what the user has set; play reads Live() at each hit, so a change counts at once.
 */
struct AI_LOOTER_SHOOTER_API FGunfeelReactTuning
{
	// --- The flinch on every hit (each kind's OnHurt): FlinchScale x clamp(0.6 + DamageGain x share, 0.6, FlinchMax) ---
	/** Looter.Gunfeel.Flinch.Scale: every flinch at once. */
	float FlinchScale = 1.f;
	/** Looter.Gunfeel.Flinch.DamageGain: how much a hit's share of health adds (a third of its health: 0.6 + 1.0). */
	float FlinchDamageGain = 3.f;
	/** Looter.Gunfeel.Flinch.Max: the most a hit flinches it (before a crit's own). */
	float FlinchMax = 2.5f;
	/** Looter.Gunfeel.Flinch.CritScale: a critical hit's flinch, times. */
	float FlinchCritScale = 1.4f;
	/** Looter.Gunfeel.Flinch.BossScale: a boss flinches this share of today's flinch, whatever the hit (its moves never interrupted). */
	float FlinchBossScale = 1.f;
	/** Looter.Gunfeel.Flinch.Slow / .SlowSeconds: any hit holds its move speed to this share this long (not on a boss). */
	float FlinchSlow = 0.55f;
	float FlinchSlowSeconds = 0.12f;

	// --- Staggers ---
	/** Looter.Gunfeel.Stagger.HeavyShare: a burst worth this share of its health is a heavy hit (a stagger and a hit-stop). */
	float StaggerHeavyShare = 0.12f;
	/** Looter.Gunfeel.Stagger.Cooldown: after a stagger ends, none again for this long (no stun-lock). */
	float StaggerCooldown = 0.7f;
	/** Looter.Gunfeel.Stagger.Seconds: a Basic creature's stagger; the tougher ranks' scale from it. */
	float StaggerSeconds = 0.4f;
	/** Looter.Gunfeel.Stagger.Shove: a stagger shoves a full-size Basic one back this fast (cm/s). */
	float StaggerShove = 420.f;

	// --- Hit-stops (its own clock all but stopped) ---
	/** Looter.Gunfeel.HitStop.Crit / .Heavy / .Kill / .CritKill (seconds). Heavy: a heavy body hit, cooled down as a crit's. */
	float HitStopCrit = 0.05f;
	float HitStopHeavy = 0.035f;
	float HitStopKill = 0.075f;
	float HitStopCritKill = 0.1f;

	// --- Kills by gun ---
	/** Looter.Gunfeel.Kill.ShotgunKnock / .ShotgunLift: a shotgun's kill throws the corpse this fast (cm/s), this high (cm) at the arc's top. */
	float KillShotgunKnock = 650.f;
	float KillShotgunLift = 35.f;
	/** Looter.Gunfeel.Kill.RifleKnock: a rifle's kill crumples where it stands, only this much of a knock (cm/s). */
	float KillRifleKnock = 180.f;
	/** Looter.Gunfeel.Kill.RevolverBurst: a revolver's crit kill bursts at the crit spot this many times as big. */
	float KillRevolverBurst = 1.6f;

	// --- Hit spurts (FWeaponFX::SpawnCreatureHit) ---
	/** Looter.Gunfeel.Spurt.Scale: what a hit spills, times. */
	float SpurtScale = 1.5f;
	/** Looter.Gunfeel.Spurt.Exit: the spray out behind the target along the shot, as a share of the entry's (0: none). */
	float SpurtExit = 0.6f;
	/** Looter.Gunfeel.Spurt.BlastBudget: a shotgun blast's pellets share this many full hits' spurts between them. */
	float SpurtBlastBudget = 2.f;

	// --- The rifle's job: pressure ---
	/** Looter.Gunfeel.Rifle.PressureHits within Looter.Gunfeel.Rifle.PressureWindow seconds on one creature: a stagger. */
	int32 RiflePressureHits = 5;
	float RiflePressureWindow = 1.f;

	// --- The shotgun's job: power ---
	/** Looter.Gunfeel.Shotgun.KnockShare: a blast worth this share of its health, within KnockRange cm, knocks or staggers it. */
	float ShotgunKnockShare = 0.35f;
	float ShotgunKnockRange = 600.f;
	/** Looter.Gunfeel.Shotgun.KnockSize: a creature this size or smaller is knocked back; a bigger one staggered. */
	float ShotgunKnockSize = 1.2f;
	/** Looter.Gunfeel.Shotgun.KnockSpeed / .KnockHeight: the knock's launch across (cm/s) and its arc's top (cm). */
	float ShotgunKnockSpeed = 520.f;
	float ShotgunKnockHeight = 40.f;
	/** Looter.Gunfeel.Shotgun.KnockHold: once down from the knock it gathers itself this long (s) before it comes on again. */
	float ShotgunKnockHold = 0.25f;

	// --- The revolver's job: precision ---
	/** Looter.Gunfeel.Revolver.CritHitStop: a revolver's crit stops its clock this long (instead of HitStopCrit). */
	float RevolverCritHitStop = 0.09f;

	/** The cvars as set now. */
	static FGunfeelReactTuning Live();
};

namespace GunfeelReact
{
	/**
	 * Of a shotgun blast's spurt budget (Budget, Spent so far by its earlier pellets), what a pellet that Wants this much gets:
	 * what's left, never more than it wants and never below 0. Everything is in full hits' worth of particles.
	 */
	AI_LOOTER_SHOOTER_API float BlastSpurtShare(float Budget, float Spent, float Want);

	/** The across speed of a launch whose arc tops out Height cm up (Gravity cm/s^2) turned into the distance it covers. */
	AI_LOOTER_SHOOTER_API float KnockDistance(float Speed, float Height, float Gravity);
}
