#pragma once

#include "CoreMinimal.h"

/**
 * Every number of how a fight is run (Phase 12.2, FIGHT-AI; Saved/Fight/CONTRACT.md): attack turns, the surround ring, the
 * ranged kinds' spots, and the difficulty (chase speed, creature damage, the player's regen). Each is a Looter.Fight.* console
 * variable so the user tunes it live in PIE; the fields' own values are the cvars' defaults. The rules (FightRules,
 * FFightTokens) take numbers, so tests drive them with a default-made tuning, never with what the user has set; play reads
 * Live() where it uses them, so a change counts at once.
 */
struct AI_LOOTER_SHOOTER_API FFightTuning
{
	/** Looter.Fight.Director: 1 runs the fight director (turns, ring, ranged spots); 0 is the old free-for-all. */
	int32 Director = 1;

	// --- Attack turns (tokens, per target) ---
	/**
	 * Looter.Fight.MeleeTokens: creatures that may be in a melee attack (asked at the ring, wind-up to recovery) at once. 5 since
	 * the audit's change 4 (2026-11-03; was 3): with 3 a nine-pack hit like three for three times as long and the rest stood in a
	 * line, so a pack now comes on together, its hits smaller (DamageScale).
	 */
	int32 MeleeTokens = 5;
	/** Looter.Fight.RangedTokens: creatures that may be in a spit or shot (tell to recovery) at once (4 since 2026-11-03; was 2). */
	int32 RangedTokens = 4;
	/** Looter.Fight.TokenMaxSeconds: a turn held this long (s) is taken back (a creature that never got there). */
	float TokenMaxSeconds = 3.f;
	/** Looter.Fight.TokenRest: a creature that just attacked waits this long (s) before it asks for a turn again. */
	float TokenRest = 0.6f;
	/** Looter.Fight.TurnKeep: one in line that stops asking (a stagger, a hop) keeps its place this long (s); a staggered holder keeps the head of the line as long. */
	float TurnKeep = 0.5f;
	/** Looter.Fight.RangedGap: two ranged turns on one target begin at least this far apart (s), so spitters' shots come one after another. */
	float RangedGap = 0.35f;

	// --- The surround ring ---
	/**
	 * Looter.Fight.RingRadius: the ring's radius round the target (cm); a creature whose reach is longer waits RingMargin past its
	 * reach. 300 since 2026-11-03 (was 480): a spider (reach 220 + 80) waits at its own reach, not two body lengths out, so the
	 * pack crowds the player and a turn's walk in is short (the slime still waits at 530 past its leap, the Unpaid at 460, was 480).
	 */
	float RingRadius = 300.f;
	/** Looter.Fight.RingMargin: a ring spot is at least this far (cm) past the creature's attack reach. */
	float RingMargin = 80.f;
	/** Looter.Fight.RingSlots: spots round the ring (3 to 32). */
	int32 RingSlots = 8;
	/** Looter.Fight.RingRefresh: the director looks at each fight this often (s): slots, turns' upkeep, feints, ranged spots. */
	float RingRefresh = 0.4f;
	/** Looter.Fight.RingDrift: the ring turns round the target this fast (degrees a second), so the waiting circle prowls. */
	float RingDrift = 25.f;
	/** Looter.Fight.FeintChance: each refresh a waiting creature feints in (a short step in and back) with this chance (0.08 since 2026-11-03, was 0.15: with more turns, fewer bluffs). */
	float FeintChance = 0.08f;
	/** Looter.Fight.FeintDepth / FeintSeconds: a feint comes in to this share of its ring radius and back, over this long (s). */
	float FeintDepth = 0.55f;
	float FeintSeconds = 0.5f;
	/** Looter.Fight.BehindBias: how much a slot in front of the player's view costs against one beside or behind (0: no preference). */
	float BehindBias = 0.35f;
	/** Looter.Fight.RingKeep: a creature's own slot costs this much less, so the ring doesn't reshuffle at every look. */
	float RingKeep = 0.15f;
	/** Looter.Fight.RingMinPack: fewer melee creatures than this hunting one target skip the ring and just come in. */
	int32 RingMinPack = 3;
	/** Looter.Fight.RingApproach: the flank fan brings it in to this many ring radii; there it takes its ring spot and asks for its turn. */
	float RingApproach = 1.6f;

