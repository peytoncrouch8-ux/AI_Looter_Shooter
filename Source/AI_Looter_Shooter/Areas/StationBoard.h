#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

class UAreaDefinition;
class UMissionDefinition;
struct FCampaignRecord;

/** What a station board's line is. */
enum class EStationLine : uint8
{
	/** An opened area of the story: a trip to its station (the first cast-off's: to the story's first arrival). */
	Area,
	/** A practice area (Skyreach): outside the story, listed once the player has left it for the first time. */
	Practice,
	/** The blank line: no destination, only the mission that opens the next area ("Finish ‘The Lantern Leans’"). */
	NextMission,
};

/** One line of a station board, as the story stands (StationBoard::BuildLines). */
struct AI_LOOTER_SHOOTER_API FStationBoardLine
{
	EStationLine Kind = EStationLine::Area;

	/** Where it goes, and that area's id (none for the blank line). */
	TWeakObjectPtr<UAreaDefinition> Area;
	FName AreaId;

	/** What the line says: "Ransom's Rest", "Skyreach (practice)", "Finish ‘The Lantern Leans’". */
	FText Name;

	/** Where a trip on it arrives: the area's station (its first landing), or the story's first arrival. */
	FName Landing;

	/** The blank line's mission: the one that opens the next area. */
	FName MissionId;

	/** The board stands in this area: the line shows where the player is, and goes nowhere. */
	bool bHere = false;

	/** The destination's level is in the game. Choosing a line without one says so, and nobody goes. */
	bool bLevelBuilt = false;

	/** Leaving Skyreach for the first time: the skiff's ride into the cloud, and the story's first arrival. */
	bool bFirstCastOff = false;

	/** Choosing it asks to travel: a destination, and not the one the board stands in. */
	bool IsDestination() const { return Kind != EStationLine::NextMission && !bHere; }
};

/** How a board speaks: a station's departures board, or Skyreach's skiff jetty. */
struct AI_LOOTER_SHOOTER_API FStationBoardWords
{
	/** The panel's title. */
	FText Title;

	/** What leaving is called on the confirm's button: "Travel", "Cast off". */
	FText Depart;

	/** What it says of a destination whose level isn't in the game yet; {0} is the place. */
	FText NotOpen;

	/** A station's: "Departures", "Travel", "The line to {0} isn't open yet." */
	static FStationBoardWords Station();

	/** Skyreach's jetty's: "Skiff jetty", "Cast off", "The skiff can't reach {0} yet." */
	static FStationBoardWords Jetty();
};

/**
 * The station boards' rules, apart from the world so the tests feed them (Docs/Areas/RansomsRest.md, "The exit and the
 * unlock"). Every board (the depot on Ransom's Rest, every later station, Skyreach's jetty) lists:
 *  - every opened area of the story (the campaign record's, and the story's first area once the player has left Skyreach),
 *    the one it stands in marked as here; later areas aren't shown;
 *  - one blank line naming the mission that opens the next area;
 *  - "Skyreach (practice)" once the player has left it for the first time, on every board but Skyreach's own.
 * Before the first cast-off a board offers one trip only: off Skyreach to the story's first arrival (the family plot's
 * grave on Ransom's Rest, until the cold open exists). "Skip the tutorial" counts as that first cast-off.
 */
namespace StationBoard
{
	/** The story's first area: the first cast-off (and "Skip the tutorial") goes there. */
	AI_LOOTER_SHOOTER_API FName FirstAreaId();

	/** Where the story's first arrival wakes the player until the cold open exists: the family plot's grave. */
	AI_LOOTER_SHOOTER_API FName FirstArrivalLanding();

	/** The first cast-off recorded in Campaign: the story has begun, and its first area is open to travel. */
	AI_LOOTER_SHOOTER_API void RecordFirstCastOff(FCampaignRecord& Campaign);

	/** The area is open to travel: opened by a mission, or the story's first area once the player has left Skyreach. */
	AI_LOOTER_SHOOTER_API bool IsAreaOpen(const FCampaignRecord& Campaign, FName AreaId);

	/**
	 * The mission that opens the next area: the first of Missions (main ones first, as UMissionDefinition::LoadAll orders
	 * them) not finished yet that opens an area of the story that isn't open. Null when none does.
	 */
	AI_LOOTER_SHOOTER_API const UMissionDefinition* FindNextMission(const FCampaignRecord& Campaign, const TArray<UMissionDefinition*>& Missions,
		const TArray<UAreaDefinition*>& Areas);

	/**
	 * A board's lines in the board's order, from Areas (in their order, UAreaDefinition::LoadAll) and Missions, standing in
	 * the area HereAreaId (None: a level that is no area's).
	 */
	AI_LOOTER_SHOOTER_API TArray<FStationBoardLine> BuildLines(const FCampaignRecord& Campaign, const TArray<UAreaDefinition*>& Areas,
		const TArray<UMissionDefinition*>& Missions, FName HereAreaId);

	/** An area's name on a board: its display name, else its id. */
	AI_LOOTER_SHOOTER_API FText AreaName(const UAreaDefinition& Area);

	/** "Skyreach (practice)". */
	AI_LOOTER_SHOOTER_API FText PracticeName(const UAreaDefinition& Area);

	/** "Finish ‘The Lantern Leans’". */
	AI_LOOTER_SHOOTER_API FText NextMissionText(const UMissionDefinition& Mission);
}
