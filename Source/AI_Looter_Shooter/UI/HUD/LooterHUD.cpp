#include "UI/HUD/LooterHUD.h"
#include "Audio/LooterSound.h"
#include "Bestiary/Ledger.h"
#include "UI/Bench/BenchWidget.h"
#include "UI/Bestiary/BestiaryWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Inventory/MissionsWidget.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/HUD/HudCaptionWidget.h"
#include "UI/HUD/HudControlHintWidget.h"
#include "UI/HUD/HudMissionTrackerWidget.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/World/StationBoardWidget.h"
#include "Areas/StationBoard.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "EnhancedInputComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Slate/SGameLayerManager.h"
#include "Templates/UnrealTemplate.h"

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
	// The captions are a widget of their own over the HUD's: they keep ticking while the HUD is collapsed under a menu, so
	// they can hold their lines until it closes.
	CaptionWidget = CreateWidget<UHudCaptionWidget>(PC, UHudCaptionWidget::StaticClass());
	if (CaptionWidget)
	{
		CaptionWidget->AddToViewport(UHudCaptionWidget::ViewportZOrder);
	}
	// The mission tracker is its own widget too, over the inventory's pages: it stays up on a step done in the inventory
	// (the tutorial's last), and follows the tracked mission and the menus by itself.
	MissionTracker = CreateWidget<UHudMissionTrackerWidget>(PC, UHudMissionTrackerWidget::StaticClass());
	if (MissionTracker)
	{
		MissionTracker->AddToViewport(UHudMissionTrackerWidget::ViewportZOrder);
	}
	// The contextual control hint, above the tracker: it follows UControlHintSubsystem and the menus by itself.
	ControlHintWidget = CreateWidget<UHudControlHintWidget>(PC, UHudControlHintWidget::StaticClass());
	if (ControlHintWidget)
	{
		ControlHintWidget->AddToViewport(UHudControlHintWidget::ViewportZOrder);
	}
	InventoryWidget = CreateWidget<ULoadoutWidget>(PC, ULoadoutWidget::StaticClass());
	BestiaryWidget = CreateWidget<UBestiaryWidget>(PC, UBestiaryWidget::StaticClass());
	MissionsWidget = CreateWidget<UMissionsWidget>(PC, UMissionsWidget::StaticClass());
	PauseMenuWidget = CreateWidget<USettingsMenuWidget>(PC, USettingsMenuWidget::StaticClass());
	if (PauseMenuWidget)
	{
		PauseMenuWidget->OnClose.BindUObject(this, &ALooterHUD::ClosePauseMenu);
		PauseMenuWidget->OnSaveAndQuit.BindUObject(this, &ALooterHUD::SaveAndQuit);
	}

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

	// Our own input component on the controller, so menu keys work whatever pawn is possessed (or none, while dead).
	EnableInput(PC);
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		Input->BindAction(Bindings->GetPauseAction(), ETriggerEvent::Started, this, &ALooterHUD::HandlePausePressed);
		Input->BindAction(Bindings->GetInventoryAction(), ETriggerEvent::Started, this, &ALooterHUD::HandleInventoryPressed);
	}
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

	// A scene (the skiff ride, later the cold open) holds the player with the gameplay HUD put away; the captions stay.
	const bool bHideHUD = IsMenuOpen() || USceneSubsystem::HidesGameplayHUD(this);
	HUDWidget->SetVisibility(bHideHUD ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	SetWorldLabelsVisible(!bHideHUD);
}

ALooterHUD* ALooterHUD::FindFor(const AActor* Player)
{
	const APlayerController* Controller = Cast<APlayerController>(Player);
	if (!Controller)
	{
		const APawn* Pawn = Cast<APawn>(Player);
		Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	}
	return Controller && Controller->IsLocalController() ? Cast<ALooterHUD>(Controller->GetHUD()) : nullptr;
}

