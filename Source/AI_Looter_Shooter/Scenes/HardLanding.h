#pragma once

#include "CoreMinimal.h"
#include "Scenes/SceneSubsystem.h"

class ALamplighterShip;

/**
 * Landfall's hard landing (Main 1 step 3: the missions wait for Scene.HardLanding). Both grapple lines are cut and the
 * last one tore the Lamplighter's bag as it parted: she breaks free (ALamplighterShip's Breaking, the player still on their
 * feet for BreakSeconds), then this scene holds the player on her deck (look only) while Holloway takes her down: she sinks
 * faster and faster toward the Mooring Ledge, nose down and listing to her torn side, the gas jetting from her bag, the
 * view shaking; she flares at the last moment, hits the ledge hard (the crash, dust thrown out round her hull, sparks off
 * her keel, the jolt), skids on in a judder of sparks and stops, rocking, exactly at Moored. About 5.8 s; skippable (a skip
 * lands her at Moored with the crash's state and none of its noise).
 *
 * Her path is plain math of the time since she broke free (PoseAt), so the ship's own breaking and the scene are one
 * smooth fall, and the tests check it.
 */
namespace HardLanding
{
	/** "HardLanding": the missions hear Scene.HardLanding. */
	AI_LOOTER_SHOOTER_API FName SceneName();

	/** Seconds from breaking free: the scene takes over; the hull meets the ledge; the skid; the rock dying away. */
	inline constexpr float BreakSeconds = 2.f;
	inline constexpr float TouchAt = 6.f;
	inline constexpr float SkidSeconds = 0.8f;
	inline constexpr float SettleSeconds = 1.f;
	inline constexpr float TotalSeconds = TouchAt + SkidSeconds + SettleSeconds;
	/** The scene's own length (it starts at BreakSeconds). */
	inline constexpr float SceneSeconds = TotalSeconds - BreakSeconds;

	/** How far she lists to her torn side, noses down, and flares before the hit, at most (degrees). */
	inline constexpr float MaxList = 9.f;
	inline constexpr float MaxNoseDown = 6.f;
	inline constexpr float Flare = 3.f;
	/** How far she squashes into the ledge on the hit and bounces (cm). */
	inline constexpr float Squash = 25.f;

	/** How far down she's come (0-1): slowly at first, faster and faster, all the way at TouchAt. */
	AI_LOOTER_SHOOTER_API float DropAt(float Seconds);

	/** How far across she's come (0-1): eased out of the stop, arriving with the skid's end. */
	AI_LOOTER_SHOOTER_API float GlideAt(float Seconds);

	/**
	 * Her pose Seconds after breaking free from From, down to Moored: along the glide and the drop, listing toward TornSide
	 * (+1 her starboard, -1 port), nosing down and flaring, squashing and rocking after the hit. Moored exactly from
	 * TotalSeconds on.
	 */
	AI_LOOTER_SHOOTER_API FTransform PoseAt(const FTransform& From, const FTransform& Moored, float Seconds, float TornSide);

	/** The scene for USceneSubsystem::Play: the player carried on her deck, look only, until she lies moored. */
	AI_LOOTER_SHOOTER_API FScenePlay Make(USceneSubsystem& Scenes, ALamplighterShip& Ship);
}
