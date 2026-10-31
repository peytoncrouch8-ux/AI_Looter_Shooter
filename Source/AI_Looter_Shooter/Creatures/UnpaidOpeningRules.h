#pragma once

#include "CoreMinimal.h"
#include "Creatures/UnpaidFightRules.h"

/**
 * The Unpaid punish an opening (2026-10-31; the brief: "smarter pack play: flank and come from behind, take turns, punish a
 * reload"). A player reloading, swapping guns, standing on an empty magazine or drinking from the soul flask is open: the
 * pack hears it. One of them (behind the player or at their side, near enough, free to act) claims the punish: it hisses (the
 * tell: heard from where it is, its coal flaring), its pack's frenzy opens for a moment (one more turn at the player at once,
 * the frenzy's pace, its coals high), and every turn picked while the player is open favours a quick commit (the lunge, a
 * phase-strike round to their back) over a feint or a hang-back. One punish per player per Gap: reload behind cover, or away
 * from the pack, and nothing comes. UnpaidCreatureOpening.cpp plays it; plain rules here, so the tests check them.
 */

/** What the pack reads of a player at a look. */
struct AI_LOOTER_SHOOTER_API FUnpaidOpeningRead
{
	bool bReloading = false;
	bool bSwapping = false;
	/** A gun in hand with nothing in it, not yet reloading. */
	bool bEmpty = false;
	bool bDrinking = false;

	bool IsOpen() const { return bReloading || bSwapping || bEmpty || bDrinking; }
};

/** One player's opening at the pack's last look (UUnpaidPackSubsystem keeps one per player). */
struct AI_LOOTER_SHOOTER_API FUnpaidOpeningGate
{
	/** When it last read the player (world s: once a frame, whoever asks), and whether they were open then. */
	double ReadAt = -1.0;
	bool bOpen = false;
	/** When their opening began (world s; below 0: none now). */
	double OpenSince = -1.0;
	/** The last punish at them (world s), and whose (an Unpaid's unique id; 0: none). */
	double LastPunish = -1000.0;
	uint32 Punisher = 0;
	/** This opening may still be punished (its first moments, and the gap since the last allows it). */
	bool bDue = false;
};

namespace UnpaidOpeningRules
{
	/**
	 * The gate moved on to Now with the player bOpenNow: an opening's start makes a punish due when Gap has passed since the
	 * last; it stays due ClaimWindow seconds or until claimed. True on the look an opening begins.
	 */
	AI_LOOTER_SHOOTER_API bool Step(FUnpaidOpeningGate& Gate, bool bOpenNow, double Now, float Gap, float ClaimWindow);

	/**
	 * Whether an Unpaid at Frontness (FightRules::Frontness: 1 in front of the player's view, 0 behind) and Distance (cm, flat)
	 * may claim the punish: within Range, and out of their view's middle (behind or at a side) unless it's right on them (Near).
	 */
	AI_LOOTER_SHOOTER_API bool MayClaim(float Frontness, float Distance, float Range, float Near, float MaxFront);

	/** Id claims Gate's punish at Now: true once an opening, for the first to ask while it's due. */
	AI_LOOTER_SHOOTER_API bool Claim(FUnpaidOpeningGate& Gate, uint32 Id, double Now);

	/** Whether Id is the punisher of Gate's opening now: claimed this opening, within Seconds of its claim. */
	AI_LOOTER_SHOOTER_API bool IsPunisher(const FUnpaidOpeningGate& Gate, uint32 Id, double Now, float Seconds);

	/**
	 * A move's weight's multiplier at a player who is open (bOpen) for an Unpaid that is (bPunisher) or isn't the punisher, in
	 * front of the player's view or not (bInFront): the quick commits up, the feint and the hang-back down; the punisher in
	 * front blinks round to their back, one behind lunges.
	 */
	AI_LOOTER_SHOOTER_API float WeightScale(EUnpaidMove Move, bool bOpen, bool bPunisher, bool bInFront);
}
