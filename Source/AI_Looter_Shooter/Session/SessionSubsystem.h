#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Misc/DateTime.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SessionSubsystem.generated.h"

class ULooterSessionSave;
class UWorld;
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

	/** Where the player is, for people ("Tutorial Island"). */
	FString Place;

	bool bTutorialDone = false;
};

/**
 * The game's three sessions (save slots "Session1" to "Session3"), each a separate playthrough: the player (where they
 * stand, health, level and experience, bestiary, guns and ammo) and the world (ULooterSessionSave).
 *
 * The main menu picks one (PlaySession), which opens its level with ?Session=N in the URL; the game mode then loads it
 * (BeginPlayWorld) and, once every actor has begun play, puts the player and the world back as they were
 * (RestorePlayWorld). While a session is played it saves every minute, a few seconds after progress (SaveSoon), on the
 * pause menu's Save & Quit, and whenever its level ends (quitting the game, travelling, stopping Play-In-Editor).
 *
 * A level played without a session (Play-In-Editor straight on a level) saves nothing: the player starts as a new game.
 * The progress saved before sessions existed ("PlayerProgress") becomes session 1 the first time the game starts.
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

	/** Writes the session being played, the player and world as they are now. False when no session is being played. */
	bool SaveNow();

	/** Saves SaveSoonDelay seconds from now (several calls make one write). */
	void SaveSoon();

	/** Saves the session being played and goes back to the main menu (the pause menu's Save & Quit). */
	void SaveAndQuitToMenu();

	// --- For the game modes ---

	/**
	 * A play level is starting (its game mode's InitGame): loads the session its URL names (?Session=1 to 3) and gives
	 * the player its progress; with no session, a new game's progress and nothing is saved.
	 */
	void BeginPlayWorld(UWorld* World, const FString& Options);

	/** Every actor in the play level has begun play: puts the player and the world as the session saved them. */
	void RestorePlayWorld(UWorld* World);

	/** The main menu is starting: no session is played. */
	void BeginMenuWorld(UWorld* World);

	// --- Helpers (also for tests) ---

	/** The save slot of session Index: "Session1" to "Session3". */
	static FString SlotName(int32 Index);

	/** Play time for people: "45 s", "12 min", "3 h 05 min". */
	static FString FormatPlayTime(double Seconds);

	/** When a session was saved, for people: "Today 14:32", "Yesterday 09:05", "Sep 30, 2026". */
	static FString FormatSavedTime(const FDateTime& Saved, const FDateTime& Now);

	/** A level's name for people: "/Game/Maps/Lvl_TutorialIsland" is "Tutorial Island". */
	static FString PlaceName(const FString& Map);

	/** Writes the player (not their progress, which the progression subsystem holds) and the world into Save. */
	static void CaptureWorld(UWorld* World, ULooterSessionSave& Save);

	/** Puts the player and the world as Save has them (SessionSubsystemWorld.cpp). */
	static void RestoreWorld(UWorld* World, const ULooterSessionSave& Save);

private:
	/** The save in slot Index, or null when the slot is empty (or unreadable). */
	ULooterSessionSave* LoadSlot(int32 Index) const;

	/** The progress saved before sessions existed becomes session 1, once. */
	void ImportLegacyProgress();

	/** Hands the local player's progression subsystem the progress being played. */
	void SetPlayerProgress(const FPlayerProgressData& Progress) const;
	void GetPlayerProgress(FPlayerProgressData& OutProgress) const;

	/** The level the main menu's new games start in: the game's default map. */
	static FString NewGameMap();

	void HandleWorldBeginTearDown(UWorld* World);
	bool HandleAutosave(float DeltaTime);
	bool HandleSaveDue(float DeltaTime);
	void StopTimers();

	/** The session being played (null when none). */
	UPROPERTY(Transient)
	TObjectPtr<ULooterSessionSave> Current;

	int32 ActiveIndex = INDEX_NONE;

	/** The level being played, and whether it has been put back as the session saved it (only then is it saved). */
	TWeakObjectPtr<UWorld> PlayWorld;
	bool bWorldRestored = false;

	/** The play level's game time at the last save (or its start), for the time played. */
	double PlayClock = 0.0;

	FTSTicker::FDelegateHandle AutosaveTicker;
	FTSTicker::FDelegateHandle PendingSave;
	FDelegateHandle TearDownHandle;
};
