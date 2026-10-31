#include "UI/Menus/MainMenuHUD.h"
#include "UI/Menus/MainMenuWidget.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/StartupLogosWidget.h"
#include "AI_Looter_Shooter.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The settings menu sits over the main menu (the same layer it takes over the game); the startup cards over both. */
	constexpr int32 MenuZOrder = 10;
	constexpr int32 SettingsZOrder = 40;
	constexpr int32 LogosZOrder = 100;

	void ReplayStartupLogos(UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AMainMenuHUD* HUD = PC ? Cast<AMainMenuHUD>(PC->GetHUD()) : nullptr;
		if (!HUD)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Startup.Replay: only on the main menu."));
			return;
		}
		if (!HUD->PlayStartupLogos())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Startup.Replay: the cards are showing already, or the settings are open."));
		}
	}

	FAutoConsoleCommandWithWorld CmdReplayLogos(TEXT("Looter.Startup.Replay"),
		TEXT("Plays the startup cards (studio, Unreal Engine) again over the main menu, skippable, for a look at them."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ReplayStartupLogos));
}

void AMainMenuHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	MenuWidget = CreateWidget<UMainMenuWidget>(PC, UMainMenuWidget::StaticClass());
	if (MenuWidget)
	{
		MenuWidget->AddToViewport(MenuZOrder);
	}
	// Made now and kept, so opening it is instant; it's only on screen while open.
	SettingsWidget = CreateWidget<USettingsMenuWidget>(PC, USettingsMenuWidget::StaticClass());
	if (SettingsWidget)
	{
		SettingsWidget->OnClose.BindUObject(this, &AMainMenuHUD::CloseSettings);
	}

	// As the game starts (once a run): the startup cards first, the menu arriving as they go.
	if (MenuWidget && UStartupLogosWidget::ShouldPlay(GetWorld()))
	{
		ShowStartupLogos(false);
		return;
	}
	FocusWidget(MenuWidget.Get());
}

bool AMainMenuHUD::PlayStartupLogos()
{
	if (LogosWidget || bSettingsOpen || !MenuWidget)
	{
		return false;
	}
	ShowStartupLogos(true);
	return LogosWidget != nullptr;
}

void AMainMenuHUD::ShowStartupLogos(bool bForceSkippable)
{
	APlayerController* PC = GetOwningPlayerController();
	LogosWidget = PC ? CreateWidget<UStartupLogosWidget>(PC, UStartupLogosWidget::StaticClass()) : nullptr;
	if (!LogosWidget)
	{
		FocusWidget(MenuWidget.Get());
		return;
	}
	// The menu waits unseen under the cards; it arrives when they're done.
	MenuWidget->HoldReveal();
	LogosWidget->OnRevealing.BindUObject(this, &AMainMenuHUD::HandleLogosRevealing);
	LogosWidget->OnFinished.BindUObject(this, &AMainMenuHUD::HandleLogosFinished);
	LogosWidget->AddToViewport(LogosZOrder);
	FocusWidget(LogosWidget.Get());
	// No pointer over the cards (it's back with the menu).
	PC->SetShowMouseCursor(false);
	LogosWidget->Begin(bForceSkippable);
}

void AMainMenuHUD::HandleLogosRevealing()
{
	FocusWidget(MenuWidget.Get());
	if (MenuWidget)
	{
		MenuWidget->BeginReveal();
	}
}

void AMainMenuHUD::HandleLogosFinished()
{
	if (LogosWidget)
	{
		LogosWidget->RemoveFromParent();
		LogosWidget = nullptr;
	}
}

void AMainMenuHUD::OpenSettings()
{
	if (!SettingsWidget || bSettingsOpen)
	{
		return;
	}
	SettingsWidget->Open(ESettingsMenuMode::MainMenu);
	SettingsWidget->AddToViewport(SettingsZOrder);
	// The settings are a full page: the menu's column and lockup step out from under it (the planet stays).
	if (MenuWidget)
	{
		MenuWidget->SetRenderOpacity(0.f);
	}
	bSettingsOpen = true;
	FocusWidget(SettingsWidget.Get());
}

void AMainMenuHUD::CloseSettings()
{
	if (!bSettingsOpen)
	{
		return;
	}
	if (SettingsWidget)
	{
		SettingsWidget->RemoveFromParent();
	}
	if (MenuWidget)
	{
		MenuWidget->SetRenderOpacity(1.f);
	}
	bSettingsOpen = false;
	FocusWidget(MenuWidget.Get());
}

void AMainMenuHUD::FocusWidget(UUserWidget* Widget)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !Widget)
	{
		return;
	}
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Widget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
}
