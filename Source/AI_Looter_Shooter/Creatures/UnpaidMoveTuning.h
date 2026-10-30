#pragma once

#include "CoreMinimal.h"

/**
 * The Unpaid's varied fight and its failed-lunge shriek (2026-10-29, the user's brief): every number a Looter.Unpaid.* console
 * variable, live in PIE; the fields' own values are the defaults, so the rules' tests (a default-made tuning) and a fresh game
 * agree. Distances are cm at size 1 (a bigger rank's body scales them), times seconds. Plain Unpaid with their kit only
 * (AUnpaidCreature::HasFightKit: every rank; never a boss, a boss's add, or the kinds on its rig). The older kit's numbers
 * (the lunge, the rake's swipes, cinders, flanks, rallies) stay FUnpaidFightTuning's.
 */
struct AI_LOOTER_SHOOTER_API FUnpaidMoveTuning
{
	/** Looter.Unpaid.Variety: 1 the varied moves and the failed-lunge shriek; 0 the kit as it was (rake close in, else lunge). */
	int32 Variety = 1;

	// --- Each move's weight in a turn's pick (Looter.Unpaid.Weight.*), before range, cooldown and the pack's rules ---
	float LungeWeight = 1.f;
	float RakeWeight = 1.4f;
	float FeintWeight = 0.55f;
	float PhaseStrikeWeight = 0.6f;
	float LowSwipeWeight = 0.7f;
	float HangBackWeight = 0.35f;

	// --- Each move's own cooldown per Unpaid (s; Looter.Unpaid.Cooldown.*): the lunge has none ---
	float RakeCooldown = 2.f;
	float FeintCooldown = 5.f;
	float PhaseStrikeCooldown = 7.f;
	float LowSwipeCooldown = 4.5f;
	float HangBackCooldown = 6.f;

	// --- The pack (one pressures while another flanks) ---
	/** Looter.Unpaid.PairBoost: a phase-strike weighs this many times more while a pack-mate comes at the player from the front. */
	float PairBoost = 2.2f;
	/** Looter.Unpaid.PressBoost: the frontal moves weigh this many times more while a pack-mate is at the player's side or back. */
	float PressBoost = 1.4f;
	/** Looter.Unpaid.MissFeintBoost: after its lunge missed, a feint weighs this many times more (it reads the dodge). */
	float MissFeintBoost = 1.6f;

	// --- Closing in for a close move ---
	/** Looter.Unpaid.CloseInRange: a rake or a low swipe may be planned from this near (cm); it closes in to its start first. */
	float CloseInRange = 560.f;
	/** Looter.Unpaid.CloseInSeconds: closing in longer than this (a backpedalling player), it lunges or lets the turn go. */
	float CloseInSeconds = 1.4f;
	/** Looter.Unpaid.CloseInPace: it closes in this many times its chase. */
	float CloseInPace = 1.15f;
	/** Looter.Unpaid.CloseInShare: planned from beyond its start, a close move weighs this share of its weight. */
	float CloseInShare = 0.6f;

	/** Looter.Unpaid.LungeMinRange: nearer than this (cm) it doesn't lunge (it rakes or swipes). */
	float LungeMinRange = 150.f;

	// --- The feint (the lunge's tell; it flies part way, stops short and slides aside) ---
	/** Looter.Unpaid.Feint.MinRange: it feints only from this far (cm). */
	float FeintMinRange = 260.f;
	/** Looter.Unpaid.Feint.Share: it flies this share of the lunge's way... */
	float FeintShare = 0.45f;
	/** Looter.Unpaid.Feint.Keep: ...never nearer the player than this (cm). */
	float FeintKeep = 230.f;
	/** Looter.Unpaid.Feint.Slide / .SlideSeconds: then slides aside this far (cm) over this long (s). */
	float FeintSlide = 260.f;
	float FeintSlideSeconds = 0.4f;

	// --- The phase-strike (fades out, comes back at their side or behind, claws after its own tell) ---
	/** Looter.Unpaid.PhaseStrike.MinRange: it blinks to strike only from this far (cm). */
	float PhaseStrikeMinRange = 250.f;
	/** Looter.Unpaid.PhaseStrike.Tell: back in, it holds its claw up this long (s) before it strikes: the tell. */
	float PhaseStrikeTell = 0.4f;
	/** Looter.Unpaid.PhaseStrike.Land: it comes back this far from the player (cm). */
	float PhaseStrikeLand = 170.f;
	/** Looter.Unpaid.PhaseStrike.Reach / .Arc: its claw reaches this far (cm), across this arc (degrees). */
	float PhaseStrikeReach = 240.f;
	float PhaseStrikeArc = 150.f;
	/** Looter.Unpaid.PhaseStrike.DamageShare: a landed claw hurts this share of its attack. */
	float PhaseStrikeDamageShare = 0.9f;
	/** Looter.Unpaid.PhaseStrike.Recovery: after the claw it hangs this long (s). */
	float PhaseStrikeRecovery = 0.6f;

