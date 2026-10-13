#pragma once

#include "CoreMinimal.h"

struct FCampaignRecord;

/** Why walking up to a grave marks nothing now (GraveMarkRules::Check), in the order the rules ask. None: it marks. */
enum class EGraveMarkBlock : uint8
{
	None,
	/** The grave doesn't mark on approach (bMarkOnApproach off). */
	Off,
	/** It is a trip's landing (a platform, a jetty): nothing is saved there. */
	Landing,
	/** No living character to mark it. */
	NoPlayer,
	/** The player is down: the death's fade is running. */
	Dying,
	/** A scene is playing or holding the player. */
	Scene,
	/** A trip to another level, or a fast travel, is under way. */
	Travelling,
	/** Its mission isn't finished yet (the story keeps the grave shut). */
	NotYet,
	/** The player is farther than its mark radius. */
	TooFar,
	/** Something solid stands between the player and the lantern. */
	NoSight,
};

/** What a walk up to a grave did. */
enum class EGraveMark : uint8
{
	/** Nothing: it was blocked, or it is the save point already. */
	None,
	/** Its first mark: the lantern catches, the banner shows, the grave opens (if the story hadn't) and becomes the save point. */
	First,
	/** A grave marked before, becoming the save point again: the flame lifts, no banner. */
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
	/** MarkAfterMission (or, without one, ActiveAfterMission) is finished, or the grave has none. */
	bool bStoryAllows = true;
	/** From the player's feet to the grave (cm). */
	double Distance = TNumericLimits<double>::Max();
	double Radius = 600.0;
	/** A line from the player's eyes to the lantern is clear (looked at last, only when everything else allows). */
	bool bClearLine = true;
};

/**
 * Borderlands-style save points as plain rules, apart from the world so the tests drive them: when walking up to a respawn
 * grave marks it, what that does (a first mark, or the save point moving back to a grave marked before), and how the
 * campaign record keeps it. A grave marked is the last one of its area (FCampaignRecord::LastGraves, which Continue wakes
 * at) and stays lit for good (FCampaignRecord::MarkedGraves). A save from before save points reads as nothing marked,
 * which is right.
 */
namespace GraveMarkRules
{
	/** Closer than this a grave is marked (cm), unless the grave says another. */
	inline constexpr double DefaultRadius = 600.0;

	/**
	 * The lantern's flare (seconds): a first mark's light and flame spike at once and ease down over FirstFlareSeconds; a grave
	 * marked again lifts its flame for the shorter LiftSeconds. The rise to the peak takes FlareRiseSeconds in both.
	 */
	inline constexpr float FirstFlareSeconds = 1.1f;
	inline constexpr float LiftSeconds = 0.7f;
	inline constexpr float FlareRiseSeconds = 0.09f;

	/** How long Kind's flare lasts (0 for None). */
	AI_LOOTER_SHOOTER_API float FlareSeconds(EGraveMark Kind);

	/** The flare's strength Age seconds in: 0 before it and after it, rising to 1 by FlareRiseSeconds, then easing back to 0. */
	AI_LOOTER_SHOOTER_API float FlarePulse(float Age, EGraveMark Kind);

	/** The grave Marker has been marked at some point in this story. */
	AI_LOOTER_SHOOTER_API bool IsMarked(const FCampaignRecord& Campaign, FName Marker);

	/** Marker is the grave last marked in Area (the save point Continue wakes at). */
	AI_LOOTER_SHOOTER_API bool IsSavePoint(const FCampaignRecord& Campaign, FName Area, FName Marker);

	/** Whether walking up to the grave marks it now, with the first block that holds, or None. */
	AI_LOOTER_SHOOTER_API EGraveMarkBlock Check(const FGraveMarkSenses& Senses);

	/** What a walk up to the grave does when nothing blocks it: a first mark, the save point moving back, or nothing (it is it already). */
	AI_LOOTER_SHOOTER_API EGraveMark Outcome(bool bMarkedBefore, bool bIsSavePoint);

	/**
	 * Writes the mark into Campaign: Marker is marked for good and becomes Area's save point. Returns what it was (None when
	 * it already was both). It does not open the grave: ARespawnMarker::Activate does.
	 */
	AI_LOOTER_SHOOTER_API EGraveMark Record(FCampaignRecord& Campaign, FName Area, FName Marker);

	/** Takes the mark back (the console, to see a first mark again): Marker is no longer marked, nor Area's save point. False when it wasn't marked. */
	AI_LOOTER_SHOOTER_API bool Forget(FCampaignRecord& Campaign, FName Area, FName Marker);

	/** The console's and the map's words for a block ("too far", "its mission isn't finished"); empty for None. */
	AI_LOOTER_SHOOTER_API FString Describe(EGraveMarkBlock Block);
}
