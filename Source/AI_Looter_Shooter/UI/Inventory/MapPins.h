#pragma once

#include "CoreMinimal.h"

class ARespawnMarker;
class UMissionRunner;
class UMissionSubsystem;
class UWorld;
struct FCampaignRecord;

/** What a pin on the map page marks. Pins draw in this order, so the later ones sit on top. */
enum class EMapPinKind : uint8
{
	/** A loot chest opened and emptied: kept dim. */
	ChestLooted,
	/** A loot chest found but not opened yet. */
	Chest,
	/** A gunsmith's bench. */
	Bench,
	/** Where trips leave from: the train's depot, Skyreach's skiff jetty. */
	Station,
	/** A respawn grave open in this story: fast travel goes there. */
	Grave,
	/** A mission waiting to be turned in, at its giver. */
	TurnIn,
	/** The tracked mission's objective. */
	Objective,
};

/** One pin: what it is, where, and its words when pointed at. */
struct AI_LOOTER_SHOOTER_API FMapPin
{
	EMapPinKind Kind = EMapPinKind::Chest;
	FVector Location = FVector::ZeroVector;
	/** The pin's name ("Boot Hill", "Gunsmith's bench", the mission's title). */
	FText Name;
	/** A second line under it, when it has one ("Turn in to Delia", "Fast travel"). */
	FText Detail;
	/** The grave, for a Grave pin. */
	TWeakObjectPtr<const ARespawnMarker> Grave;
	/** The grave's marker id, the chest's save key or the mission's id. */
	FName Id;
};

/** Where the pins come from: the level, the story's record, the chests found, the missions. Any may be missing. */
struct FMapPinSources
{
	const UWorld* World = nullptr;
	/** Which graves are open (without one, only those open from the start). */
	const FCampaignRecord* Campaign = nullptr;
	/** The chests the player has come across (UMapDiscoverySubsystem); without it, only opened ones show. */
	const TSet<FName>* FoundChests = nullptr;
	/** The tracked mission's objective and its turn-in. */
	const UMissionSubsystem* Missions = nullptr;
	/** The missions waiting to be turned in, and their givers. */
	const UMissionRunner* Runner = nullptr;
};

/**
 * The map page's pins gathered from a level (apart from the widget, so the tests read them from a test level): the open
 * respawn graves, the stations, the gunsmith's benches, the chests found (or opened), the missions waiting to be turned
 * in at their givers, and the tracked mission's objective. Closed graves and unfound chests stay off the map: what's
 * ahead is the player's to find. The player's own arrow is the widget's.
 */
namespace MapPins
{
	AI_LOOTER_SHOOTER_API TArray<FMapPin> Gather(const FMapPinSources& Sources);

	/** The legend's words for a kind ("Respawn grave"). */
	AI_LOOTER_SHOOTER_API FText KindName(EMapPinKind Kind);

	/** How many pins of Kind there are. */
	AI_LOOTER_SHOOTER_API int32 Count(TConstArrayView<FMapPin> Pins, EMapPinKind Kind);
}
