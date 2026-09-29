#include "Player/PlayerViewComponent.h"
#include "AI_Looter_Shooter.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The gun in the body's hands flips this much harder than the first-person gun, so the kick reads from behind. */
	constexpr float ThirdPersonKickScale = 1.35f;

	// Live tuning for the third-person camera (negative = use the component's settings).
	TAutoConsoleVariable<float> CVarArmLength(TEXT("Looter.ThirdPerson.ArmLength"), -1.f, TEXT("Override the third-person camera distance (cm)."));
	TAutoConsoleVariable<float> CVarShoulderRight(TEXT("Looter.ThirdPerson.ShoulderRight"), -1.f, TEXT("Override the third-person camera's offset to the right (cm)."));
	TAutoConsoleVariable<float> CVarShoulderUp(TEXT("Looter.ThirdPerson.ShoulderUp"), -1.f, TEXT("Override the third-person camera's offset upward (cm)."));

	float Tuned(const TAutoConsoleVariable<float>& Variable, float Value)
	{
		const float Override = Variable.GetValueOnGameThread();
		return Override >= 0.f ? Override : Value;
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
	UpdateRecoil(DeltaTime);
	UpdateBoom(DeltaTime);
	UpdateFieldOfView();
	UpdateHeldWeapon();
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
	}
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

	// The pivot tracks the feet at shoulder height, eased with the crouch pose, so crouching lowers the camera
	// smoothly even though the capsule itself shrinks in a single frame.
	const UPlayerLocomotionComponent* Loco = Locomotion.Get();
	const float Crouch = Loco ? Loco->GetCrouchAlpha() : (Owner->bIsCrouched ? 1.f : 0.f);
	const float HalfHeight = Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Boom->SetRelativeLocation(FVector(0.f, 0.f, FMath::Lerp(PivotHeight, CrouchedPivotHeight, Crouch) - HalfHeight));

	PullOut = FMath::FInterpConstantTo(PullOut, 1.f, DeltaTime, 1.f / PullOutTime);
	const float Out = 1.f - FMath::Pow(1.f - PullOut, 3.f); // ease out

	if (Mode == EPlayerViewMode::ThirdPerson)
	{
		const FVector Shoulder(ShoulderOffset.X, Tuned(CVarShoulderRight, ShoulderOffset.Y), Tuned(CVarShoulderUp, ShoulderOffset.Z));
		Boom->TargetArmLength = Tuned(CVarArmLength, ArmLength) * Out;
		Boom->SocketOffset = Shoulder * Out;
	}
	else
	{
		// Front view: the arm points back at the character from where they're looking.
		const FRotator Control = Owner->GetControlRotation();
		Boom->SetWorldRotation(FRotator(-FRotator::NormalizeAxis(Control.Pitch), Control.Yaw + 180.f, 0.f));
		Boom->TargetArmLength = FrontArmLength * Out;
		Boom->SocketOffset = FVector(0.f, 0.f, 10.f) * Out;
	}
}

void UPlayerViewComponent::UpdateFieldOfView()
{
	const float Offset = Locomotion.IsValid() ? Locomotion->GetFieldOfViewOffset() : 0.f;
	UCameraComponent* Active = IsFirstPerson() ? FirstPersonCamera.Get() : ThirdPersonCamera.Get();
	const float Wanted = (IsFirstPerson() ? FirstPersonFieldOfView : ThirdPersonFieldOfView) + Offset;
	if (Active && !FMath::IsNearlyEqual(Active->FieldOfView, Wanted, 0.01f))
	{
		Active->SetFieldOfView(Wanted);
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
