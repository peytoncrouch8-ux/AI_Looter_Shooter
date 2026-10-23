#pragma once

#include "CoreMinimal.h"

/**
 * Every scale knob of the VFX pass (Saved/Vfx/PLAN.md): the Looter.Vfx.* console variables, read live (PIE) through
 * Live(); the fields' own values are the cvars' defaults, so the rules' tests (a default-made one) and a fresh game agree.
 * These scale only what the pass adds or changes; the Gunfeel spurt cvars (Looter.Gunfeel.Spurt.*) still size the spills.
 */
struct AI_LOOTER_SHOOTER_API FVfxTuning
{
	// --- Strength of each family of looks (1 = as designed, 0 = the pass's layers off) ---
	/** Looter.Vfx.Hit.Scale: the creature hits' new layers (pop, chunks, spray streaks, crit ring), counts and sizes. */
	float HitScale = 1.f;
	/** Looter.Vfx.Hit.Pop: the starburst every hit flashes (bright at range, what reads a hit at 40 m). */
	float HitPop = 1.f;
	/** Looter.Vfx.Impact.Scale: the world hits' new layers (dirt geysers, stone jets, metal pings). */
	float ImpactScale = 1.f;
	/** Looter.Vfx.Kill.Scale: the kill layers on top of each kind's death burst (gore, mist, rings, tiers). */
	float KillScale = 1.f;
	/** Looter.Vfx.Kill.Ring: the shock rings of a kill (soul-light and tiers; 0 none). */
	float KillRing = 1.f;
	/** Looter.Vfx.Muzzle.Scale: the smoke, sparks, side jets and rings round a shot's flash. */
	float MuzzleScale = 1.f;

	// --- The muzzle flash itself (AWeaponBase's quads on M_FX_Muzzle; the rework, 2026-10-22) ---
	/** Looter.Vfx.Muzzle.Flash / .Glow / .Time: the flash's size, strength and how long it shows, times. */
	float MuzzleFlash = 1.f;
	float MuzzleGlow = 1.f;
	float MuzzleTime = 1.f;
	/** Looter.Vfx.Muzzle.FadePower: how fast it fades over its time ((1 - age share) ^ this; higher is quicker). */
	float MuzzleFadePower = 2.f;
	/** Looter.Vfx.Muzzle.Jitter: each shot's flash this share bigger or smaller at random. */
	float MuzzleJitter = 0.15f;
	/**
	 * Looter.Vfx.Muzzle.Star1P / .Tongue1P / .Bloom1P / .Jets1P: the layers' sizes in the player's own view (on top of
	 * Looter.Gunfeel.Muzzle.Scale1P). Anything reaching along the barrel runs toward the crosshair there, so the tongues are short.
	 */
	float MuzzleStar1P = 0.5f;
	float MuzzleTongue1P = 0.3f;
	float MuzzleBloom1P = 0.5f;
	float MuzzleJets1P = 0.35f;
	/** Looter.Vfx.Muzzle.Aim / .AimGlow: the flash's size and strength aimed down the sights (first person). */
	float MuzzleAim = 0.55f;
	float MuzzleAimGlow = 0.6f;
	/**
	 * Looter.Vfx.Muzzle.Clear / .AimClear (degrees): round the crosshair nothing of the flash shows, from the hip / aimed (the
	 * flash shrinks to keep out, down to Looter.Vfx.Muzzle.ClearMin of its size, and M_FX_Muzzle fades what still reaches in).
	 */
	float MuzzleClear = 3.5f;
	float MuzzleAimClear = 2.f;
	float MuzzleClearMin = 0.15f;
	/** Looter.Vfx.Muzzle.Sparks1P / .Smoke1P: the share of a shot's sparks, and its smoke's opacity, in the player's own view. */
	float MuzzleSparks1P = 0.67f;
	float MuzzleSmoke1P = 0.75f;
	/** Looter.Vfx.Muzzle.Light: the flash's point light (the one it always had; 0 none). */
	float MuzzleLight = 1.f;
	/**
	 * Looter.Vfx.Muzzle.Max1P (degrees): in the player's own view the flash never reaches farther than this from its muzzle
	 * on screen (a revolver drawn big and close flashed over half the screen; the play-check, 2026-10-22).
	 */
	float MuzzleMax1P = 8.f;

