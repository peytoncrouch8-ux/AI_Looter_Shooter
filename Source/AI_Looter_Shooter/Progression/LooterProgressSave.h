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
	static constexpr int32 CurrentVersion = 1;

	/** 0 for a save written before versions existed; new saves get CurrentVersion. */
	UPROPERTY()
	int32 Version = 0;

	UPROPERTY()
	int32 Level = 1;

	/** Experience earned into the current level (not in total), so a change to the curve never changes the level. */
	UPROPERTY()
	int64 XP = 0;
};
