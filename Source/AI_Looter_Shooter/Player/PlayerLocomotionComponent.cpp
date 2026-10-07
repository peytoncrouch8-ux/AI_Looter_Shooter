#include "Player/PlayerLocomotionComponent.h"
#include "AI_Looter_Shooter.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

// The component's lifetime, keys and stance. The first-person view and gun motion are in PlayerLocomotionViewModel.cpp,
// the slide and the jump key in PlayerLocomotionSlide.cpp.

namespace
{
	using EStance = FStanceIntent::EStance;

	const FName SprintId(TEXT("Sprint"));
	const FName CrouchId(TEXT("Crouch"));

	/** Walk speed while aiming, as a share of the normal walk. */
	constexpr float AimWalkSpeedMultiplier = 0.65f;

	/** Seconds the body takes to drop into the slide pose, and to come back up out of it. */
	constexpr float SlideBlendInTime = 0.12f;
	constexpr float SlideBlendOutTime = 0.2f;
	/**
	 * The body's animation rate in a full slide. The legs are posed by code then, but the run cycle underneath would
	 * still pump the arms and twist the feet at the slide's speed; nearly stopped, it reads as one held pose.
	 */
	constexpr float SlideAnimRate = 0.2f;

	TAutoConsoleVariable<bool> CVarDebugStance(TEXT("Looter.DebugStance"), false,
		TEXT("Print the player's stance each frame: sprint/crouch alphas, speed, and head/camera heights above the feet."));
}

UPlayerLocomotionComponent::UPlayerLocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

bool UPlayerLocomotionComponent::IsCrouching() const
{
	const ACharacter* Owner = Character.Get();
	return Owner && Owner->bIsCrouched;
}

float UPlayerLocomotionComponent::GetCrouchedHeadHeight() const
{
	// The movement component keeps the crouched half height before the character's scale, so this is in the full-size
	// body's units, like the mesh's component space the stance layer works in.
	const UCharacterMovementComponent* Move = Movement.Get();
	const float HalfHeight = Move ? Move->GetCrouchedHalfHeight() : CrouchedHalfHeight;
	return HalfHeight * 2.f - CrouchHeadClearance;
}

float UPlayerLocomotionComponent::GetSpreadMultiplier() const
{
	// A slide uses the crouched capsule but isn't a steady stance.
	return FMath::Lerp(1.f, CrouchSpreadMultiplier, CrouchAlpha * (1.f - SlideAlpha));
}

float UPlayerLocomotionComponent::GetBodyScale() const
{
	const ACharacter* Owner = Character.Get();
	return Owner ? FMath::Max(static_cast<float>(Owner->GetActorScale3D().Z), UE_KINDA_SMALL_NUMBER) : 1.f;
}

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

void UPlayerLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner)
	{
		UE_LOG(LogLooter, Warning, TEXT("%s needs to be on a Character; sprint and crouch are disabled."), *GetName());
		SetComponentTickEnabled(false);
		return;
	}

	Character = Owner;
	Movement = Owner->GetCharacterMovement();
	Camera = UPlayerViewComponent::FindFirstPersonCamera(Owner);
	WeaponManager = Owner->FindComponentByClass<UWeaponManagerComponent>();
	PlayerView = Owner->FindComponentByClass<UPlayerViewComponent>();

	if (UCharacterMovementComponent* Move = Movement.Get())
	{
		BaseWalkSpeed = Move->MaxWalkSpeed;
		Move->GetNavAgentPropertiesRef().bCanCrouch = true;
		Move->SetCrouchedHalfHeight(CrouchedHalfHeight);
		Move->MaxWalkSpeedCrouched = CrouchSpeed;
		Move->bCanWalkOffLedgesWhenCrouching = true;
		// Read this frame's movement result...
		AddTickPrerequisiteComponent(Move);
	}
	if (USkeletalMeshComponent* Body = Owner->GetMesh())
	{
		// ...and hand the body animation this frame's stance, not last frame's.
		Body->AddTickPrerequisiteComponent(this);
	}
	if (UCameraComponent* View = Camera.Get())
	{
		USceneComponent* Parent = View->GetAttachParent();
		const bool bRidesArms = Parent && Parent != Owner->GetRootComponent() && Parent != Owner->GetMesh();
		EyeRig = bRidesArms ? Parent : static_cast<USceneComponent*>(View);
		EyeRigBaseLocation = EyeRig->GetRelativeLocation();
	}

	Owner->LandedDelegate.AddDynamic(this, &UPlayerLocomotionComponent::HandleLanded);
	Owner->ReceiveControllerChangedDelegate.AddDynamic(this, &UPlayerLocomotionComponent::HandleControllerChanged);
	SetupInput(Owner->GetController());
	LastControlRotation = Owner->GetControlRotation();
}

void UPlayerLocomotionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TeardownInput();
	if (ACharacter* Owner = Character.Get())
	{
		Owner->LandedDelegate.RemoveDynamic(this, &UPlayerLocomotionComponent::HandleLanded);
		Owner->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UPlayerLocomotionComponent::HandleControllerChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UPlayerLocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Character.IsValid() || !Movement.IsValid())
	{
		return;
	}

	UpdateSlide(DeltaTime);
	UpdateStance(DeltaTime);
	UpdateAlphas(DeltaTime);
	UpdateCamera();
	UpdateViewModel(DeltaTime);

#if !UE_BUILD_SHIPPING
	if (CVarDebugStance.GetValueOnGameThread())
	{
		const ACharacter* Owner = Character.Get();
		const float Feet = Owner->GetActorLocation().Z - Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const USkeletalMeshComponent* Body = Owner->GetMesh();
		const float BodyHead = Body ? Body->GetSocketLocation(TEXT("head")).Z - Feet : 0.f;
		const float Eye = Camera.IsValid() ? Camera->GetComponentLocation().Z - Feet : 0.f;
		const USceneComponent* EyeParent = Camera.IsValid() ? Camera->GetAttachParent() : nullptr;
		const float EyeParentHead = EyeParent ? EyeParent->GetSocketLocation(TEXT("head")).Z - Feet : 0.f;
		const UAnimInstance* BodyAnim = Body ? Body->GetAnimInstance() : nullptr;
		// Camera relative to the capsule and the move direction (0 = forward, -90 = left), for spotting view jitter.
		const FVector Velocity = Owner->GetVelocity();
		const float MoveDirection = Velocity.Size2D() > 10.f ? FMath::RadiansToDegrees(FMath::Atan2(
			FVector::DotProduct(Velocity, Owner->GetActorRightVector()), FVector::DotProduct(Velocity, Owner->GetActorForwardVector()))) : 0.f;
		const FVector CameraLocal = Camera.IsValid() ? Owner->GetActorTransform().InverseTransformPosition(Camera->GetComponentLocation()) : FVector::ZeroVector;
		// The body's feet in its own space, for spotting leg jitter from the locomotion clips.
		const FVector FootL = Body ? Body->GetSocketTransform(TEXT("foot_l"), RTS_Component).GetLocation() : FVector::ZeroVector;
		const FVector FootR = Body ? Body->GetSocketTransform(TEXT("foot_r"), RTS_Component).GetLocation() : FVector::ZeroVector;
		const FString Line = FString::Printf(TEXT("Stance: sprint %d a=%.2f crouch %d a=%.2f speed %.0f/%.0f dir %.0f rate %.2f body %s head %.1f (camera parent %s head %.1f, socket %s) eye %.1f fov %.1f cam %.2f,%.2f,%.2f feet %.1f,%.1f,%.1f %.1f,%.1f,%.1f slide %d a=%.2f"),
			bSprinting, SprintAlpha, Owner->bIsCrouched, CrouchAlpha, Velocity.Size2D(), Movement->MaxWalkSpeed, MoveDirection,
			Body ? Body->GlobalAnimRateScale : 0.f, *GetNameSafe(BodyAnim ? BodyAnim->GetClass() : nullptr), BodyHead,
			*GetNameSafe(EyeParent), EyeParentHead, Camera.IsValid() ? *Camera->GetAttachSocketName().ToString() : TEXT("-"), Eye,
			Camera.IsValid() ? Camera->FieldOfView : 0.f, CameraLocal.X, CameraLocal.Y, CameraLocal.Z,
			FootL.X, FootL.Y, FootL.Z, FootR.X, FootR.Y, FootR.Z, Slide.IsActive(), SlideAlpha);
		UE_LOG(LogLooter, Log, TEXT("%s"), *Line);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(reinterpret_cast<uint64>(this), 0.f, FColor::Cyan, Line);
		}
	}
