#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/StanceIntent.h"
#include "Settings/PawnInputBinding.h"
#include "PlayerLocomotionComponent.generated.h"

class AController;
class ACharacter;
class APawn;
class UCameraComponent;
class UCharacterMovementComponent;
class UKeyBindingSubsystem;
class UPlayerViewComponent;
class UWeaponManagerComponent;

/**
 * Sprint and crouch for the player character, plus the first-person motion that sells them.
 *
 *  - Input: binds the code-built Sprint/Crouch actions from UKeyBindingSubsystem (rebindable, hold or toggle).
 *  - Movement: sprint raises MaxWalkSpeed while moving forward; crouch uses the character's built-in crouch
 *    (shorter capsule, MaxWalkSpeedCrouched) and tightens weapon spread.
 *  - Animation: exposes smoothed Sprint/Crouch alphas that ULooterCharacterAnimInstance turns into a full-body
 *    crouch/lean (which the first-person arms and camera inherit), and drives the held weapon procedurally:
 *    sprint carry pose, step bob, look sway, crouch cant, jump/land kick, and each shot's recoil kick (from the view
 *    component). Sprint also widens the FOV a little (via GetFieldOfViewOffset).
 *
 * Firing ends a sprint (the gun comes up immediately). Everything resets when the character loses its controller
 * (Build Mode, death), so no key can get stuck.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UPlayerLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerLocomotionComponent();

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	bool IsSprinting() const { return bSprinting; }

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	bool IsCrouching() const;

	/** 0..1, eased. How far into the sprint pose the body and weapon are. */
	UFUNCTION(BlueprintPure, Category = "Locomotion|Animation")
	float GetSprintAlpha() const { return SprintAlpha; }

	/** 0..1, eased. How far into the crouch pose the body is. Follows the actual capsule, not the key. */
	UFUNCTION(BlueprintPure, Category = "Locomotion|Animation")
	float GetCrouchAlpha() const { return CrouchAlpha; }

	/** Height above the feet (component space) the head should sit at when fully crouched. */
	float GetCrouchedHeadHeight() const;

	/** Multiplier for weapon spread from the current stance (crouching steadies your aim). */
	UFUNCTION(BlueprintPure, Category = "Locomotion")
	float GetSpreadMultiplier() const;

	/** Degrees the camera's field of view should widen right now (sprinting). Applied by the view component. */
	float GetFieldOfViewOffset() const { return SprintFovBoost * SprintAlpha; }

	// --- Tuning ---

	/** Sprint speed as a multiple of the character's normal walk speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Sprint", meta = (ClampMin = "1"))
	float SprintSpeedMultiplier = 1.55f;

	/** Movement input must point within this many degrees of where you face to sprint (no sideways/backward sprint). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Sprint", meta = (ClampMin = "0", ClampMax = "90"))
	float SprintMaxInputAngle = 60.f;

	/** After you stop shooting, a held sprint key resumes sprinting after this long (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Sprint", meta = (ClampMin = "0"))
	float SprintResumeDelay = 0.3f;

	/** A toggled sprint ends when you stop moving for this long (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Sprint", meta = (ClampMin = "0"))
	float SprintToggleStopGrace = 0.25f;

	/** Degrees added to the camera's field of view at full sprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Sprint", meta = (ClampMin = "0"))
	float SprintFovBoost = 6.f;

	/** Capsule half height while crouched (standing is 96). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Crouch", meta = (ClampMin = "30"))
	float CrouchedHalfHeight = 68.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Crouch", meta = (ClampMin = "0"))
	float CrouchSpeed = 300.f;

	/** How far below the crouched capsule's top the head (and camera) sits, so low ceilings never clip the view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Crouch", meta = (ClampMin = "0"))
	float CrouchHeadClearance = 16.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Crouch", meta = (ClampMin = "0", ClampMax = "1"))
	float CrouchSpreadMultiplier = 0.75f;

	/** Seconds to blend into / out of the stance poses. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Animation", meta = (ClampMin = "0.01"))
	float SprintBlendTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Animation", meta = (ClampMin = "0.01"))
	float CrouchBlendTime = 0.22f;

	/** Blend-out time when a sprint is interrupted by shooting (the gun has to come up fast). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Animation", meta = (ClampMin = "0.01"))
	float SprintInterruptBlendTime = 0.08f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);

	void SetupInput(AController* Controller);
	void TeardownInput();
	UKeyBindingSubsystem* GetBindings() const;

	void HandleSprintPressed();
	void HandleSprintReleased();
	void HandleCrouchPressed();
	void HandleCrouchReleased();
	void RefreshModes();
	void ReleaseKeysNoLongerHeld();

	void UpdateStance(float DeltaTime);
	void UpdateAlphas(float DeltaTime);
	void UpdateCamera();
	void UpdateViewModel(float DeltaTime);

	bool IsMovingForward() const;
	bool IsWeaponFiring() const;

	TWeakObjectPtr<ACharacter> Character;
	TWeakObjectPtr<UCharacterMovementComponent> Movement;
	TWeakObjectPtr<UCameraComponent> Camera;
	TWeakObjectPtr<UWeaponManagerComponent> WeaponManager;
	/** Holds the recoil springs the first-person gun kicks with. */
	TWeakObjectPtr<UPlayerViewComponent> PlayerView;

	/**
	 * What we lower to crouch the view: the first-person arms the camera rides on (so arms, camera and gun move together),
	 * or the camera itself. The template's first-person rig holds the head steady, so the body's crouch alone doesn't move it.
	 */
	TWeakObjectPtr<USceneComponent> EyeRig;
	FVector EyeRigBaseLocation = FVector::ZeroVector;
	/** Camera height above the feet when standing, measured on the ground (the crouch drop is derived from it). */
	float StandingEyeHeight = -1.f;

	FPawnInputBinding InputBinding;

	FStanceIntent Intent;
	bool bSprinting = false;
	float BaseWalkSpeed = 600.f;
	float LastFiringTime = -100.f;
	float NotMovingTime = 0.f;

	// Animation state
	float SprintAlpha = 0.f;       // eased value handed out
	float SprintLinear = 0.f;      // linear ramp behind it
	float CrouchAlpha = 0.f;
	float CrouchLinear = 0.f;
	bool bSprintInterrupted = false;

	// View model
	float StepPhase = 0.f;
	float MoveWeight = 0.f;
	float IdleTime = 0.f;
	FRotator LastControlRotation = FRotator::ZeroRotator;
	FVector2D LookSway = FVector2D::ZeroVector;   // yaw, pitch lag in degrees
	float KickOffset = 0.f;        // vertical spring (cm) for jumps, landings, stance changes
	float KickVelocity = 0.f;
	bool bWasFalling = false;
	bool bWasCrouched = false;
	float FallSpeed = 0.f;
};
