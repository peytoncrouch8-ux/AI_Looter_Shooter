#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "EmberDamageType.generated.h"

/**
 * Damage dealt by an ember power (Embers/Powers/*): never critical, and not a weapon's shot, so nothing that counts shots
 * counts it; the hit marker and the kill feed can tell a power did it.
 *
 * A power's kill gives its experience (UPlayerProgressionSubsystem::AwardKill, through the instigator) but notches no gun
 * (the contract, 2026-10-15). So a power applies its damage with the player's controller as InstigatedBy and NO
 * DamageCauser (null): AWeaponBase::FindKillWeapon walks the causer to the gun in the player's hand, and with no causer
 * finds none. The creature still turns on the player (it reads the instigator's pawn), and the damage number still shows.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberDamageType : public UDamageType
{
	GENERATED_BODY()
};