#endif
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

UKeyBindingSubsystem* UPlayerLocomotionComponent::GetBindings() const
{
	return FPawnInputBinding::GetBindings(InputBinding.GetController());
}

void UPlayerLocomotionComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	SetupInput(NewController);
}

void UPlayerLocomotionComponent::SetupInput(AController* Controller)
{
	TeardownInput();

	UKeyBindingSubsystem* Bindings = FPawnInputBinding::GetBindings(Controller);
	if (!Bindings || !Bindings->GetSprintAction() || !Bindings->GetCrouchAction())
	{
		return;
	}
	UEnhancedInputComponent* Input = InputBinding.Setup(GetOwner(), Controller, Bindings->GetCharacterContext(), UKeyBindingSubsystem::CharacterContextPriority);
	if (!Input)
	{
		return;
	}
	Input->BindAction(Bindings->GetSprintAction(), ETriggerEvent::Started, this, &UPlayerLocomotionComponent::HandleSprintPressed);
	Input->BindAction(Bindings->GetSprintAction(), ETriggerEvent::Completed, this, &UPlayerLocomotionComponent::HandleSprintReleased);
	Input->BindAction(Bindings->GetSprintAction(), ETriggerEvent::Canceled, this, &UPlayerLocomotionComponent::HandleSprintReleased);
	Input->BindAction(Bindings->GetCrouchAction(), ETriggerEvent::Started, this, &UPlayerLocomotionComponent::HandleCrouchPressed);
	Input->BindAction(Bindings->GetCrouchAction(), ETriggerEvent::Completed, this, &UPlayerLocomotionComponent::HandleCrouchReleased);
	Input->BindAction(Bindings->GetCrouchAction(), ETriggerEvent::Canceled, this, &UPlayerLocomotionComponent::HandleCrouchReleased);
	RefreshModes();
}

void UPlayerLocomotionComponent::TeardownInput()
{
	InputBinding.Teardown();

	// Losing control (death, unpossess) means release events may never arrive: stop sliding, stand up and walk.
	Intent.Reset();
	EndSlide();
	bSprinting = false;
	if (UCharacterMovementComponent* Move = Movement.Get())
	{
		Move->MaxWalkSpeed = BaseWalkSpeed;
	}
	if (ACharacter* Owner = Character.Get(); Owner && Owner->bIsCrouched)
	{
		Owner->UnCrouch();
	}
}

void UPlayerLocomotionComponent::RefreshModes()
{
	if (const UKeyBindingSubsystem* Bindings = GetBindings())
	{
		Intent.SetModes(Bindings->IsToggleMode(SprintId), Bindings->IsToggleMode(CrouchId));
	}
}

void UPlayerLocomotionComponent::HandleSprintPressed()
{
	RefreshModes();
	Intent.Press(EStance::Sprint);
}

void UPlayerLocomotionComponent::HandleSprintReleased()
{
	RefreshModes();
	Intent.Release(EStance::Sprint);
}

void UPlayerLocomotionComponent::HandleCrouchPressed()
{
	RefreshModes();
	Intent.Press(EStance::Crouch);
	// Crouching out of a sprint slides (only on the ground: TryStartSlide checks).
	if (Intent.WantsCrouch())
	{
		TryStartSlide();
	}
}

void UPlayerLocomotionComponent::HandleCrouchReleased()
{
	RefreshModes();
	Intent.Release(EStance::Crouch);
}

