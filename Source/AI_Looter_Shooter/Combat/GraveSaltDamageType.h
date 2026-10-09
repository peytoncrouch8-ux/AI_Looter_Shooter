#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "GraveSaltDamageType.generated.h"

/**
 * Damage dealt by the player's grave-salt grenade (GraveSaltBurst): never critical, and not a weapon's shot, so nothing
 * that counts shots counts it. It comes from the player, so the gun in their hand gets the notch for a kill, as with a
 * melee strike.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGraveSaltDamageType : public UDamageType
{
	GENERATED_BODY()
};
