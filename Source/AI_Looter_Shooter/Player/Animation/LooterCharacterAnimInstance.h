#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "BonePose.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/TraversalRules.h"
#include "LooterCharacterAnimInstance.generated.h"

class FNumericProperty;
class UPlayerLocomotionComponent;
class UPlayerMeleeComponent;
class UPlayerViewComponent;
class UWeaponManagerComponent;

/**
 * Runs the Anim Blueprint's graph as usual, then layers the procedural stance pose on top of the result
 * (LooterStancePose::Apply: crouch, sprint, slide, mantle and vault, the aim and the gun in the hands).
 */
struct FLooterCharacterAnimInstanceProxy : public FAnimInstanceProxy
{
	FLooterCharacterAnimInstanceProxy() = default;
	explicit FLooterCharacterAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) override;

private:
	void ApplyUpperBodyPose(FPoseContext& Output) const;
	void ApplyReloadOverlay(FPoseContext& Output) const;

	FLooterStanceInput Stance;
};

/**
 * Parent class for character Anim Blueprints (ABP_Unarmed). Reads the owner's UPlayerLocomotionComponent and adds
 * the crouch, sprint, slide, mantle and vault body poses the template animations don't have. Characters without that
 * component (target dummies) are left untouched.
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

	UPROPERTY(BlueprintReadOnly, Category = "Stance")
	float SlideAlpha = 0.f;

	/** Lean of the torso in a full slide (degrees, spread over the spine; negative leans back). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance|Slide")
	float SlideTorsoLean = -24.f;

	/** Height of the hips above the feet in a full slide (cm, the full-size body's): sat down near the ground. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance|Slide")
	float SlideHipHeight = 42.f;

	/** How far ahead of the hips the leading (right) foot reaches in a slide (cm); the left leg folds under it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance|Slide")
	float SlideLegReach = 58.f;

	/** Forward lean of the torso at the height of a mantle / a vault (degrees, spread over the spine), the head kept up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance|Climb")
	float MantleTorsoLean = 18.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stance|Climb")
	float VaultTorsoLean = 24.f;

	/**
	 * The Anim Blueprint's ground speed variable (the template's event graph sets it from the movement component; its
	 * blend spaces sample it). Rescaled to the body's own size before the graph reads it: see NativeThreadSafeUpdateAnimation.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion")
	FName GroundSpeedVariable = TEXT("GroundSpeed");

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
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	/**
	 * Worker thread, after the event graph and before the graph's nodes: the blend spaces' clips were made for the
	 * full-size mannequin, so the ground speed the event graph read off the movement component (world units) goes to
	 * them in the body's own size. A 0.85-size player walking at 0.85 of the speed then plays exactly the full-size jog,
	 * each step covering the ground it does, instead of a part-walk blend with sliding feet. Above the jog (a sprint) the
	 * blend space has no faster clip: the locomotion component plays the body's animation faster to match
	 * (GlobalAnimRateScale, UPlayerLocomotionComponent::UpdateAlphas).
	 */
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

private:
	struct FStandaloneHold
	{
		FVector Grip = FVector::ZeroVector;
		FVector Foregrip = FVector::ZeroVector;
		FName Socket = NAME_None;
		FQuat Facing = FQuat::Identity;
	};

	/** Works out the mantle / vault pose from the locomotion component's move (LooterCharacterAnimInstanceClimb.cpp). */
	void RefreshClimbInput();
	/** A mantle or vault starts: the obstacle's top over the feet, which the hands reach for. */
	void HandleTraversalStarted(ETraversalKind Kind, float TopOverFeet);

	FLooterStanceInput StanceInput;
	TOptional<FStandaloneHold> StandaloneHold;
	float UpperBodyTime = 0.f;
	/** The mesh's scale against the full-size body, read on the game thread each update. */
	float BodyScale = 1.f;
	/** The Blueprint's ground speed variable (GroundSpeedVariable), if it has one, and the value last written to it. */
	const FNumericProperty* GroundSpeedProperty = nullptr;
	double WrittenGroundSpeed = -1.0;
	/** The obstacle's top over the feet as the latest move began (world cm), or LooterClimbPose::UnknownTop. */
	float ClimbTopOverFeet = LooterClimbPose::UnknownTop;
	TWeakObjectPtr<UPlayerLocomotionComponent> Locomotion;
	TWeakObjectPtr<const UWeaponManagerComponent> WeaponManager;
	TWeakObjectPtr<const UPlayerViewComponent> View;
	TWeakObjectPtr<const UPlayerMeleeComponent> Melee;
};
