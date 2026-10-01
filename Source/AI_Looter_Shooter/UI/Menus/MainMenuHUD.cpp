#include "UI/Menus/MainMenuHUD.h"
#include "UI/Menus/MainMenuWidget.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The settings menu sits over the main menu (the same layer it takes over the game). */
	constexpr int32 MenuZOrder = 10;
	constexpr int32 SettingsZOrder = 40;
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
	FocusWidget(MenuWidget.Get());
}

void AMainMenuHUD::OpenSettings()
{
	if (!SettingsWidget || bSettingsOpen)
	{
		return;
	}
	SettingsWidget->Open(ESettingsMenuMode::MainMenu);
	SettingsWidget->AddToViewport(SettingsZOrder);
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