void UPlayerLocomotionComponent::ReleaseKeysNoLongerHeld()
{
	// Safety net for a missed key release (pause menu, alt-tab): a held stance ends when its key isn't down.
	const APlayerController* PC = InputBinding.GetController();
	const UEnhancedPlayerInput* PlayerInput = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
	const UKeyBindingSubsystem* Bindings = GetBindings();
	if (!PlayerInput || !Bindings)
	{
		return;
	}

	const TPair<EStance, const UInputAction*> Stances[] = { { EStance::Sprint, Bindings->GetSprintAction() }, { EStance::Crouch, Bindings->GetCrouchAction() } };
	for (const TPair<EStance, const UInputAction*>& Stance : Stances)
	{
		if (Intent.IsHeldMode(Stance.Key) && Intent.IsActive(Stance.Key) && !PlayerInput->GetActionValue(Stance.Value).Get<bool>())
		{
			Intent.Release(Stance.Key);
		}
	}
}

// ---------------------------------------------------------------------------
// Stance
// ---------------------------------------------------------------------------

void UPlayerLocomotionComponent::HandleMoveInput(const FVector2D& Input)
{
	MoveInput = Input;
	MoveInputFrame = GFrameCounter;
	bHasMoveInput = true;
}

FVector2D UPlayerLocomotionComponent::GetHeldMoveInput() const
{
	// The character passes the keys on every frame they're down (Enhanced Input's Triggered), so a report older than the
	// last frame means they were let go; no release event has to arrive (a menu or alt-tab can swallow it).
	return MoveInputFrame + 1 >= GFrameCounter ? MoveInput : FVector2D::ZeroVector;
}

bool UPlayerLocomotionComponent::IsMovingForward() const
{
	const float MinForward = FMath::Cos(FMath::DegreesToRadians(SprintMaxInputAngle));
	// The keys themselves when the character passes them on: through a slide the last move's input is the slide's own
	// line, which says nothing about whether the player still runs forward.
	if (bHasMoveInput)
	{
		const FVector2D Held = GetHeldMoveInput();
		return !Held.IsNearlyZero() && Held.GetSafeNormal().Y >= MinForward;
	}
	const ACharacter* Owner = Character.Get();
	const FVector Input = Owner->GetLastMovementInputVector().GetSafeNormal2D();
	if (Input.IsNearlyZero())
	{
		return false;
	}
	const FVector Facing = Owner->GetActorForwardVector().GetSafeNormal2D();
	return FVector::DotProduct(Input, Facing) >= MinForward;
}

bool UPlayerLocomotionComponent::IsWeaponFiring() const
{
	const UWeaponManagerComponent* Manager = WeaponManager.Get();
	const AWeaponBase* Weapon = Manager ? Manager->GetActiveWeapon() : nullptr;
	return Weapon && Weapon->WantsToFire();
}

