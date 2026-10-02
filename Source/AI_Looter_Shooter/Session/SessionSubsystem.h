#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Misc/DateTime.h"
#include "Session/SessionSaveGate.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SessionSubsystem.generated.h"

class UAreaDefinition;
class ULooterSessionSave;
class UWorld;
struct FCampaignRecord;
struct FPlayerProgressData;

/** What one session slot holds, for the main menu's session picker. */
struct FSessionSummary
{
	/** The slot, 0 to USessionSubsystem::MaxSessions - 1 (shown as Session 1 to 3). */
	int32 Index = 0;

	/** Something is saved in it; an empty slot starts a new game. */
	bool bExists = false;

	int32 Level = 1;

	/** Guns carried: the equipped ones and the backpack's. */
	int32 Weapons = 0;

	double PlayedSeconds = 0.0;

	/** When it was last saved, in local time. */
	FDateTime Saved;

	/**
	 * Where the session continues, for people: its area's name ("Skyreach", "Ransom's Rest"), or the level's file name
	 * made readable when no area is played there.
	 */
	FString Place;

	bool bTutorialDone = false;
};

/**
 * The game's three sessions (save slots "Session1" to "Session3"), each a separate playthrough: the player (the level
 * they're in, where they stand, health, level and experience, bestiary, guns and ammo), each map's world and the story
 * so far (ULooterSessionSave).
 *
 * The main menu picks one (PlaySession), which opens its level with ?Session=N in the URL; the game mode then loads it
 * (BeginPlayWorld) and, once every actor has begun play, puts the player and that map's world back as they were
 * (RestorePlayWorld). While a session is played it saves every minute, a few seconds after progress (SaveSoon), on the
 * pause menu's Save & Quit, and whenever its level ends (quitting the game, stopping Play-In-Editor).
 *
 * Travel (TravelToArea, TravelToMap; SessionSubsystemTravel.cpp) saves the session pointing at the destination and
 * opens it with the same ?Session=N. Nothing else saves until the destination begins (FSessionSaveGate), and there the
 * player arrives at the trip's landing (AreaLandings), or else at the level's start.
 *
 * A level played without a session (Play-In-Editor straight on a level) saves nothing: the player starts as a new game.
 * A trip from there carries the player and the worlds to the next level in memory, still writing nothing. The progress
 * saved before sessions existed ("PlayerProgress") becomes session 1 the first time the game starts.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxSessions = 3;

	/** Seconds between autosaves while a session is played. */
	static constexpr float AutosaveInterval = 60.f;

	/** Seconds from a SaveSoon to its write: a fight's worth of kills makes one write. */
	static constexpr float SaveSoonDelay = 5.f;

	static USessionSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- The main menu ---

	/** What slot Index (0 to MaxSessions - 1) holds, for the session picker. Reads its save. */
	FSessionSummary GetSummary(int32 Index) const;

	/**
	 * Plays slot Index: continues where it was saved, or starts a new game when it's empty. Opens its level (on the next
	 * frame); false when it couldn't start.
	 */
	bool PlaySession(int32 Index);

	/**
	 * Deletes everything saved in slot Index (it starts a new game after). False when there was nothing to delete, or it
	 * is the session being played.
	 */
	bool DeleteSession(int32 Index);

	/** Opens the main menu: the game's first level with the menu over it (DefaultEngine.ini's LocalMapOptions). */
	void OpenMainMenu();

	// --- The session being played ---

	/** The slot being played, or INDEX_NONE (the main menu, or a level played straight from the editor). */
	int32 GetActiveSession() const { return ActiveIndex; }
	bool IsPlayingSession() const { return ActiveIndex != INDEX_NONE; }

	/**
	 * Writes the session being played, the player and world as they are now. False when no session is being played, or
	 * a trip is under way (its own save already holds everything).
	 */
	bool SaveNow();

	/** Saves SaveSoonDelay seconds from now (several calls make one write). */
	void SaveSoon();

	/** Saves the session being played and goes back to the main menu (the pause menu's Save & Quit). */
	void SaveAndQuitToMenu();

	/**
	 * Holds autosaves and save-soons for Reason while something plays out (a ride, a fade, a scene), so the session
	 * isn't saved halfway through it. What waited saves once the last hold is released. A level change releases all.
	 */
	void HoldSaves(FName Reason);
	void ReleaseSaves(FName Reason);

	/** The story so far of what's being played: the session's, or kept in memory without one. Null on the main menu. */
	FCampaignRecord* GetCampaign();

	// --- Travel (SessionSubsystemTravel.cpp) ---

	/**
	 * Travels to an area's level, arriving at Landing (None: the area's first landing). False, with the reason logged,
	 * when nobody can go: its level isn't in the game yet, a trip is under way, or no level is being played.
	 */
	bool TravelToArea(const UAreaDefinition& Area, FName Landing = NAME_None);

	/**
	 * Travels to a level (its package name). The world being left is kept under its own map; the session then points at
	 * the destination, with no player spot there and the landing to arrive at, and is saved; then the destination opens
	 * with ?Session=N. Without a session nothing is written, but the player and the worlds come along in memory. False,
	 * with the reason logged, when it can't go.
	 */
	bool TravelToMap(const FString& MapPackage, FName Landing = NAME_None);

	/** A trip's save is written and its destination is opening. */
	bool IsTravelling() const { return SaveGate.IsTravelling(); }

	/** The landing the player arrives at in the level starting now (None: its start); the game mode spawns them there. */
	FName GetArrivalLanding() const { return ArrivalLanding; }

	// --- Promotions (SessionSubsystemPromotions.cpp) ---

	/** Time played between two promotion rolls on one map (20 minutes). */
	static constexpr double PromotionCooldown = 20.0 * 60.0;

	/**
	 * Whether World's placed creatures may be promoted as it starts (UAreaRulesSubsystem asks once per level start). In a
	 * session a map rolls at most once per PromotionCooldown of time played, saved with its world (PromotionsRolledAt),
	 * so Save & Quit then Continue doesn't reroll; the roll is noted once the level has begun. Without a session every
	 * start rolls (noted in memory when a trip carries the worlds along).
	 */
	bool ClaimPromotionRoll(UWorld* World);

	/** Whether a map whose promotions last rolled at RolledAt (time played; negative: never) rolls again at PlayedSeconds. */
	static bool IsPromotionRollDue(double RolledAt, double PlayedSeconds);

	// --- For the game modes ---

	/**
	 * A play level is starting (its game mode's InitGame): loads the session its URL names (?Session=1 to 3) and gives
	 * the player its progress; with no session, a new game's progress and nothing is saved.
	 */
	void BeginPlayWorld(UWorld* World, const FString& Options);

	/**
	 * Every actor in the play level has begun play: puts the player and this map's world as the session saved them, and
	 * a player who just travelled at the trip's landing.
	 */
	void RestorePlayWorld(UWorld* World);

	/** The main menu is starting: no session is played. */
	void BeginMenuWorld(UWorld* World);

	// --- Helpers (also for tests) ---

	/** The save slot of session Index: "Session1" to "Session3". */
	static FString SlotName(int32 Index);

	/** Reads a session save from its bytes and brings it up to date (ULooterSessionSave::Upgrade); null when it isn't one. */
	static ULooterSessionSave* ReadSave(const TArray<uint8>& Bytes);

	/** The level a world plays, by package name, as sessions name it ("/Game/Maps/Lvl_TutorialIsland", in the editor too). */
	static FString MapOf(const UWorld* World);

	/** Where a session saved in SavedMap continues: there when the level is in the game, else the game's first level. */
	static FString ContinueMap(const FString& SavedMap);

	/** Play time for people: "45 s", "12 min", "3 h 05 min". */
	static FString FormatPlayTime(double Seconds);

	/** When a session was saved, for people: "Today 14:32", "Yesterday 09:05", "Sep 30, 2026". */
	static FString FormatSavedTime(const FDateTime& Saved, const FDateTime& Now);

	/** A level's file name for people: "/Game/Maps/Lvl_TutorialIsland" is "Tutorial Island". */
	static FString PlaceName(const FString& Map);

	/** A level's place for people: the name of the area played there ("Skyreach"), else its PlaceName. */
	static FString AreaName(const FString& Map);

	/** Writes the player (where they stand, in which level, not their progress) and this map's world into Save. */
	static void CaptureWorld(UWorld* World, ULooterSessionSave& Save);

	/** Puts the player and this map's world as Save has them (SessionSubsystemWorld.cpp). */
	static void RestoreWorld(UWorld* World, const ULooterSessionSave& Save);

	/** Moves the player onto the landing named Landing in World; logged when the level has none (they stay at its start). */
	static void PlaceAtLanding(UWorld* World, FName Landing);

