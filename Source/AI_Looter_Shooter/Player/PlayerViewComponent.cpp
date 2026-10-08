#include "Player/PlayerViewComponent.h"
#include "AI_Looter_Shooter.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The gun in the body's hands flips this much harder than the first-person gun, so the kick reads from behind. */
	constexpr float ThirdPersonKickScale = 1.35f;
	/**
	 * The eye's lowering (crouches' worth) for the third-person pivot: as it is up to a crouch, then easing toward 1.3 (a
	 * slide's eye goes about 1.5), with no kink where it passes a crouch (its slope is 1 there).
	 */
	float ThirdPersonLowering(float Lowering)
	{
		constexpr float Room = 0.3f;
		const float Past = FMath::Max(Lowering - 1.f, 0.f) / Room;
		return Lowering <= 1.f ? FMath::Max(Lowering, -0.5f) : 1.f + Room * Past / (1.f + Past);
	}

	// Live tuning for the third-person camera (negative = use the component's settings).
	TAutoConsoleVariable<float> CVarArmLength(TEXT("Looter.ThirdPerson.ArmLength"), -1.f, TEXT("Override the third-person camera distance (cm)."));
	TAutoConsoleVariable<float> CVarShoulderRight(TEXT("Looter.ThirdPerson.ShoulderRight"), -1.f, TEXT("Override the third-person camera's offset to the right (cm)."));
	TAutoConsoleVariable<float> CVarShoulderUp(TEXT("Looter.ThirdPerson.ShoulderUp"), -1.f, TEXT("Override the third-person camera's offset upward (cm)."));

	float Tuned(const TAutoConsoleVariable<float>& Variable, float Value)
	{
		const float Override = Variable.GetValueOnGameThread();
		return Override >= 0.f ? Override : Value;
	}

	/** The player's first-person field of view from the settings menu, or Fallback for a character no local player controls. */
	float FirstPersonFieldOfViewSetting(const ACharacter* Owner, float Fallback)
	{
		const APlayerController* OwnerController = Owner ? Cast<APlayerController>(Owner->GetController()) : nullptr;
		const ULocalPlayer* LocalPlayer = OwnerController ? OwnerController->GetLocalPlayer() : nullptr;
		const UGraphicsSettingsSubsystem* Graphics = LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
		return Graphics ? Graphics->GetFirstPersonFieldOfView() : Fallback;
	}

	/** Unzoomed narrowed by a sight's magnification (2x shows half as wide), Aim of the way to the eye. */
	float SightFieldOfView(float Unzoomed, float Zoom, float Aim)
	{
		const float Zoomed = FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(Unzoomed) * 0.5f) / Zoom));
		return FMath::Lerp(Unzoomed, Zoomed, Aim);
	}
}

UPlayerViewComponent::UPlayerViewComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// After the body has animated (the held gun follows the hand), before the camera is read at the end of the frame.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	ArmedAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Rifle/ABP_Rifle.ABP_Rifle_C")));
}

