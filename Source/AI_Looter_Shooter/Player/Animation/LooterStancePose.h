#pragma once

#include "CoreMinimal.h"

struct FCompactPose;
struct FLooterStanceInput;

/**
 * The procedural stance pose, layered on whatever the Anim Blueprint's graph produced (ULooterCharacterAnimInstance
 * calls it from the animation thread). A free function on a plain pose so the tests can run it on the mannequin's
 * reference pose, bone by bone, without a world:
 *  - crouch: torso leans forward, hips drop until the head reaches the crouched head height, and both legs are solved
 *    with two-bone IK back onto the feet the graph planted (so crouch-walking keeps the walk cycle's steps)
 *  - sprint: the torso leans into the run
 *  - slide: the torso leans back, the hips sit down near the ground, the right leg reaches out ahead and the left folds under
 *  - mantle and vault (Player/Animation/LooterClimbPose.h): the torso leans into the climb, the legs tuck up, the hands
 *    reach for the ledge (a vault plants the free hand)
 *  - armed: the torso pitches with the player's aim, and the left hand is solved onto the held gun's foregrip
 *    (the rifle animations were made for a different gun); a short gun (HoldReach) is first pushed out along its barrel
 *    by the right arm, and its foregrip is on its grip, so both hands hold it out like a pistol
 *  - recoil: each shot rocks the chest back and lets the right arm give, so the gun (in that hand) jumps back and the
 *    left hand rides along on the foregrip
 *  - melee: the strike's jab twists the chest into the blow and drives the right hand (the gun's stock, or a fist) forward
 */
namespace LooterStancePose
{
	/** Layers the stance on Pose in place. False, the pose untouched, for a skeleton without the mannequin's bones. */
	bool Apply(FCompactPose& Pose, const FLooterStanceInput& Stance);

	/**
	 * How far a melee strike's jab is out SwingTime seconds into the swing (UPlayerMeleeComponent::GetSwingTime), on
	 * FMeleeRules' clock: cocked back a little (-0.3) by MeleeMotion::CockSeconds, driven out to 1 at the blow
	 * (ContactSeconds), held a moment and eased back to 0 by the swing's end. 0 outside a swing.
	 */
	float MeleeJab(float SwingTime);
}
