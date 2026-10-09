#pragma once

#include "CoreMinimal.h"
#include "PlayerThrowSave.generated.h"

/**
 * The grenades the player carries, as a session saves them (UPlayerThrowComponent::SaveThrowables and
 * RestoreThrowables), beside the guns and ammo (FWeaponInventorySave). A session saved before grenades existed has none
 * of this: the player is given their two as their first gun comes into hand, as in a new game.
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FThrowablesSave
{
	GENERATED_BODY()

	/** Grave-salt grenades carried (0 to FThrowRules::MaxGrenades). */
	UPROPERTY()
	int32 Grenades = 0;

	/** The player has been given grenades (the first gun's two, or one found): the HUD shows the counter from then on. */
	UPROPERTY()
	bool bUnlocked = false;
};
