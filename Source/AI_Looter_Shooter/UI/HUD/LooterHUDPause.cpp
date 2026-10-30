// ALooterHUD: the pause menu, and the pause itself, which the inventory's pages share (the user's call, 2026-10-27). Escape
// opens the pause menu's list (UPauseMenuWidget; the user, 2026-10-29 night: "Hitting ESC shouldn't take me right to
// settings"): Resume Game, the inventory's pages, Settings, Save & Quit. What the list opens comes back to it when it
// closes; only the list itself goes back to the game. The game stays paused throughout.

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

bool ALooterHUD::PausePageOf(EPauseMenuItem Item, EInventoryPage& OutPage)
{
	switch (Item)
	{
	case EPauseMenuItem::Inventory: OutPage = EInventoryPage::Loadout;  return true;
	case EPauseMenuItem::Missions:  OutPage = EInventoryPage::Missions; return true;
	case EPauseMenuItem::Map:       OutPage = EInventoryPage::Map;      return true;
	case EPauseMenuItem::Bestiary:  OutPage = EInventoryPage::Bestiary; return true;
	case EPauseMenuItem::Embers:    OutPage = EInventoryPage::Embers;   return true;
	default:                        return false;
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
	bPauseVisitCounted = false;
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
	// A page the list opened goes with it, quietly (the pause menu's close sounds for both), and not back to the list.
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

void ALooterHUD::ShowPauseHub(EPauseMenuItem Focus)
{
	if (!PauseHubWidget)
	{
		return;
	}
	PauseScreen = EPauseScreen::Hub;
	PauseHubWidget->Open(Focus);
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
	if (Item == EPauseMenuItem::Resume)
	{
		ClosePauseMenu();
		return;
	}
	if (Item == EPauseMenuItem::SaveQuit)
	{
		SaveAndQuit();
		return;
	}
	// The rest open from the list only (a console's pick while a page is up waits for the list).
	if (PauseScreen != EPauseScreen::Hub)
	{
		return;
	}
	EInventoryPage Page = EInventoryPage::Loadout;
	if (Item == EPauseMenuItem::Settings)
	{
		OpenPauseSettings();
	}
	else if (PausePageOf(Item, Page))
	{
		OpenPausePage(Item, Page);
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
	// Over the game: its top row is Back, to the list (Save & Quit is the list's alone now).
	PauseSettingsWidget->Open(ESettingsMenuMode::Pause);
	PauseSettingsWidget->AddToViewport(PauseMenuZOrder);
	PauseScreen = EPauseScreen::Settings;
	FocusMenu(PauseSettingsWidget);
	PlayPageSound(true);
}

void ALooterHUD::OpenPausePage(EPauseMenuItem Item, EInventoryPage Page)
{
	// One pause is one visit: the pages looked at from this pause's list count together (WasPageShownLastVisit), as the
	// inventory's tabs within one visit do.
	if (!bPauseVisitCounted)
	{
		PagesShownLastVisit.Reset();
		bPauseVisitCounted = true;
	}
	const EInventoryPage Before = InventoryPage;
	InventoryPage = Page;
	if (PauseHubWidget)
	{
		PauseHubWidget->RemoveFromParent();
	}
	// The page opens as its key opens it (its own Open, the keys on it), over the paused game.
	if (!OpenInventoryPage())
	{
		// No player to show it for (dead, between pawns): the list stays, and says no.
		InventoryPage = Before;
		ShowPauseHub(Item);
		LooterSound::Play2D(this, LooterSoundCue::Denied);
		return;
	}
	bInventoryOpen = true;
	PauseScreen = EPauseScreen::Page;
	PauseReturnItem = Item;
	PlayPageSound(true);
	ApplyGamePause();
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
	// Back on the row it left from, the game still paused; the screen's close is its sound.
	ShowPauseHub(PauseReturnItem);
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