	// --- The low swipe (sinks low, skims in, claws at the feet: jump it) ---
	/** Looter.Unpaid.LowSwipe.Start / .MinRange: it skims in from this near (cm), never from nearer than MinRange. */
	float LowSwipeStart = 400.f;
	float LowSwipeMinRange = 120.f;
	/** Looter.Unpaid.LowSwipe.Windup: sunk low, claws spread along the ground (s), before its rank's quickening. */
	float LowSwipeWindup = 0.55f;
	/** Looter.Unpaid.LowSwipe.Speed / .Distance / .Past: it skims this fast (cm/s), at most this far (cm), on past the player this far. */
	float LowSwipeSpeed = 1150.f;
	float LowSwipeDistance = 480.f;
	float LowSwipePast = 80.f;
	/** Looter.Unpaid.LowSwipe.Radius: it catches feet this near its line (cm). */
	float LowSwipeRadius = 120.f;
	/** Looter.Unpaid.LowSwipe.Clear: feet this high over its own (cm) pass over it (a jump). */
	float LowSwipeClear = 30.f;
	/** Looter.Unpaid.LowSwipe.DamageShare / .Recovery: a landed swipe hurts this share; it hangs this long after (s). */
	float LowSwipeDamageShare = 0.8f;
	float LowSwipeRecovery = 0.6f;

	// --- The hang-back (its turn given back, it circles wide until a pack-mate commits) ---
	/** Looter.Unpaid.HangBack.Seconds / .Min: it hangs back at most this long (s), at least this long before a pack-mate's commit ends it. */
	float HangBackSeconds = 2.2f;
	float HangBackMin = 0.6f;
	/** Looter.Unpaid.HangBack.Out / .Pace: it circles this far outside the ring (cm), at this share of its chase. */
	float HangBackOut = 220.f;
	float HangBackPace = 0.55f;

	// --- The failed-lunge shriek (Looter.Unpaid.Shriek.*) ---
	/** .Chance: a missed lunge turns into a shriek this often; .Cooldown per Unpaid (s); .PackGap per pack at one player (s). */
	float ShriekChance = 0.35f;
	float ShriekCooldown = 8.f;
	float ShriekPackGap = 4.f;
	/** .Tell / .Scream / .Recover: its three parts (s). Shooting the coal in the tell interrupts it. */
	float ShriekTell = 0.5f;
	float ShriekScream = 1.2f;
	float ShriekRecover = 0.6f;
	/** .RattleRadius: players this near (cm, with a clear line) are rattled: a short aim sway and a slight slow, never a stun. */
	float ShriekRattleRadius = 600.f;
	/** .FeltRadius: farther, to this (cm), they still feel it: the shake (falling off), the arc on the HUD, a breath of frost. */
	float ShriekFeltRadius = 1600.f;
	/** .RallyRadius / .RallySeconds: other Unpaid this near (cm) are rallied (Looter.Unpaid.FrenzyChase pace) this long (s). */
	float ShriekRallyRadius = 1200.f;
	float ShriekRallySeconds = 3.f;
	/** .Slow / .SlowSeconds: a rattled player moves at this share of their speed this long (s), at full rattle. */
	float ShriekSlow = 0.85f;
	float ShriekSlowSeconds = 1.f;
	/** .Sway / .SwaySeconds: their aim sways this far (degrees) over this long (s), at full rattle. */
	float ShriekSway = 1.1f;
	float ShriekSwaySeconds = 0.9f;
	/** .Shake: the camera's shake at full rattle (0-1; the player's camera-shake setting scales it). */
	float ShriekShake = 0.55f;
	/** .RattleGap: one player is swayed and slowed at most once every this many seconds, however many shriek. */
	float ShriekRattleGap = 3.f;
	/** .ExposedSeconds / .ExposedCoal: its coal shot in the tell, its crit spot is this many times bigger this long (s). */
	float ShriekExposedSeconds = 1.5f;
	float ShriekExposedCoal = 2.2f;
	/** .Light / .Fx: its coal's light (0 none) and its particles' share (0 none): the performance knobs. */
	float ShriekLight = 1.f;
	float ShriekFx = 1.f;

	/** The shriek's whole length (s). */
	float ShriekLength() const { return ShriekTell + ShriekScream + ShriekRecover; }

	/** The cvars' values now (read at each ask, so a change in PIE counts at once). */
	static FUnpaidMoveTuning Live();
	/** Looter.Unpaid.Variety alone, now: asked many times a frame. */
	static bool IsVarietyOn();
};
