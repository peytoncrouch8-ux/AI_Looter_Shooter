#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"

class ACreatureBase;

/** How a kill pays its silver: the rank whose odds it rolls and whether it pays as the Unpaid (pockets) or as a beast. */
struct FKillSilver
{
	ECreatureRank Rank = ECreatureRank::Basic;
	bool bUnpaid = false;
};

/**
 * Which loot table and which purse a creature's kill pays, past its own rank (the RR tuning the user took, 2026-10-29
 * night): a Regular Stone Sentinel pays as a Restless one (StoneSentinelRules::LeastLootRank), and the living who carry
 * pockets pay the Unpaid's silver odds: the Gravepicker placeholders (their pack, PackTag Gravepicker), as the dead
 * miners and the Foreman (Unpaid by class) already did. ULootDropComponent and ACreatureBase::ApplyRank read it.
 */
namespace KillLootRules
{
	/** The rank whose loot table Creature carries at Rank: Rank, or a Sentinel's least. */
	AI_LOOTER_SHOOTER_API ECreatureRank LootRankFor(const ACreatureBase& Creature, ECreatureRank Rank);

	/** Its kill's silver: its rank (a Sentinel's at least Restless) and whether it pays as the Unpaid. */
	AI_LOOTER_SHOOTER_API FKillSilver SilverFor(const ACreatureBase& Creature);

	/** Whether Creature pays the Unpaid's silver odds: the Unpaid (dead miners, the Foreman, Abel), a Sentinel, the Gravepickers. */
	AI_LOOTER_SHOOTER_API bool PaysAsUnpaid(const ACreatureBase& Creature);
}
