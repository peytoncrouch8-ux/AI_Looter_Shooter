#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainMenuHUD.generated.h"

class UMainMenuWidget;
class USettingsMenuWidget;
class UStartupLogosWidget;
class UUserWidget;

/**
 * The main menu's HUD (the menu game mode's HUD class): owns the main menu and the settings menu it opens, and gives the
 * keyboard to whichever is on top. Nothing is played under the menu, so input goes to the UI alone, with the mouse free.
 * As the game starts, the startup cards (UStartupLogosWidget) play over the menu first, holding the keys; the menu
 * arrives as they fade (UMainMenuWidget::BeginReveal). Without them the menu arrives at once.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AMainMenuHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** Opens the settings menu over the main menu; its Back, its corner X and Escape close it again (CloseSettings). */
	void OpenSettings();

	/** Closes the settings menu and gives the keyboard back to the main menu. */
	void CloseSettings();

	bool IsSettingsOpen() const { return bSettingsOpen; }

	/** Plays the startup cards over the menu now (Looter.Startup.Replay), skippable. False if they're already showing. */
	bool PlayStartupLogos();

	bool IsShowingStartupLogos() const { return LogosWidget != nullptr; }

protected:
	virtual void BeginPlay() override;

private:
	/** UI-only input with Widget holding the keyboard, and the mouse shown and free to leave the window. */
	void FocusWidget(UUserWidget* Widget);

	/** Puts the startup cards over the menu, holding its arrival and the keys. */
	void ShowStartupLogos(bool bForceSkippable);
	/** The cards are done: the menu takes the keys and arrives as the dark fades. */
	void HandleLogosRevealing();
	void HandleLogosFinished();

	UPROPERTY(Transient)
	TObjectPtr<UMainMenuWidget> MenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<USettingsMenuWidget> SettingsWidget;

	/** The startup cards while they show. */
	UPROPERTY(Transient)
	TObjectPtr<UStartupLogosWidget> LogosWidget;

	bool bSettingsOpen = false;
};
