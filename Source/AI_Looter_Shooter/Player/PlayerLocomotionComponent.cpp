#include "Player/PlayerLocomotionComponent.h"
#include "AI_Looter_Shooter.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

namespace
{
	using EStance = FStanceIntent::EStance;

	const FName SprintId(TEXT("Sprint"));
	const FName CrouchId(TEXT("Crouch"));

	// View-model tuning (camera space: X forward, Y right, Z up; cm and degrees).
	const FVector SprintPoseOffset(-4.f, -5.f, -7.f);
	const FRotator SprintPoseRotation(-12.f, -32.f, -22.f);
	const FVector CrouchPoseOffset(-1.f, -1.5f, -1.f);
	constexpr float CrouchPoseRoll = -6.f;
	/** The point the stance poses turn the gun around, from its grip: a little ahead of and above the hand. */
	const FVector GripPivotFromGrip(9.f, 0.f, 2.f);
	constexpr float WalkStepLength = 190.f;
	constexpr float SprintStepLength = 250.f;
	constexpr float KickStiffness = 170.f;
	constexpr float KickDamping = 18.f;
	/** Where a freshly drawn gun starts, low and tipped, before it comes up over its ready time. */
	const FVector DrawPoseOffset(-3.f, 2.f, -16.f);
	const FRotator DrawPoseRotation(-35.f, 8.f, 18.f);
	/** Aiming: how far in front of the eye the sight's aim point sits, and how far below the line of sight. */
	constexpr float AimSightDistance = 22.f;
	constexpr float AimSightClearance = 0.6f;
	/** How much of the walking bob and look sway aiming takes out. */
	constexpr float AimSteadiness = 0.85f;
	/** Walk speed while aiming, as a share of the normal walk. */
	constexpr float AimWalkSpeedMultiplier = 0.65f;

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
	const UCharacterMovementComponent* Move = Movement.Get();
	const float HalfHeight = Move ? Move->GetCrouchedHalfHeight() : CrouchedHalfHeight;
	return HalfHeight * 2.f - CrouchHeadClearance;
}

