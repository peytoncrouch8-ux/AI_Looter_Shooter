#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponParts.generated.h"

class UStaticMesh;
class UWeaponDefinition;

/** How a part changes a gun's rolled stats: multipliers, 1 = no change. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponPartStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (ClampMin = "0"))
	float Damage = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (ClampMin = "0"))
	float FireRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (ClampMin = "0"))
	float MagazineSize = 1.f;

	/** Below 1 reloads faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (ClampMin = "0"))
	float ReloadTime = 1.f;

	/** Below 1 shoots tighter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (ClampMin = "0"))
	float Spread = 1.f;
};

/** One way a part slot can look: its mesh, how it changes the gun's stats, and the word it can give the gun's name. */
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

	/** Only guns of this rarity and better can have it: rarity unlocks the better parts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	EWeaponRarity MinRarity = EWeaponRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FWeaponPartStats Stats;

	/** The word it puts before the gun's name ("Scoped"), if its NamePriority is the highest among the gun's parts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FText NamePrefix;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	int32 NamePriority = 0;
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
	 * Picks a rolled gun's parts and paint from its seed, among the options its rarity allows. Every slot and paint takes
	 * one draw whatever it allows, so a slot's pick never shifts the ones after it. The draws are their own, apart from
	 * the stats' random variance (see UWeaponRollLibrary::ComputeStats).
	 */
	AI_LOOTER_SHOOTER_API FWeaponLook Pick(const UWeaponDefinition& Definition, int32 Seed, EWeaponRarity Rarity);

	/** The picked parts' stat changes, all multiplied together. */
	AI_LOOTER_SHOOTER_API FWeaponPartStats CombinedStats(const FWeaponLook& Look);

	/** The word the gun's parts put before its name (empty when none does). */
	AI_LOOTER_SHOOTER_API FText NamePrefix(const FWeaponLook& Look);

	/** How strongly a gun's rarity parts glow: commons faintly. */
	AI_LOOTER_SHOOTER_API float RarityGlow(EWeaponRarity Rarity);
}