UCameraComponent* UPlayerViewComponent::FindFirstPersonCamera(const AActor* Owner)
{
	if (!Owner)
	{
		return nullptr;
	}
	TInlineComponentArray<UCameraComponent*> Cameras(Owner);
	for (UCameraComponent* Camera : Cameras)
	{
		if (!Cast<USpringArmComponent>(Camera->GetAttachParent()))
		{
			return Camera;
		}
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

void UPlayerViewComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	FirstPersonCamera = FindFirstPersonCamera(Owner);
	if (!Owner || !FirstPersonCamera.IsValid())
	{
		UE_LOG(LogLooter, Warning, TEXT("%s needs a Character with a camera; third-person view is disabled."), *GetName());
		SetComponentTickEnabled(false);
		return;
	}

	Character = Owner;
	Locomotion = Owner->FindComponentByClass<UPlayerLocomotionComponent>();
	WeaponManager = Owner->FindComponentByClass<UWeaponManagerComponent>();
	FirstPersonFieldOfView = FirstPersonCamera->FieldOfView;

	// The template's first-person arms only carry the camera now (the gun is held by the camera itself), and with a
	// rifle pose they'd float empty-handed in view. Keep them animating for the camera, but hidden.
	USkeletalMeshComponent* ArmsMesh = Cast<USkeletalMeshComponent>(FirstPersonCamera->GetAttachParent());
	if (ArmsMesh && ArmsMesh != Owner->GetMesh())
	{
		Arms = ArmsMesh;
		ArmsMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		ArmsMesh->SetVisibility(false);
	}

	// Third-person rig: a spring arm on the capsule, collision-tested against the Camera channel.
	Boom = NewObject<USpringArmComponent>(Owner, TEXT("ThirdPersonBoom"));
	Boom->SetupAttachment(Owner->GetRootComponent());
	Boom->bUsePawnControlRotation = true;
	Boom->bDoCollisionTest = true;
	Boom->ProbeChannel = ECC_Camera;
	Boom->ProbeSize = ProbeSize;
	Boom->bEnableCameraLag = false;
	Boom->bEnableCameraRotationLag = false;
	Boom->TargetArmLength = 0.f;
	Boom->RegisterComponent();

	ThirdPersonCamera = NewObject<UCameraComponent>(Owner, TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(Boom, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false;
	ThirdPersonCamera->SetFieldOfView(ThirdPersonFieldOfView);
	ThirdPersonCamera->SetAutoActivate(false);
	ThirdPersonCamera->RegisterComponent();

	if (USkeletalMeshComponent* Body = Owner->GetMesh())
	{
		UnarmedAnimClass = Body->GetAnimClass();
	}
	LoadedArmedAnimClass = ArmedAnimClass.LoadSynchronous();
	if (UWeaponManagerComponent* Manager = WeaponManager.Get())
	{
		Manager->OnActiveWeaponChanged.AddDynamic(this, &UPlayerViewComponent::HandleActiveWeaponChanged);
		BindFiringWeapon(Manager->GetActiveWeapon());
	}
	RefreshBodyAnimation();

	// Run after the character has moved and animated this frame, and before the spring arm places the camera.
	if (UCharacterMovementComponent* Move = Owner->GetCharacterMovement())
	{
		AddTickPrerequisiteComponent(Move);
	}
	if (UPlayerLocomotionComponent* Loco = Locomotion.Get())
	{
		AddTickPrerequisiteComponent(Loco);
	}
	if (USkeletalMeshComponent* Body = Owner->GetMesh())
	{
		AddTickPrerequisiteComponent(Body);
	}
	Boom->AddTickPrerequisiteComponent(this);

	Owner->ReceiveControllerChangedDelegate.AddDynamic(this, &UPlayerViewComponent::HandleControllerChanged);
	SetupInput(Owner->GetController());
	ApplyMode();
}

void UPlayerViewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	InputBinding.Teardown();
	if (ACharacter* Owner = Character.Get())
	{
		Owner->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UPlayerViewComponent::HandleControllerChanged);
	}
	if (UWeaponManagerComponent* Manager = WeaponManager.Get())
	{
		Manager->OnActiveWeaponChanged.RemoveDynamic(this, &UPlayerViewComponent::HandleActiveWeaponChanged);
	}
	BindFiringWeapon(nullptr);
	Super::EndPlay(EndPlayReason);
}

void UPlayerViewComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Character.IsValid())
	{
		return;
	}
	UpdateAim(DeltaTime);
	UpdateRecoil(DeltaTime);
	UpdateBoom(DeltaTime);
	UpdateFieldOfView();
	UpdateHeldWeapon();
}

// ---------------------------------------------------------------------------
// Aiming down the sights
// ---------------------------------------------------------------------------

void UPlayerViewComponent::HandleAimPressed()
{
	// Toggle mode: press to aim, press again to stop; hold mode: aim while held.
	bAimWanted = bAimToggle ? !bAimWanted : true;
}

void UPlayerViewComponent::HandleAimReleased()
{
	if (!bAimToggle)
	{
		bAimWanted = false;
	}
}

