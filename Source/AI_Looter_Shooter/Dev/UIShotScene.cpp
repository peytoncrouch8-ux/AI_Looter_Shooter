// The scene of Looter.UIShots: how each state of the main menu, the pause menu (its list, then its settings) and the
// level-up popup is made (not in shipping builds). UIShotDevCommands.cpp runs the steps and takes the pictures; UIShotScene.h says what a step is.

#include "Dev/UIShotScene.h"
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Menus/MainMenuHUD.h"
#include "UI/Menus/MainMenuWidget.h"
#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/SWindow.h"

namespace
{
	using UIShotScene::EHalf;
	using UIShotScene::FStep;
	using SettingsMenu::ESettingsPage;

	/** After a request the screenshot is written at the end of the frame; this long before the next step changes the picture. */
	constexpr float DefaultGapSeconds = 0.6f;
	/** After the last picture, before the game quits: the file is written from another thread. */
	constexpr float LastGapSeconds = 1.5f;
	/** After the pause menu closes: the gameplay HUD is shown again on the HUD's next tick, and its clusters settle. */
	constexpr float HudBackSeconds = 1.2f;
	/** The menu's backdrop (Teropa) streaming in; the level's first frames, streaming and eye adaptation. */
	constexpr float MenuWarmupSeconds = 3.f;
	constexpr float LevelWarmupSeconds = 6.f;
	/** The level-up popup's two moments, seconds into its show: during the count, and holding the new level. */
	constexpr float LevelUpCountSeconds = 0.5f;
	constexpr float LevelUpHoldSeconds = 1.4f;
	/** A main button counts as lit (its words gold, the reticle on it) from here (0 to 1). */
	constexpr float LitEnough = 0.98f;
	/** The most key presses it takes to walk the focus to a button (four main buttons, round the ends). */
	constexpr int32 MaxFocusPresses = 6;
	/** Where the mouse is parked, from the window's top-left (pixels): the left edge, clear of every button and row. */
	const FVector2D ParkedMouse(6.0, 420.0);

	const FName ActionSinglePlayer(TEXT("SinglePlayer"));
	const FName ActionSettings(TEXT("Settings"));

	/** When the level-up popup's show began (FPlatformTime seconds; below zero: not yet), for its second picture. */
	double LevelUpShownAt = -1.0;

	// --- Finding things ---

	/** The widget of class T in World (in the viewport only, if asked), or null. */
	template <typename T>
	T* UIShotFindWidget(UWorld& World, bool bInViewport)
	{
		for (TObjectIterator<T> It; It; ++It)
		{
			T* Widget = *It;
			if (Widget && !Widget->IsTemplate() && Widget->GetWorld() == &World && (!bInViewport || Widget->IsInViewport()))
			{
				return Widget;
			}
		}
		return nullptr;
	}

	UMainMenuWidget* UIShotMainMenu(UWorld& World)
	{
		return UIShotFindWidget<UMainMenuWidget>(World, /*bInViewport*/ true);
	}

	AMainMenuHUD* UIShotMenuHud(UWorld& World)
	{
		const APlayerController* Controller = World.GetFirstPlayerController();
		return Controller ? Cast<AMainMenuHUD>(Controller->GetHUD()) : nullptr;
	}

	/** The settings menu on screen: over the main menu, or the pause menu over the game. */
	USettingsMenuWidget* UIShotSettings(UWorld& World)
	{
		return UIShotFindWidget<USettingsMenuWidget>(World, /*bInViewport*/ true);
	}

	ALooterHUD* UIShotLevelHud(UWorld& World)
	{
		return ALooterHUD::FindFor(World.GetFirstPlayerController());
	}

	/** The pause menu's list on screen (Escape's menu over the game). */
	UPauseMenuWidget* UIShotPauseHub(UWorld& World)
	{
		return UIShotFindWidget<UPauseMenuWidget>(World, /*bInViewport*/ true);
	}

	UPlayerProgressionSubsystem* UIShotProgression(UWorld& World)
	{
		const APlayerController* Controller = World.GetFirstPlayerController();
		const ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
		return LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	}

	// --- Keys and the mouse ---

	/**
	 * A key pressed and let go on Widget, through its own key handling (the one Slate's key events reach): the same path a
	 * player's key takes, without depending on which widget holds Slate's keyboard focus in a window nobody is at.
	 */
	template <typename TWidget>
	void UIShotPress(TWidget& Widget, const FKey& Key)
	{
		UE_LOG(LogLooter, Display, TEXT("Looter.UIShots: key %s"), *Key.ToString());
		Widget.HandleNavKey(Key, /*bRepeat*/ false);
		Widget.HandleNavKeyUp(Key);
	}

