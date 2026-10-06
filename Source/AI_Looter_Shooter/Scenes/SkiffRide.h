#pragma once

#include "CoreMinimal.h"
#include "Scenes/SceneSubsystem.h"

class AActor;
class USceneComponent;

/**
 * The first cast-off from Skyreach (Docs/Story.md: Leaving the tutorial island). The player stands on the packet skiff's
 * deck with look-only control; the ropes slip (CastOff); the skiff drifts out along its bow while Skyreach falls behind,
 * easing out of its moorings, climbing slowly, turning gently to starboard and bobbing; it sails into a cloud bank and the
 * screen goes white over the last 2.5 s (Whiteout, full white 12 s in). The white then holds through the trip
 * (UTransitionScreenSubsystem), and the player stays held on deck: travel follows.
 *
 * The course is plain math of time, so tests can check it, and a skip lands exactly where the ride would have ended.
 */
namespace SkiffRide
{
	/** The scene ("SkiffRide": the missions hear Scene.SkiffRide), its moments, and the skiff's deck socket. */
	AI_LOOTER_SHOOTER_API FName SceneName();
	AI_LOOTER_SHOOTER_API FName CastOffMoment();
	AI_LOOTER_SHOOTER_API FName WhiteoutMoment();
	AI_LOOTER_SHOOTER_API FName DeckSocket();

	/** Seconds into the ride: the ropes slip, the white begins, the screen is fully white (the ride's end). */
	inline constexpr float CastOffTime = 0.75f;
	inline constexpr float WhiteStart = 9.5f;
	inline constexpr float FullWhiteTime = 12.f;

	/** The skiff's speed once clear of the moorings (cm/s, a gas-bag skiff's drift), reached this long after the ropes slip. */
	inline constexpr float CruiseSpeed = 650.f;
	inline constexpr float EaseOutSeconds = 4.5f;

	/** The slow climb, in height per length of course; and how far it has turned to starboard by full white (degrees). */
	inline constexpr float ClimbPerLength = 0.13f;
	inline constexpr float TurnDegrees = 14.f;

	/** Where the cloud bank's front stands: on the course, where the skiff is this far into the ride. */
	inline constexpr float CloudBankTime = 11.f;

	/** How far along its course the skiff has drifted Seconds into the ride (cm). */
	AI_LOOTER_SHOOTER_API float DistanceAt(float Seconds);

	/** How far it has turned to starboard Seconds into the ride (degrees). */
	AI_LOOTER_SHOOTER_API float TurnAt(float Seconds);

	/** How white the screen is Seconds into the ride: 0 until WhiteStart, 1 from FullWhiteTime. */
	AI_LOOTER_SHOOTER_API float WhiteAt(float Seconds);

	/** Where the skiff is Seconds into the ride, from Moored (where it was moored), its bow along Bow when moored. */
	AI_LOOTER_SHOOTER_API FTransform PoseAt(const FTransform& Moored, const FVector& Bow, float Seconds);

	/**
	 * The skiff's bow along the ground: its mesh's +X (the model's front, Blender's -Y, as the importer turns it), taken from
	 * the component with the deck. Moored at the jetty, its port side (-Y, the gangplank's) faces the jetty, so the bow
	 * points out over the drop and the turn to starboard takes it away from the jetty.
	 */
	AI_LOOTER_SHOOTER_API FVector BowOf(const AActor& Skiff);

	/** Where the player's feet go on the skiff's deck (its Deck socket), and the component carrying it. */
	AI_LOOTER_SHOOTER_API FVector DeckSpotOf(const AActor& Skiff, USceneComponent*& OutCarrier);

	/** The ride as a scene for USceneSubsystem::Play; OnWhiteout runs once it has ended at full white. */
	AI_LOOTER_SHOOTER_API FScenePlay Make(USceneSubsystem& Scenes, AActor& Skiff, FOnSceneMoment OnWhiteout);
}
