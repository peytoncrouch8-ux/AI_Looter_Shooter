#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponParts.generated.h"

class UStaticMesh;
class UWeaponDefinition;

/** One way a part slot can look. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponPartOption
{
	GENERATED_BODY()

	/**
	 * Modeled in the gun's own space (origin at the back of the receiver, +X toward the muzzle), or in the space of the
	 * socket its slot hangs from.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	TObjectPtr<UStaticMesh> Mesh;

	/** Its chance against the slot's other options. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (ClampMin = "0"))
	float Weight = 1.f;

	/** Only guns of this rarity and better can have it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	EWeaponRarity MinRarity = EWeaponRarity::Common;
};

/** A place on the gun that one part fills: the roll's seed picks one of the options the gun's rarity allows. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponPartSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FName Name;

	/** Hangs from this socket on the part of an earlier slot; none = at the gun's origin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FName Socket;

	/** A slot none of whose options the gun's rarity allows stays empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	TArray<FWeaponPartOption> Options;
};

/** One color a paint can take. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponColorOption
{
	GENERATED_BODY()

	/** Linear. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paint")
	FLinearColor Color = FLinearColor::White;

	/** Its chance against the paint's other colors. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paint", meta = (ClampMin = "0"))
	float Weight = 1.f;
};

/** A material the gun's parts share, colored per gun: the roll's seed picks one of its colors. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponPaint
{
	GENERATED_BODY()

	/** The parts' material slot it colors (the Blender material's name, such as GunPaint). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paint")
	FName Slot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paint")
	TArray<FWeaponColorOption> Colors;
};

/** What a rolled gun is made of: the same definition, seed and rarity always make the same gun. */
struct FWeaponLook
{
	/** One per part slot, in the definition's order; null where a slot stays empty. */
	TArray<const FWeaponPartOption*> Parts;
	/** Each paint's color, by material slot. */
	TMap<FName, FLinearColor> Colors;
};

namespace WeaponParts
{
	/**
	 * Picks a rolled gun's parts and paint from its seed. Every slot and paint takes one draw from the seed whatever its
	 * rarity allows, so guns that differ only in rarity differ only in their rarity parts.
	 */
	AI_LOOTER_SHOOTER_API FWeaponLook Pick(const UWeaponDefinition& Definition, int32 Seed, EWeaponRarity Rarity);

	/** How strongly a gun's rarity parts glow: commons faintly. */
	AI_LOOTER_SHOOTER_API float RarityGlow(EWeaponRarity Rarity);
}