private:
	/** The save in slot Index (brought up to date), or null when the slot is empty (or unreadable). */
	ULooterSessionSave* LoadSlot(int32 Index) const;

	/** A new game's save, written nowhere until it is first saved. */
	ULooterSessionSave* NewSave();

	/** The progress saved before sessions existed becomes session 1, once. */
	void ImportLegacyProgress();

	/** Hands the local player's progression subsystem the progress being played. */
	void SetPlayerProgress(const FPlayerProgressData& Progress) const;
	void GetPlayerProgress(FPlayerProgressData& OutProgress) const;

	/** The level the main menu's new games start in: the game's default map. */
	static FString NewGameMap();

	/** Saves the session being played for Reason when the gate lets it; a save held back saves once the holds end. */
	bool SaveFor(ESessionSaveReason Reason);

	/** Fills Current from the level being played: the time played, the progress, the player and this map's world. */
	void CaptureSession(UWorld& World);

	/** Writes Current to the slot being played; false (logged) when it couldn't. */
	bool WriteSession();

	void HandleWorldBeginTearDown(UWorld* World);
	bool HandleAutosave(float DeltaTime);
	bool HandleSaveDue(float DeltaTime);
	void StopTimers();

	/** The session being played; without one, what a trip carried along in memory. Null on the main menu. */
	UPROPERTY(Transient)
	TObjectPtr<ULooterSessionSave> Current;

	int32 ActiveIndex = INDEX_NONE;

	/** The level being played, and whether it has been put back as the session saved it (only then is it saved). */
	TWeakObjectPtr<UWorld> PlayWorld;
	bool bWorldRestored = false;

	/** The play level's game time at the last save (or its start), for the time played. */
	double PlayClock = 0.0;

	/** Whether a save may happen now: a trip under way, or holds. */
	FSessionSaveGate SaveGate;

	/** An autosave or save-soon came while saves were held: it saves once they're released. */
	bool bSaveWanted = false;

	/** Where the player arrives in the level starting now (a trip's landing), until they're there. */
	FName ArrivalLanding;

	FTSTicker::FDelegateHandle AutosaveTicker;
	FTSTicker::FDelegateHandle PendingSave;
	FDelegateHandle TearDownHandle;
};
