#include "Core/LooterCharacter.h"
#include "AI_Looter_Shooter.h"
#include "Audio/PlayerSoundComponent.h"
#include "Components/CapsuleComponent.h"
#include "Embers/EmberComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Interaction/InteractionComponent.h"
#include "Misc/App.h"
#include "Player/AimAssist.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "Player/Animation/HandJarComponent.h"
#include "Player/FirstPersonHandsComponent.h"
#include "Player/PlayerFlaskComponent.h"
#include "Player/PlayerFootfallComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/**
	 * The right stick turns the view (AimAssist.h): a dead zone and a response curve, then degrees a second times this
	 * frame's real time (not the world's, which hit-stop slows: the mouse isn't slowed either), so a full push turns at the
	 * same speed at 60 or 120 fps. By the look sensitivity and a zoomed sight's slowing like the mouse, slower on a creature
	 * by the aim assist's friction, nudged toward one by its magnet. Looter.Look.PadDegreesPerSecond 0 gives the old per-frame
	 * stick back. It lives apart from Look() (the mouse's) because the action says which device turned the view; guessing from
	 * the last key pressed would treat a mouse as a stick whenever the guess was late.
	 */
	void LookByStick(ALooterCharacter& Character, const FInputActionValue& Value)
	{
		AController* Controller = Character.GetController();
		APlayerController* Player = Cast<APlayerController>(Controller);
		const UPlayerViewComponent* View = Character.FindComponentByClass<UPlayerViewComponent>();
		const float SightSlowing = View ? View->GetLookSensitivityMultiplier() : 1.f;
		const bool bInvertY = UControlSettingsSubsystem::IsInvertYFor(Controller);
		const FVector2D Stick = Value.Get<FVector2D>();
		const AimAssist::FTuning Tuning = AimAssist::LiveTuning();

		if (Tuning.YawDegreesPerSecond <= 0.f)
		{
			const FVector2D Old = AimAssist::InvertY(UControlSettingsSubsystem::ScaleLookInputFor(Controller, Stick) * SightSlowing, bInvertY);
			Character.AddControllerYawInput(Old.X);
			Character.AddControllerPitchInput(Old.Y);
			return;
		}

		const FVector2D Shaped = AimAssist::ShapeStick(Stick, Tuning.StickDeadZone, Tuning.StickCurve);
		if (Shaped.IsNearlyZero())
		{
			return;
		}
		const float DeltaSeconds = static_cast<float>(FApp::GetDeltaTime());
		const FVector2D Turn = AimAssist::StickTurn(Shaped, Tuning.YawDegreesPerSecond, Tuning.PitchDegreesPerSecond, DeltaSeconds, Tuning.MaxStepSeconds);
		const AimAssist::FFrame Assist = Player ? AimAssist::ForPlayer(*Player, /*bStickMoving*/ true, DeltaSeconds) : AimAssist::FFrame();

		// Degrees to the controller's input: its legacy scales (2.5 and -2.5 here) are divided out, so 220 is 220 degrees. The
		// magnet's pull is a rotation already (yaw right, pitch up), so it is not inverted or scaled by the sensitivity: it is
		// the game's nudge, not the player's input.
		const FVector2D Look = AimAssist::InvertY(UControlSettingsSubsystem::ScaleLookInputFor(Controller, Turn * Assist.FrictionScale) * SightSlowing, bInvertY);
		const FVector2D Scales = Player ? AimAssist::LegacyInputScales(*Player) : FVector2D(1.0, 1.0);
		const FVector2D Input = AimAssist::DegreesToInput(Look, Scales) + AimAssist::RotationToInput(Assist.Pull, Scales);
		Character.AddControllerYawInput(Input.X);
		Character.AddControllerPitchInput(Input.Y);
	}
}

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
	// Walking at the full-size speed scaled the same, and slowed to the game's pace (the user's 10%); sprint, slide and
	// aim are shares of it (UPlayerLocomotionComponent), so they slow with it.
	GetCharacterMovement()->MaxWalkSpeed = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::MoveScale;

	// Made in C++ so every character has it without touching the Blueprint; the weapon manager offers it the loot.
	Interaction = CreateDefaultSubobject<UInteractionComponent>(TEXT("Interaction"));
	Sounds = CreateDefaultSubobject<UPlayerSoundComponent>(TEXT("Sounds"));
	Footfalls = CreateDefaultSubobject<UPlayerFootfallComponent>(TEXT("Footfalls"));
	Melee = CreateDefaultSubobject<UPlayerMeleeComponent>(TEXT("Melee"));
	Throw = CreateDefaultSubobject<UPlayerThrowComponent>(TEXT("Throw"));
	Embers = CreateDefaultSubobject<UEmberComponent>(TEXT("Embers"));
	Flask = CreateDefaultSubobject<UPlayerFlaskComponent>(TEXT("Flask"));
	// The jar in the throwing hand, which the third-person body otherwise lobbed with an empty fist.
	HandJar = CreateDefaultSubobject<UHandJarComponent>(TEXT("HandJar"));
	HandJar->SetupAttachment(GetMesh(), UHandJarComponent::HandSocket());
	// The first-person arms on the gun. The camera is the Blueprint's, so it waits on the capsule until play moves it there.
	Hands = CreateDefaultSubobject<UFirstPersonHandsComponent>(TEXT("FirstPersonHands"));
	Hands->SetupAttachment(GetCapsuleComponent());
	// Ellis over the mannequin, copying its pose (the user, 2026-10-15: his body in place of the white mannequin).
	Figure = CreateDefaultSubobject<UBodyFigureComponent>(TEXT("Figure"));
	Figure->SetupAttachment(GetMesh());
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
	// The stick and the mouse turn the view differently (a speed against a distance), so each has its own handler.
	Input->BindActionValueLambda(LookAction, ETriggerEvent::Triggered, [Self = TWeakObjectPtr<ALooterCharacter>(this)](const FInputActionValue& Value)
	{
		if (ALooterCharacter* Character = Self.Get())
		{
			LookByStick(*Character, Value);
		}
	});
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
	// The mouse: its delta is a distance already, so no time or curve (and the engine's mouse smoothing and FOV scaling are off in
	// DefaultInput.ini). The player's look sensitivity (Settings > Controls), then slower through a zoomed sight, so the
	// crosshair crosses a target at the same pace as unzoomed; Invert Y last. No aim assist: that is the stick's alone.
	const UPlayerViewComponent* View = FindComponentByClass<UPlayerViewComponent>();
	const FVector2D Input = AimAssist::InvertY(UControlSettingsSubsystem::ScaleLookInputFor(GetController(), Value.Get<FVector2D>())
		* (View ? View->GetLookSensitivityMultiplier() : 1.f), UControlSettingsSubsystem::IsInvertYFor(GetController()));
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}