void UPlayerViewComponent::UpdateAim(float DeltaTime)
{
	// Only with a gun in hand, and not while it's being reloaded or carried low in a sprint (aiming stops a sprint first).
	const UWeaponManagerComponent* Manager = WeaponManager.Get();
	const AWeaponBase* Weapon = Manager ? Manager->GetActiveWeapon() : nullptr;
	const bool bSprinting = Locomotion.IsValid() && Locomotion->GetSprintAlpha() > 0.5f;
	const bool bCanAim = Weapon && !Weapon->IsReloading() && !bSprinting && Mode != EPlayerViewMode::ThirdPersonFront;
	if (!Weapon)
	{
		bAimWanted = false;
	}
	// Quicker with better handling: a Handling 1 gun takes BaseAimSeconds to come up to the eye.
	const float Speed = (Weapon ? Weapon->GetStats().Handling : 1.f) / BaseAimSeconds;
	const float WasAimAlpha = AimAlpha;
	AimAlpha = FMath::FInterpConstantTo(AimAlpha, bAimWanted && bCanAim ? 1.f : 0.f, DeltaTime, Speed);
	AimZoom = Weapon ? FMath::Max(Weapon->GetStats().Zoom, MinAimZoom) : AimZoom;
	// The sights coming up: the gun's aim-in sound, once as aiming starts.
	if (WasAimAlpha <= 0.f && AimAlpha > 0.f && Weapon)
	{
		Weapon->PlayAimIn();
	}
}

float UPlayerViewComponent::GetAimSpreadMultiplier() const
{
	return FMath::Lerp(1.f, AimSpreadMultiplier, GetAimAlpha());
}

float UPlayerViewComponent::GetLookSensitivityMultiplier() const
{
	// Through a sight the view turns slower, so the crosshair moves across the target as it did unzoomed.
	return FMath::Lerp(1.f, 1.f / AimZoom, GetAimAlpha());
}

float UPlayerViewComponent::GetAimAlpha() const
{
	// Eased, so the gun settles into the sight instead of stopping dead.
	return FMath::InterpEaseInOut(0.f, 1.f, AimAlpha, 2.f);
}

// ---------------------------------------------------------------------------
// Recoil
// ---------------------------------------------------------------------------

void UPlayerViewComponent::BindFiringWeapon(AWeaponBase* Weapon)
{
	if (AWeaponBase* Old = FiringWeapon.Get())
	{
		Old->OnFired.RemoveDynamic(this, &UPlayerViewComponent::HandleWeaponFired);
	}
	FiringWeapon = Weapon;
	if (Weapon)
	{
		Weapon->OnFired.AddUniqueDynamic(this, &UPlayerViewComponent::HandleWeaponFired);
	}
}

void UPlayerViewComponent::HandleWeaponFired()
{
	if (const AWeaponBase* Weapon = FiringWeapon.Get())
	{
		Recoil.AddShot(Weapon->GetRecoilProfile(), RecoilRandom);
	}
}

void UPlayerViewComponent::UpdateRecoil(float DeltaTime)
{
	AController* Controller = Character->GetController();
	// Whatever moved the view since our change last tick was the player (pulling down against the kick uses it up).
	const float PlayerPitch = Controller && bHaveLastViewPitch ? FRotator::NormalizeAxis(Controller->GetControlRotation().Pitch - LastViewPitch) : 0.f;
	const FRotator AimChange = Recoil.Tick(DeltaTime, PlayerPitch);

	// Only a player's own view kicks (the camera manager clamps the pitch on its next update).
	if (Controller && Controller->IsLocalPlayerController() && !AimChange.IsNearlyZero(1.e-4f))
	{
		FRotator View = Controller->GetControlRotation();
		View.Pitch = FRotator::ClampAxis(View.Pitch + AimChange.Pitch);
		View.Yaw = FRotator::ClampAxis(View.Yaw + AimChange.Yaw);
		Controller->SetControlRotation(View);
	}
	bHaveLastViewPitch = Controller != nullptr;
	LastViewPitch = Controller ? Controller->GetControlRotation().Pitch : 0.f;
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void UPlayerViewComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	SetupInput(NewController);
}