void UPlayerLocomotionComponent::UpdateStance(float DeltaTime)
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	const float Now = GetWorld()->GetTimeSeconds();

	if (InputBinding.IsBound())
	{
		RefreshModes();
		ReleaseKeysNoLongerHeld();
	}

	const bool bFiring = IsWeaponFiring();
	if (bFiring)
	{
		LastFiringTime = Now;
		Intent.CancelSprintToggle();
	}
	// Raising the sight ends a sprint the same way shooting does.
	const UPlayerViewComponent* View = PlayerView.Get();
	const bool bWantsAim = View && View->WantsToAim();
	if (bWantsAim)
	{
		Intent.CancelSprintToggle();
	}

	// Crouch: the character's built-in crouch owns the capsule; we only say what we want. A slide keeps the crouched
	// capsule until it ends, whatever the keys say meanwhile.
	const bool bWantsCrouch = Intent.WantsCrouch() || Slide.IsActive();
	if (bWantsCrouch != Move->bWantsToCrouch)
	{
		if (bWantsCrouch)
		{
			Owner->Crouch();
		}
		else
		{
			Owner->UnCrouch();
		}
	}

	// Sprint: forward only, standing, not shooting, and started on the ground (a sprint jump keeps its speed).
	const bool bMovingForward = IsMovingForward();
	NotMovingTime = bMovingForward ? 0.f : NotMovingTime + DeltaTime;
	if (NotMovingTime > SprintToggleStopGrace)
	{
		Intent.CancelSprintToggle();
	}

	const bool bCanSprint = Intent.WantsSprint() && bMovingForward && !Owner->bIsCrouched && !Slide.IsActive() && !bFiring && !bWantsAim
		&& Now - LastFiringTime >= SprintResumeDelay && (bSprinting || Move->IsMovingOnGround());

	if (bCanSprint != bSprinting)
	{
		bSprinting = bCanSprint;
		bSprintInterrupted = !bSprinting && (bFiring || bWantsAim);
		UE_LOG(LogLooter, Verbose, TEXT("Sprint %s%s"), bSprinting ? TEXT("on") : TEXT("off"), bSprintInterrupted ? TEXT(" (fired or aimed)") : TEXT(""));
	}
	// Aiming slows the walk as the sight comes up.
	const float WalkSpeed = bSprinting ? BaseWalkSpeed * SprintSpeedMultiplier
		: BaseWalkSpeed * FMath::Lerp(1.f, AimWalkSpeedMultiplier, View ? View->GetAimAlpha() : 0.f);
	if (!FMath::IsNearlyEqual(Move->MaxWalkSpeed, WalkSpeed))
	{
		Move->MaxWalkSpeed = WalkSpeed;
	}

	if (Owner->bIsCrouched != bWasCrouched)
	{
		bWasCrouched = Owner->bIsCrouched;
		// A little weight into the stance change for the gun.
		KickVelocity -= bWasCrouched ? 28.f : 14.f;
		UE_LOG(LogLooter, Verbose, TEXT("Crouch %s"), bWasCrouched ? TEXT("on") : TEXT("off"));
	}
}

void UPlayerLocomotionComponent::UpdateAlphas(float DeltaTime)
{
	// A slide that ends in the sprint brings the sprint pose (and its wider view) in as it eases out, so they hand over.
	const bool bSprintPose = bSprinting || (bSlideExitsToSprint && Slide.IsEasingOut());
	const float SprintTime = bSprintPose ? SprintBlendTime : (bSprintInterrupted ? SprintInterruptBlendTime : SprintBlendTime);
	SprintLinear = FMath::FInterpConstantTo(SprintLinear, bSprintPose ? 1.f : 0.f, DeltaTime, 1.f / SprintTime);
	SprintAlpha = FMath::SmoothStep(0.f, 1.f, SprintLinear);
	if (SprintLinear <= 0.f)
	{
		bSprintInterrupted = false;
	}

	CrouchLinear = FMath::FInterpConstantTo(CrouchLinear, IsCrouching() ? 1.f : 0.f, DeltaTime, 1.f / CrouchBlendTime);
	CrouchAlpha = FMath::SmoothStep(0.f, 1.f, CrouchLinear);

	// The slide pose comes in fast and starts letting go as the slide slows at its end.
	const bool bSlidePose = Slide.IsActive() && !Slide.IsEasingOut();
	SlideLinear = FMath::FInterpConstantTo(SlideLinear, bSlidePose ? 1.f : 0.f, DeltaTime, 1.f / (bSlidePose ? SlideBlendInTime : SlideBlendOutTime));
	SlideAlpha = FMath::SmoothStep(0.f, 1.f, SlideLinear);

	// The body's run cycle tops out at walking speed; play it faster while sprinting so the feet don't slide. (Both are
	// the character's own, so this holds at any size.) A slide holds the body nearly still instead.
	const ACharacter* Owner = Character.Get();
	if (USkeletalMeshComponent* Body = Owner->GetMesh())
	{
		const float Speed = Owner->GetVelocity().Size2D();
		const float Running = Movement->IsMovingOnGround() ? FMath::Max(1.f, Speed / FMath::Max(BaseWalkSpeed, 1.f)) : 1.f;
		const float Rate = FMath::Lerp(Running, SlideAnimRate, SlideAlpha);
		Body->GlobalAnimRateScale = FMath::FInterpTo(Body->GlobalAnimRateScale, Rate, DeltaTime, 8.f);
	}
}
