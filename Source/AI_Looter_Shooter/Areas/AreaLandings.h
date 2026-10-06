#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/**
 * Landings: where trips between areas arrive (a station's platform, Skyreach's jetty; UAreaDefinition::Landings). A
 * landing is an actor in the level tagged with its name, or a player start whose Player Start Tag is its name. An actor
 * can carry its landing as one of its scene components tagged with the name too (ASkiffJetty's spot on the deck,
 * ATrainStation's on the platform): the player arrives there (GetSpot). Landing names start with "Landing_", so a level's
 * own start is never taken for one, and a new game never begins on a landing.
 */
namespace AreaLandings
{
	/** Every landing's name starts with this ("Landing_Depot", "Landing_Jetty"). */
	inline constexpr const TCHAR* NamePrefix = TEXT("Landing_");

	/** The name is a landing's: it starts with NamePrefix. */
	AI_LOOTER_SHOOTER_API bool IsLandingTag(FName Tag);

	/** The actor marks a landing: a player start with a landing's Player Start Tag, or any actor with a landing's tag. */
	AI_LOOTER_SHOOTER_API bool IsLanding(const AActor* Actor);

	/** The landing named Landing in World: a player start tagged so first, then any actor with the tag; null when none. */
	AI_LOOTER_SHOOTER_API AActor* Find(const UWorld* World, FName Landing);

	/**
	 * Where on Actor (a landing Find found) the player arrives: its scene component tagged Landing when it carries one,
	 * else the actor itself. Identity without an actor.
	 */
	AI_LOOTER_SHOOTER_API FTransform GetSpot(const AActor* Actor, FName Landing);
}