void UPlayerViewComponent::SetupInput(AController* Controller)
{
	InputBinding.Teardown();
	UKeyBindingSubsystem* Bindings = FPawnInputBinding::GetBindings(Controller);
	if (!Bindings || !Bindings->GetToggleViewAction())
	{
		return;
	}
	if (UEnhancedInputComponent* Input = InputBinding.Setup(GetOwner(), Controller, Bindings->GetCharacterContext(), UKeyBindingSubsystem::CharacterContextPriority))
	{
		Input->BindAction(Bindings->GetToggleViewAction(), ETriggerEvent::Started, this, &UPlayerViewComponent::CycleViewMode);
		if (Bindings->GetAimAction())
		{
			Input->BindAction(Bindings->GetAimAction(), ETriggerEvent::Started, this, &UPlayerViewComponent::HandleAimPressed);
			Input->BindAction(Bindings->GetAimAction(), ETriggerEvent::Completed, this, &UPlayerViewComponent::HandleAimReleased);
			Input->BindAction(Bindings->GetAimAction(), ETriggerEvent::Canceled, this, &UPlayerViewComponent::HandleAimReleased);
		}
		bAimToggle = Bindings->IsToggleMode(TEXT("Aim"));
	}
	bAimWanted = false;
}

// ---------------------------------------------------------------------------
// Modes
// ---------------------------------------------------------------------------

void UPlayerViewComponent::CycleViewMode()
{
	switch (Mode)
	{
	case EPlayerViewMode::FirstPerson: SetViewMode(EPlayerViewMode::ThirdPerson); break;
	case EPlayerViewMode::ThirdPerson: SetViewMode(EPlayerViewMode::ThirdPersonFront); break;
	default: SetViewMode(EPlayerViewMode::FirstPerson); break;
	}
}

void UPlayerViewComponent::SetViewMode(EPlayerViewMode NewMode)
{
	if (Mode == NewMode)
	{
		return;
	}
	const bool bWasFirstPerson = IsFirstPerson();
	Mode = NewMode;
	if (bWasFirstPerson)
	{
		// Leaving the head: ease the camera out along the arm instead of cutting to it.
		PullOut = 0.f;
	}
	ApplyMode();
	UE_LOG(LogLooter, Verbose, TEXT("View: %s"), *UEnum::GetValueAsString(Mode));
}

void UPlayerViewComponent::ApplyMode()
{
	ACharacter* Owner = Character.Get();
	if (!Owner)
	{
		return;
	}
	const bool bFirstPerson = IsFirstPerson();

	FirstPersonCamera->SetActive(bFirstPerson);
	ThirdPersonCamera->SetActive(!bFirstPerson);
	Boom->bUsePawnControlRotation = Mode == EPlayerViewMode::ThirdPerson;

	// First person: the body is a world-space representation (casts the player's shadow, never drawn for them).
	// Third person: a normal, visible mesh.
	if (USkeletalMeshComponent* Body = Owner->GetMesh())
	{
		Body->SetFirstPersonPrimitiveType(bFirstPerson ? EFirstPersonPrimitiveType::WorldSpaceRepresentation : EFirstPersonPrimitiveType::None);
		Body->SetOwnerNoSee(bFirstPerson);
	}

	// Guns ride the camera in first person and sit in the body's hands in third person.
	if (UWeaponManagerComponent* Manager = WeaponManager.Get())
	{
		Manager->SetThirdPersonHold(!bFirstPerson);
	}
	UpdateBoom(0.f);
	UpdateFieldOfView();
}

void UPlayerViewComponent::HandleActiveWeaponChanged(AWeaponBase* NewWeapon, AWeaponBase* OldWeapon)
{
	BindFiringWeapon(NewWeapon);
	RefreshBodyAnimation();
}

void UPlayerViewComponent::RefreshBodyAnimation()
{
	USkeletalMeshComponent* Body = Character.IsValid() ? Character->GetMesh() : nullptr;
	if (!Body)
	{
		return;
	}
	const bool bArmed = WeaponManager.IsValid() && WeaponManager->GetActiveWeapon();
	const TSubclassOf<UAnimInstance> Wanted = bArmed && LoadedArmedAnimClass ? LoadedArmedAnimClass : UnarmedAnimClass;
	if (Wanted && Body->GetAnimClass() != Wanted)
	{
		Body->SetAnimInstanceClass(Wanted);
	}
}

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------

