#pragma once

#include "CoreMinimal.h"
#include "Scenes/SceneSubsystem.h"

class AActor;
class AFerryLadder;

/**
 * A climb on the Ferryman's rope ladder (AFerryLadder: Gravewind Point's deck to the landing under it), as a short scene
 * like the ore lift's ride: look only, skippable. The view turns to the ladder (TurnSeconds), goes down (or up) its rungs
 * hand over hand, a step and a settle at each (ClimbSeconds), and steps off facing the way on (StepOffSeconds); the rope
 * creaks the while. The climber's own body is stood at the far end, unseen, as it starts, so a skip, a save or a quit
 * mid-climb finds them there, and the view ends at that body's eyes so it goes back to them without a seam.
 */
namespace FerryLadderClimb
{
	/** The scene ("FerryLadder": the missions hear Scene.FerryLadder). */
	AI_LOOTER_SHOOTER_API FName SceneName();

	inline constexpr float TurnSeconds = 0.4f;
	inline constexpr float ClimbSeconds = 1.8f;
	inline constexpr float StepOffSeconds = 0.4f;

	/** The whole climb (s). */
	AI_LOOTER_SHOOTER_API float Duration();

	/** How far down the ladder's rungs the climb is at Alpha (0-1 through ClimbSeconds): a step and a settle at each rung. */
	AI_LOOTER_SHOOTER_API float RungAlpha(float Alpha, int32 Rungs);

	/**
	 * The climber's eye Seconds in: from StartEye to the ladder's near end (OnFrom), down (or up) to its far end (OnTo), and
	 * off it to EndEye. Plain math: the tests walk it.
	 */
	AI_LOOTER_SHOOTER_API FVector EyeAt(float Seconds, const FVector& StartEye, const FVector& OnFrom, const FVector& OnTo,
		const FVector& EndEye, int32 Rungs);

	/** The climb as a scene for USceneSubsystem::Play: Climber down (bDown) or up Ladder. */
	AI_LOOTER_SHOOTER_API FScenePlay Make(USceneSubsystem& Scenes, AFerryLadder& Ladder, AActor& Climber, bool bDown);
}
