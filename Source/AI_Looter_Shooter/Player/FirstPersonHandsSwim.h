#pragma once

#include "CoreMinimal.h"
#include "Player/FirstPersonHandsTypes.h"

/**
 * The first-person hands swimming (round 4): a breaststroke seen from the eye, in camera space (X ahead, Y right, Z up; cm).
 * The hands come up together under the chin, reach out ahead, sweep apart and back, and drop under the frame to come round
 * again; treading water, they scull in small circles low in the view. UFirstPersonHandsComponent::PlaceSwimHands
 * (FirstPersonHandsSwim.cpp) puts them there once the gun is lowered away (UPlayerLocomotionComponent::GetSwimHandsShare),
 * rising from their rest under the frame. Plain functions of the stroke's phase, so the tests read them (Looter.Swim.Hands).
 */
namespace FirstPersonHandsSwim
{
	/** The right hand in the stroke at Phase (0..1, a cycle); the left is its mirror across the view's middle. */
	AI_LOOTER_SHOOTER_API FHandPlacement Stroke(EFirstPersonHand Hand, float Phase);

	/** Treading water at Time (s): small circles low in the view, the two hands half a turn apart. */
	AI_LOOTER_SHOOTER_API FHandPlacement Tread(EFirstPersonHand Hand, float Time);

	/** The swim's hands: treading blended into the stroke by Effort (0 treading .. 1 a full stroke). */
	AI_LOOTER_SHOOTER_API FHandPlacement Pose(EFirstPersonHand Hand, float Phase, float Effort, float Time);

	/** The same placement for the other hand: mirrored across the view's middle. */
	AI_LOOTER_SHOOTER_API FHandPlacement Mirrored(const FHandPlacement& Placement);
}