	/** While the mouse drove the menus the first key only brings the focus look up: that press, if it's due. */
	template <typename TWidget>
	void UIShotReveal(TWidget& Widget, const FKey& Key)
	{
		if (!LooterUI::IsMenuFocusShown())
		{
			UIShotPress(Widget, Key);
		}
	}

	/** Key pressed until the focus is on Target (a few presses at most). False if it never got there. */
	template <typename TWidget>
	bool UIShotFocusOn(TWidget& Widget, const FKey& Key, const UWidget* Target)
	{
		if (!Target)
		{
			return false;
		}
		UIShotReveal(Widget, Key);
		for (int32 Press = 0; Press < MaxFocusPresses && Widget.GetFocusedWidget() != Target; ++Press)
		{
			UIShotPress(Widget, Key);
		}
		return Widget.GetFocusedWidget() == Target;
	}

	/** The mouse to a spot over no button or row, so the screens' hover follows only the keys. */
	void UIShotParkMouse()
	{
		const TSharedPtr<SWindow> Window = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
		if (Window.IsValid() && FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().SetCursorPos(Window->GetPositionInScreen() + ParkedMouse);
		}
	}

	// --- The main menu's steps ---

	bool UIShotMenuRevealed(UWorld& World)
	{
		const UMainMenuWidget* Menu = UIShotMainMenu(World);
		return Menu && Menu->IsRevealed();
	}

