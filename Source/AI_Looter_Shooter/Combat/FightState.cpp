#include "Combat/FightState.h"
#include "Bosses/BossComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A boss's fight, as the music hears it: a boss fighting, or a Soulfed monster with a lair after the player. */
	bool IsBossFight(const ACreatureBase& Creature, const APawn* Player)
	{
		if (const UBossComponent* Boss = Creature.FindComponentByClass<UBossComponent>())
		{
			if (Boss->IsFighting())
			{
				return true;
			}
		}
		return Player && Creature.GetTarget() == Player && Creature.GetRank() == ECreatureRank::Legendary;
	}
}

bool FightState::IsInCombat(const UWorld* World, const APawn* Player)
{
	if (!World || !Player)
	{
		return false;
	}
	for (TActorIterator<ACreatureBase> It(const_cast<UWorld*>(World)); It; ++It)
	{
		const ACreatureBase* Creature = *It;
		if (IsValid(Creature) && !Creature->IsDead() && (Creature->IsFightingWith(Player) || IsBossFight(*Creature, Player)))
		{
			return true;
		}
	}
	return false;
}

bool FightState::IsBossFightOn(const UWorld* World, const APawn* Player)
{
	if (!World)
	{
		return false;
	}
	for (TActorIterator<ACreatureBase> It(const_cast<UWorld*>(World)); It; ++It)
	{
		const ACreatureBase* Creature = *It;
		if (IsValid(Creature) && !Creature->IsDead() && IsBossFight(*Creature, Player))
		{
			return true;
		}
	}
	return false;
}