float UPlayerLocomotionComponent::GetSpreadMultiplier() const
{
	return FMath::Lerp(1.f, CrouchSpreadMultiplier, CrouchAlpha);
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
		const FString Line = FString::Printf(TEXT("Stance: sprint %d a=%.2f crouch %d a=%.2f speed %.0f/%.0f dir %.0f rate %.2f body %s head %.1f (camera parent %s head %.1f, socket %s) eye %.1f fov %.1f cam %.2f,%.2f,%.2f feet %.1f,%.1f,%.1f %.1f,%.1f,%.1f"),
			bSprinting, SprintAlpha, Owner->bIsCrouched, CrouchAlpha, Velocity.Size2D(), Movement->MaxWalkSpeed, MoveDirection,
			Body ? Body->GlobalAnimRateScale : 0.f, *GetNameSafe(BodyAnim ? BodyAnim->GetClass() : nullptr), BodyHead,
			*GetNameSafe(EyeParent), EyeParentHead, Camera.IsValid() ? *Camera->GetAttachSocketName().ToString() : TEXT("-"), Eye,
			Camera.IsValid() ? Camera->FieldOfView : 0.f, CameraLocal.X, CameraLocal.Y, CameraLocal.Z,
			FootL.X, FootL.Y, FootL.Z, FootR.X, FootR.Y, FootR.Z);
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

	// Losing control (death, unpossess) means release events may never arrive: stand up and walk.
	Intent.Reset();
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

bool UPlayerLocomotionComponent::IsMovingForward() const
{
	const ACharacter* Owner = Character.Get();
	const FVector Input = Owner->GetLastMovementInputVector().GetSafeNormal2D();
	if (Input.IsNearlyZero())
	{
		return false;
	}
	const FVector Facing = Owner->GetActorForwardVector().GetSafeNormal2D();
	return FVector::DotProduct(Input, Facing) >= FMath::Cos(FMath::DegreesToRadians(SprintMaxInputAngle));
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

	// Crouch: the character's built-in crouch owns the capsule; we only say what we want.
	const bool bWantsCrouch = Intent.WantsCrouch();
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

	const bool bCanSprint = Intent.WantsSprint() && bMovingForward && !Owner->bIsCrouched && !bFiring && !bWantsAim
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
	const float SprintTime = bSprinting ? SprintBlendTime : (bSprintInterrupted ? SprintInterruptBlendTime : SprintBlendTime);
	SprintLinear = FMath::FInterpConstantTo(SprintLinear, bSprinting ? 1.f : 0.f, DeltaTime, 1.f / SprintTime);
	SprintAlpha = FMath::SmoothStep(0.f, 1.f, SprintLinear);
	if (SprintLinear <= 0.f)
	{
		bSprintInterrupted = false;
	}

	CrouchLinear = FMath::FInterpConstantTo(CrouchLinear, IsCrouching() ? 1.f : 0.f, DeltaTime, 1.f / CrouchBlendTime);
	CrouchAlpha = FMath::SmoothStep(0.f, 1.f, CrouchLinear);

	// The body's run cycle tops out at walking speed; play it faster while sprinting so the feet don't slide.
	const ACharacter* Owner = Character.Get();
	if (USkeletalMeshComponent* Body = Owner->GetMesh())
	{
		const float Speed = Owner->GetVelocity().Size2D();
		const float Rate = Movement->IsMovingOnGround() ? FMath::Max(1.f, Speed / FMath::Max(BaseWalkSpeed, 1.f)) : 1.f;
		Body->GlobalAnimRateScale = FMath::FInterpTo(Body->GlobalAnimRateScale, Rate, DeltaTime, 8.f);
	}
}

void UPlayerLocomotionComponent::UpdateCamera()
{
	UCameraComponent* View = Camera.Get();
	if (!View)
	{
		return;
	}

	// Crouch the first-person view: learn the standing eye height while standing on the ground, then lower the eye rig by the
	// difference to the crouched head height, eased with the crouch pose so view and body move together.
	const ACharacter* Owner = Character.Get();
	const float Feet = Owner->GetActorLocation().Z - Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	if (CrouchAlpha <= 0.f && Movement->IsMovingOnGround())
	{
		StandingEyeHeight = View->GetComponentLocation().Z - Feet;
	}
	if (USceneComponent* Rig = EyeRig.Get())
	{
		const float Drop = StandingEyeHeight > 0.f ? FMath::Max(StandingEyeHeight - GetCrouchedHeadHeight(), 0.f) * CrouchAlpha : 0.f;
		const FVector Wanted = EyeRigBaseLocation - FVector(0.f, 0.f, Drop);
		if (!Rig->GetRelativeLocation().Equals(Wanted, 0.01f))
		{
			Rig->SetRelativeLocation(Wanted);
		}
	}
}

// ---------------------------------------------------------------------------
// First-person weapon motion
// ---------------------------------------------------------------------------

void UPlayerLocomotionComponent::HandleLanded(const FHitResult& Hit)
{
	// Harder landings push the gun down further.
	KickVelocity -= FMath::Clamp(FallSpeed / 12.f, 10.f, 90.f);
	FallSpeed = 0.f;
}

void UPlayerLocomotionComponent::UpdateViewModel(float DeltaTime)
{
	const ACharacter* Owner = Character.Get();
	const UCharacterMovementComponent* Move = Movement.Get();
	const float Dt = FMath::Min(DeltaTime, 0.05f);

	// Jumps and falls feed the kick spring.
	const bool bFalling = Move->IsFalling();
	if (bFalling)
	{
		FallSpeed = FMath::Max(FallSpeed, -Owner->GetVelocity().Z);
		if (!bWasFalling && Owner->GetVelocity().Z > 50.f)
		{
			KickVelocity -= 18.f; // the gun lags as you jump
		}
	}
	bWasFalling = bFalling;
	KickVelocity += (-KickStiffness * KickOffset - KickDamping * KickVelocity) * Dt;
	KickOffset += KickVelocity * Dt;

	// The gun lags behind fast mouse movement and springs back.
	const FRotator Control = Owner->GetControlRotation();
	const FRotator Turn = (Control - LastControlRotation).GetNormalized();
	LastControlRotation = Control;
	const float TurnScale = 0.012f / FMath::Max(Dt, UE_KINDA_SMALL_NUMBER);
	const FVector2D SwayTarget(FMath::Clamp(-Turn.Yaw * TurnScale, -4.f, 4.f), FMath::Clamp(-Turn.Pitch * TurnScale, -3.f, 3.f));
	LookSway = FMath::Vector2DInterpTo(LookSway, SwayTarget, Dt, 9.f);

	// Step cycle: one dip per footstep, one side-to-side sway per stride.
	const float Speed = Owner->GetVelocity().Size2D();
	const bool bGrounded = Move->IsMovingOnGround();
	const float StepLength = FMath::Lerp(WalkStepLength, SprintStepLength, SprintAlpha) * FMath::Lerp(1.f, 0.75f, CrouchAlpha);
	if (bGrounded)
	{
		StepPhase = FMath::Fmod(StepPhase + Dt * Speed / StepLength * UE_PI, 2.f * UE_PI);
	}
	MoveWeight = FMath::FInterpTo(MoveWeight, bGrounded ? FMath::Clamp(Speed / FMath::Max(BaseWalkSpeed, 1.f), 0.f, 1.6f) : 0.f, Dt, 6.f);
	IdleTime += Dt;

	const float Sprint = SprintAlpha;
	const float Crouch = CrouchAlpha;
	const float StepSide = FMath::Sin(StepPhase);
	const float StepDip = StepSide * StepSide;
	const float BobDepth = FMath::Lerp(0.5f, 1.3f, Sprint) * MoveWeight;
	const float BobSide = FMath::Lerp(0.7f, 2.f, Sprint) * MoveWeight;
	const float BobRoll = FMath::Lerp(0.6f, 3.f, Sprint) * MoveWeight;
	const float Breath = 1.f - FMath::Min(MoveWeight, 1.f);

	FVector Offset(0.f, StepSide * BobSide, -StepDip * BobDepth);
	FRotator Rotation(-StepDip * BobDepth * 0.6f, 0.f, StepSide * BobRoll);

	Offset.Z += FMath::Sin(IdleTime * 1.6f) * 0.15f * Breath;
	Rotation.Pitch += FMath::Sin(IdleTime * 1.6f + 0.6f) * 0.25f * Breath;

	Rotation.Yaw += LookSway.X;
	Rotation.Pitch += LookSway.Y;
	Offset.Y += LookSway.X * 0.35f;
	Offset.Z += LookSway.Y * 0.25f;

	Offset += SprintPoseOffset * Sprint + CrouchPoseOffset * Crouch;
	Rotation += SprintPoseRotation * Sprint;
	Rotation.Roll += CrouchPoseRoll * Crouch;

	Offset.Z += KickOffset;
	Rotation.Pitch += KickOffset * 0.8f;

	// Apply on top of the manager's hold offset, rotating around the grip rather than the back of the gun.
	const UWeaponManagerComponent* Manager = WeaponManager.Get();
	AWeaponBase* Weapon = Manager ? Manager->GetActiveWeapon() : nullptr;
	if (!Weapon || Weapon->GetAttachParentActor() != Owner || Manager->IsThirdPersonHold())
	{
		// In third person the gun is in the body's hands and moves with the animation instead.
		return;
	}

	// Reloading brings the gun in to work the magazine or pump (the weapon moves the part itself, on the same timeline).
	FVector ReloadOffset;
	FRotator ReloadRotation;
	LooterReload::ViewModelPose(Weapon->GetReloadPart(), Weapon->GetReloadProgress(), ReloadOffset, ReloadRotation);
	Offset += ReloadOffset;
	Rotation += ReloadRotation;

	// Recoil: each shot jumps the gun back toward you and flips its muzzle up, springing back into place.
	if (const UPlayerViewComponent* ViewComponent = PlayerView.Get())
	{
		Offset.X -= ViewComponent->GetKickBack();
		Rotation += ViewComponent->GetKickRotation();
	}

	// A freshly drawn gun comes up from below over its ready time (quicker with better handling).
	const float Ready = FMath::InterpEaseOut(0.f, 1.f, Weapon->GetReadyAlpha(), 2.f);
	Offset += DrawPoseOffset * (1.f - Ready);
	Rotation += DrawPoseRotation * (1.f - Ready);

	// Aiming: the hold moves from the hip to straight in front of the eye, with the sight's aim point on the line of
	// sight; walking bob and look sway mostly settle so the sight stays on target.
	const float Aim = PlayerView.IsValid() ? PlayerView->GetAimAlpha() : 0.f;
	FTransform Hold = Manager->GetFirstPersonHold(Weapon);
	if (Aim > 0.f)
	{
		const FVector AimLocation = FVector(AimSightDistance, 0.f, -AimSightClearance) - Weapon->GetAimPoint() * Hold.GetScale3D();
		Hold.SetLocation(FMath::Lerp(Hold.GetLocation(), AimLocation, Aim));
		Hold.SetRotation(FQuat::Slerp(Hold.GetRotation(), FQuat::Identity, Aim));
		const float Settle = 1.f - AimSteadiness * Aim;
		Offset *= Settle;
		Rotation *= Settle;
	}

	const FQuat FinalRotation = Rotation.Quaternion() * Hold.GetRotation();
	// The stance poses turn the gun around (roughly) its grip, wherever this gun has it.
	const FVector Pivot = (Weapon->GetGripPoint().IsZero() ? Manager->AttachGrip : Weapon->GetGripPoint()) + GripPivotFromGrip;
	const FVector FinalLocation = Hold.GetLocation() + Offset + Hold.GetRotation().RotateVector(Pivot) - FinalRotation.RotateVector(Pivot);
	Weapon->SetActorRelativeTransform(FTransform(FinalRotation, FinalLocation, Hold.GetScale3D()));
}