void UPlayerViewComponent::UpdateBoom(float DeltaTime)
{
	const ACharacter* Owner = Character.Get();
	if (IsFirstPerson() || !Boom)
	{
		return;
	}

	// The pivot tracks the feet at shoulder height, lowered as far as the first-person eye is (a share of a crouch's
	// drop, eased on its curve), so crouching or sliding lowers the camera smoothly even though the capsule itself
	// shrinks in a single frame, and a crouch or stand in the air (which moves the feet) doesn't jolt it. A slide dips it
	// a little lower than a crouch, not all the way. The heights and distances are for the full-size body: the pivot sits
	// in the capsule's own (scaled) space, and the arm, which works in the world's, is scaled to match, so a smaller
	// character keeps the framing the view was tuned with.
	const UPlayerLocomotionComponent* Loco = Locomotion.Get();
	const float Lowering = Loco ? ThirdPersonLowering(Loco->GetViewLowering()) : (Owner->bIsCrouched ? 1.f : 0.f);
	const float HalfHeight = Owner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	Boom->SetRelativeLocation(FVector(0.f, 0.f, FMath::Lerp(PivotHeight, CrouchedPivotHeight, Lowering) - HalfHeight));
	const float Scale = static_cast<float>(Owner->GetActorScale3D().Z);

	PullOut = FMath::FInterpConstantTo(PullOut, 1.f, DeltaTime, 1.f / PullOutTime);
	const float Out = 1.f - FMath::Pow(1.f - PullOut, 3.f); // ease out

	if (Mode == EPlayerViewMode::ThirdPerson)
	{
		const FVector Shoulder(ShoulderOffset.X, Tuned(CVarShoulderRight, ShoulderOffset.Y), Tuned(CVarShoulderUp, ShoulderOffset.Z));
		Boom->TargetArmLength = Tuned(CVarArmLength, ArmLength) * Scale * Out;
		Boom->SocketOffset = Shoulder * Scale * Out;
	}
	else
	{
		// Front view: the arm points back at the character from where they're looking.
		const FRotator Control = Owner->GetControlRotation();
		Boom->SetWorldRotation(FRotator(-FRotator::NormalizeAxis(Control.Pitch), Control.Yaw + 180.f, 0.f));
		Boom->TargetArmLength = FrontArmLength * Scale * Out;
		Boom->SocketOffset = FVector(0.f, 0.f, 10.f) * Scale * Out;
	}
}

void UPlayerViewComponent::UpdateFieldOfView()
{
	const float Offset = Locomotion.IsValid() ? Locomotion->GetFieldOfViewOffset() : 0.f;
	UCameraComponent* Active = IsFirstPerson() ? FirstPersonCamera.Get() : ThirdPersonCamera.Get();
	// First person uses the player's setting, read every frame so a change in the settings menu shows without a restart.
	// Aiming narrows the view by the sight's magnification (2x shows half as wide) from there, so a sight zooms the same
	// whatever the setting; its aim point sits on the camera's axis, which no field of view moves, so it stays true.
	const float Aim = GetAimAlpha();
	const float Unzoomed = (IsFirstPerson() ? FirstPersonFieldOfViewSetting(Character.Get(), FirstPersonFieldOfView) : ThirdPersonFieldOfView) + Offset;
	const float Wanted = SightFieldOfView(Unzoomed, AimZoom, Aim);
	if (Active && !FMath::IsNearlyEqual(Active->FieldOfView, Wanted, 0.01f))
	{
		Active->SetFieldOfView(Wanted);
	}

	// The setting widens the world, not the gun: what the camera draws as first person (its view model, through the
	// engine's first-person rendering) keeps the angle the view had before the setting existed, sprint and sight zoom
	// included, so the gun keeps its size and still grows through a sight as it always has. Only meshes drawn as first
	// person follow it (AWeaponBase::AttachToHolder decides that for the gun).
	UCameraComponent* Camera = FirstPersonCamera.Get();
	if (Camera && IsFirstPerson())
	{
		const float ViewModel = SightFieldOfView(FirstPersonFieldOfView + Offset, AimZoom, Aim);
		Camera->SetEnableFirstPersonFieldOfView(true);
		if (!FMath::IsNearlyEqual(Camera->FirstPersonFieldOfView, ViewModel, 0.01f))
		{
			Camera->SetFirstPersonFieldOfView(ViewModel);
		}
	}
}

