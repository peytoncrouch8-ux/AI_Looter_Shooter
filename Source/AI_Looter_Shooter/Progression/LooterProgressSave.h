#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Progression/PlayerProgressData.h"
#include "LooterProgressSave.generated.h"

/**
 * The player's progress from before sessions existed: one save for the whole game, in the "PlayerProgress" slot. The
 * first time the game starts with sessions it becomes session 1 (USessionSubsystem); the file is left as it was. New
 * progress is saved with its session (ULooterSessionSave).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULooterProgressSave : public USaveGame
{
	GENERATED_BODY()

public:
	/**
	 * 2: defeat counts for the bestiary (older saves start them at 0).
	 * 3: kinds met, for the bestiary's unknown pages (older saves have met whatever they defeated).
	 */
	static constexpr int32 CurrentVersion = 3;

	/** The slot it lived in. */
	static constexpr const TCHAR* SlotName = TEXT("PlayerProgress");

	/** 0 for a save written before versions existed. */
	UPROPERTY()
	int32 Version = 0;

	UPROPERTY()
	int32 Level = 1;

	/** Experience earned into the current level (not in total), so a change to the curve never changes the level. */
	UPROPERTY()
	int64 XP = 0;

	/** The tutorial island's prompts have been followed to the end (or skipped). */
	UPROPERTY()
	bool bTutorialDone = false;

	/** How many of each kind of actor the player has defeated, by class path (the bestiary's counts). */
	UPROPERTY()
	TMap<FString, int32> Defeated;

	/** Every kind of actor the player has met (been hunted by, hurt or defeated), by class path: its bestiary page is open. */
	UPROPERTY()
	TSet<FString> Encountered;

	/** Its progress as a session holds it, older versions brought up to date. */
	FPlayerProgressData ToProgress() const
	{
		FPlayerProgressData Progress;
		Progress.Level = Level;
		Progress.XP = XP;
		Progress.bTutorialDone = bTutorialDone;
		Progress.Defeated = Defeated;
		Progress.Encountered = Encountered;
		if (Version < 3)
		{
			// Before version 3 nothing recorded meetings: whatever the player defeated, they have met.
			for (const TPair<FString, int32>& Pair : Defeated)
			{
				Progress.Encountered.Add(Pair.Key);
			}
		}
		return Progress;
	}
};
