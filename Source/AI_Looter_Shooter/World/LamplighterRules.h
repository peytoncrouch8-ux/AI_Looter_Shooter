#pragma once

#include "CoreMinimal.h"
#include "Story/StoryLine.h"

class UObject;
class UStoryLineSet;
struct FCampaignRecord;

/** How the Lamplighter begins as Ransom's Rest opens (LandfallRules::StartFor). */
enum class ELandfallStart : uint8
{
	/** Landfall from its start: the ship sails in from the west with the player on deck (a new game, a play with no session,
	 *  or a session saved mid-flight, which flies again from the start). */
	Flight,
	/** She's down on the Mooring Ledge already: Main 1 is past the landing, or done. */
	Moored,
	/** A save from before Landfall (the grave start) with Main 1 running: moored, anchored, and Main 1 moved to the Point. */
	MooredOldSave,
};

/**
 * Landfall's rules apart from the world (Docs/Work/2026-10-31-Wardens/CONTRACT.md, "Landfall"), so tests check them: the
 * names the mission data, the build scripts and the code share, Main 1's step indices, how a level begins the ship from
 * the campaign, and the crew's line sets.
 */
namespace LandfallRules
{
	// --- Names (the contract's, exact) ---

	/** "Main1": Landfall keeps the old mission id (saves, conditions and tests key on it). */
	AI_LOOTER_SHOOTER_API FName MissionId();
	/** "Landfall.DeckClear": every boarder on deck is down (Main 1 step 1). */
	AI_LOOTER_SHOOTER_API FName DeckClearEvent();
	/** "Lamplighter_Prow": the marker at the Great Lantern (Main 1 step 0, Reach, radius 350). */
	AI_LOOTER_SHOOTER_API FName ProwTag();
	/** "Landfall_Grapple": each of the tender's two grapples (Main 1 step 2, hold Interact, count 2). */
	AI_LOOTER_SHOOTER_API FName GrappleTag();
	/** "Landfall_AnchorWinch": the anchor winch on the ledge (Main 1 step 4, hold Interact). */
	AI_LOOTER_SHOOTER_API FName WinchTag();
	/** "Landfall_Spyglass": the spyglass at the lookout (Main 1 step 6, Interact). */
	AI_LOOTER_SHOOTER_API FName SpyglassTag();
	/** "Landfall_Barge": Vane's dredge barge over the Sink (WORLD's AStoryProp), what the spyglass looks at. */
	AI_LOOTER_SHOOTER_API FName BargeTag();
	/** "Landing_Lamplighter": the ship's deck as the story's first arrival (StationBoard::FirstArrivalLanding). */
	AI_LOOTER_SHOOTER_API FName LandingTag();
	/** "Lamplighter_HobPerch": where Hob sits in the ship's rigging (a scene component on the ship). */
	AI_LOOTER_SHOOTER_API FName HobPerchTag();
	/** "Speaker_Delia": what the Great Lantern leans toward (Delia on her porch: where Abel was taken). */
	AI_LOOTER_SHOOTER_API FName LanternTargetTag();

	/** Story flags: the flight has begun in this session (a save without it, Main 1 running, is from before Landfall); the
	 *  anchor is down; the spyglass's reveal has played. */
	AI_LOOTER_SHOOTER_API FName BegunFlag();
	AI_LOOTER_SHOOTER_API FName AnchoredFlag();
	AI_LOOTER_SHOOTER_API FName RevealedFlag();

	/** The session's save hold from the flight's start until she's moored (a save mid-air would wake the player in the sky). */
	AI_LOOTER_SHOOTER_API FName SaveHold();

	// --- Main 1's steps (the contract's table; indices are the contract) ---

	inline constexpr int32 StepProw = 0;
	inline constexpr int32 StepDeckFight = 1;
	inline constexpr int32 StepGrapples = 2;
	inline constexpr int32 StepLanding = 3;
	inline constexpr int32 StepAnchor = 4;
	inline constexpr int32 StepPoint = 5;
	inline constexpr int32 StepSpyglass = 6;
	inline constexpr int32 StepTrail = 7;
	inline constexpr int32 StepPorch = 8;

	/** Where an old save's Main 1 goes: the climb to Ransom's Point (the contract). */
	inline constexpr int32 OldSaveStep = StepPoint;

	/** Holloway's tube lines (DA_Lines_LandfallPoint) play as this step begins: they send Sully up to the Point. */
	inline constexpr int32 PointLinesStep = StepPoint;

	// --- Line sets (STORY's, exact names; /Game/Data/Story/<name>) ---

	AI_LOOTER_SHOOTER_API FName DeckLines();
	AI_LOOTER_SHOOTER_API FName VaneHornLines();
	AI_LOOTER_SHOOTER_API FName BoarderLines();
	AI_LOOTER_SHOOTER_API FName GrappleLines();
	AI_LOOTER_SHOOTER_API FName LandingLines();
	AI_LOOTER_SHOOTER_API FName AnchorLines();
	AI_LOOTER_SHOOTER_API FName PointLines();
	AI_LOOTER_SHOOTER_API FName VaneValleyLines();
	AI_LOOTER_SHOOTER_API FName TrailLines();

	// --- How the ship begins ---

	/** How the ship begins in a level whose story is Campaign. */
	AI_LOOTER_SHOOTER_API ELandfallStart StartFor(const FCampaignRecord& Campaign);

	/**
	 * Writes Start into Campaign before the missions resume: a flight passes the old opening (the cold open, the grave and the
	 * Rest's intro are never due again), marks the flight begun and, for a session saved mid-flight, puts Main 1 back to its
	 * first step; an old save is moored, anchored, its old Main 1 optionals forgotten, and Main 1 moved to OldSaveStep.
	 */
	AI_LOOTER_SHOOTER_API void ApplyStart(FCampaignRecord& Campaign, ELandfallStart Start);

	/** The anchor is down in Campaign (set at the winch, or Main 1 past its anchor step, or done). */
	AI_LOOTER_SHOOTER_API bool IsAnchoredIn(const FCampaignRecord& Campaign);

	/** Main 1's step in Campaign while it runs; INDEX_NONE when it doesn't (not begun, or done). */
	AI_LOOTER_SHOOTER_API int32 MainStepIn(const FCampaignRecord& Campaign);

	// --- Lines ---

	/** A line set's lines (empty, warned once, while STORY's script hasn't made it). */
	AI_LOOTER_SHOOTER_API TArray<FStoryLine> LoadLines(FName SetName);

	/**
	 * The line set itself, from memory when it's loaded, else from disk (a hitch mid-play: hold it from begin play, as
	 * UAbelCorruptionComponent does); null while it isn't made. Nothing holds it: it may be collected once unused.
	 */
	AI_LOOTER_SHOOTER_API UStoryLineSet* LoadLineSet(FName SetName);

	/** Plays a line set as captions after whatever is being said (bInterrupt: now). The conversation, or INDEX_NONE. */
	AI_LOOTER_SHOOTER_API int32 Say(const UObject* Context, FName SetName, bool bInterrupt = false);
}
