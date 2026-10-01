#include "Core/LooterPlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

ALooterPlayerController::ALooterPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> Movement(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseLook(TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"));
	DefaultContexts.Add(Movement.Object);
	DefaultContexts.Add(MouseLook.Object);
}

void ALooterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	// The game window keeps the main menu's input mode (the mouse free, the game ignoring keys) across the level change:
	// play takes the keyboard and mouse back.
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
}

void ALooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Runs once the controller has its local player, so the input subsystem exists.
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Input)
	{
		return;
	}
	for (const UInputMappingContext* Context : DefaultContexts)
	{
		if (Context)
		{
			Input->AddMappingContext(Context, 0);
		}
	}
}

void ALooterPlayerController::SpawnPlayerCameraManager()
{
	Super::SpawnPlayerCameraManager();
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = ViewPitchMin;
		PlayerCameraManager->ViewPitchMax = ViewPitchMax;
	}
}
