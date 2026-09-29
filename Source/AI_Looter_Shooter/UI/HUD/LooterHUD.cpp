#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Menus/PauseMenuWidget.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "BuildMode/BuildModeComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	UKeyBindingSubsystem* GetKeyBindings(const APlayerController* PC)
	{
		const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
		return LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	}
}

ALooterHUD::ALooterHUD()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALooterHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	HUDWidget = CreateWidget<UPlayerHUDWidget>(PC, UPlayerHUDWidget::StaticClass());
	if (HUDWidget)
	{
		HUDWidget->AddToViewport(0);
	}
	InventoryWidget = CreateWidget<ULoadoutWidget>(PC, ULoadoutWidget::StaticClass());
	PauseMenuWidget = CreateWidget<UPauseMenuWidget>(PC, UPauseMenuWidget::StaticClass());

#if !UE_BUILD_SHIPPING
	// Dev-only environment editor. Never exists in shipping builds.
	if (!PC->FindComponentByClass<UBuildModeComponent>())
	{
		UBuildModeComponent* BuildMode = NewObject<UBuildModeComponent>(PC, TEXT("BuildMode"));
		BuildMode->RegisterComponent();
	}
#endif

	BindMenuInput();
}

void ALooterHUD::BindMenuInput()
{
	APlayerController* PC = GetOwningPlayerController();
	UKeyBindingSubsystem* Bindings = GetKeyBindings(PC);
	if (!Bindings)
	{
		return;
	}

	Bindings->SyncContexts();

	// Our own input component on the controller, so menu keys work with any pawn (including Build Mode's camera).
	EnableInput(PC);
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		Input->BindAction(Bindings->GetPauseAction(), ETriggerEvent::Started, this, &ALooterHUD::HandlePausePressed);
		Input->BindAction(Bindings->GetInventoryAction(), ETriggerEvent::Started, this, &ALooterHUD::HandleInventoryPressed);
		Input->BindAction(Bindings->GetBuildModeAction(), ETriggerEvent::Started, this, &ALooterHUD::HandleBuildModePressed);
	}
}

bool ALooterHUD::IsBuildModeActive() const
{
	const APlayerController* PC = GetOwningPlayerController();
	const UBuildModeComponent* BuildMode = PC ? PC->FindComponentByClass<UBuildModeComponent>() : nullptr;
	return BuildMode && BuildMode->IsBuildMode();
}

void ALooterHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !HUDWidget)
	{
		return;
	}

	// Keep the player's rebound keys active even when other code (re)adds the original input assets.
	if (UKeyBindingSubsystem* Bindings = GetKeyBindings(PC))
	{
		Bindings->SyncContexts();
	}

	const bool bBuildMode = IsBuildModeActive();
	if (bBuildMode && bInventoryOpen)
	{
		CloseInventory();
	}

	const bool bHideHUD = bBuildMode || bInventoryOpen || bPauseMenuOpen;
	HUDWidget->SetVisibility(bHideHUD ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

// ---------------------------------------------------------------------------
// Hotkeys
// ---------------------------------------------------------------------------

void ALooterHUD::HandlePausePressed()
{
	// While a menu is open it has focus and handles Escape itself; this only fires from gameplay.
	if (!bPauseMenuOpen)
	{
		OpenPauseMenu();
	}
}

void ALooterHUD::HandleInventoryPressed()
{
	if (!IsBuildModeActive() && !bPauseMenuOpen && !bInventoryOpen)
	{
		OpenInventory();
	}
}

void ALooterHUD::HandleBuildModePressed()
{
	APlayerController* PC = GetOwningPlayerController();
	UBuildModeComponent* BuildMode = PC ? PC->FindComponentByClass<UBuildModeComponent>() : nullptr;
	if (BuildMode && !bPauseMenuOpen && !bInventoryOpen)
	{
		BuildMode->SetBuildMode(!BuildMode->IsBuildMode());
	}
}

void ALooterHUD::RestoreGameInput()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	UBuildModeComponent* BuildMode = PC->FindComponentByClass<UBuildModeComponent>();
	if (BuildMode && BuildMode->IsBuildMode())
	{
		BuildMode->ApplyInputMode();
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

// ---------------------------------------------------------------------------
// Inventory
// ---------------------------------------------------------------------------

void ALooterHUD::OpenInventory()
{
	APlayerController* PC = GetOwningPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UWeaponManagerComponent* Manager = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	if (!InventoryWidget || !Manager || bInventoryOpen)
	{
		return;
	}

	Manager->StopFire();
	InventoryWidget->Open(this, Manager);
	InventoryWidget->AddToViewport(20);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
	bInventoryOpen = true;
}

void ALooterHUD::CloseInventory()
{
	if (!bInventoryOpen)
	{
		return;
	}
	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
	}
	bInventoryOpen = false;
	RestoreGameInput();
}

// ---------------------------------------------------------------------------
// Pause / settings
// ---------------------------------------------------------------------------

void ALooterHUD::OpenPauseMenu()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PauseMenuWidget || bPauseMenuOpen)
	{
		return;
	}

	CloseInventory();
	if (const APawn* Pawn = PC->GetPawn())
	{
		if (UWeaponManagerComponent* Manager = Pawn->FindComponentByClass<UWeaponManagerComponent>())
		{
			Manager->StopFire();
		}
	}

	PauseMenuWidget->Open(this);
	PauseMenuWidget->AddToViewport(40);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);

	UGameplayStatics::SetGamePaused(this, true);
	bPauseMenuOpen = true;
}

void ALooterHUD::ClosePauseMenu()
{
	if (!bPauseMenuOpen)
	{
		return;
	}
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
	}
	UGameplayStatics::SetGamePaused(this, false);
	bPauseMenuOpen = false;
	RestoreGameInput();
}

void ALooterHUD::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayerController(), EQuitPreference::Quit, false);
}
