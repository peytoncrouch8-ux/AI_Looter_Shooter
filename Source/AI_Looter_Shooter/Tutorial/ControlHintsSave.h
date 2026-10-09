#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ControlHintsSave.generated.h"

/** One control hint's memory as saved: by its name (FControlHintRules::Id), so hints added later never shift the rest. */
USTRUCT()
struct FControlHintRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FName Hint;

	UPROPERTY()
	int32 Shows = 0;

	UPROPERTY()
	bool bLearned = false;
};

/**
 * The control hints the player has seen and the controls they've shown they know, kept for the whole profile in the
 * "ControlHints" slot rather than per session: a control learned in one session needn't be taught again in the next.
 * UControlHintSubsystem reads it as a level begins and writes it when something changes.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULooterControlHintsSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FControlHintRecord> Hints;
};
