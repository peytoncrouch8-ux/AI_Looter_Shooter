#pragma once

#include "CoreMinimal.h"
#include "CreatureRank.generated.h"

/**
 * How long a creature has fed on the dark, which Ellis sees as soul-light (Docs/Story.md, "Enemy ranks and legendary
 * drops"). The five tiers every regular enemy can be (Regular, Restless, Gravebound, Hallowed, Ethereal) are each a little
 * bigger and tougher, show a word on their tag in their tier's colour, and drop from a better loot table; Legendary
 * monsters and bosses stand above them. UCreatureRankSettings says what each one does.
 *
 * The values are saved by number (levels, encounter groups, egg sacs), so new ones are only ever appended: Hallowed and
 * Ethereal come after Boss. Their numbers say nothing about their order: compare ranks with LooterRanks::IsAbove,
 * IsAtLeast or Order, never with < or >.
 */
UENUM(BlueprintType)
enum class ECreatureRank : uint8
{
	/** Regular: the tag as it always was (no word); the default loot table. */
	Basic,
	/** "Restless", blue. */
	Rare,
	/** "Gravebound", purple. */
	Epic,
	/** "Soulfed" (or a name of its own), orange: hand-placed monsters with a lair, which don't come back on their own. */
	Legendary,
	/** Story fights: orange, with a boss bar. */
	Boss,
	/** "Hallowed", saint-gold yellow: the fourth tier (2026-10-20). */
	Hallowed,
	/** "Ethereal", red: the fifth and rarest tier (2026-10-20). */
	Ethereal
};

namespace LooterRanks
{
	inline constexpr int32 NumRanks = static_cast<int32>(ECreatureRank::Ethereal) + 1;

	/** Every rank, lowest first: the five tiers, then Legendary monsters, then bosses. */
	inline constexpr ECreatureRank All[] = { ECreatureRank::Basic, ECreatureRank::Rare, ECreatureRank::Epic, ECreatureRank::Hallowed,
		ECreatureRank::Ethereal, ECreatureRank::Legendary, ECreatureRank::Boss };
	static_assert(UE_ARRAY_COUNT(All) == NumRanks, "Every rank, once");

	/** The five tiers any regular enemy can be, lowest first (Regular, Restless, Gravebound, Hallowed, Ethereal). */
	inline constexpr ECreatureRank Tiers[] = { ECreatureRank::Basic, ECreatureRank::Rare, ECreatureRank::Epic, ECreatureRank::Hallowed,
		ECreatureRank::Ethereal };
	inline constexpr int32 NumTiers = UE_ARRAY_COUNT(Tiers);

	/** Where Rank stands in All, lowest first (Basic 0 ... Boss 6): the one way to put ranks in order. */
	constexpr int32 Order(ECreatureRank Rank)
	{
		for (int32 Index = 0; Index < NumRanks; ++Index)
		{
			if (All[Index] == Rank)
			{
				return Index;
			}
		}
		return 0;
	}

	/** Where Rank stands among the tiers (Basic 0 ... Ethereal 4); INDEX_NONE for a Legendary monster or a boss. */
	constexpr int32 TierIndex(ECreatureRank Rank)
	{
		for (int32 Index = 0; Index < NumTiers; ++Index)
		{
			if (Tiers[Index] == Rank)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/** One of the five tiers a regular enemy can be (not a Legendary monster, not a boss). */
	constexpr bool IsTier(ECreatureRank Rank)
	{
		return TierIndex(Rank) != INDEX_NONE;
	}

	/** A stands above B: Basic < Rare < Epic < Hallowed < Ethereal < Legendary < Boss. */
	constexpr bool IsAbove(ECreatureRank A, ECreatureRank B)
	{
		return Order(A) > Order(B);
	}

	/** A is B or above it. */
	constexpr bool IsAtLeast(ECreatureRank A, ECreatureRank B)
	{
		return Order(A) >= Order(B);
	}

	/** The tier below Tier (Basic stays Basic); a Legendary monster or a boss as it is. */
	constexpr ECreatureRank TierBelow(ECreatureRank Tier)
	{
		const int32 Index = TierIndex(Tier);
		return Index > 0 ? Tiers[Index - 1] : Tier;
	}

	static_assert(Order(ECreatureRank::Hallowed) > Order(ECreatureRank::Epic) && Order(ECreatureRank::Legendary) > Order(ECreatureRank::Ethereal),
		"The tiers sit between Gravebound and the Legendary monsters");
}
