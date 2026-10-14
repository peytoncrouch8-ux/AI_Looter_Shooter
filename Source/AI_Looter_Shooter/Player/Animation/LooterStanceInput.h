#pragma once

#include "CoreMinimal.h"
#include "Player/Animation/LooterClimbPose.h"
#include "Player/Animation/LooterStancePoseCarry.h"

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
	/** How far out along its barrel the hands push the held gun (cm; its definition's HoldReach): a six-gun at arm's length. */
	float HoldReach = 0.f;
	/**
	 * How much bigger than modeled the held gun is drawn (UWeaponModelComponent::ScaleFor): the grip points above already
	 * are; spots the stance layer keeps as numbers off the gun's model (the six-gun's cylinder) are scaled by it.
	 */
	float WeaponScale = 1.f;

	/**
	 * Carried rather than shouldered (LooterStancePoseCarry.h): how far, and the gun's turn with no kick in it, where the
	 * hands hold it: shouldered along the aim (AimRotation), and as carried now (CarryRotation; the aim's when not carried).
	 * Component space. The hands turn from the one to the other, so the gun keeps its place in them.
	 */
	FLooterCarry Carry;
	FQuat AimRotation = FQuat::Identity;
	FQuat CarryRotation = FQuat::Identity;

	/** The gun's recoil right now: how far it has kicked back toward the shooter (cm) and flipped up (degrees). */
	float RecoilBack = 0.f;
	float RecoilPitch = 0.f;

	/** A melee strike's jab right now (LooterStancePose::MeleeJab): a little below 0 cocked back, 1 at the blow, 0 at rest. */
	float MeleeJab = 0.f;

	/**
	 * A grenade throw's clock (seconds in, UPlayerThrowComponent::GetThrowTime; negative with none under way): the left
	 * arm's toss, the chest's turn and the gun hand's dip (LooterStancePose::DescribeThrow).
	 */
	float ThrowTime = -1.f;

	/**
	 * A revolver's reload (the Drover's cylinder): its progress, 0 to 1, or negative when the gun in hand isn't reloading
	 * one. It replaces the reload animation (ReloadAnimation is null then) and shows by ReloadWeight like it
	 * (LooterStancePose::DescribeCylinderReload).
	 */
	float CylinderReload = -1.f;

	/** Upper-body pose held over the locomotion (armed): the animation and where in its loop we are. */
	const UAnimSequenceBase* UpperBodyPose = nullptr;
	float UpperBodyTime = 0.f;

	/**
	 * Upper-body reload overlay: the animation, where in it we are, and how strongly the reload shows (a revolver's too,
	 * with no animation: CylinderReload).
	 */
	const UAnimSequenceBase* ReloadAnimation = nullptr;
	float ReloadTime = 0.f;
	float ReloadWeight = 0.f;

	bool NeedsLayer() const
	{
		return CrouchAlpha > UE_KINDA_SMALL_NUMBER || SprintAlpha > UE_KINDA_SMALL_NUMBER || SlideAlpha > UE_KINDA_SMALL_NUMBER
			|| Climb.IsActive() || bAimWithTorso || bHandOnForegrip || ReloadWeight > UE_KINDA_SMALL_NUMBER || UpperBodyPose
			|| !FMath::IsNearlyZero(MeleeJab) || ThrowTime > 0.f;
	}
};
