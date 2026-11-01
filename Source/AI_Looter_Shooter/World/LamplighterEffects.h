#pragma once

#include "CoreMinimal.h"

class FWeaponFX;
class UObject;
struct FViewKick;

/**
 * Landfall's big moments drawn with the game's own code-drawn effects (FWeaponFX: instanced puffs, sparks and chips; no new
 * Niagara, the performance rule) and felt on the player's view (UCameraShakeModifier: the one modifier every shake goes
 * through, scaled by the camera shake setting): the grapple's bite, the cut, the torn bag's gas, the tender's smoke and
 * fire, the scrape and the crash's dust. Nothing is drawn or shaken in a world with no player (a test level).
 */
namespace LamplighterEffects
{
	/** The level's effects, ready to draw (null without a world that draws them). */
	AI_LOOTER_SHOOTER_API FWeaponFX* Find(const UObject* Context);

	/** Shakes the local player's view: Strength 0-1 (more stacks a little past 1), dying away over Seconds. */
	AI_LOOTER_SHOOTER_API void Shake(const UObject* Context, float Strength, float Seconds);

	/** Jolts the local player's view (a lurch, the impact). */
	AI_LOOTER_SHOOTER_API void Kick(const UObject* Context, const FViewKick& Kick);

	/** Dust thrown out round Center in a ring Radius cm across (the crash), Count puffs plus grit, rising and spreading. */
	AI_LOOTER_SHOOTER_API void DustSkirt(const UObject* Context, const FVector& Center, float Radius, int32 Count);

	/** White-hot sparks flung from Location along Direction (iron on iron, iron on rock), Count of them, Scale their size. */
	AI_LOOTER_SHOOTER_API void Sparks(const UObject* Context, const FVector& Location, const FVector& Direction, int32 Count, float Scale);

	/** One breath of the torn bag's gas jetting from Location along Direction: a pale puff thinning fast. */
	AI_LOOTER_SHOOTER_API void GasPuff(const UObject* Context, const FVector& Location, const FVector& Direction);

	/** One puff of the falling tender's black smoke at Location, drifting at Velocity. */
	AI_LOOTER_SHOOTER_API void SmokePuff(const UObject* Context, const FVector& Location, const FVector& Velocity);

	/** The tender's bag going up: a ball of fire blooming at Location with sparks and smoke. */
	AI_LOOTER_SHOOTER_API void Fireball(const UObject* Context, const FVector& Location, const FVector& Velocity);
}
