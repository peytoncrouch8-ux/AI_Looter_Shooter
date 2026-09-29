#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "LooterDamageTypes.generated.h"

/** Damage dealt by a weapon hit on a normal body part. */
UCLASS()
class AI_LOOTER_SHOOTER_API UWeaponDamageType : public UDamageType
{
	GENERATED_BODY()
};

/** Damage dealt by a weapon hit on a critical spot (x1.5, LooterCombat). Receivers use it for crit feedback. */
UCLASS()
class AI_LOOTER_SHOOTER_API UWeaponCritDamageType : public UWeaponDamageType
{
	GENERATED_BODY()
};

/** Damage dealt by a creature's melee attack (bites, claws). */
UCLASS()
class AI_LOOTER_SHOOTER_API UCreatureAttackDamageType : public UDamageType
{
	GENERATED_BODY()
};
