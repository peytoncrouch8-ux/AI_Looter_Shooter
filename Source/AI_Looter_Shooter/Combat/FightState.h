#pragma once

#include "CoreMinimal.h"

class APawn;
class UWorld;

/**
 * Whether the player is in a fight, asked the way the music asks it (UMusicDirectorSubsystem::Sense): a creature coming
 * at them (ACreatureBase::IsFightingWith; one that lost them and only searches doesn't count), or a boss's fight running.
 * Health's regen waits on it (FWoundsClose), boss fights' helpers drop more soul-motes, and the soul flask's kills read it.
 * A walk over the level's creatures: cheap at a few dozen, call it a few times a second, not per creature.
 */
namespace FightState
{
	/** Some living creature is fighting Player now, or a boss fight is on. */
	AI_LOOTER_SHOOTER_API bool IsInCombat(const UWorld* World, const APawn* Player);

	/** A boss's fight runs (a UBossComponent fighting, or a Legendary creature after Player: the Gravemother). */
	AI_LOOTER_SHOOTER_API bool IsBossFightOn(const UWorld* World, const APawn* Player);
}
