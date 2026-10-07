#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerSlide.h"
#include "Player/StanceIntent.h"
#include "Player/PawnInputBinding.h"
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
 * Sprint, crouch and slide for the player character, plus the first-person motion that sells them.
 *
 *  - Input: binds the code-built Sprint/Crouch actions from UKeyBindingSubsystem (rebindable, hold or toggle). The
 *    character hands it the jump key too: while crouched, jump stands up instead (PlayerLocomotionSlide.cpp).
 *  - Movement: sprint raises MaxWalkSpeed while moving forward; crouch uses the character's built-in crouch
 *    (shorter capsule, MaxWalkSpeedCrouched) and tightens weapon spread. Crouching out of a sprint slides
 *    (FPlayerSlide): the crouched capsule carried along the run, 10% faster, for a moment.
 *  - Animation: exposes smoothed Sprint/Crouch/Slide alphas that ULooterCharacterAnimInstance turns into a full-body
 *    crouch/lean/slide pose, and drives the first-person view and held weapon procedurally
 *    (PlayerLocomotionViewModel.cpp): the eye dropping into a crouch or slide, sprint carry pose, step bob, look sway,
 *    crouch cant, jump/land kick, and each shot's recoil kick (from the view component). Sprinting and sliding also
 *    widen the FOV a little (via GetFieldOfViewOffset).
 *
 * Speeds and heights here are for the full-size body (Player/PlayerSize.h): the character's scale shrinks the heights,
 * and the crouch speed default is already scaled. Firing ends a sprint (the gun comes up immediately). Everything
 * resets when the character loses its controller (death, unpossess), so no key can get stuck.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UPlayerLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerLocomotionComponent();

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	bool IsCrouching() const;

	/** 0..1, eased. How far into the sprint pose the body and weapon are. */
	UFUNCTION(BlueprintPure, Category = "Locomotion|Animation")
	float GetSprintAlpha() const { return SprintAlpha; }

	/** 0..1, eased. How far into the crouch pose the body is. Follows the actual capsule, not the key. */
	UFUNCTION(BlueprintPure, Category = "Locomotion|Animation")
	float GetCrouchAlpha() const { return CrouchAlpha; }

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	bool IsSliding() const { return Slide.IsActive(); }

	/** 0..1, eased. How far into the slide pose the body (and the first-person view) is. */
	UFUNCTION(BlueprintPure, Category = "Locomotion|Animation")
	float GetSlideAlpha() const { return SlideAlpha; }

	/** The slide under way (or the last one): its line, speed and time, for the tests and the debug line. */
	const FPlayerSlide& GetSlide() const { return Slide; }

	/**
	 * Height above the feet the head should sit at when fully crouched, in the full-size body's units: the mesh's
	 * component space (the character's scale shrinks it in the world).
	 */
	float GetCrouchedHeadHeight() const;

	/** Multiplier for weapon spread from the current stance (crouching steadies your aim; a slide doesn't). */
	UFUNCTION(BlueprintPure, Category = "Locomotion")
	float GetSpreadMultiplier() const;

	/** Degrees the camera's field of view should widen right now (sprinting or sliding). Applied by the view component. */
	float GetFieldOfViewOffset() const { return SprintFovBoost * FMath::Max(SprintAlpha, SlideAlpha); }

	// --- The keys (bound to the player's input; public so the tests can press them) ---

	void HandleSprintPressed();
	void HandleSprintReleased();
	/** Crouch; out of a sprint on the ground, a slide. */
	void HandleCrouchPressed();
	void HandleCrouchReleased();
	/**
	 * The jump key (the character passes it on): jumps, except while crouched or sliding, when it stands up instead
	 * (or does nothing when there's no room to stand). The next press jumps.
	 */
	void HandleJumpPressed();
	/**
	 * The movement keys (X strafe, Y forward), passed on by the character every frame they're down, even while a slide
	 * ignores them for steering: whether the player runs forward decides the sprint and how a slide ends.
	 */
	void HandleMoveInput(const FVector2D& Input);

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

	/** Capsule half height while crouched (standing is 96), before the character's scale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Crouch", meta = (ClampMin = "30"))
	float CrouchedHalfHeight = 68.f;

	/** Crouched walking speed (cm/s): the full-size 300, scaled with the player (Player/PlayerSize.h). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Crouch", meta = (ClampMin = "0"))
	float CrouchSpeed = LooterPlayerSize::FullSizeCrouchSpeed * LooterPlayerSize::SpeedScale;

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

	void RefreshModes();
	void ReleaseKeysNoLongerHeld();

	void UpdateStance(float DeltaTime);
	void UpdateAlphas(float DeltaTime);
	void UpdateCamera();
	void UpdateViewModel(float DeltaTime);

	/** Starts a slide if the character is sprinting on the ground (PlayerLocomotionSlide.cpp). */
	bool TryStartSlide();
	/** Moves a slide on: ends it on time, against a wall or off the ground; otherwise holds its speed and line. */
	void UpdateSlide(float DeltaTime);
	/** Ends a slide (if one is under way) and gives the crouched walk its own speed back. */
	void EndSlide();
	/**
	 * How a slide would end right now: back into the sprint (forward held, and not a held crouch key), else in the crouch.
	 * The slide eases to the speed that goes with it (GetSlideExitSpeed), so it hands over without a dip.
	 */
	bool SlideEndsInSprint() const;
	float GetSlideExitSpeed() const;
	/** Out of a slide with forward held: stands at once (if there's room) and runs on in the sprint. */
	void SprintOutOfSlide();

	/** The movement keys held this frame (HandleMoveInput); zero once they stop coming. */
	FVector2D GetHeldMoveInput() const;
	bool IsMovingForward() const;
	bool IsWeaponFiring() const;
	/** The character's scale against the full-size body (1 for any character not scaled). */
	float GetBodyScale() const;

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
	/**
	 * Camera height above the feet when standing, measured on the ground, in the full-size body's units (the crouch drop
	 * is derived from it).
	 */
	float StandingEyeHeight = -1.f;
	/** The roll a slide tips the first-person view by, as last given to the camera. */
	float AppliedViewRoll = 0.f;

	FPawnInputBinding InputBinding;

	FStanceIntent Intent;
	FPlayerSlide Slide;
	/** A slide set the crouched walk's speed, and it still has to be given back (the slide may already have ended). */
	bool bSlideHoldsSpeed = false;
	/** The slide under way will end in the sprint (as of this frame's keys): the sprint pose comes in as it eases out. */
	bool bSlideExitsToSprint = false;
	/** The movement keys as the character last passed them on, and the frame it did (GFrameCounter). */
	FVector2D MoveInput = FVector2D::ZeroVector;
	uint64 MoveInputFrame = 0;
	/** The character passes the keys on (the player's does), so they are read from there rather than the last move. */
	bool bHasMoveInput = false;
	bool bSprinting = false;
	float BaseWalkSpeed = 600.f;
	float LastFiringTime = -100.f;
	float NotMovingTime = 0.f;

	// Animation state
	float SprintAlpha = 0.f;       // eased value handed out
	float SprintLinear = 0.f;      // linear ramp behind it
	float CrouchAlpha = 0.f;
	float CrouchLinear = 0.f;
	float SlideAlpha = 0.f;
	float SlideLinear = 0.f;
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
	/** 0..1 toward the lowered pose while a menu is open. */
	float MenuLinear = 0.f;
};
