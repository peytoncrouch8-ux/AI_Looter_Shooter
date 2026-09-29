#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Weapons/AmmoTypes.h"
#include "LootTable.generated.h"

class UWeaponDefinition;

USTRUCT(BlueprintType)
struct FLootTableEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UWeaponDefinition> Weapon = nullptr;

	/** Relative chance of this weapon being picked when a weapon drops. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	float Weight = 1.f;
};

USTRUCT(BlueprintType)
struct FAmmoLootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	EAmmoType Type = EAmmoType::AssaultRifle;

	/** Relative chance of this ammo class being picked for each ammo box. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	float Weight = 1.f;
};

/**
 * What an enemy, chest or boss drops when it dies. Create via Content Browser > Miscellaneous > Data Asset > LootTable.
 *
 * Every kill rolls two things independently:
 *  - Ammo: a few boxes, each of a random ammo class (weighted by AmmoTypes).
 *  - Weapons: only WeaponDropChance of kills drop one. Its rarity comes from the weapon's own rarity table
 *    (Common most often, Legendary least), shifted up by Luck.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API ULootTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULootTable();

	// --- Weapons ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapons")
	TArray<FLootTableEntry> Entries;

	/** Chance (0-1) that a kill drops weapons at all. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapons", meta = (ClampMin = "0", ClampMax = "1"))
	float WeaponDropChance = 0.3f;

	/** How many weapons drop when a weapon drop happens. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapons", meta = (ClampMin = "0"))
	int32 MinWeaponDrops = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapons", meta = (ClampMin = "0"))
	int32 MaxWeaponDrops = 1;

	/** Shifts rarity odds upward. 0 = the weapon's own odds, 1 = each tier above Common is twice as likely per tier. Bosses want 1+. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapons", meta = (ClampMin = "0"))
	float Luck = 0.f;

	// --- Ammo ---

	/** Ammo classes that can drop and how likely each is. Defaults to every class, equally likely. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Ammo")
	TArray<FAmmoLootEntry> AmmoTypes;

	/** Chance (0-1) that a kill drops ammo at all. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Ammo", meta = (ClampMin = "0", ClampMax = "1"))
	float AmmoDropChance = 1.f;

	/** Ammo boxes per ammo drop; each box rolls its own class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Ammo", meta = (ClampMin = "0"))
	int32 MinAmmoDrops = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Ammo", meta = (ClampMin = "0"))
	int32 MaxAmmoDrops = 2;
};
