#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "BonePose.h"
#include "LooterCharacterAnimInstance.generated.h"

class UPlayerLocomotionComponent;
class UPlayerViewComponent;
class UWeaponManagerComponent;

/** Everything the stance layer needs, copied from the game thread once per update. */
struct FLooterStanceInput
{
	float CrouchAlpha = 0.f;
	float SprintAlpha = 0.f;
	/** Component-space height the head settles at when fully crouched. */
	float CrouchedHeadHeight = 120.f;
	/** The character's facing, in the mesh's component space (horizontal, unit length). */
	FVector Forward = FVector::YAxisVector;

	float CrouchTorsoLean = 0.f;
	float SprintTorsoLean = 0.f;
	float CrouchHipsBack = 0.f;
	float MaxHipDrop = 0.f;
	float KneeSplay = 0.f;

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

	/** Upper-body pose held over the locomotion (armed): the animation and where in its loop we are. */
	const UAnimSequenceBase* UpperBodyPose = nullptr;
	float UpperBodyTime = 0.f;

	/** Upper-body reload overlay: the animation, where in it we are, and how strongly it shows. */
	const UAnimSequenceBase* ReloadAnimation = nullptr;
	float ReloadTime = 0.f;
	float ReloadWeight = 0.f;

	bool NeedsLayer() const
	{
		return CrouchAlpha > UE_KINDA_SMALL_NUMBER || SprintAlpha > UE_KINDA_SMALL_NUMBER || bAimWithTorso || bHandOnForegrip
			|| ReloadWeight > UE_KINDA_SMALL_NUMBER || UpperBodyPose;
	}
};

/**
 * Runs the Anim Blueprint's graph as usual, then layers the procedural stance pose on top of the result:
 *  - crouch: torso leans forward, hips drop until the head reaches the crouched head height, and both legs are
 *    solved with two-bone IK back onto the feet the graph planted (so crouch-walking keeps the walk cycle's steps)
 *  - sprint: the torso leans into the run
 *  - armed: the torso pitches with the player's aim, and the left hand is solved onto the held gun's foregrip
 *    (the rifle animations were made for a different gun)
 *  - recoil: each shot rocks the chest back and lets the right arm give, so the gun (in that hand) jumps back and the
 *    left hand rides along on the foregrip
 */
struct FLooterCharacterAnimInstanceProxy : public FAnimInstanceProxy
{
	FLooterCharacterAnimInstanceProxy() = default;
	explicit FLooterCharacterAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) override;

private:
	void ApplyStance(FPoseContext& Output) const;
	void ApplyUpperBodyPose(FPoseContext& Output) const;
	void ApplyReloadOverlay(FPoseContext& Output) const;
	void ApplyRecoil(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FVector& Right) const;
	void ApplyLeftHandOnForegrip(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FVector& Right) const;

	FLooterStanceInput Stance;
};

/**
 * Parent class for character Anim Blueprints (ABP_Unarmed). Reads the owner's UPlayerLocomotionComponent and adds
 * the crouch and sprint body poses the template animations don't have. Characters without that component
 * (target dummies) are left untouched.
 */
UCLASS(Transient, Blueprintable, BlueprintType)
class AI_LOOTER_SHOOTER_API ULooterCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Stance")
	float CrouchAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stance")
	float SprintAlpha = 0.f;

	/** Forward lean of the torso when fully crouched (degrees, spread over the spine). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance")
	float CrouchTorsoLean = 18.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance")
	float SprintTorsoLean = 9.f;

	/** Hips push back this far (cm) when crouched, to balance the lean. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance")
	float CrouchHipsBack = 7.f;

	/** Never drop the hips further than this (cm), so the legs always have a solution. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance")
	float MaxHipDrop = 50.f;

	/** Knees point this far (cm) outward from straight ahead when bent. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance")
	float KneeSplay = 12.f;

	/** Armed Anim Blueprints (ABP_Rifle): the torso follows the aim and the left hand holds the gun's foregrip. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance")
	bool bHoldsWeapon = false;

	/** Aim pitch the torso follows, in degrees either way; the head does the rest. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance", meta = (ClampMin = "0", ClampMax = "90"))
	float MaxTorsoAimPitch = 65.f;

	/** Armed Anim Blueprints: played on the upper body (over any locomotion) while the held weapon reloads. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance")
	TObjectPtr<UAnimSequenceBase> ReloadAnimation;

	/**
	 * Armed Anim Blueprints: the upper body (spine and up) holds this pose, looping, over whatever the legs play, with
	 * the spine kept upright in mesh space so hip sway doesn't rock the gun. The legs can then use any clean
	 * locomotion set; the rifle's own directional clips don't share one cycle and jittered when blended.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance")
	TObjectPtr<UAnimSequenceBase> UpperBodyPose;

	/** Game thread: reads the owner's locomotion component into StanceInput (and the Blueprint-visible alphas). */
	void RefreshStanceInput(float DeltaSeconds);
	const FLooterStanceInput& GetStanceInput() const { return StanceInput; }

	/**
	 * For a character shown without a pawn (the loadout screen's stand-in): hold a gun as if standing still and aiming
	 * level, left hand on its foregrip. Grip and foregrip are in the gun's own space; the gun sits in HoldSocket and points
	 * the way the character faces (Facing, world space).
	 */
	void SetStandaloneHold(const FVector& Grip, const FVector& Foregrip, FName HoldSocket, const FQuat& Facing);
	void ClearStandaloneHold() { StandaloneHold.Reset(); }

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
	struct FStandaloneHold
	{
		FVector Grip = FVector::ZeroVector;
		FVector Foregrip = FVector::ZeroVector;
		FName Socket = NAME_None;
		FQuat Facing = FQuat::Identity;
	};

	FLooterStanceInput StanceInput;
	TOptional<FStandaloneHold> StandaloneHold;
	float UpperBodyTime = 0.f;
	TWeakObjectPtr<const UPlayerLocomotionComponent> Locomotion;
	TWeakObjectPtr<const UWeaponManagerComponent> WeaponManager;
	TWeakObjectPtr<const UPlayerViewComponent> View;
};
