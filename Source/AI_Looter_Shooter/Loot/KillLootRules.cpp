#include "Loot/KillLootRules.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/StoneSentinelCreature.h"
#include "Creatures/StoneSentinelRules.h"
#include "Creatures/UnpaidCreature.h"

namespace
{
	/** The Gravepicker placeholders' pack (create_placeholder_creatures.py: the diggers and their lookouts share it). */
	const FName GravepickerPack(TEXT("Gravepicker"));

	/** Rank, raised to Least when it's lower (ranks compare by LooterRanks, never by number). */
	ECreatureRank AtLeast(ECreatureRank Rank, ECreatureRank Least)
	{
		return LooterRanks::IsAtLeast(Rank, Least) ? Rank : Least;
	}
}

ECreatureRank KillLootRules::LootRankFor(const ACreatureBase& Creature, ECreatureRank Rank)
{
	return Creature.IsA<AStoneSentinelCreature>() ? AtLeast(Rank, StoneSentinelRules::LeastLootRank) : Rank;
}

bool KillLootRules::PaysAsUnpaid(const ACreatureBase& Creature)
{
	// The living grave robbers carry coin as the dead do (the user's tuning: they pay the Unpaid's odds).
	return Creature.IsA<AUnpaidCreature>() || Creature.IsA<AStoneSentinelCreature>() || Creature.PackTag == GravepickerPack;
}

FKillSilver KillLootRules::SilverFor(const ACreatureBase& Creature)
{
	FKillSilver Silver;
	Silver.Rank = LootRankFor(Creature, Creature.GetRank());
	Silver.bUnpaid = PaysAsUnpaid(Creature);
	return Silver;
}
