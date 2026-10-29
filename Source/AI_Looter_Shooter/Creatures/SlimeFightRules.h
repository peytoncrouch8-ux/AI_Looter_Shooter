#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureTypes.h"

/**
 * The meadow slime's fight numbers (FIGHTS.md section 3, 2026-10-28): a Restless or better slime bursts into slimelets,
 * the pack's leaps land one after another (the melee gap), and a slime shot at while it waits its turn hops aside. Each
 * of the four main ones is a Looter.Slime.* console variable; the fields' own values are the cvars' defaults, so the
 * rules' tests (which use a default-made tuning) and a fresh game agree. Play reads Live().
 */
struct AI_LOOTER_SHOOTER_API FSlimeFightTuning
{
	/** Looter.Slime.Split: 1 lets a Restless or better slime die into slimelets; 0 is a plain death. */
	bool bSplit = true;
	/** Looter.Slime.SplitCap: a split is refused when it would leave more than this many slimes (slimelets too) alive in the level. */
	int32 SplitCap = 10;
	/** Looter.Slime.MeleeGap: two slimes' leaps at one target begin at least this far apart (s): a drumbeat. 0: off. */
	float MeleeGap = 0.45f;
	/** Looter.Slime.Dodge: the chance that three hits in DodgeWindow, while it waits its turn, make it hop aside. 0: never. */
	float DodgeChance = 0.5f;

	/** The slimelets of a split: how many, their size and health against their parent's class, their damage and experience against a slime's. */
	int32 SplitPieces = 2;
	float SplitSize = 0.55f;
	float SplitHealth = 0.3f;
	float SplitDamage = 0.5f;
	float SplitXP = 0.25f;
	/** The two slimelets land this far apart (cm). */
	float SplitSpread = 150.f;

	/** The dodge: hits needed within the window (s), how far the hop goes (cm, at size 1) and how long before the next (s). */
	int32 DodgeHits = 3;
	float DodgeWindow = 0.8f;
	float DodgeLength = 250.f;
	float DodgeCooldown = 4.f;

	/** The cvars' values now. */
	static FSlimeFightTuning Live();
};

/** Hits taken lately and when the next dodge may come, as plain data so the tests check it without a level. */
struct AI_LOOTER_SHOOTER_API FSlimeDodgeTracker
{
	/**
	 * A hit lands at Now. True when the slime should hop aside now: the window holds DodgeHits hits, it waits its turn
	 * (bWaiting) and isn't busy (bBusy: mid-hop, in an attack, held by a hit), the cooldown is over, and Roll (0 to 1)
	 * is under DodgeChance. A dodge, or a lost roll, forgets the hits, so each burst of hits rolls once.
	 */
	bool Note(double Now, const FSlimeFightTuning& Tuning, bool bWaiting, bool bBusy, float Roll);

	/** Hits still counted in the window, for the tests. */
	int32 NumRecent() const { return Hits.Num(); }

private:
	TArray<double, TInlineAllocator<8>> Hits;
	double ReadyAt = 0.0;
};

namespace SlimeFightRules
{
	/**
	 * How many slimelets a slime that just died makes: Tuning.SplitPieces when its rank is Restless or better (one of the
	 * five tiers above Basic), it is not itself a slimelet, splitting is on, and the pieces fit under both caps; else 0
	 * ("a denied split just dies"). SlimesAlive is the living slimes after its death; NearAlive the living creatures of any
	 * kind near the player, with NearCap the level's cap on them (0: none).
	 */
	AI_LOOTER_SHOOTER_API int32 SplitPieces(const FSlimeFightTuning& Tuning, ECreatureRank Rank, bool bSlimelet, int32 SlimesAlive,
		int32 NearAlive, int32 NearCap);

	/** The level a slimelet starts at: its parent's less its rank's offset (a slimelet is Basic), at least 1. */
	AI_LOOTER_SHOOTER_API int32 SlimeletLevel(int32 ParentLevel, int32 ParentRankOffset);

	/** How a slimelet starts (Basic, its level, SplitSize, SplitHealth): the damage and experience shares are applied by the slime. */
	AI_LOOTER_SHOOTER_API FCreatureRuntimeSpawn SlimeletSpawn(const FSlimeFightTuning& Tuning, int32 ParentLevel, int32 ParentRankOffset);

	/** Where each of Pieces lands round the corpse (cm, flat): on a circle, neighbours Spread apart, the first at StartYawDegrees. One piece: the middle. */
	AI_LOOTER_SHOOTER_API TArray<FVector> SplitOffsets(int32 Pieces, float Spread, float StartYawDegrees);

	/** The way of a side hop from a creature whose target lies along ToTarget: flat, a unit long, to its right or its left. Zero ToTarget: along the world's X. */
	AI_LOOTER_SHOOTER_API FVector DodgeDirection(const FVector& ToTarget, bool bRight);
}
