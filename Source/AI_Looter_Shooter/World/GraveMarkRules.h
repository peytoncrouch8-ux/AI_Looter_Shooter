#pragma once

#include "CoreMinimal.h"

struct FCampaignRecord;

/**
 * Why walking up to one of Hob's roosts finds nothing now (GraveMarkRules::Check), in the order the rules ask. None: it does.
 * (The roosts replaced the respawn graves and kept their rules and save data, so the names still say grave.)
 */
enum class EGraveMarkBlock : uint8
{
	None,
	/** The roost isn't found on approach (bMarkOnApproach off). */
	Off,
	/** It is a trip's landing (a platform, a jetty): nothing is saved there. */
	Landing,
	/** No living character to find it. */
	NoPlayer,
	/** The player is down: the death's carry is running. */
	Dying,
	/** A scene is playing or holding the player. */
	Scene,
	/** A trip to another level, or a fast travel, is under way. */
	Travelling,
	/** Its mission isn't finished yet (the story keeps the roost shut). */
	NotYet,
	/** The player is farther than its find radius. */
	TooFar,
	/** Something solid stands between the player and the roost. */
	NoSight,
};

/** What a walk up to a roost did. */
enum class EGraveMark : uint8
{
	/** Nothing: it was blocked, or it is the save point already. */
	None,
	/** Found: its beacon rises, the banner shows, the roost opens (if the story hadn't) and becomes the save point. */
	First,
	/** A roost found before, becoming the save point again: a pulse up its beacon, no banner. */
	Again,
};

/** What the rules look at, gathered from the world (ARespawnMarker::GatherSenses) or made up by a test. */
struct AI_LOOTER_SHOOTER_API FGraveMarkSenses
{
	bool bMarkOnApproach = true;
	bool bLanding = false;
	bool bHasPlayer = false;
	bool bDying = false;
	bool bInScene = false;
	bool bTravelling = false;
	/** MarkAfterMission (or, without one, ActiveAfterMission) is finished, or the roost has none. */
	bool bStoryAllows = true;
	/** From the player's feet to the roost's spot (cm). */
	double Distance = TNumericLimits<double>::Max();
	double Radius = 900.0;
	/** A line from the player's eyes to the roost is clear (looked at last, only when everything else allows). */
	bool bClearLine = true;
};

/**
 * Borderlands-style save points as plain rules, apart from the world so the tests drive them: when walking up to one of
 * Hob's roosts finds it, what that does (a first find, or the save point moving back to a roost found before), and how the
 * campaign record keeps it. A roost found is the last one of its area (FCampaignRecord::LastGraves, which Continue wakes at)
 * and stays lit for good (FCampaignRecord::MarkedGraves). A save from before save points reads as nothing found, which is
 * right; a save from the graves' days reads its marked graves as found roosts (the same ids).
 */
namespace GraveMarkRules
{
	/** Closer than this a roost is found (cm), unless the roost says another: it stands beside a road, so passing finds it. */
	inline constexpr double DefaultRadius = 900.0;

	/**
	 * The light's flare as a roost is found (seconds): its light spikes at once and eases down over FirstFlareSeconds (the
	 * beacon's column rises on its own curve, RoostRules::BeamRise); one made the save point again lifts for the shorter
	 * LiftSeconds. The rise to the peak takes FlareRiseSeconds in both.
	 */
	inline constexpr float FirstFlareSeconds = 1.1f;
	inline constexpr float LiftSeconds = 0.7f;
	inline constexpr float FlareRiseSeconds = 0.09f;

	/** How long Kind's flare lasts (0 for None). */
	AI_LOOTER_SHOOTER_API float FlareSeconds(EGraveMark Kind);

	/** The flare's strength Age seconds in: 0 before it and after it, rising to 1 by FlareRiseSeconds, then easing back to 0. */
	AI_LOOTER_SHOOTER_API float FlarePulse(float Age, EGraveMark Kind);

	/** The roost Marker has been found at some point in this story. */
	AI_LOOTER_SHOOTER_API bool IsMarked(const FCampaignRecord& Campaign, FName Marker);

	/** Marker is the roost last found in Area (the save point Continue wakes at). */
	AI_LOOTER_SHOOTER_API bool IsSavePoint(const FCampaignRecord& Campaign, FName Area, FName Marker);

	/** Whether walking up to the roost finds it now, with the first block that holds, or None. */
	AI_LOOTER_SHOOTER_API EGraveMarkBlock Check(const FGraveMarkSenses& Senses);

	/** What a walk up to the roost does when nothing blocks it: a first find, the save point moving back, or nothing (it is it already). */
	AI_LOOTER_SHOOTER_API EGraveMark Outcome(bool bMarkedBefore, bool bIsSavePoint);

	/**
	 * Writes the find into Campaign: Marker is found for good and becomes Area's save point. Returns what it was (None when
	 * it already was both). It does not open the roost: ARespawnMarker::Activate does.
	 */
	AI_LOOTER_SHOOTER_API EGraveMark Record(FCampaignRecord& Campaign, FName Area, FName Marker);

	/** Takes the find back (the console, to see a first find again): Marker is no longer found, nor Area's save point. False when it wasn't. */
	AI_LOOTER_SHOOTER_API bool Forget(FCampaignRecord& Campaign, FName Area, FName Marker);

	/** The console's and the map's words for a block ("too far", "its mission isn't finished"); empty for None. */
	AI_LOOTER_SHOOTER_API FString Describe(EGraveMarkBlock Block);
}
