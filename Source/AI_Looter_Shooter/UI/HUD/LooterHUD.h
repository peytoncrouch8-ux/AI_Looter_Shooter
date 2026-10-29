#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LooterHUD.generated.h"

class ANoticeBoard;
class IGameLayerManager;
class ULocalPlayer;
class UBenchWidget;
class UBestiaryWidget;
class UEmbersWidget;
class UHudCaptionWidget;
class UHudControlHintWidget;
class UHudMissionTrackerWidget;
class ULoadoutWidget;
class UMapWidget;
class UMissionDefinition;
class UMissionOfferWidget;
class UMissionsWidget;
class UNoticeBoardWidget;
class AVendor;
class UVendorWidget;
class UPlayerHUDWidget;
class USettingsMenuWidget;
class UStationBoardWidget;
class UUserWidget;
struct FFindBannerShow;
struct FStationBoardWords;

/** The inventory's pages, in the order of their tabs. */
enum class EInventoryPage : uint8
{
	Loadout,
	/** The hero's ember powers, one to equip (UEmbersWidget; the user's call, 2026-10-15: the second tab). */
	Embers,
	Bestiary,
	Missions,
	/** The whole level from above, its pins, and fast travel between open respawn graves (UMapWidget). */
	Map,
};

/**
 * Owns the player HUD, the mission tracker, the captions, inventory screen (over the paused game), the station board (a
 * jetty's or a station's, opened by holding Interact at it), the gunsmith's bench's screen (opened with Interact at a bench)
 * and pause menu (the settings menu over the paused game), plus the always-on menu hotkeys. Set as the HUD Class on the game
 * mode. The gameplay HUD hides while a menu is open (the mission tracker and the captions step aside by themselves). Its
 * pages sound as they open and close (LooterSoundCue::Open, Close; Tab between the inventory's pages).
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

	/**
	 * Whether the inventory showed this page during its last (or current) visit: kept after it closes, since the game is
	 * paused while it's open and mission steps only look once it's shut (UMissionOpenPageObjective).
	 */
	bool WasPageShownLastVisit(EInventoryPage Page) const { return PagesShownLastVisit.Contains(Page); }

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OpenPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ClosePauseMenu();

	bool IsInventoryOpen() const { return bInventoryOpen; }

	/**
	 * The game stands still under the pause menu and under the inventory's pages, the map's too (the user's call, 2026-10-27:
	 * the inventory pauses like the pause menu); the world's screens (a vendor, the bench, a board, the offer card) leave it
	 * running. LooterHUDPause.cpp.
	 */
	static bool PausesGame(bool bPauseMenuIsOpen, bool bInventoryIsOpen);

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

	/**
	 * Opens a notice board's screen (UNoticeBoardWidget: its postings, to track or turn in) for Board, the mouse on it
	 * (LooterHUDNoticeBoard.cpp). False when it can't (no player, the pause menu up).
	 */
	bool OpenNoticeBoard(ANoticeBoard* Board);

	void CloseNoticeBoard();

	bool IsNoticeBoardOpen() const { return bNoticeBoardOpen; }

	/**
	 * Opens a vendor's screen (UVendorWidget: buy ammo, grave salt and the gun of the day, sell guns) for Vendor, the mouse
	 * on it (LooterHUDVendor.cpp). False when it can't (no player, the pause menu up).
	 */
	bool OpenVendor(AVendor* Vendor);

	void CloseVendor();

	bool IsVendorOpen() const { return bVendorOpen; }

	/**
	 * Opens the mission offer card (UMissionOfferWidget) for Mission, offered by Giver (someone at a speaker point, a notice
	 * board, a mission artifact): its story, objectives and rewards with Accept and Not now. False when another menu is open
	 * or the mission can't be offered now. LooterHUDMissionOffer.cpp.
	 */
	bool OpenMissionOffer(const UMissionDefinition* Mission, AActor* Giver);

	void CloseMissionOffer();

	bool IsMissionOfferOpen() const { return bMissionOfferOpen; }

	/**
	 * A menu covers the game for the captions: IsMenuOpen, except the offer card in its offer mode alone, whose giver's
	 * words go on beside it, the captions lifted over its dimming (the user's call, 2026-10-22). LooterHUDMissionOffer.cpp.
	 */
	bool CoversCaptions() const;

	/**
	 * Opens the card in its choice mode for a mission turned in with a choice of two rewards (Back Pay: pay the eleven, or
	 * keep the payroll), at Giver. False when another menu is open or the mission has no choice waiting.
	 * LooterHUDMissionOffer.cpp; bMissionOfferOpen covers it.
	 */
	bool OpenTurnInChoice(const UMissionDefinition* Mission, AActor* Giver);

	/** The short banner over the HUD when a respawn grave is marked as the save point ("GRAVE MARKED", its name). LooterHUDGraves.cpp. */
	void ShowGraveMarked(const FText& GraveName);

	/** The "FOUND" banner over the HUD for a find off the path (UFindSubsystem; FINDS.md). It queues behind banners showing. LooterHUDGraves.cpp. */
	void ShowFind(const FFindBannerShow& Find);

	/** The inventory, the station board, the bench's screen, a notice board, a vendor or the pause menu is up (the gameplay HUD is hidden). */
	bool IsMenuOpen() const;

	/** The gameplay HUD (null before BeginPlay, and for a controller that isn't local): the mission tracker keeps clear of its loot card. */
	UPlayerHUDWidget* GetHUDWidget() const { return HUDWidget; }

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

	/**
	 * Shows or hides Player's labels drawn over the world (creature tags, loot labels, damage numbers, barks): the engine's
	 * layer of screen-space widget components in Layers. That layer belongs to the game's viewport, not to a level, so it
	 * outlives every HUD; it's set from what it shows now, never from what a HUD last asked of it. False while the layer
	 * doesn't exist yet (it's made as the first of them is shown).
	 */
	static bool ShowWorldLabels(IGameLayerManager& Layers, ULocalPlayer* Player, bool bVisible);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BindMenuInput();
	void HandlePausePressed();
	void HandleInventoryPressed();
	/** The Map key (M, down on the D-pad): the inventory opens on its map page. */
	void HandleMapPressed();

	/** Gives input back to the game after closing a menu. */
	void RestoreGameInput();

	/**
	 * Pauses the game or lets it run as the open menus want (PausesGame), after one opens or closes. The HUD doesn't tick
	 * while the game is paused, so pausing puts the gameplay HUD and the world's labels away at once (Tick brings them back
	 * after). LooterHUDPause.cpp.
	 */
	void ApplyGamePause();

	/** A page opening or closing sounds (closing stays quiet while another page takes its place: bQuietClose). */
	void PlayPageSound(bool bOpening) const;

	/** Set while one page closes to make way for another, which sounds for both. */
	bool bQuietClose = false;

	/** Shows or hides the labels drawn over the world (creature tags, loot labels, damage numbers), which sit over menus. */
	void SetWorldLabelsVisible(bool bVisible);

	/** Puts the current inventory page on screen with the keyboard on it. False when it can't open (no player). */
	bool OpenInventoryPage();
	UUserWidget* GetInventoryPageWidget() const;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerHUDWidget> HUDWidget;

	/** What's said aloud, low on the screen over the HUD. */
	UPROPERTY(Transient)
	TObjectPtr<UHudCaptionWidget> CaptionWidget;

	/**
	 * The tracked mission under the minimap: a viewport widget of its own over the inventory's pages, so it can stay up on a
	 * step done in the inventory (it steps aside under every other menu by itself).
	 */
	UPROPERTY(Transient)
	TObjectPtr<UHudMissionTrackerWidget> MissionTracker;

	/** The inventory's first page (the loadout: your guns and the backpack as one list, the chosen gun on show and its card). */
	UPROPERTY(Transient)
	TObjectPtr<ULoadoutWidget> InventoryWidget;

	/** The inventory's second page: the hero's three ember powers, the equipped one marked. */
	UPROPERTY(Transient)
	TObjectPtr<UEmbersWidget> EmbersWidget;

	/** The inventory's third page: every creature, enemy, NPC and friend (the Bestiary, the Ledger once given). */
	UPROPERTY(Transient)
	TObjectPtr<UBestiaryWidget> BestiaryWidget;

	/** The inventory's fourth page: the missions, active, available and finished. */
	UPROPERTY(Transient)
	TObjectPtr<UMissionsWidget> MissionsWidget;

	/** The inventory's fifth page: the map, and fast travel between graves. */
	UPROPERTY(Transient)
	TObjectPtr<UMapWidget> MapWidget;

	EInventoryPage InventoryPage = EInventoryPage::Loadout;

	/** The pages the last (or current) inventory visit showed (WasPageShownLastVisit). */
	TSet<EInventoryPage> PagesShownLastVisit;

	/** The pause menu: the settings menu, over the paused game. */
	UPROPERTY(Transient)
	TObjectPtr<USettingsMenuWidget> PauseMenuWidget;

	/** The station board, made the first time one is opened. */
	UPROPERTY(Transient)
	TObjectPtr<UStationBoardWidget> StationBoardWidget;

	/** The gunsmith's bench's screen, made the first time a bench is used. */
	UPROPERTY(Transient)
	TObjectPtr<UBenchWidget> BenchWidget;

	/** A notice board's screen, made the first time a board is read. */
	UPROPERTY(Transient)
	TObjectPtr<UNoticeBoardWidget> NoticeBoardWidget;

	/** A vendor's screen, made the first time one is used. */
	UPROPERTY(Transient)
	TObjectPtr<UVendorWidget> VendorWidget;

	/** The contextual control hint, a viewport widget of its own over the HUD (it steps aside under menus by itself). */
	UPROPERTY(Transient)
	TObjectPtr<UHudControlHintWidget> ControlHintWidget;

	bool bInventoryOpen = false;
	bool bPauseMenuOpen = false;
	bool bStationBoardOpen = false;
	bool bBenchOpen = false;
	bool bNoticeBoardOpen = false;
	bool bVendorOpen = false;

	/** The mission offer card, made the first time a mission is offered. */
	UPROPERTY(Transient)
	TObjectPtr<UMissionOfferWidget> MissionOfferWidget;

	bool bMissionOfferOpen = false;

	/**
	 * What the open world screen belongs to (the station board's jetty or station, the bench, the notice board, the vendor,
	 * the offer card's giver): a player beyond ScreenStayRadius of it has walked (or been carried) away, and the screen
	 * closes, as the bounty board's does (the 2026-10-23 play-check: the Rangers' board stayed open after a teleport). Null:
	 * opened from afar (the console, a test), never closed by distance. LooterHUDReach.cpp.
	 */
	TWeakObjectPtr<AActor> ScreenAnchor;

	static constexpr float ScreenStayRadius = 450.f;

	/** The player stands within ScreenStayRadius of where Anchor is used (false without either). */
	bool IsWithinScreenReach(const AActor* Anchor) const;

	/** Closes the world screens when the player is beyond ScreenStayRadius of their ScreenAnchor (each tick). */
	void CloseScreensOutOfReach();

	/** The captions sit over the offer card while it's open in its offer mode (LiftCaptions), back over the HUD after. */
	bool bCaptionsOverCard = false;

	/** Puts the caption widget over the offer card (bOver) or back in its place over the gameplay HUD. LooterHUDMissionOffer.cpp. */
	void LiftCaptions(bool bOver);
};
