#pragma once

#include "CoreMinimal.h"
#include "CreatureRank.generated.h"

/**
 * How long a creature has fed on the dark, which Ellis sees as soul-light (Docs/Story.md, "Enemy ranks and legendary
 * drops"). Every rank above Basic is a little bigger and tougher, shows a word on its tag in the rarity color of the odds
 * it carries, and drops from a better loot table. UCreatureRankSettings says what each one does.
 */
UENUM(BlueprintType)
enum class ECreatureRank : uint8
{
	/** The tag as it always was; the default loot table. */
	Basic,
	/** "Restless", blue. */
	Rare,
	/** "Gravebound", purple. */
	Epic,
	/** "Soulfed" (or a name of its own), orange: hand-placed monsters with a lair, which don't come back on their own. */
	Legendary,
	/** Story fights: orange, with a boss bar. */
	Boss
};

namespace LooterRanks
{
	inline constexpr int32 NumRanks = static_cast<int32>(ECreatureRank::Boss) + 1;

	/** Every rank, lowest first. */
	inline constexpr ECreatureRank All[] = { ECreatureRank::Basic, ECreatureRank::Rare, ECreatureRank::Epic,
		ECreatureRank::Legendary, ECreatureRank::Boss };
	static_assert(UE_ARRAY_COUNT(All) == NumRanks, "Every rank, once");
}
