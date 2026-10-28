#pragma once

#include "CoreMinimal.h"

/**
 * The Unpaid's fighting kit (round 2, 2026-10-27; the user: "Unpaid have terrible ai, need better attacks and targeting
 * sequences", "most mobs are not frightening enough"): every number is a Looter.Unpaid.* console variable, live in PIE; the
 * fields' own values are the defaults, so the rules' tests (a default-made tuning) and a fresh game agree. Distances are cm
 * at size 1 (a bigger rank's body scales them), times seconds. AUnpaidCreature plays them (UnpaidCreatureMoves.cpp).
 *  - Lunge: aimed where the target will be (LeadShare of the way, at most LeadMaxSeconds ahead), started from farther
 *    out (LungeStart), flying farther (LungeDistance) and bending a little after a dodging target (LungeHoming).
 *  - Rake: close in (RakeRange) it claws twice, one arm then the other, across its front.
 *  - Cinder: waiting its turn on the ring it throws a burning coal at the player now and then (a slow glowing pellet,
 *    UEnemyProjectileSubsystem), after a tell (CinderTell: the coal swells at its chest).
 *  - Flank: given its turn while in front of the player's eyes, it fades and comes back at their side or back.
 *  - Rally: a pack waiting on the ring screams together: a ring of its coal's light runs out, and for FrenzySeconds the
 *    pack comes on at once (FrenzyTokens turns, FrenzyChase pace).
 *  - Howl: the first of a pack to see the player howls, heard far off.
 */
struct AI_LOOTER_SHOOTER_API FUnpaidFightTuning
{
	/** Looter.Unpaid.Kit: 1 gives the Unpaid their fighting kit; 0 is the old one (one shriek, a straight lunge). */
	int32 Kit = 1;

	// --- Leading the target ---
	/**
	 * Looter.Unpaid.LeadShare: how much of the target's run it allows for (0 aims at where they stand, 1 a perfect lead).
	 * DEMO 2026-10-27: at 0.7 a walking strafe was hit by 11 of 13 lunges; a quarter keeps a strafe the answer to it.
	 */
	float LeadShare = 0.25f;
	/** Looter.Unpaid.LeadMaxSeconds: it never leads more than this far ahead (s), so a strafe or a sprint-step still shakes it. */
	float LeadMaxSeconds = 0.2f;

	// --- The lunge ---
	/** Looter.Unpaid.LungeStart: it starts a lunge from this far out (cm; the old kit 380). */
	float LungeStart = 520.f;
	/** Looter.Unpaid.LungeDistance: a lunge flies at most this far (cm; the old 320). */
	float LungeDistance = 470.f;
	/** Looter.Unpaid.LungeHoming: in flight it turns after its target at most this fast (degrees a second); 0: its aim is committed at the end of the wind-up (DEMO: 40 caught strafes). */
	float LungeHoming = 0.f;
	/** Looter.Unpaid.LungeHitRadius: its lunge lands on a target this near its line (cm at size 1; the old kit's 130 caught a strafe). */
	float LungeHitRadius = 85.f;
	/** Looter.Unpaid.LungeDamageShare: a landed lunge hurts this share of its attack (DEMO: 15-18 a hit at level 3 was too lethal). */
	float LungeDamageShare = 0.85f;
	/**
	 * Looter.Unpaid.LungeCommit: the last stretch of a lunge's flight (s at full speed) is committed: the hit slow is held off
	 * it and it flies at its full speed to where it aimed (DEMO 2026-10-28: 59 attacks, 2 hits; every rifle hit's 0.12 s at 55%
	 * kept each lunge short). Before that stretch a slowed one flies slower but still the whole way.
	 */
	float LungeCommit = 0.3f;
	/** Looter.Unpaid.HitSlowGap: an Unpaid takes the hit slow at most once every this many seconds (0: every hit, as other kinds). */
	float HitSlowGap = 0.5f;

	// --- Pressed past its ground's edge (2026-10-28: shot at from the chapel's doorway, nine stood in a line at the edge) ---
	/** Looter.Unpaid.PressReach: hurt (or its pack hurt) from this near past its ground's edge (cm), it fights them there; 0: never. */
	float PressReach = 800.f;
	/** Looter.Unpaid.PressSeconds: a press lasts this long once they're out of that reach (s); within it, it's renewed. */
	float PressSeconds = 6.f;

	// --- The rake (the second attack) ---
	/** Looter.Unpaid.RakeRange: nearer than this (cm) it may rake instead of lunging. */
	float RakeRange = 260.f;
	/** Looter.Unpaid.RakeChance: the chance it rakes when it's that near. */
	float RakeChance = 0.7f;
	/** Looter.Unpaid.RakeWindup: its arm drawn back before the first swipe (s), before its rank's quickening. */
	float RakeWindup = 0.5f;
	/** Looter.Unpaid.RakeGap: the second swipe comes this long (s) after the first. */
	float RakeGap = 0.32f;
	/** Looter.Unpaid.RakeRecovery: after the second swipe it hangs this long (s) before its next move. */
	float RakeRecovery = 0.55f;
	/** Looter.Unpaid.RakeRadius / RakeArc: a swipe reaches this far (cm) in an arc this wide (degrees) across its front. */
	float RakeRadius = 240.f;
	float RakeArc = 130.f;
	/** Looter.Unpaid.RakeDamageShare: each swipe hurts this share of its attack (two land: about one old lunge). */
	float RakeDamageShare = 0.5f;
	/** Looter.Unpaid.RakeDrift: it drifts in this fast (cm/s) while it swipes, so a slow backpedal still gets caught. */
	float RakeDrift = 240.f;

