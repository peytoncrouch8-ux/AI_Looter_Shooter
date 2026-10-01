#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LooterHUD.generated.h"

class ULoadoutWidget;
class UPauseMenuWidget;
class UPlayerHUDWidget;

/**
 * Owns the player HUD, inventory screen and pause/settings menu, plus the always-on menu hotkeys.
 * Set as the HUD Class on the game mode. The gameplay HUD hides while a menu is open.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALooterHUD : public AHUD
{
	GENERATED_BODY()

public:
	ALooterHUD();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OpenInventory();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void CloseInventory();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OpenPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ClosePauseMenu();

	bool IsInventoryOpen() const { return bInventoryOpen; }

	/** The inventory or the pause menu is up (the gameplay HUD is hidden). */
	bool IsMenuOpen() const { return bInventoryOpen || bPauseMenuOpen; }

	/** Ends the play session in the editor, or exits the app in a packaged build. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void QuitGame();

protected:
	virtual void BeginPlay() override;

private:
	void BindMenuInput();
	void HandlePausePressed();
	void HandleInventoryPressed();

	/** Gives input back to the game after closing a menu. */
	void RestoreGameInput();

	UPROPERTY(Transient)
	TObjectPtr<UPlayerHUDWidget> HUDWidget;

	/** The inventory screen (the loadout: your character with your guns, and the backpack). */
	UPROPERTY(Transient)
	TObjectPtr<ULoadoutWidget> InventoryWidget;

	UPROPERTY(Transient)
	TObjectPtr<UPauseMenuWidget> PauseMenuWidget;

	bool bInventoryOpen = false;
	bool bPauseMenuOpen = false;
};
