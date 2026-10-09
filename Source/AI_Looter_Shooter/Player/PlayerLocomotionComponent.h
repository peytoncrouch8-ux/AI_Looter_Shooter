#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerSlide.h"
#include "Player/PlayerTraversal.h"
#include "Player/SlideDust.h"
#include "Player/StanceIntent.h"
#include "Player/PawnInputBinding.h"
#include "Player/TraversalProbe.h"
#include "Player/ViewEase.h"
#include "PlayerLocomotionComponent.generated.h"

class AController;
class ACharacter;
class APawn;
class UAudioComponent;
class UCameraComponent;
class UCharacterMovementComponent;
class UKeyBindingSubsystem;
class UPlayerViewComponent;
class UWeaponManagerComponent;

/** A mantle or vault starting, or landing: its kind and the height climbed (the obstacle's top over the feet, cm). */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayerTraversal, ETraversalKind /*Kind*/, float /*Height*/);

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
 *  - The view never jumps: the eye's height above the feet and the slide's roll ease on minimum-jerk curves (FViewEase)
 *    whatever the capsule does. The engine's crouch changes the capsule in one frame (and sets the body's place in it
 *    without moving it there); the eye absorbs it, so the camera moves only on its curve (UpdateCamera).
 *  - A slide kicks up dust and grit from the feet (FSlideDust) and plays its sounds (Player.Slide, Player.SlideLoop).
 *  - Traversal (PlayerLocomotionTraversal.cpp): the jump key at a chest-high ledge climbs onto it (a mantle), and at a
 *    fence or low wall taken at a run vaults over it (FTraversalProbe finds them, FPlayerTraversal plans the move); a
 *    ledge met in the air with forward held is caught. The movement stands aside during a move (its custom mode) while
 *    the capsule rides the plan; the eye follows its own smooth curve, the gun lowers a little, and the view settles
 *    with a soft dip as it ends. The jump forgives (PlayerLocomotionJumpAssist.cpp): a late press off an edge still
 *    jumps (coyote time), an early one before landing jumps on landing (the buffer); a player hung on rocks or props is
 *    nudged free.
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

	/**
	 * Degrees the camera's field of view should widen right now (sprinting or sliding). Applied by the view component.
	 * The two blend as a smooth union, so handing over from one to the other has no kink.
	 */
	float GetFieldOfViewOffset() const { return SprintFovBoost * (1.f - (1.f - SprintAlpha) * (1.f - SlideAlpha)); }

	/**
	 * How far the view is lowered from standing, as a share of a crouch's drop: 0 standing, 1 crouched, more in a slide.
	 * Eased and continuous through every change of stance, the capsule's included; the third-person camera follows it.
	 */
	float GetViewLowering() const;

	/**
	 * The first-person eye's height above the feet right now (world cm), and the standing one it rises back to. Through a
	 * mantle or vault the eye rides its own world path, so this is that path over the feet (it dips well under standing
	 * while the body hops); GetViewLowering follows it, and with it the third-person camera.
	 */
	float GetEyeHeight() const { return Traversal.IsActive() ? TraversalEyeHeight : EyeHeight.GetValue(); }
	float GetStandingEyeHeight() const { return StandingEye; }

	// --- Traversal: mantle, vault, unstick (the view and the body's pose read these) ---

	/** A mantle, vault or unstick is carrying the player: the movement stands aside until it ends. */
	bool IsTraversing() const { return Traversal.IsActive(); }
	/** The move under way (or the last one), and how far through it it is (0..1). */
	ETraversalKind GetTraversalKind() const { return Traversal.GetKind(); }
	float GetTraversalProgress() const { return Traversal.GetProgress(); }
	/** 0..1, eased: how far the climbing pose (the gun lowered and turned in) has come in. A vault takes 0.6 of it. */
	float GetTraversalAlpha() const { return TraversalAlpha; }
	/** The move's whole plan: the capsule's and the eye's paths (the tests, a camera that wants to look ahead). */
	const FPlayerTraversal& GetTraversal() const { return Traversal; }
	/** Why the last look for a ledge found none (the tests, the debug line). */
	ETraversalRefusal GetLastTraversalRefusal() const { return LastTraversalRefusal; }
	/** Whether the jump key would still jump now though the feet have just walked off an edge (coyote time). */
	bool CanCoyoteJump() const;

	/** A mantle or vault starts (the view could kick its field of view, the body start a climb pose). */
	FOnPlayerTraversal OnTraversalStarted;
	/** It lands, on the ledge's top or the floor beyond (the view could add a landing shake). */
	FOnPlayerTraversal OnTraversalLanded;

	/** Degrees a slide tips the first-person view by right now. */
	float GetViewRoll() const { return AppliedViewRoll; }

	/** The dust the slide kicks up (for the tests). */
	const FSlideDust& GetDust() const { return Dust; }

	// --- The keys (bound to the player's input; public so the tests can press them) ---

	void HandleSprintPressed();
	void HandleSprintReleased();
	/** Crouch; out of a sprint on the ground, a slide. */
	void HandleCrouchPressed();
	void HandleCrouchReleased();
	/**
	 * The jump key (the character passes it on): climbs or vaults what's in front if it can, else jumps (a moment after
	 * walking off an edge too), and in the air holds the press for the landing. Crouched or sliding it stands up instead
	 * (or does nothing when there's no room to stand); the next press jumps. Mid-climb it does nothing.
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
	/** Lowers and tips the first-person view on its eased curves (PlayerLocomotionViewModel.cpp). */
	void UpdateCamera(float DeltaTime);
	void UpdateViewModel(float DeltaTime);

	/**
	 * The engine's crouch and stand set the body's height in the capsule straight into its relative location, without
	 * moving it there (ACharacter::OnStartCrouch / OnEndCrouch): until the capsule next moves, the body, and the
	 * first-person camera riding it, sit a quarter metre off. Puts it where it belongs.
	 */
	void RefreshBodyTransform();
	/**
	 * Stands the capsule up at once, through the engine's headroom test (false, still crouched, when there's no room),
	 * and notes how far the feet moved doing it (in the air the engine stands up about the capsule's middle), so the eye
	 * takes the jump back and the view doesn't move.
	 */
	bool StandUpNow();
	/** World height of the capsule's bottom. */
	double GetFeetHeight() const;

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
	/** This frame's slide dust (FSlideDust). */
	void UpdateDust(float DeltaTime);

	// --- Traversal (PlayerLocomotionTraversal.cpp) ---

	/** The jump key standing: a mantle or vault in front, else a jump (coyote time in the air), else the press kept. */
	void JumpOrTraverse();
	/** Looks for a mantle or vault in front and starts it; bJumpKey: asked by the key (else a catch in the air). */
	bool TryStartTraversal(bool bJumpKey);
	/** Plans the move to what the probe found (or to a free spot, unsticking) and hands the capsule to it. */
	bool StartTraversal(const FTraversalFind& Found);
	/** The clock, a move under way, and between moves the jump's forgiveness, the catch in the air and the stuck check. */
	void UpdateTraversal(float DeltaTime);
	void AdvanceTraversal(float DeltaTime);
	/** The move reached its end: the movement takes over again (walking, or falling after an unstick). */
	void FinishTraversal();
	/** Something else moved the player mid-move (a respawn, fall recovery): let go where they are. */
	void AbortTraversal();
	/**
	 * Remembers the ground (the last floor, walking off an edge) and fires a jump held for the landing
	 * (PlayerLocomotionJumpAssist.cpp, with the next two).
	 */
	void UpdateJumpAssist();
	void CoyoteJump();
	/** Hung in the air in one spot (wedged on rocks or props): a short glide to a free spot with ground under it. */
	void UpdateStuck();
	/** Which way the player means to go: the keys' way, else the run's, else the look. */
	FVector GetTraversalHeading() const;
	/** Something else holds the player (a scene carrying them, move input shut off while dying or on a trip): no climbs, no nudges. */
	bool IsMovementHeld() const;
	void PlayTraversalSounds(bool bLanding) const;
	/** Draws the probe's findings and the planned path (Looter.DebugTraversal). */
	void DrawTraversalDebug(const FTraversalFind& Found) const;

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

	// The first-person eye (world cm), UpdateCamera.
	/** The eye's height above the feet: eased to the stance's, and shifted at once when the feet jump under it. */
	FViewEase EyeHeight;
	/** The slide's tip of the view (degrees), eased in and out. */
	FViewEase ViewRoll;
	/** The eye's height above the feet when standing with nothing lowered (learned while standing). */
	float StandingEye = 0.f;
	/**
	 * How high the camera sits over its rig's parent with nothing lowered: the rig's own offset plus the camera's place on
	 * it (the arms' head socket). Measured each frame, so whatever the capsule and body do, the eye lands on its curve.
	 */
	float EyeRest = 0.f;
	bool bHaveEye = false;
	/** The capsule's half height last frame, and whether the movement kept the feet in place resizing it (on the ground). */
	float LastHalfHeight = -1.f;
	bool bLastResizeKeptFeet = true;
	/** How far the feet jumped under the eye since it last looked (a resize in the air): the eye takes it back. */
	float PendingFeetJump = 0.f;
	/** The roll a slide tips the first-person view by, as last given to the camera. */
	float AppliedViewRoll = 0.f;

	FPawnInputBinding InputBinding;

	FStanceIntent Intent;
	FPlayerSlide Slide;
	/** A slide set the crouched walk's speed, and it still has to be given back (the slide may already have ended). */
	bool bSlideHoldsSpeed = false;
	/** The slide under way will end in the sprint (as of this frame's keys): the sprint pose comes in as it eases out. */
	bool bSlideExitsToSprint = false;
	/** A slide started since the last frame (the dust's burst). */
	bool bSlideJustStarted = false;
	FSlideDust Dust;
	/** The slide's scrape, held while it runs. */
	TWeakObjectPtr<UAudioComponent> SlideLoop;
	// Traversal (PlayerLocomotionTraversal.cpp)
	FPlayerTraversal Traversal;
	ETraversalRefusal LastTraversalRefusal = ETraversalRefusal::None;
	/** Where the move last put the capsule: anything else moving it ends the move. */
	FVector TraversalLocation = FVector::ZeroVector;
	/** The eye's height over the feet through a move (its own world path less the feet), for UpdateCamera. */
	float TraversalEyeHeight = 0.f;
	/** The obstacle's top over the feet as the move started, and the floor it ends on (its surface's step). */
	float TraversalHeight = 0.f;
	FHitResult TraversalFloor;
	FHitResult TraversalTop;
	float TraversalAlpha = 0.f;
	FViewEase TraversalEase;
	/**
	 * The component's own clock (s), for coyote time, the jump buffer and the stuck check: test levels never tick the
	 * world's.
	 */
	double Clock = 0.0;
	double LeftGroundClock = -100.0;
	double JumpBufferedClock = -100.0;
	bool bWasGrounded = false;
	/** The feet last left the ground by walking off it (not a jump, a launch or a move): a late jump is still allowed. */
	bool bLeftByWalking = false;
	/** The feet's height when last on the ground: a ledge in the air is reached for from there. */
	float LastFloorZ = 0.f;
	bool bHaveFloor = false;
	FVector StuckAnchor = FVector::ZeroVector;
	double StuckSinceClock = 0.0;

	/** The movement keys as the character last passed them on, and the frame it did (GFrameCounter). */
	FVector2D MoveInput = FVector2D::ZeroVector;
	uint64 MoveInputFrame = 0;
	/** The character passes the keys on (the player's does), so they are read from there rather than the last move. */
	bool bHasMoveInput = false;
	bool bSprinting = false;
	float BaseWalkSpeed = 600.f;
	float LastFiringTime = -100.f;
	float NotMovingTime = 0.f;

	// Animation state: the alphas handed out, and the minimum-jerk eases behind them (a stance that changes back mid-way
	// turns around smoothly instead of reversing at once).
	float SprintAlpha = 0.f;
	float CrouchAlpha = 0.f;
	float SlideAlpha = 0.f;
	FViewEase SprintEase;
	FViewEase CrouchEase;
	FViewEase SlideEase;
	bool bSprintInterrupted = false;
	/** The body's run cycle rate as of the last frame on the ground: held through a jump, so a sprint jump lands in stride. */
	float GroundedAnimRate = 1.f;

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