	// --- The cinder (pressure from the ring) ---
	/** Looter.Unpaid.CinderWait: it throws only after waiting on the ring this long (s). */
	float CinderWait = 1.2f;
	/** Looter.Unpaid.CinderCooldown: one Unpaid throws at most once every this many seconds (each throw jittered 0.8-1.2x). */
	float CinderCooldown = 6.f;
	/** Looter.Unpaid.CinderGap: cinders at one player at least this far apart (s), however many throw. */
	float CinderGap = 2.f;
	/** Looter.Unpaid.CinderTell: the coal swells at its chest this long (s) before it throws: time to see it and move. */
	float CinderTell = 0.7f;
	/** Looter.Unpaid.CinderSpeed / CinderRadius: how fast it flies (cm/s) and how big it is (cm). */
	float CinderSpeed = 1500.f;
	float CinderRadius = 16.f;
	/** Looter.Unpaid.CinderDamageShare: a cinder hurts this share of its attack. */
	float CinderDamageShare = 0.4f;
	/** Looter.Unpaid.CinderMinRange / CinderMaxRange: it throws at a player this far off (cm), never nearer or farther. */
	float CinderMinRange = 450.f;
	float CinderMaxRange = 2200.f;

	// --- The flank ---
	/** Looter.Unpaid.FlankChance: waiting on the ring in front of the player's eyes, its next turn opens with a flank with this chance (rolled once a wait). */
	float FlankChance = 0.6f;
	/** Looter.Unpaid.FlankFront: "in front of their eyes": its bearing's frontness in their view at least this (0 behind, 1 dead ahead; 0.6 is within about 78 degrees). */
	float FlankFront = 0.6f;
	/** Looter.Unpaid.FlankMinRange / FlankMaxRange: it flanks a player this far off (cm; the ring is 4.8 m; a turn held farther than about 11.5 m is taken back anyway). */
	float FlankMinRange = 300.f;
	float FlankMaxRange = 1100.f;
	/** Looter.Unpaid.FlankRadius: it comes back this far (cm) from the player, at their side or behind them. */
	float FlankRadius = 330.f;
	/** Looter.Unpaid.FlankCooldown: one Unpaid flanks at most once every this many seconds. */
	float FlankCooldown = 7.f;

	// --- The rally scream and the frenzy ---
	/** Looter.Unpaid.RallyMinPack: at least this many Unpaid hunting one player before one screams them on. */
	int32 RallyMinPack = 3;
	/** Looter.Unpaid.RallyWait: it screams only after waiting on the ring this long (s). */
	float RallyWait = 2.5f;
	/** Looter.Unpaid.RallyCooldown: one player hears a rally at most once every this many seconds. */
	float RallyCooldown = 16.f;
	/** Looter.Unpaid.RallyTell: the scream lasts this long (s) before the pack comes: the warning. */
	float RallyTell = 0.8f;
	/** Looter.Unpaid.RallyRing: the scream's ring of light runs out this far (cm). */
	float RallyRing = 1400.f;
	/** Looter.Unpaid.FrenzySeconds: after the scream, the pack comes on together this long (s). */
	float FrenzySeconds = 4.f;
	/** Looter.Unpaid.FrenzyTokens: melee turns at that player during the frenzy (Looter.Fight.MeleeTokens otherwise). */
	int32 FrenzyTokens = 4;
	/** Looter.Unpaid.FrenzyChase: the pack chases this many times faster in the frenzy. */
	float FrenzyChase = 1.2f;

	// --- Closing in ---
	/** Looter.Unpaid.ClosingScale / ClosingRange: farther than this (cm) from its target it chases this many times faster. */
	float ClosingScale = 1.2f;
	float ClosingRange = 900.f;

	// --- The howl ---
	/** Looter.Unpaid.HowlCooldown: one player hears a howl at most once every this many seconds. */
	float HowlCooldown = 12.f;

	// --- Tells ---
	/** Looter.Unpaid.TellFlare: the coal flares up to this through every wind-up (the material's Heat; a crit's flare is 0.8). */
	float TellFlare = 0.6f;

	/** The cvars' values now (read at each ask, so a change in PIE counts at once). */
	static FUnpaidFightTuning Live();
	/** Looter.Unpaid.Kit alone, now: asked many times a frame. */
	static bool IsKitOn();
};

/** The kit's sounds (Art/Sounds/recipes/unpaid_kit.py). Each plays nothing until it's rendered and imported. */
namespace UnpaidKitCue
{
	/** A claw's swish and a dry rattle of old bones, one swipe. */
	inline constexpr const TCHAR* Rake = TEXT("Creature.Unpaid.Rake");
	/** The coal swelling at its chest: a crackle and a drawn breath (the cinder's tell). */
	inline constexpr const TCHAR* Cinder = TEXT("Creature.Unpaid.Cinder");
	/** A cold whisper where it will come back (the flank's tell, heard from that side). */
	inline constexpr const TCHAR* Flank = TEXT("Creature.Unpaid.Flank");
	/** The pack's scream: many voices, rising, far-carrying. */
	inline constexpr const TCHAR* Rally = TEXT("Creature.Unpaid.Rally");
	/** The first sight of the player: a long howl heard far off. */
	inline constexpr const TCHAR* Howl = TEXT("Creature.Unpaid.Howl");
}
