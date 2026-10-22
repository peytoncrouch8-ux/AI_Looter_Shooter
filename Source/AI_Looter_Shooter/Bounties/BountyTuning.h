#pragma once

#include "CoreMinimal.h"
#include "Bounties/BountyTypes.h"

/**
 * The bounty board's feel numbers (Docs/Work/2026-10-22-Next/BOUNTY.md), as the Looter.Bounty.* cvars set them
 * (FBountyTuning::Current), so they're tuned live in play. A default-made one holds the defaults, which the tests read.
 */
struct AI_LOOTER_SHOOTER_API FBountyTuning
{
	/** The board posts and its fights spawn (0: the board stays shut and no fight comes). */
	bool bEnabled = true;

	/** Minutes of play between rotations: the postings not taken come down, new ones go up. */
	float RotateMinutes = 120.f;

	/** Minutes of play before a slot paid out takes a new posting. */
	float RefillMinutes = 10.f;

	/** A bounty fight's next push comes once this many of the last are left standing. */
	int32 PushWhenLeft = 2;

	/** A bounty fight springs on the player coming this near its arena's middle (cm), where its ground doesn't spring it. */
	float SpringRadius = 2200.f;

	/** The strongbox's guns by tier, and their luck (a Strongbox's 1: 15.5% legendary each). */
	int32 RewardGuns[3] = { 2, 2, 3 };
	float RewardLuck = 1.f;

	/** The strongbox's ammo pickups (36 rounds each, a chest's). */
	int32 RewardAmmo = 2;

	/** The purse paid at the board by tier, at level 1 (grown with the arena's level as the economy grows). */
	int32 Purse[3] = { 35, 55, 80 };

	/** The experience paid at the board by tier, as a share of the step from the arena's top level to the next. */
	float XPShare[3] = { 0.05f, 0.07f, 0.09f };

	/** The strongbox drops from this high over its spot (cm). */
	float ChestDrop = 260.f;

	/** A posting going up chimes when the player is this near the board (cm). */
	float PostedChimeRange = 4000.f;

	int32 GunsFor(EBountyTier Tier) const { return RewardGuns[TierIndex(Tier)]; }
	int32 PurseFor(EBountyTier Tier) const { return Purse[TierIndex(Tier)]; }
	float XPShareFor(EBountyTier Tier) const { return XPShare[TierIndex(Tier)]; }

	/** The tuning as the cvars hold it now (read each time: a live change counts at once). */
	static FBountyTuning Current();

	static int32 TierIndex(EBountyTier Tier) { return FMath::Clamp(static_cast<int32>(Tier), 0, 2); }
};
