#include "Player/PlayerViewComponent.h"
#include "AI_Looter_Shooter.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

// UPlayerViewComponent's lifetime, input and view modes. Aiming and recoil are in PlayerViewAim.cpp, the cameras, field
// of view and held gun in PlayerViewCamera.cpp, the view's kicks in PlayerViewKicks.cpp.

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
	// Being hurt jolts the view (PlayerViewKicks.cpp).
	BindOwnerHealth(true);

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
	BindOwnerHealth(false);
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