	/** The focus walked down to Settings with the menu's own keys; caught once its words are gold and the reticle is on it. */
	void UIShotBeginLightSettings(UWorld& World)
	{
		UMainMenuWidget* Menu = UIShotMainMenu(World);
		if (Menu && !UIShotFocusOn(*Menu, EKeys::Down, Menu->GetMainButton(ActionSettings)))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: the focus didn't reach Settings."));
		}
	}

	bool UIShotSettingsLit(UWorld& World)
	{
		const UMainMenuWidget* Menu = UIShotMainMenu(World);
		return Menu && Menu->GetButtonLit(ActionSettings) >= LitEnough;
	}

	/** Enter on the lit Settings: the button's own click opens the settings over the menu (on Graphics, its first page). */
	void UIShotBeginMenuSettings(UWorld& World)
	{
		UMainMenuWidget* Menu = UIShotMainMenu(World);
		AMainMenuHUD* Hud = UIShotMenuHud(World);
		if (Menu)
		{
			UIShotPress(*Menu, EKeys::Enter);
		}
		if (Hud && !Hud->IsSettingsOpen())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: Enter on Settings didn't open them; opened through the menu's HUD."));
			Hud->OpenSettings();
		}
		USettingsMenuWidget* Settings = UIShotSettings(World);
		if (Settings && Settings->GetPage() != static_cast<int32>(ESettingsPage::Graphics))
		{
			Settings->SelectPage(static_cast<int32>(ESettingsPage::Graphics));
		}
	}

	bool UIShotMenuSettingsOpen(UWorld& World)
	{
		const AMainMenuHUD* Hud = UIShotMenuHud(World);
		return Hud && Hud->IsSettingsOpen() && UIShotSettings(World);
	}

	/** The settings closed with Esc, the focus walked up to Single Player and Enter: the session picker. */
	void UIShotBeginSessions(UWorld& World)
	{
		AMainMenuHUD* Hud = UIShotMenuHud(World);
		if (USettingsMenuWidget* Settings = UIShotSettings(World); Settings && Hud && Hud->IsSettingsOpen())
		{
			UIShotPress(*Settings, EKeys::Escape);
		}
		if (Hud && Hud->IsSettingsOpen())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: Esc didn't close the settings; closed through the menu's HUD."));
			Hud->CloseSettings();
		}
		UMainMenuWidget* Menu = UIShotMainMenu(World);
		if (!Menu)
		{
			return;
		}
		if (!UIShotFocusOn(*Menu, EKeys::Up, Menu->GetMainButton(ActionSinglePlayer)))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: the focus didn't reach Single Player."));
		}
		UIShotPress(*Menu, EKeys::Enter);
	}

	bool UIShotPickerShown(UWorld& World)
	{
		const UMainMenuWidget* Menu = UIShotMainMenu(World);
		return Menu && Menu->IsPickerShown();
	}

	// --- The level's steps ---

	/** The pause menu's list as Escape opens it (the game paused under it), the mouse parked once it holds the pointer. */
	void UIShotBeginPause(UWorld& World)
	{
		if (ALooterHUD* Hud = UIShotLevelHud(World))
		{
			Hud->OpenPauseMenu();
		}
		UIShotParkMouse();
	}

	bool UIShotPauseHubOpen(UWorld& World)
	{
		const ALooterHUD* Hud = UIShotLevelHud(World);
		return Hud && Hud->GetPauseScreen() == EPauseScreen::Hub && UIShotPauseHub(World);
	}

	/** Down twice from Resume Game (after the press that brings the focus look up): Missions, the ribbon on it. */
	void UIShotBeginPauseHubRow(UWorld& World)
	{
		if (UPauseMenuWidget* Hub = UIShotPauseHub(World))
		{
			UIShotReveal(*Hub, EKeys::Down);
			UIShotPress(*Hub, EKeys::Down);
			UIShotPress(*Hub, EKeys::Down);
		}
	}

	/** The focus walked down the list to Settings and Enter: the settings over the game, on Graphics. */
	void UIShotBeginPauseSettings(UWorld& World)
	{
		if (UPauseMenuWidget* Hub = UIShotPauseHub(World))
		{
			if (!UIShotFocusOn(*Hub, EKeys::Down, Hub->GetItemButton(EPauseMenuItem::Settings)))
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: the focus didn't reach the pause menu's Settings."));
			}
			UIShotPress(*Hub, EKeys::Enter);
		}
		ALooterHUD* Hud = UIShotLevelHud(World);
		if (Hud && Hud->GetPauseScreen() != EPauseScreen::Settings)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: Enter on Settings didn't open them; opened through the HUD."));
			Hud->ChoosePauseItem(EPauseMenuItem::Settings);
		}
		USettingsMenuWidget* Settings = UIShotSettings(World);
		if (Settings && Settings->GetPage() != static_cast<int32>(ESettingsPage::Graphics))
		{
			Settings->SelectPage(static_cast<int32>(ESettingsPage::Graphics));
		}
	}

	bool UIShotPauseOpen(UWorld& World)
	{
		const USettingsMenuWidget* Settings = UIShotSettings(World);
		return Settings && Settings->GetMode() == ESettingsMenuMode::Pause && Settings->GetPage() == static_cast<int32>(ESettingsPage::Graphics);
	}

	/** Down twice from Back (after the press that brings the focus look up): the page's second row, the ribbon on it. */
	void UIShotBeginPauseRow(UWorld& World)
	{
		if (USettingsMenuWidget* Settings = UIShotSettings(World))
		{
			UIShotReveal(*Settings, EKeys::Down);
			UIShotPress(*Settings, EKeys::Down);
			UIShotPress(*Settings, EKeys::Down);
		}
	}

	/** Page Down: the next tab, as the keyboard turns them (RB on a controller). */
	void UIShotBeginNextPage(UWorld& World)
	{
		USettingsMenuWidget* Settings = UIShotSettings(World);
		if (!Settings)
		{
			return;
		}
		const int32 Before = Settings->GetPage();
		UIShotPress(*Settings, EKeys::PageDown);
		if (Settings->GetPage() == Before)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: Page Down didn't turn the tab; turned it as its tab's click does."));
			Settings->SelectPage((Before + 1) % SettingsMenu::NumPages);
		}
	}

	template <ESettingsPage Page>
	bool UIShotOnPage(UWorld& World)
	{
		const USettingsMenuWidget* Settings = UIShotSettings(World);
		return Settings && Settings->GetPage() == static_cast<int32>(Page);
	}

	void UIShotEndPause(UWorld& World)
	{
		if (ALooterHUD* Hud = UIShotLevelHud(World))
		{
			Hud->ClosePauseMenu();
		}
	}

	/**
	 * The level-up popup's show for the next level, played on the HUD's own banner (as the player frame's experience bar
	 * plays it when it reaches a level), so its timing is exact; the player's level itself doesn't change.
	 */
	void UIShotBeginLevelUp(UWorld& World)
	{
		UHudLevelUpBannerWidget* Banner = UIShotFindWidget<UHudLevelUpBannerWidget>(World, /*bInViewport*/ false);
		if (!Banner)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: no level-up popup in the HUD."));
			return;
		}
		const UPlayerProgressionSubsystem* Progression = UIShotProgression(World);
		const int32 Level = Progression ? Progression->GetLevel() : 1;
		Banner->Show(Level + 1);
		LevelUpShownAt = FPlatformTime::Seconds();
	}

	bool UIShotLevelUpHeld(UWorld& World)
	{
		return LevelUpShownAt >= 0.0 && FPlatformTime::Seconds() - LevelUpShownAt >= LevelUpHoldSeconds;
	}

	// The main menu arrives on its first frame (no startup cards with -NoLogos); its first picture waits for the arrival to
	// end. The settings open in about 0.25 s, the picker's page fades in, the lit button's gold eases on.
	const FStep UIShotMenuSteps[] =
	{
		// Name                     Seconds  Begin                         Ready                     After      Gap
		{ TEXT("menu"),                0.5f, nullptr,                      &UIShotMenuRevealed,      nullptr,   DefaultGapSeconds },
		{ TEXT("menu_lit_settings"),   1.0f, &UIShotBeginLightSettings,    &UIShotSettingsLit,       nullptr,   DefaultGapSeconds },
		{ TEXT("menu_settings"),       1.0f, &UIShotBeginMenuSettings,     &UIShotMenuSettingsOpen,  nullptr,   DefaultGapSeconds },
		{ TEXT("menu_sessions"),       1.2f, &UIShotBeginSessions,         &UIShotPickerShown,       nullptr,   LastGapSeconds },
	};

	// The pause menu's list as Escape opens it (the halo spinning up, the plate dropping in: about 0.25 s) and its ribbon on
	// a row, then its Settings and each tab's ribbon sliding to its row. The level-up's second picture waits for its moment
	// by the show's own clock (the first one's gap is short).
	const FStep UIShotLevelSteps[] =
	{
		// Name                     Seconds  Begin                         Ready                                         After              Gap
		{ TEXT("pause_hub"),           1.0f, &UIShotBeginPause,            &UIShotPauseHubOpen,                          nullptr,           DefaultGapSeconds },
		{ TEXT("pause_hub_row"),       0.8f, &UIShotBeginPauseHubRow,      nullptr,                                      nullptr,           DefaultGapSeconds },
		{ TEXT("pause_graphics"),      1.0f, &UIShotBeginPauseSettings,    &UIShotPauseOpen,                             nullptr,           DefaultGapSeconds },
		{ TEXT("pause_row"),           0.8f, &UIShotBeginPauseRow,         nullptr,                                      nullptr,           DefaultGapSeconds },
		{ TEXT("pause_audio"),         0.9f, &UIShotBeginNextPage,         &UIShotOnPage<ESettingsPage::Audio>,          nullptr,           DefaultGapSeconds },
		{ TEXT("pause_interface"),     0.9f, &UIShotBeginNextPage,         &UIShotOnPage<ESettingsPage::Interface>,      nullptr,           DefaultGapSeconds },
		{ TEXT("pause_controls"),      0.9f, &UIShotBeginNextPage,         &UIShotOnPage<ESettingsPage::Controls>,       &UIShotEndPause,   HudBackSeconds },
		{ TEXT("levelup_count"),       LevelUpCountSeconds, &UIShotBeginLevelUp, nullptr,                                nullptr,           0.3f },
		{ TEXT("levelup_hold"),        0.f,  nullptr,                      &UIShotLevelUpHeld,                           nullptr,           LastGapSeconds },
	};
}

