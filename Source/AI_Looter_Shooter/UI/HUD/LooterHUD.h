#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LooterHUD.generated.h"

class UBestiaryWidget;
class ULoadoutWidget;
class UPauseMenuWidget;
class UPlayerHUDWidget;
class UUserWidget;

/** The inventory's pages, in the order of their tabs. */
enum class EInventoryPage : uint8
{
	Loadout,
	Bestiary,
};

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

	/** Switches the open inventory to another page (its tabs), or opens the inventory on it. */
	void ShowInventoryPage(EInventoryPage Page);

	/** The page the inventory shows, or opens on next (the one last shown). */
	EInventoryPage GetInventoryPage() const { return InventoryPage; }

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

	/** Puts the current inventory page on screen with the keyboard on it. False when it can't open (no player). */
	bool OpenInventoryPage();
	UUserWidget* GetInventoryPageWidget() const;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerHUDWidget> HUDWidget;

	/** The inventory's first page (the loadout: your character with your guns, and the backpack). */
	UPROPERTY(Transient)
	TObjectPtr<ULoadoutWidget> InventoryWidget;

	/** The inventory's second page: every creature, enemy, NPC and friend. */
	UPROPERTY(Transient)
	TObjectPtr<UBestiaryWidget> BestiaryWidget;

	EInventoryPage InventoryPage = EInventoryPage::Loadout;

	UPROPERTY(Transient)
	TObjectPtr<UPauseMenuWidget> PauseMenuWidget;

	bool bInventoryOpen = false;
	bool bPauseMenuOpen = false;
};