void ALooterHUD::SetWorldLabelsVisible(bool bVisible)
{
	if (bWorldLabelsVisible == bVisible)
	{
		return;
	}
	// Creature tags, loot labels and damage numbers are widget components drawn in screen space: the engine puts them on
	// one layer of their own, in front of every menu. A menu covers the game, so the layer goes with it. The layer only
	// exists once the first of them has been shown; until then there's nothing to hide (and this tries again).
	APlayerController* PC = GetOwningPlayerController();
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	const UGameViewportClient* Viewport = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr;
	const TSharedPtr<IGameLayerManager> LayerManager = Viewport ? Viewport->GetGameLayerManager() : nullptr;
	const TSharedPtr<IGameLayer> Layer = LayerManager.IsValid() ? LayerManager->FindLayerForPlayer(LocalPlayer, TEXT("WidgetComponentScreenLayer")) : nullptr;
	if (Layer.IsValid())
	{
		Layer->AsWidget()->SetVisibility(bVisible ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
		bWorldLabelsVisible = bVisible;
	}
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
	if (!bPauseMenuOpen && !bInventoryOpen && !bStationBoardOpen && !bBenchOpen && !bNoticeBoardOpen)
	{
		OpenInventory();
	}
}

void ALooterHUD::RestoreGameInput()
{
	if (APlayerController* PC = GetOwningPlayerController())
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
	if (bInventoryOpen)
	{
		return;
	}
	{
		// Never over the bench's screen: it shows the same guns.
		TGuardValue<bool> Quiet(bQuietClose, true);
		CloseBench();
	}
	// It opens on the page it was last closed on.
	bInventoryOpen = OpenInventoryPage();
	if (bInventoryOpen)
	{
		PlayPageSound(true);
	}
}

void ALooterHUD::CloseInventory()
{
	if (!bInventoryOpen)
	{
		return;
	}
	if (UUserWidget* Page = GetInventoryPageWidget())
	{
		Page->RemoveFromParent();
	}
	bInventoryOpen = false;
	RestoreGameInput();
	PlayPageSound(false);
}

void ALooterHUD::ShowInventoryPage(EInventoryPage Page)
{
	if (!bInventoryOpen)
	{
		InventoryPage = Page;
		OpenInventory();
		return;
	}
	if (Page == InventoryPage)
	{
		return;
	}
	if (UUserWidget* Old = GetInventoryPageWidget())
	{
		Old->RemoveFromParent();
	}
	InventoryPage = Page;
	if (!OpenInventoryPage())
	{
		bInventoryOpen = false;
		RestoreGameInput();
		PlayPageSound(false);
		return;
	}
	// Turning to another page of the open inventory: the tab's own sound, not a whole screen's.
	LooterSound::Play2D(this, LooterSoundCue::Tab);
}

bool ALooterHUD::OpenInventoryPage()
{
	APlayerController* PC = GetOwningPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UWeaponManagerComponent* Manager = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	UUserWidget* Page = GetInventoryPageWidget();
	if (!Page || !Manager)
	{
		return false;
	}

	Manager->StopFire();
	switch (InventoryPage)
	{
	case EInventoryPage::Bestiary:
		BestiaryWidget->Open(this);
		break;
	case EInventoryPage::Missions:
		MissionsWidget->Open(this);
		break;
	case EInventoryPage::Loadout:
		InventoryWidget->Open(this, Manager);
		break;
	}
	Page->AddToViewport(20);
	// Its tabs stand made (a page builds them as it's first shown): the bestiary's reads "Ledger" once Sexton has handed it over.
	if (Page->WidgetTree)
	{
		LoadoutParts::TitlePageTabs(*Page->WidgetTree, Ledger::IsOpenIn(this));
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Page->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
	return true;
}

UUserWidget* ALooterHUD::GetInventoryPageWidget() const
{
	switch (InventoryPage)
	{
	case EInventoryPage::Bestiary: return BestiaryWidget.Get();
	case EInventoryPage::Missions: return MissionsWidget.Get();
	case EInventoryPage::Loadout:  break;
	}
	return InventoryWidget.Get();
}

// ---------------------------------------------------------------------------
// The station board
// ---------------------------------------------------------------------------

bool ALooterHUD::OpenStationBoard(AActor* From, const FStationBoardWords& Words)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController() || bPauseMenuOpen)
	{
		return false;
	}
	{
		TGuardValue<bool> Quiet(bQuietClose, true);
		CloseInventory();
		CloseBench();
	}
	if (!StationBoardWidget)
	{
		StationBoardWidget = CreateWidget<UStationBoardWidget>(PC, UStationBoardWidget::StaticClass());
	}
	if (!StationBoardWidget)
	{
		return false;
	}
	if (const APawn* Pawn = PC->GetPawn())
	{
		if (UWeaponManagerComponent* Manager = Pawn->FindComponentByClass<UWeaponManagerComponent>())
		{
			Manager->StopFire();
		}
	}

	// The missions hear it read before its lines are: a mission it finishes (Main 7) opens what it opens, and the board
	// shows it.
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(StationBoard::ReadEvent(), From));
	}
	StationBoardWidget->Open(this, From, Words);
	if (!bStationBoardOpen)
	{
		StationBoardWidget->AddToViewport(25);
		PlayPageSound(true);
	}
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(StationBoardWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
	bStationBoardOpen = true;
	return true;
}

void ALooterHUD::CloseStationBoard()
{
	if (!bStationBoardOpen)
	{
		return;
	}
	if (StationBoardWidget)
	{
		StationBoardWidget->RemoveFromParent();
	}
	bStationBoardOpen = false;
	RestoreGameInput();
	PlayPageSound(false);
}

// The gunsmith's bench's screen: LooterHUDBench.cpp.

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

	{
		TGuardValue<bool> Quiet(bQuietClose, true);
		CloseInventory();
		CloseStationBoard();
		CloseBench();
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
	// Before the pause, which holds the game's sounds.
	PlayPageSound(true);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);

	UGameplayStatics::SetGamePaused(this, true);
	bPauseMenuOpen = true;
	// The HUD doesn't tick while the game is paused, so the world's labels go now (Tick brings them back after).
	SetWorldLabelsVisible(false);
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

// ---------------------------------------------------------------------------
// The mission tracker
// ---------------------------------------------------------------------------

void ALooterHUD::ShowMissionClosingLine(const FText& Title, int32 StepCount, const FString& Line, float Seconds)
{
	if (MissionTracker)
	{
		MissionTracker->ShowClosingLine(Title, StepCount, Line, Seconds);
	}
}

void ALooterHUD::ClearMissionClosingLine()
{
	if (MissionTracker)
	{
		MissionTracker->ClearClosingLine();
	}
}