namespace UIShotScene
{
	UWorld* FindGameWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.World() && Context.World()->IsGameWorld())
				{
					return Context.World();
				}
			}
		}
		return nullptr;
	}

	EHalf FindHalf(UWorld& World)
	{
		const APlayerController* Controller = World.GetFirstPlayerController();
		if (!Controller)
		{
			return EHalf::None;
		}
		if (Cast<AMainMenuHUD>(Controller->GetHUD()))
		{
			return UIShotMainMenu(World) ? EHalf::Menu : EHalf::None;
		}
		const ALooterHUD* Hud = UIShotLevelHud(World);
		const bool bReady = Hud && Hud->GetHUDWidget() && Controller->GetPawn() && UIShotProgression(World);
		return bReady ? EHalf::Level : EHalf::None;
	}

	TConstArrayView<FStep> Steps(EHalf Half)
	{
		switch (Half)
		{
		case EHalf::Menu: return MakeArrayView(UIShotMenuSteps);
		case EHalf::Level: return MakeArrayView(UIShotLevelSteps);
		default: return TConstArrayView<FStep>();
		}
	}

	int32 FirstNumber(EHalf Half)
	{
		return Half == EHalf::Level ? static_cast<int32>(UE_ARRAY_COUNT(UIShotMenuSteps)) + 1 : 1;
	}

	float WarmupSeconds(EHalf Half)
	{
		return Half == EHalf::Level ? LevelWarmupSeconds : MenuWarmupSeconds;
	}

	void Prepare(UWorld& World, EHalf Half)
	{
		LevelUpShownAt = -1.0;
		if (Half == EHalf::Menu)
		{
			// The pointer shown over the menu, parked where it lights nothing, long before the first key.
			UIShotParkMouse();
			if (AMainMenuHUD* Hud = UIShotMenuHud(World); Hud && Hud->IsSettingsOpen())
			{
				Hud->CloseSettings();
			}
			return;
		}
		// In play the pointer is the camera's: it's parked once the pause menu shows it (UIShotBeginPause).
		if (ALooterHUD* Hud = UIShotLevelHud(World))
		{
			Hud->CloseInventory();
			Hud->ClosePauseMenu();
		}
	}

	const TCHAR* HalfName(EHalf Half)
	{
		switch (Half)
		{
		case EHalf::Menu: return TEXT("menu");
		case EHalf::Level: return TEXT("level");
		default: return TEXT("none");
		}
	}
}

#endif
