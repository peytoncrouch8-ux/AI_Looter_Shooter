#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "MeleeDamageType.generated.h"

/**
 * Damage dealt by the player's melee strike (UPlayerMeleeComponent): a gun's stock or a fist. Never critical (a strike
 * has no critical spot), and not a weapon's shot, so nothing that counts shots counts it.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMeleeDamageType : public UDamageType
{
	GENERATED_BODY()
};