	// --- Sparks and tracers as soft streaks (FWeaponFX's SoftStreaks on M_FX_Muzzle; the play-check, 2026-10-22: their flat
	// M_FX_Glow diamonds covered close targets) ---
	/** Looter.Vfx.SoftStreaks: sparks and tracers on M_FX_Muzzle's soft tapered streak when it's in the game (read as a level's effects start). */
	bool bSoftStreaks = true;
	/** Looter.Vfx.Spark.Glow / .Width / .Length: a spark's strength there, times; its greatest width and length on screen (radians). */
	float SparkGlow = 0.6f;
	float SparkWidth = 0.004f;
	float SparkLength = 0.06f;
	/** Looter.Vfx.Tracer.Glow / .Width: a tracer's strength there, times; its greatest width on screen (radians; the least is the bullets'). */
	float TracerGlow = 1.f;
	float TracerWidth = 0.006f;
	// --- A world hit's weight by its gun's kind (the gun feel pass, 2026-10-22: impacts looked the same from every gun) ---
	/**
	 * Looter.Vfx.Impact.Weight.<Kind>: how heavy a round's world hit is drawn, a rifle round's 1: its pieces times this, their
	 * size and throw times this ^ Impact.SizePower. The shotgun's is each pellet's share of its blast (its size stays the mark's).
	 */
	float ImpactWeightRifle = 1.f;
	float ImpactWeightShotgun = 0.5f;
	float ImpactWeightRevolver = 1.3f;
	float ImpactWeightSMG = 0.6f;
	float ImpactWeightSniper = 1.8f;
	float ImpactWeightLMG = 1.05f;
	/** Looter.Vfx.Impact.SizePower: how a weight sizes the pieces (0.5: a sniper's 1.8 throws them 1.34x as big). */
	float ImpactSizePower = 0.5f;
	/** Looter.Vfx.Impact.LMGFullEvery / .LMGLight: an LMG's every Nth round (its tracer's beat) a full hit with a chunk burst, the rest this share of it. */
	int32 ImpactLMGFullEvery = 3;
	float ImpactLMGLight = 0.6f;
	/** Looter.Vfx.Impact.ChunkFrom: from this weight a hit on a hard surface throws an extra burst of it (the revolver's, the sniper's). */
	float ImpactChunkFrom = 1.2f;
	/** Looter.Vfx.Impact.PunchFrom: from this weight a hit punches (a ring of air and dust, a tall column, big chunks: the sniper's). */
	float ImpactPunchFrom = 1.5f;
	/** Looter.Vfx.Impact.Sheet: a shotgun blast's one dust sheet across its pattern, times (0 none). */
	float ImpactSheet = 1.f;
	/** Looter.Vfx.Impact.SheetWait (s) / .SheetMin: the sheet waits this long at most for the blast's last pellet; fewer pellets than this on the world, none. */
	float ImpactSheetWait = 0.12f;
	int32 ImpactSheetMin = 3;
	/** Looter.Vfx.Hit.CritPop: a crit's own look by what the body spills (shell shards, gobs, a gel ring, a coal's flare, rock chips, clods), times. */
	float HitCritPop = 1.f;
	/**
	 * Looter.Vfx.Hit.FlashMax (radians): a creature hit's glows (its pop, a crit's flash and ring) never look wider than this on
	 * screen (the play-check, 2026-10-23: a shotgun blast's crit flashes at 6 m made a white disc over the spider; 0: no cap).
	 */
	float HitFlashMax = 0.03f;
	/** Looter.Vfx.Hit.GlowWindow (s) / .GlowReach (cm): glows at one wound this close share their light (each 1/n): steady fire and a blast never stack into white. */
	float HitGlowWindow = 0.2f;
	float HitGlowReach = 60.f;
	/** Looter.Vfx.Hit.PelletGlow: a pellet's glows, times (nine land together). */
	float HitPelletGlow = 0.5f;
	/** Looter.Vfx.Hit.DropGlow: how much the spiders' ichor drops glow (read as a level's effects start; 0.35 read as lime chips). */
	float HitDropGlow = 0.1f;

	/** Looter.Vfx.Power.Scale: the ember powers' new punch (activation rings, body hits, blasts, strikes). */
	float PowerScale = 1.f;
	/** Looter.Vfx.Blast.Scale: explosions (a rocket's, Slag Bomb's, Last Rites'). */
	float BlastScale = 1.f;
	/** Looter.Vfx.Glow: every new glow's strength (rings, fireballs, starbursts). */
	float Glow = 1.f;
	/** Looter.Vfx.Smoke: every new puff's opacity (mist, geysers, columns). */
	float Smoke = 1.f;

	// --- Detail by distance and in a stream of fire (the cost side) ---
	/** Looter.Vfx.Detail.Near / .Far (cm): full detail within Near of the camera, DetailMin's share from Far on. */
	float DetailNear = 1500.f;
	float DetailFar = 6000.f;
	/** Looter.Vfx.Detail.Min: the least share of a look's pieces far off (a hit's pop is never thinned). */
	float DetailMin = 0.35f;
	/** Looter.Vfx.Stream.Window (s) / .Reach (cm): hits this close together count as one stream (thinned). */
	float StreamWindow = 0.3f;
	float StreamReach = 150.f;
	/** Looter.Vfx.MaxParticles: FWeaponFX's pool (the oldest gives way past it). */
	int32 MaxParticles = 1000;

	// --- What draws them (read as a world's effects start) ---
	/** Looter.Vfx.Shapes: rings, fireballs and starbursts on M_FX_Shape when it's in the game (0: M_FX_Glow's stand-ins). */
	bool bShapes = true;
	/** Looter.Vfx.Puffs: smoke on M_FX_Puff (noise-broken, eroding) when it's in the game (0: M_FX_Smoke's soft discs). */
	bool bPuffs = true;
	/** Looter.Vfx.Fire: blasts' fire and soot on M_FX_Fire (alpha-blended) when it's in the game (0: heat-tinted puffs). */
	bool bFire = true;
	/** Looter.Vfx.BurnTint: a power's burn tint on bodies (Slag Bomb's pool: the body drawn again in an overlay). */
	bool bBurnTint = true;
	/** Looter.Vfx.Splats: the ground splats under kills and blasts (decals). */
	bool bSplats = true;
	/** Looter.Vfx.Splat.Max / .Hold (s) / .Fade (s): the splats' pool, how long one holds, how long it fades. */
	int32 SplatMax = 12;
	float SplatHold = 9.f;
	float SplatFade = 3.f;

	/** The cvars as set now. */
	static FVfxTuning Live();
};