	// --- Ranged kinds ---
	/** Looter.Fight.RangedOffAngle: a ranged spot lies at least this far round the target (degrees) from the melee pack's side. */
	float RangedOffAngle = 50.f;
	/** Looter.Fight.RangedRelocate: after each shot a ranged creature moves to a new spot with this chance. */
	float RangedRelocate = 0.6f;
	/** Looter.Fight.RangedBackOff: a ranged creature backs off from a player nearer than this (cm). */
	float RangedBackOff = 900.f;
	/** Looter.Fight.RangedMoveShare: it walks to a new spot at this share of its chase speed. */
	float RangedMoveShare = 0.7f;

	// --- Pressure (FIGHTS.md section 2, 2026-10-28; FightDirectorPressure.cpp) ---
	/** Looter.Fight.MeleeGap: each kind's own melee gap (ACreatureBase::GetMeleeGap: the slime's 0.45 s), times this; 0 turns every gap off. */
	float MeleeGap = 1.f;
	/** Looter.Fight.StillSeconds / StillRadius: a player who keeps within this circle (cm, flat) this long (s) stands still. */
	float StillSeconds = 2.5f;
	float StillRadius = 150.f;
	/** Looter.Fight.HiddenSeconds: no creature fighting a player has had a clear eye line at them this long (s): they hide. */
	float HiddenSeconds = 2.f;
	/** Looter.Fight.CampMinFighters: standing still or hiding is camping only while this many or more fight them. */
	int32 CampMinFighters = 3;
	/** Looter.Fight.Ease: 1 lets the close-call ease work (one turn fewer of each kind at a nearly dead player); 0 never. */
	int32 Ease = 1;
	/** Looter.Fight.EaseBelow / EaseSeconds / EaseGap: under this share of health and ward, the ease lasts this long (s), once in this long (s). */
	float EaseBelow = 0.25f;
	float EaseSeconds = 4.f;
	float EaseGap = 20.f;

	// --- Pack leaders (PackLeaderRules) ---
	/** Looter.Fight.LeaderReach: a leader's fall shakes the packmates within this of it (cm); its pack counts as led this near it. */
	float LeaderReach = 2000.f;
	/** Looter.Fight.LeaderFalter: they falter this long (s): turns back, wind-ups called off, standing. */
	float LeaderFalter = 1.f;
	/** Looter.Fight.LeaderBreakChance: a Basic spider or slime breaks off at its leader's fall with this chance. */
	float LeaderBreakChance = 0.35f;
	/** Looter.Fight.LeaderLull: after its leader's fall a pack rallies and frenzies no more for this long (s). */
	float LeaderLull = 16.f;

	// --- Difficulty (Borderlands normal) ---
	/** Looter.Fight.ChaseScale: every regular kind chases this many times its own speed (never a boss or Legendary monster). */
	float ChaseScale = 1.3f;
	/**
	 * Looter.Fight.DamageScale: every regular creature's damage, times (bosses and Legendary monsters keep theirs). 1.2 since
	 * 2026-11-03 (1.8 from 2026-10-22, 1.5 before): more turns at once, so each hit smaller (a spider's bite 13 at level 1, was 20).
	 */
	float DamageScale = 1.2f;
	/** Looter.Fight.RegenDelay: seconds unhurt before the player's wounds start to close (was 6). */
	float RegenDelay = 7.f;
	/** Looter.Fight.RegenRate: the share of the player's most health healed a second at the ramp's top: 0.035, about 30 s a bar,
	 *  since the user's playtest call of 2026-10-30 (0.07 from 2026-10-21, 0.12 before). */
	float RegenRate = 0.035f;

	// --- Senses ---
	/**
	 * Looter.Fight.HearGunfire: a player's gunshot is heard this far (cm, 30 m; 0 = nobody hears). A creature within it that is
	 * idle or wandering turns and hunts the shooter (ACreatureBase::HearGunfire, the rule GunfireHearing; 2026-11-03): before,
	 * only its eyes and a hurt packmate's call set one on the player, so a fight never spilled over to the pack next door.
	 */
	float HearGunfire = 3000.f;

	/** The cvars' values now (read at each use, so a change in PIE counts at once). */
	static FFightTuning Live();

	/** Looter.Fight.ChaseScale and DamageScale alone, now: every chaser reads them each frame, so not the whole set. */
	static float LiveChaseScale();
	static float LiveDamageScale();
	/** Looter.Fight.RangedBackOff alone, now (a Spitter reads it each frame). */
	static float LiveRangedBackOff();
	/** Looter.Fight.RangedMoveShare alone, now. */
	static float LiveRangedMoveShare();
	/** Looter.Fight.HearGunfire alone, now (asked at every shot a player fires). */
	static float LiveHearGunfire();
};
