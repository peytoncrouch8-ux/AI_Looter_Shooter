#pragma once

#include "CoreMinimal.h"

/**
 * The numbers of the Unpaid's 2026-10-31 round (the user: "their animations need improved upon", "their AI is lackluster", the
 * pick: ambush): their body language (UnpaidGaitRules, UnpaidCreatureGait.cpp), their rise out of the ground (UnpaidRiseRules,
 * UnpaidCreatureRise.cpp) and the pack's punish of an opening (UnpaidOpeningRules, UnpaidCreatureOpening.cpp). Every number a
 * Looter.Unpaid.* console variable, live in PIE; the fields' own values are the defaults, so the tests (a default-made tuning)
 * and a fresh game agree. Distances cm at size 1, times seconds.
 */
struct AI_LOOTER_SHOOTER_API FUnpaidLifeTuning
{
	// --- Its body (Looter.Unpaid.Gait.*): every Unpaid on its rig but a boss ---
	/** Looter.Unpaid.Gait: 1 the lurch, the breath and the head's tilt, the wind-ups' coil, the rake's twist, the hit's loll, the falls; 0 as before. */
	int32 Gait = 1;
	/** Looter.Unpaid.Gait.Lurch / .Tilt: how much of the lurch and of the head's tilt it shows (1 as designed, 0 none). */
	float LurchScale = 1.f;
	float TiltScale = 1.f;

	// --- The rise (Looter.Unpaid.Rise.*): an ambush's dead (RiseAmbush) ---
	/** Looter.Unpaid.Rise.Radius: round the player this far (cm) when their ambush gives no distance of its own. */
	float RiseRadius = 480.f;
	/** Looter.Unpaid.Rise.First / .Gap: the first hand breaks the ground this long after the ambush springs, each next this long after (s). */
	float RiseFirst = 0.35f;
	float RiseGap = 0.45f;
	/** Looter.Unpaid.Rise.Rumble: the ground stirs over each one this long before its hand breaks it (s). */
	float RiseRumble = 0.7f;
	/** Looter.Unpaid.Rise.Window: risers this near together in time (s) are one burst round the player. */
	float RiseWindow = 1.5f;
	/** Looter.Unpaid.Rise.Clear / .Spacing: never nearer the player than this, nor two nearer each other (cm). */
	float RiseClear = 320.f;
	float RiseSpacing = 230.f;
	/** Looter.Unpaid.Rise.Turn: a burst's pattern turns this far either way off the player's view (degrees). */
	float RiseTurn = 20.f;

	// --- The opening (Looter.Unpaid.Opening.*): plain Unpaid with their kit, with a fight director ---
	/** Looter.Unpaid.Opening: 1 the pack punishes a reload, a swap, an empty gun or the flask; 0 never. */
	int32 Opening = 1;
	/** Looter.Unpaid.Opening.Gap: one punish per player every this long (s). */
	float OpeningGap = 6.f;
	/** Looter.Unpaid.Opening.Claim: an opening is punished only within this long of its start (s). */
	float OpeningClaim = 0.6f;
	/** Looter.Unpaid.Opening.Range / .Near: the punisher is within this (cm), behind the player or at a side, or anywhere within Near. */
	float OpeningRange = 900.f;
	float OpeningNear = 300.f;
	/** Looter.Unpaid.Opening.Front: "behind or at a side" is a frontness at most this (FightRules::Frontness; 0.55 about 84 degrees off their view). */
	float OpeningFront = 0.55f;
	/** Looter.Unpaid.Opening.Frenzy: its pack's frenzy at that player this long (s), unless one is on or its leader just fell. */
	float OpeningFrenzy = 2.f;
	/** Looter.Unpaid.Opening.Punish: the punisher's next turn favours its punish this long after its claim (s). */
	float OpeningPunish = 2.5f;

	/** The cvars' values now (read at each ask, so a change in PIE counts at once). */
	static FUnpaidLifeTuning Live();
};

/** The round's sounds (Art/Sounds/recipes/unpaid_rise.py); a cue not yet rendered plays nothing. */
namespace UnpaidLifeCue
{
	/** The ground stirring over one of the buried: a low rumble, soil shifting, a crackle of the coal's heat under it. */
	inline constexpr const TCHAR* RiseRumble = TEXT("Creature.Unpaid.RiseRumble");
	/** The dirt bursting as its hand claws up out of it: a heavy crack of earth, clods thrown, a gasp under the ground. */
	inline constexpr const TCHAR* RiseBurst = TEXT("Creature.Unpaid.RiseBurst");
	/** Its neck cracking over at rest: a short dry crackle of old bone (near only). */
	inline constexpr const TCHAR* NeckCrack = TEXT("Creature.Unpaid.NeckCrack");
	/** The pack hearing an opening: a sharp, hungry hiss from the punisher. */
	inline constexpr const TCHAR* Opening = TEXT("Creature.Unpaid.Opening");
}
