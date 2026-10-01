#pragma once

#include "CoreMinimal.h"
#include "PlayerProgressData.generated.h"

/**
 * The player's progress as a session saves it (ULooterSessionSave): level and experience, the tutorial, and the
 * bestiary's kinds met and defeat counts. The defaults are a new game: level 1 with no experience, nothing met. It grows
 * with the player save (rewards, skill points, unlocks).
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FPlayerProgressData
{
	GENERATED_BODY()

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
};
