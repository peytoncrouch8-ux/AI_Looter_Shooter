#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LooterHUD.generated.h"

class UBenchWidget;
class UBestiaryWidget;
class UHudCaptionWidget;
class UHudMissionTrackerWidget;
class ULoadoutWidget;
class UMissionsWidget;
class UPlayerHUDWidget;
class USettingsMenuWidget;
class UStationBoardWidget;
class UUserWidget;
struct FStationBoardWords;

/** The inventory's pages, in the order of their tabs. */
enum class EInventoryPage : uint8
{
	Loadout,
	Bestiary,
	Missions,
};

/**
 * Owns the player HUD, the mission tracker, the captions, inventory screen, the station board (a jetty's or a station's,
 * opened by holding Interact at it), the gunsmith's bench's screen (opened with Interact at a bench) and pause menu (the
 * settings menu over the paused game), plus the always-on menu hotkeys. Set as the HUD Class on the game mode. The gameplay
 * HUD hides while a menu is open (the mission tracker and the captions step aside by themselves). Its pages sound as they
 * open and close (LooterSoundCue::Open, Close; Tab between the inventory's pages).
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

	/**
	 * Opens the station board of From (a jetty, a station; null: the level's own) with its words, the mouse on it. False
	 * when it can't (no player, the pause menu up).
	 */
	bool OpenStationBoard(AActor* From, const FStationBoardWords& Words);

	void CloseStationBoard();

	bool IsStationBoardOpen() const { return bStationBoardOpen; }

	/**
	 * Opens the gunsmith's bench's screen (UBenchWidget) for the guns the player carries, the mouse on it; Bench is the bench
	 * used (null: the console's). False when it can't (no player with guns to carry, the pause menu up).
	 */
	bool OpenBench(AActor* Bench);

	void CloseBench();

	bool IsBenchOpen() const { return bBenchOpen; }

	/** The inventory, the station board, the bench's screen or the pause menu is up (the gameplay HUD is hidden). */
	bool IsMenuOpen() const { return bInventoryOpen || bPauseMenuOpen || bStationBoardOpen || bBenchOpen; }

	/** The HUD of the local player behind Player (their pawn or controller), or null. */
	static ALooterHUD* FindFor(const AActor* Player);

	/** Saves the session being played and goes back to the main menu (the pause menu's Save & Quit). */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SaveAndQuit();

	/**
	 * A mission's closing line (the tutorial's "You're ready..."): the mission tracker shows it as a last, ticked objective
	 * under Title, every one of its StepCount steps done, for Seconds of play.
	 */
	void ShowMissionClosingLine(const FText& Title, int32 StepCount, const FString& Line, float Seconds);

	/** Takes the closing line away, shown or waiting (the tutorial restarted or skipped). */
	void ClearMissionClosingLine();

protected:
	virtual void BeginPlay() override;

private:
	void BindMenuInput();
	void HandlePausePressed();
	void HandleInventoryPressed();

	/** Gives input back to the game after closing a menu. */
	void RestoreGameInput();

	/** A page opening or closing sounds (closing stays quiet while another page takes its place: bQuietClose). */
	void PlayPageSound(bool bOpening) const;

	/** Set while one page closes to make way for another, which sounds for both. */
	bool bQuietClose = false;

	/** Shows or hides the labels drawn over the world (creature tags, loot labels, damage numbers), which sit over menus. */
	void SetWorldLabelsVisible(bool bVisible);
	bool bWorldLabelsVisible = true;

	/** Puts the current inventory page on screen with the keyboard on it. False when it can't open (no player). */
	bool OpenInventoryPage();
	UUserWidget* GetInventoryPageWidget() const;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerHUDWidget> HUDWidget;

	/** What's said aloud, low on the screen over the HUD. */
	UPROPERTY(Transient)
	TObjectPtr<UHudCaptionWidget> CaptionWidget;

	/**
	 * The tracked mission on the left: a viewport widget of its own over the inventory's pages, so it can stay up on a step
	 * done in the inventory (it steps aside under every other menu by itself).
	 */
	UPROPERTY(Transient)
	TObjectPtr<UHudMissionTrackerWidget> MissionTracker;

	/** The inventory's first page (the loadout: your character with your guns, and the backpack). */
	UPROPERTY(Transient)
	TObjectPtr<ULoadoutWidget> InventoryWidget;

	/** The inventory's second page: every creature, enemy, NPC and friend. */
	UPROPERTY(Transient)
	TObjectPtr<UBestiaryWidget> BestiaryWidget;

	/** The inventory's third page: the missions, active, available and finished. */
	UPROPERTY(Transient)
	TObjectPtr<UMissionsWidget> MissionsWidget;

	EInventoryPage InventoryPage = EInventoryPage::Loadout;

	/** The pause menu: the settings menu, over the paused game. */
	UPROPERTY(Transient)
	TObjectPtr<USettingsMenuWidget> PauseMenuWidget;

	/** The station board, made the first time one is opened. */
	UPROPERTY(Transient)
	TObjectPtr<UStationBoardWidget> StationBoardWidget;

	/** The gunsmith's bench's screen, made the first time a bench is used. */
	UPROPERTY(Transient)
	TObjectPtr<UBenchWidget> BenchWidget;

	bool bInventoryOpen = false;
	bool bPauseMenuOpen = false;
	bool bStationBoardOpen = false;
	bool bBenchOpen = false;
};
