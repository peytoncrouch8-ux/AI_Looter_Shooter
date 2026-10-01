#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LooterProgressSave.generated.h"

/**
 * The player's progress, in the "PlayerProgress" save slot (about 2 KB, mostly the engine's header). No save means a
 * new game: level 1 with no experience. It starts with experience only and grows into the full player save (rewards,
 * skill points, unlocks), so it carries a version for upgrading older saves (see UPlayerProgressionSubsystem::Initialize).
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

	/** 0 for a save written before versions existed; new saves get CurrentVersion. */
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
};
