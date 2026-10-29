#pragma once

#include "CoreMinimal.h"

/**
 * Every new number of how an enemy takes a hit you can feel (HITS, 2026-10-28; Docs/Work/2026-10-28-Core/HITS.md): the
 * far boost, poise, the elites' stagger gap, the Big hit and the overkill, and one switch per layer so a morning undo is one
 * console line. Each is a Looter.Hits.<Field> console variable; the fields' own values are the cvars' defaults. The rules
 * (FHitReaction, HitFeel::*) take one of these, so tests drive them with the defaults (a default-made one), never with what
 * the user has set; play reads Live() at each hit (a kind's pose code may read it each frame: it's a handful of cvar reads).
 * The older numbers (flinch, stagger, hit-stops, kills by gun) stay FGunfeelReactTuning's (Looter.Gunfeel.*).
 */
struct AI_LOOTER_SHOOTER_API FHitFeelTuning
{
	// --- The far boost: a flinch reads on screen however far the shooter stands ---
	/** Looter.Hits.FarBoost: a hit from FarFar cm or more flinches this many times as much (x1 within FarNear; 1 = off). */
	float FarBoost = 1.5f;
	/** Looter.Hits.FarNear / .FarFar: the boost climbs from x1 at FarNear cm to FarBoost at FarFar cm (the shooter's distance). */
	float FarNear = 1000.f;
	float FarFar = 2500.f;

	// --- Poise: a stagger by the rate of damage, not just one heavy hit ---
	/** Looter.Hits.PoiseBreak: poise (each hit's share of health, added up) this high, times its rank's scale, staggers it (0 = off). */
	float PoiseBreak = 0.25f;
	/** Looter.Hits.PoiseDrain: poise drains this much a second (so a Basic one staggers at ~30%+ of its health a second sustained). */
	float PoiseDrain = 0.3f;
	/** Looter.Hits.StaggerGap: Gravebound and tougher are never staggered again sooner than this after a stagger began (s; no stun-lock). */
	float StaggerGap = 1.6f;

	// --- The Big hit (a single hit worth BigShare, a sniper's or a rocket's, a point-blank blast) ---
	/** Looter.Hits.BigShare: a single hit worth this share of its health is Big. */
	float BigShare = 0.25f;
	/** Looter.Hits.BigShove: a Big hit that neither kills nor staggers shoves a small one (size <= the shotgun's KnockSize) back this fast (cm/s; 0 = off). */
	float BigShove = 300.f;
	/** Looter.Hits.HitStopBig: a Big hit's hit-stop (s), cooled down as a crit's. */
	float HitStopBig = 0.06f;

	// --- The overkill (a killing hit worth OverkillShare of its health past dead, or a Big kill) ---
	/** Looter.Hits.OverkillShare: a killing hit this far past dead (of its max health) is an overkill. */
	float OverkillShare = 0.5f;
	/** Looter.Hits.HitStopOverkill: an overkill's hit-stop (s): the anticipation frame of a body giving way all at once. */
	float HitStopOverkill = 0.13f;
	/** Looter.Hits.OverkillKnock: an overkill's corpse knock, times the gun's. */
	float OverkillKnock = 1.5f;
	/** Looter.Hits.OverkillLift: an overkill by a shotgun or a rocket lifts the corpse this high at the arc's top (cm). */
	float OverkillLift = 60.f;

	// --- The layers' switches (Looter.Hits.<Name> 1 = on, 0 = off) ---
	/** Zone and side: off, every hit is a Body hit from the Front with no twist (FCreatureHit::Zone, Side, LateralOffset). */
	bool Zones = true;
	/** Wound stages (looks only: limps, drips, a guttering coal). The kinds read it. */
	bool Wounds = true;
	/** Kill styles (lights out, torn, popped...) and the Overkill weight, its hit-stop and its knock. */
	bool KillStyles = true;
	/** The Unpaid's hats knocked off. Lane C reads it. */
	bool Hat = true;
	/** The stagger pose (GetStaggerAlpha 0 when off: the brain still pauses as ever). */
	bool StaggerPose = true;

	// --- Each kind's new hit poses, times (Looter.Hits.SpiderScale / .UnpaidScale / .SlimeScale; 1 as planned) ---
	float SpiderScale = 1.f;
	float UnpaidScale = 1.f;
	float SlimeScale = 1.f;

	/** The cvars as set now. */
	static FHitFeelTuning Live();
};
