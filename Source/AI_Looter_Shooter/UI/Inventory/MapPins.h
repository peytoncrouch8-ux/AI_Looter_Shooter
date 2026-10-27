#pragma once

#include "CoreMinimal.h"

class ARespawnMarker;
class UMissionRunner;
class UMissionSubsystem;
class UWorld;
struct FCampaignRecord;

/**
 * What a pin on the map page marks. Pins draw in this order, so the later ones sit on top. No chests: finding them is the
 * player's (the user's call, 2026-10-27).
 */
enum class EMapPinKind : uint8
{
	/** A gunsmith's bench. */
	Bench,
	/** A vigil stone (ABossShrine) that has risen beside a boss's arena. */
	Shrine,
	/** A store (AVendor) that is open to trade: Pruitt's General Store, Crossroads Mercantile. */
	Vendor,
	/** Where trips leave from: the train's depot, Skyreach's skiff jetty. */
	Station,
	/**
	 * One of Hob's roosts found (or opened) in this story that is a fast-travel station (ARespawnMarker::bFastTravelStation):
	 * fast travel goes there. A roost that is only a place to rise after a death (the Sheriff's Office's) has no pin. (Named
	 * for the graves the roosts replaced.)
	 */
	Grave,
	/** A giver with a mission to offer (a gold ? for the story, a white ? for a side job): someone, a board, an artifact. */
	Offer,
	/** A mission waiting to be turned in, at its giver (a !, gold or white). */
	TurnIn,
	/** The tracked mission's objective. */
	Objective,
};

/** One pin: what it is, where, and its words when pointed at. */
struct AI_LOOTER_SHOOTER_API FMapPin
{
	EMapPinKind Kind = EMapPinKind::Objective;
	FVector Location = FVector::ZeroVector;
	/** The pin's name ("Main Street", "Gunsmith's bench", "Pruitt's General Store", the mission's title). */
	FText Name;
	/** A second line under it, when it has one ("Turn in to Delia", "Hob's roost: fast travel"). */
	FText Detail;
	/** The roost, for a Grave pin. */
	TWeakObjectPtr<const ARespawnMarker> Grave;
	/** The roost's marker id, the vigil stone's boss key or the mission's id. */
	FName Id;
	/** An offer's or a turn-in's mission is the story's (its mark gold), not a side job's (white). */
	bool bMain = false;
};

/** Where the pins come from: the level, the story's record, the missions. Any may be missing. */
struct FMapPinSources
{
	const UWorld* World = nullptr;
	/** Which roosts are found or open (without one, only those open from the start) and which stores trade (without one, only those with no gate). */
	const FCampaignRecord* Campaign = nullptr;
	/** The tracked mission's objective and its turn-in. */
	const UMissionSubsystem* Missions = nullptr;
	/** The missions offered and waiting to be turned in, and their givers. */
	const UMissionRunner* Runner = nullptr;
};

/**
 * The map page's pins gathered from a level (apart from the widget, so the tests read them from a test level): Hob's roosts
 * found (or opened by the story) that are travel stations, the stations, the gunsmith's benches, the vigil stones risen
 * beside the boss arenas, the stores open to trade, the givers with a mission to offer (one pin a giver), the missions
 * waiting to be turned in at their givers, and the tracked mission's objective. Roosts not found yet, stores the story keeps
 * shut and stones not yet risen stay off the map, and chests never show: what's ahead is the player's to find. The player's
 * own arrow is the widget's.
 */
namespace MapPins
{
	AI_LOOTER_SHOOTER_API TArray<FMapPin> Gather(const FMapPinSources& Sources);

	/** The legend's words for a kind ("Hob's roost"). */
	AI_LOOTER_SHOOTER_API FText KindName(EMapPinKind Kind);

	/** How many pins of Kind there are. */
	AI_LOOTER_SHOOTER_API int32 Count(TConstArrayView<FMapPin> Pins, EMapPinKind Kind);
}
