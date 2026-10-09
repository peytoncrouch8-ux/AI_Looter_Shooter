#include "Core/LooterCharacter.h"
#include "AI_Looter_Shooter.h"
#include "Audio/PlayerSoundComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Interaction/InteractionComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ALooterCharacter::ALooterCharacter()
{
	// The shared input assets; the Blueprint child can point elsewhere.
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveAsset(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookAsset(TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MouseLookAsset(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpAsset(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	MoveAction = MoveAsset.Object;
	LookAction = LookAsset.Object;
	MouseLookAction = MouseLookAsset.Object;
	JumpAction = JumpAsset.Object;

	// Jumps 15% higher than the engine's 420 cm/s: height grows with the speed squared (v^2 / 2g), 90 cm -> 103.5 cm.
	// Left as it is at the smaller size: the levels' ledges and fences were placed for this jump.
	GetCharacterMovement()->JumpZVelocity = 420.f * FMath::Sqrt(1.15f);
	// Walks up lips and steps to 45 cm on its own (the engine's step, in world cm: over a quarter of the smaller body's
	// height, a full-size 53 cm). Kept: the levels' curbs, porch steps and grave plinths were made for it, and anything
	// higher is a mantle (UPlayerLocomotionComponent), which starts just above it.
	GetCharacterMovement()->MaxStepHeight = 45.f;

	// The full-size mannequin scaled down as one (Player/PlayerSize.h): capsule, body, first-person rig, camera and the
	// gun in hand together, so first and third person stay lined up with no special cases. The engine works the crouch
	// from the scaled capsule; the step height and walkable slopes stay the world's.
	GetCapsuleComponent()->SetRelativeScale3D(FVector(LooterPlayerSize::Scale));
	BaseEyeHeight *= LooterPlayerSize::Scale;
	// Walking at the full-size speed scaled the same, so each step covers the ground the animation's does; sprint and
	// aim are shares of it (UPlayerLocomotionComponent).
	GetCharacterMovement()->MaxWalkSpeed = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale;

	// Made in C++ so every character has it without touching the Blueprint; the weapon manager offers it the loot.
	Interaction = CreateDefaultSubobject<UInteractionComponent>(TEXT("Interaction"));
	Sounds = CreateDefaultSubobject<UPlayerSoundComponent>(TEXT("Sounds"));
	Melee = CreateDefaultSubobject<UPlayerMeleeComponent>(TEXT("Melee"));
	Throw = CreateDefaultSubobject<UPlayerThrowComponent>(TEXT("Throw"));
}

void ALooterCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	if (Sounds)
	{
		Sounds->Jumped();
	}
}

void ALooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input || !MoveAction || !LookAction || !MouseLookAction || !JumpAction)
	{
		UE_LOG(LogLooter, Error, TEXT("%s: movement input is not set up (needs Enhanced Input and all four input actions)."), *GetName());
		return;
	}

	// The keys come from IMC_Default and IMC_MouseLook, which the player controller adds.
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALooterCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALooterCharacter::Look);
	Input->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ALooterCharacter::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ALooterCharacter::JumpPressed);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
}

float ALooterCharacter::GetDefaultHalfHeight() const
{
	// The engine reads the class default's capsule, which never has its scale applied: give the height at the player's
	// size, so a landing or a respawn puts the feet on the ground.
	const ACharacter* Defaults = GetClass()->GetDefaultObject<ACharacter>();
	const UCapsuleComponent* Capsule = Defaults ? Defaults->GetCapsuleComponent() : nullptr;
	return Capsule ? Capsule->GetUnscaledCapsuleHalfHeight() * static_cast<float>(Capsule->GetRelativeScale3D().Z) : Super::GetDefaultHalfHeight();
}

void ALooterCharacter::JumpPressed()
{
	// Crouched or sliding, the jump key stands the player up first; the locomotion component knows the stance.
	if (UPlayerLocomotionComponent* Locomotion = FindComponentByClass<UPlayerLocomotionComponent>())
	{
		Locomotion->HandleJumpPressed();
		return;
	}
	Jump();
}

void ALooterCharacter::Move(const FInputActionValue& Value)
{
	// The locomotion component reads the keys too (the sprint, whether a slide ends in a run, the way a ledge is climbed),
	// even while a slide holds its own line or a mantle or vault carries the body; they steer again once it ends.
	const FVector2D Input = Value.Get<FVector2D>();
	if (UPlayerLocomotionComponent* Locomotion = FindComponentByClass<UPlayerLocomotionComponent>())
	{
		Locomotion->HandleMoveInput(Input);
		if (Locomotion->IsSliding() || Locomotion->IsTraversing())
		{
			return;
		}
	}

	// X strafes and Y walks, both relative to where the character faces.
	AddMovementInput(GetActorRightVector(), Input.X);
	AddMovementInput(GetActorForwardVector(), Input.Y);
}

void ALooterCharacter::Look(const FInputActionValue& Value)
{
	// The player's look sensitivity (Settings > Controls), then slower through a zoomed sight, so the crosshair crosses a
	// target at the same pace as unzoomed.
	const UPlayerViewComponent* View = FindComponentByClass<UPlayerViewComponent>();
	const FVector2D Input = UControlSettingsSubsystem::ScaleLookInputFor(GetController(), Value.Get<FVector2D>())
		* (View ? View->GetLookSensitivityMultiplier() : 1.f);
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}
