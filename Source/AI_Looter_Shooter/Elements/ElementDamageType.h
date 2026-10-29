#pragma once

#include "CoreMinimal.h"
#include "Elements/LooterElement.h"
#include "GameFramework/DamageType.h"
#include "ElementDamageType.generated.h"

/**
 * Damage dealt by an element's status (UElementStatusComponent): a burn's piece, a smite's burst. Never critical (it isn't
 * UWeaponCritDamageType), and not a weapon's shot, so nothing that counts shots counts it. The gun that set the status is
 * its DamageCauser, so that gun gets the notch for a kill. Grave-salt's is the grave-salt grenade's own
 * (UGraveSaltDamageType: the same salt, the same name and feel; ElementRules::DamageTypeFor).
 * The damage types live in this one header, as LooterDamageTypes.h's do: each is a name and nothing else.
 */
UCLASS(Abstract)
class AI_LOOTER_SHOOTER_API UElementDamageType : public UDamageType
{
	GENERATED_BODY()

public:
	/** Which element it is. */
	ELooterElement Element = ELooterElement::None;
};

/** An Ember burn's damage. */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberElementDamageType : public UElementDamageType
{
	GENERATED_BODY()

public:
	UEmberElementDamageType() { Element = ELooterElement::Ember; }
};

/** A Soul-light smite's burst (and its leap to the next of the dead). */
UCLASS()
class AI_LOOTER_SHOOTER_API USoulLightElementDamageType : public UElementDamageType
{
	GENERATED_BODY()

public:
	USoulLightElementDamageType() { Element = ELooterElement::SoulLight; }
};

/** A Thunder jolt's burst (and its arc to the next living thing). */
UCLASS()
class AI_LOOTER_SHOOTER_API UThunderElementDamageType : public UElementDamageType
{
	GENERATED_BODY()

public:
	UThunderElementDamageType() { Element = ELooterElement::Thunder; }
};

/** A Rot's piece. */
UCLASS()
class AI_LOOTER_SHOOTER_API URotElementDamageType : public UElementDamageType
{
	GENERATED_BODY()

public:
	URotElementDamageType() { Element = ELooterElement::Rot; }
};
