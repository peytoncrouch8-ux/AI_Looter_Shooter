#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "ShieldDamageType.generated.h"

/**
 * Damage a ward's trait deals (Barbwire's cut, Last Breath's burst): never critical, dealt with no causer (as an ember
 * power's), so a kill gives its XP and notches no gun.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UShieldDamageType : public UDamageType
{
	GENERATED_BODY()
};
