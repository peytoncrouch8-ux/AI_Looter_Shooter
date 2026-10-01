#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainMenuHUD.generated.h"

class UMainMenuWidget;
class USettingsMenuWidget;
class UUserWidget;

/**
 * The main menu's HUD (the menu game mode's HUD class): owns the main menu and the settings menu it opens, and gives the
 * keyboard to whichever is on top. Nothing is played under the menu, so input goes to the UI alone, with the mouse free.
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

protected:
	virtual void BeginPlay() override;

private:
	/** UI-only input with Widget holding the keyboard, and the mouse shown and free to leave the window. */
	void FocusWidget(UUserWidget* Widget);

	UPROPERTY(Transient)
	TObjectPtr<UMainMenuWidget> MenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<USettingsMenuWidget> SettingsWidget;

	bool bSettingsOpen = false;
};
