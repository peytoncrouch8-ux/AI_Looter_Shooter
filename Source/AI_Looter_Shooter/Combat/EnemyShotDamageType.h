#pragma once

#include "CoreMinimal.h"
#include "Combat/LooterDamageTypes.h"
#include "EnemyShotDamageType.generated.h"

/**
 * Damage dealt by a creature's pellet (a boss's spectral buckshot, UEnemyProjectileSubsystem). Still a creature's attack,
 * so anything that asks whether a creature hurt the player counts it; this type says it came from afar.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEnemyShotDamageType : public UCreatureAttackDamageType
{
	GENERATED_BODY()
};
