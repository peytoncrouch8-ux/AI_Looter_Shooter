#pragma once

#include "CoreMinimal.h"
#include "PlayerFlaskSave.generated.h"

/**
 * The soul flask as a session saves it (UPlayerFlaskComponent::SaveFlask and RestoreFlask), beside the grenades
 * (FThrowablesSave). A session saved before the flask existed has none of this: bSaved reads false, and the flask is full,
 * as in a new game.
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FFlaskSave
{
	GENERATED_BODY()

	/** Full charges (0 to FFlaskRules::MaxCharges). */
	UPROPERTY()
	int32 Charges = 3;

	/** Soul toward the next charge (0 to FFlaskRules::SoulPerCharge; 0 when full). */
	UPROPERTY()
	float Soul = 0.f;

	/** Written by SaveFlask. False (an older save, or nothing captured): read as a full flask. */
	UPROPERTY()
	bool bSaved = false;
};
