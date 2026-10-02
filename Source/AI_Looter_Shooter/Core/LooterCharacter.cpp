#include "Core/LooterCharacter.h"
#include "AI_Looter_Shooter.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Interaction/InteractionComponent.h"
#include "Player/PlayerViewComponent.h"
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
	GetCharacterMovement()->JumpZVelocity = 420.f * FMath::Sqrt(1.15f);

	// Made in C++ so every character has it without touching the Blueprint; the weapon manager offers it the loot.
	Interaction = CreateDefaultSubobject<UInteractionComponent>(TEXT("Interaction"));
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
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
}

void ALooterCharacter::Move(const FInputActionValue& Value)
{
	// X strafes and Y walks, both relative to where the character faces.
	const FVector2D Input = Value.Get<FVector2D>();
	AddMovementInput(GetActorRightVector(), Input.X);
	AddMovementInput(GetActorForwardVector(), Input.Y);
}

void ALooterCharacter::Look(const FInputActionValue& Value)
{
	// Slower through a zoomed sight, so the crosshair crosses a target at the same pace as unzoomed.
	const UPlayerViewComponent* View = FindComponentByClass<UPlayerViewComponent>();
	const FVector2D Input = Value.Get<FVector2D>() * (View ? View->GetLookSensitivityMultiplier() : 1.f);
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}
