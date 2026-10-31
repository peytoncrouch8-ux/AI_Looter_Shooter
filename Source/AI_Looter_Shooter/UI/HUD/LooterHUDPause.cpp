// ALooterHUD: the pause menu, and the pause itself, which the inventory's pages share (the user's call, 2026-10-27). Escape
// opens the pause menu's hub (UPauseMenuWidget, full screen; the user, 2026-10-31: "the ESC menu should encompass the
// entire screen ... should not have whats in the inventory tabs"): Back to Game, Settings, Save & Quit, Quit Game. The
// settings page it opens comes back to it (on Settings) when it closes; only the hub itself goes back to the game. The
// game stays paused throughout.

#include "UI/HUD/LooterHUD.h"
#include "Audio/LooterSound.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Session/SessionSubsystem.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Templates/UnrealTemplate.h"

namespace
{
	/** The pause menu's viewport layer, over every other menu (the inventory's pages are 20). */
	constexpr int32 PauseMenuZOrder = 40;
}

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

EPauseScreen ALooterHUD::PauseBackFrom(EPauseScreen Screen)
{
	switch (Screen)
	{
	case EPauseScreen::Settings:
	case EPauseScreen::Page:
		return EPauseScreen::Hub;
	default:
		return EPauseScreen::None;
	}
}

void ALooterHUD::OpenPauseMenu()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PauseHubWidget || bPauseMenuOpen)
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

	bPauseMenuOpen = true;
	ShowPauseHub(EPauseMenuItem::Resume);
	PlayPageSound(true);
	ApplyGamePause();
}

void ALooterHUD::ClosePauseMenu()
{
	if (!bPauseMenuOpen)
	{
		return;
	}
	// A page the hub opened goes with it, quietly (the pause menu's close sounds for both), and not back to the hub.
	if (PauseScreen == EPauseScreen::Page)
	{
		PauseScreen = EPauseScreen::Hub;
		TGuardValue<bool> Quiet(bQuietClose, true);
		CloseInventory();
	}
	if (PauseSettingsWidget)
	{
		PauseSettingsWidget->RemoveFromParent();
	}
	if (PauseHubWidget)
	{
		PauseHubWidget->RemoveFromParent();
	}
	PauseScreen = EPauseScreen::None;
	bPauseMenuOpen = false;
	ApplyGamePause();
	RestoreGameInput();
	PlayPageSound(false);
}

void ALooterHUD::ShowPauseHub(EPauseMenuItem Focus, bool bReturning)
{
	if (!PauseHubWidget)
	{
		return;
	}
	PauseScreen = EPauseScreen::Hub;
	PauseHubWidget->Open(Focus, bReturning);
	if (!PauseHubWidget->IsInViewport())
	{
		PauseHubWidget->AddToViewport(PauseMenuZOrder);
	}
	FocusMenu(PauseHubWidget);
}

void ALooterHUD::ChoosePauseItem(EPauseMenuItem Item)
{
	if (!bPauseMenuOpen)
	{
		return;
	}
	switch (Item)
	{
	case EPauseMenuItem::Resume:
		ClosePauseMenu();
		return;
	case EPauseMenuItem::SaveQuit:
		SaveAndQuit();
		return;
	case EPauseMenuItem::QuitGame:
		SaveAndQuitGame();
		return;
	case EPauseMenuItem::Settings:
		// From the hub only (a console's pick while the settings page is up waits for the hub).
		if (PauseScreen == EPauseScreen::Hub)
		{
			OpenPauseSettings();
		}
		return;
	default:
		return;
	}
}

void ALooterHUD::OpenPauseSettings()
{
	if (!PauseSettingsWidget)
	{
		return;
	}
	PauseReturnItem = EPauseMenuItem::Settings;
	if (PauseHubWidget)
	{
		PauseHubWidget->RemoveFromParent();
	}
	// The settings page, full screen over the game: its Back (and Esc) comes back to the hub (OnClose, BeginPlay).
	PauseSettingsWidget->Open(ESettingsMenuMode::Pause);
	PauseSettingsWidget->AddToViewport(PauseMenuZOrder);
	PauseScreen = EPauseScreen::Settings;
	FocusMenu(PauseSettingsWidget);
	PlayPageSound(true);
}

void ALooterHUD::BackOutOfPauseScreen()
{
	if (!bPauseMenuOpen)
	{
		return;
	}
	if (PauseBackFrom(PauseScreen) != EPauseScreen::Hub)
	{
		ClosePauseMenu();
		return;
	}
	if (PauseScreen == EPauseScreen::Settings && PauseSettingsWidget)
	{
		PauseSettingsWidget->RemoveFromParent();
	}
	// Back on the option it left from (Settings lit), the game still paused; the page's close is its sound.
	ShowPauseHub(PauseReturnItem, /*bReturning*/ true);
	PlayPageSound(false);
	ApplyGamePause();
}

void ALooterHUD::SaveAndQuit()
{
	// The game stays paused until the main menu replaces the level, so nothing changes after the save.
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->SaveAndQuitToMenu();
	}
}

void ALooterHUD::SaveAndQuitGame()
{
	// Written now, while the game stands paused (the level's teardown on the way out saves once more; same contents), then
	// the game closes as the main menu's Quit Game closes it. Without a session SaveNow writes nothing.
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->SaveNow();
	}
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayerController(), EQuitPreference::Quit, false);
}
