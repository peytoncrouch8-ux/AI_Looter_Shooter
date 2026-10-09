#pragma once

#include "CoreMinimal.h"
#include "Player/Animation/LooterClimbPose.h"

class UAnimSequenceBase;

/** Everything the stance layer needs, copied from the game thread once per update. */
struct FLooterStanceInput
{
	float CrouchAlpha = 0.f;
	float SprintAlpha = 0.f;
	float SlideAlpha = 0.f;
	/** Component-space height the head settles at when fully crouched. */
	float CrouchedHeadHeight = 120.f;
	/** The character's facing, in the mesh's component space (horizontal, unit length). */
	FVector Forward = FVector::YAxisVector;

	float CrouchTorsoLean = 0.f;
	float SprintTorsoLean = 0.f;
	float CrouchHipsBack = 0.f;
	float MaxHipDrop = 0.f;
	float KneeSplay = 0.f;
	float SlideTorsoLean = 0.f;
	float SlideHipHeight = 0.f;
	float SlideLegReach = 0.f;

	/** A mantle or vault under way: the legs tucked, the hands on the obstacle, the lean (Player/Animation/LooterClimbPose.h). */
	FLooterClimbInput Climb;

	/** Armed Anim Blueprints only: how far the character aims up (+) or down (-), in degrees. */
	float AimPitch = 0.f;
	bool bAimWithTorso = false;
	/** A weapon is in hand: the left hand goes to its foregrip. Grip/foregrip are in the weapon's own space. */
	bool bHandOnForegrip = false;
	FVector WeaponGrip = FVector::ZeroVector;
	FVector WeaponForegrip = FVector::ZeroVector;
	/** The socket the gun's grip sits in: its bone and its transform relative to that bone. */
	FName HoldBone = NAME_None;
	FTransform HoldSocketLocal = FTransform::Identity;
	/** Which way the held gun points, in component space (it follows the aim, see UPlayerViewComponent). */
	FQuat WeaponRotation = FQuat::Identity;

	/** The gun's recoil right now: how far it has kicked back toward the shooter (cm) and flipped up (degrees). */
	float RecoilBack = 0.f;
	float RecoilPitch = 0.f;

	/** A melee strike's jab right now (LooterStancePose::MeleeJab): a little below 0 cocked back, 1 at the blow, 0 at rest. */
	float MeleeJab = 0.f;

	/** Upper-body pose held over the locomotion (armed): the animation and where in its loop we are. */
	const UAnimSequenceBase* UpperBodyPose = nullptr;
	float UpperBodyTime = 0.f;

	/** Upper-body reload overlay: the animation, where in it we are, and how strongly it shows. */
	const UAnimSequenceBase* ReloadAnimation = nullptr;
	float ReloadTime = 0.f;
	float ReloadWeight = 0.f;

	bool NeedsLayer() const
	{
		return CrouchAlpha > UE_KINDA_SMALL_NUMBER || SprintAlpha > UE_KINDA_SMALL_NUMBER || SlideAlpha > UE_KINDA_SMALL_NUMBER
			|| Climb.IsActive() || bAimWithTorso || bHandOnForegrip || ReloadWeight > UE_KINDA_SMALL_NUMBER || UpperBodyPose
			|| !FMath::IsNearlyZero(MeleeJab);
	}
};
