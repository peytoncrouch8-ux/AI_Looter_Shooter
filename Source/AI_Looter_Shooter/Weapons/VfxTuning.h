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
	/** Looter.Vfx.Splats: the ground splats under kills and blasts (decals). */
	bool bSplats = true;
	/** Looter.Vfx.Splat.Max / .Hold (s) / .Fade (s): the splats' pool, how long one holds, how long it fades. */
	int32 SplatMax = 12;
	float SplatHold = 9.f;
	float SplatFade = 3.f;

	/** The cvars as set now. */
	static FVfxTuning Live();
};
