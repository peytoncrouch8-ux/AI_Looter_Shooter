// ALooterHUD: the pause menu, and the pause itself, which the inventory's pages share (the user's call, 2026-10-27).

#include "UI/HUD/LooterHUD.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Session/SessionSubsystem.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Templates/UnrealTemplate.h"

bool ALooterHUD::PausesGame(bool bPauseMenuIsOpen, bool bInventoryIsOpen)
{
	// The inventory is a long look at guns, maps and the bestiary: nothing should reach the player meanwhile. The world's
	// screens stand in the world (a vendor's counter, the bench, a board) and the world goes on round them.
	return bPauseMenuIsOpen || bInventoryIsOpen;
}

void ALooterHUD::ApplyGamePause()
{
	// Only these menus pause, so their state alone says whether the game runs.
	const bool bPause = PausesGame(bPauseMenuOpen, bInventoryOpen);
	UGameplayStatics::SetGamePaused(this, bPause);
	if (bPause)
	{
		if (HUDWidget)
		{
			HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		SetWorldLabelsVisible(false);
	}
}

void ALooterHUD::OpenPauseMenu()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PauseMenuWidget || bPauseMenuOpen)
	{
		return;
	}

	{
		TGuardValue<bool> Quiet(bQuietClose, true);
		CloseInventory();
		CloseStationBoard();
		CloseBench();
		// An offer left open goes back to its giver unanswered (it's offered again), before the board under it closes.
		CloseMissionOffer();
		CloseNoticeBoard();
	}
	if (const APawn* Pawn = PC->GetPawn())
	{
		if (UWeaponManagerComponent* Manager = Pawn->FindComponentByClass<UWeaponManagerComponent>())
		{
			Manager->StopFire();
		}
	}

	PauseMenuWidget->Open(ESettingsMenuMode::Pause);
	PauseMenuWidget->AddToViewport(40);
	PlayPageSound(true);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);

	bPauseMenuOpen = true;
	ApplyGamePause();
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
	bPauseMenuOpen = false;
	ApplyGamePause();
	RestoreGameInput();
	PlayPageSound(false);
}

void ALooterHUD::SaveAndQuit()
{
	// The game stays paused until the main menu replaces the level, so nothing changes after the save.
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->SaveAndQuitToMenu();
	}
}