FRotator UPlayerViewComponent::GetAimRotation() const
{
	const ACharacter* Owner = Character.Get();
	if (!Owner)
	{
		return FRotator::ZeroRotator;
	}
	const FRotator Control = Owner->GetControlRotation();
	return FRotator(FRotator::NormalizeAxis(Control.Pitch), Control.Yaw, 0.f);
}

FQuat UPlayerViewComponent::GetHeldWeaponRotation() const
{
	const FRotator Aim = GetAimRotation();
	// Each shot flips the muzzle up (and twists it a little) about the grip in the hand, a bit harder than in first
	// person so it reads from behind the character.
	FQuat Aiming = Aim.Quaternion() * (Recoil.GetKickRotation() * ThirdPersonKickScale).Quaternion();

	// Reloading tips the gun down and rolls it toward the body, where the reload animation's hands work on it.
	const AWeaponBase* Weapon = WeaponManager.IsValid() ? WeaponManager->GetActiveWeapon() : nullptr;
	if (const float Reload = Weapon ? Weapon->GetReloadBlend() : 0.f; Reload > 0.f)
	{
		Aiming = FQuat::Slerp(Aiming, Aiming * FRotator(-20.f, 15.f, 45.f).Quaternion(), Reload);
	}

	// Sprinting swings the gun down across the chest (muzzle low and to the left), like the first-person sprint pose.
	const float Sprint = Locomotion.IsValid() ? Locomotion->GetSprintAlpha() : 0.f;
	if (Sprint <= 0.f)
	{
		return Aiming;
	}
	const FQuat Carry = FRotator(-35.f, Aim.Yaw - 50.f, 25.f).Quaternion();
	return FQuat::Slerp(Aiming, Carry, Sprint);
}

void UPlayerViewComponent::UpdateHeldWeapon()
{
	const ACharacter* Owner = Character.Get();
	const UWeaponManagerComponent* Manager = WeaponManager.Get();
	USkeletalMeshComponent* Body = Owner ? Owner->GetMesh() : nullptr;
	AWeaponBase* Weapon = Manager ? Manager->GetActiveWeapon() : nullptr;
	if (IsFirstPerson() || !Body || !Weapon || !Manager->IsThirdPersonHold() || Weapon->GetRootComponent()->GetAttachParent() != Body)
	{
		return;
	}

	// The animation moves the hand; the gun keeps its grip in it but points exactly where the player aims, so the
	// barrel lines up with the crosshair whatever the pose. (The left hand is solved onto it in the stance layer.)
	const FTransform Socket = Body->GetSocketTransform(Manager->ThirdPersonAttachSocket);
	const FQuat Relative = Socket.GetRotation().Inverse() * GetHeldWeaponRotation();
	Weapon->SetActorRelativeTransform(FTransform(Relative, Relative.RotateVector(-Weapon->GetGripPoint())));
}

void UPlayerViewComponent::GetAimViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const ACharacter* Owner = Character.Get();
	const AController* Controller = Owner ? Owner->GetController() : nullptr;
	if (!Owner)
	{
		OutLocation = FVector::ZeroVector;
		OutRotation = FRotator::ZeroRotator;
		return;
	}
	if (Mode == EPlayerViewMode::ThirdPersonFront || !Controller)
	{
		Owner->GetActorEyesViewPoint(OutLocation, OutRotation);
		return;
	}

	Controller->GetPlayerViewPoint(OutLocation, OutRotation);
	if (Mode == EPlayerViewMode::ThirdPerson)
	{
		// Same line as the crosshair, but starting level with the character's eyes.
		const FVector Direction = OutRotation.Vector();
		OutLocation += Direction * FMath::Max(FVector::DotProduct(Owner->GetPawnViewLocation() - OutLocation, Direction), 0.f);
	}
}
