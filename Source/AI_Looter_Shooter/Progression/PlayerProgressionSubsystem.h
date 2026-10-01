#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Progression/XPCurve.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "PlayerProgressionSubsystem.generated.h"

class AActor;
class AController;
class ULooterProgressSave;

/** Where experience came from, so later systems (rewards, stats, bonuses) can tell kills from quests and the like. */
UENUM()
enum class EXPSource : uint8
{
	Kill,   // a creature the player killed
	Debug   // the console commands (Looter.GiveXP, Looter.SetLevel, Looter.ResetProgress)
};

/** Gained is 0 when the level was set directly (a console command or a reset) rather than earned. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayerXPChanged, int64 /*Gained*/, EXPSource /*Source*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerLevelUp, int32 /*NewLevel*/);

/**
 * The player's level and experience. A local player subsystem, so it lives as long as the player does and carries
 * across level changes. Kept in the "PlayerProgress" save slot (ULooterProgressSave): a new game starts at level 1.
 * The curve comes from UProgressionSettings.
 *
 * Other systems hook in through the two events: OnLevelUp is where level rewards (skill points, unlocks, stat scaling)
 * go, and OnXPChanged keeps displays like the HUD's experience bar current without polling.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPlayerProgressionSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetLevel() const;

	/** Experience earned into the current level. */
	UFUNCTION(BlueprintPure, Category = "Progression")
	int64 GetXP() const;

	/** Experience the current level takes in all (the bar's full length); 0 at the maximum level. */
	UFUNCTION(BlueprintPure, Category = "Progression")
	int64 GetXPToNextLevel() const;

	/** How far through the current level, 0 to 1 (1 at the maximum level). */
	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetLevelProgress() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	bool IsMaxLevel() const;

	/** The curve in use (UProgressionSettings), read fresh so tuning in the editor applies at once. */
	static FXPCurve GetCurve();

	/**
	 * Adds experience, leveling up as many times as it covers; nothing at the maximum level. Fires OnLevelUp for each
	 * level gained, in order, then OnXPChanged once. Returns the levels gained.
	 */
	int32 AddXP(int64 Amount, EXPSource Source);

	/** Puts the player at the start of Level (testing). Fires OnXPChanged but not OnLevelUp: nothing was earned. */
	void SetLevel(int32 Level);

	/** Back to a new game: level 1, no experience, the tutorial not done, nothing met or defeated. */
	void ResetProgress();

	/** Whether the tutorial's prompts have been completed (or skipped); a new game starts without. */
	bool IsTutorialDone() const;
	void SetTutorialDone(bool bDone);

	/** Experience for killing this actor: a creature's XPReward. Anything else (target dummies, props) gives none. */
	static int64 KillXP(const AActor* Victim);

	/**
	 * Credits a kill to the local player whose controller landed the killing blow: its experience, and one more of its
	 * kind defeated (the bestiary). Nothing for AI kills.
	 */
	static void AwardKill(const AController* Killer, const AActor* Victim);

	/** Counts one more of the victim's kind defeated. */
	void RecordDefeat(const AActor* Victim);

	/** How many actors of this class (or classes derived from it) the player has defeated. */
	int32 GetDefeated(const UClass* ActorType) const;

	/**
	 * Notes that the local player behind Player has met this actor's kind: it hunted them, they hurt it, or they
	 * defeated it. Its bestiary page opens from then on. Nothing when Player isn't a local player.
	 */
	static void RecordEncounter(const AController* Player, const AActor* Actor);

	/** The player has met an actor of this class (or a class derived from it). */
	bool HasEncountered(const UClass* ActorType) const;

	/** Forgets every kind met and defeated: every bestiary page reads "???" again (Looter.ForgetBestiary, for testing). */
	void ForgetBestiary();

	FOnPlayerXPChanged OnXPChanged;
	FOnPlayerLevelUp OnLevelUp;

private:
	/** Writes the save now. */
	void SaveProgress();

	/** Saves a few seconds from now, so a fight's worth of kills makes one write, not one per kill. */
	void ScheduleSave();

	bool HandleSaveDue(float DeltaTime);

	UPROPERTY(Transient)
	TObjectPtr<ULooterProgressSave> SaveData;

	FTSTicker::FDelegateHandle PendingSave;
	bool bUnsaved = false;
};
