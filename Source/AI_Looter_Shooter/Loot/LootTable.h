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

	/** Relative chance of this ammo class being picked for each ammo drop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	float Weight = 1.f;
};

/**
 * How many rounds one dropped ammo pickup holds (the user's rules), the same for every ammo class. Each class's carry
 * limit still caps what the player takes from it.
 */
namespace LooterLoot
{
	/** A kill's ammo pickups hold a random amount from Min to Max, both included. */
	inline constexpr int32 KillAmmoAmountMin = 18;
	inline constexpr int32 KillAmmoAmountMax = 36;

	/** A loot chest's ammo pickups always hold this many, the most a kill's can. */
	inline constexpr int32 ChestAmmoAmount = 36;
}

/**
 * What an enemy, chest or boss drops when it dies. Create via Content Browser > Miscellaneous > Data Asset > LootTable.
 *
 * Every kill rolls two things independently:
 *  - Ammo: a few pickups, each of a random ammo class (weighted by AmmoTypes), leaning toward the class of the gun
 *    that made the kill (KillWeaponAmmoBias), each holding AmmoAmountMin to AmmoAmountMax rounds.
 *  - Weapons: only WeaponDropChance of kills drop one. Its rarity comes from the weapon's own rarity table
 *    (Common most often, Legendary least), shifted up by Luck.
 *
 * A loot chest's table drops a fixed amount of ammo per pickup instead: see UseChestAmmoAmount.
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

	/** Ammo pickups per ammo drop; each rolls its own class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Ammo", meta = (ClampMin = "0"))
	int32 MinAmmoDrops = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Ammo", meta = (ClampMin = "0"))
	int32 MaxAmmoDrops = 2;

	/**
	 * How many times its usual weight the ammo class of the gun that made the kill gets, per pickup. With five classes
	 * equally likely, 2 makes it one pickup in three instead of one in five: the gun in use keeps itself fed while the
	 * other classes still drop. 1 = no lean.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Ammo", meta = (ClampMin = "1"))
	float KillWeaponAmmoBias = 2.f;

	/**
	 * Rounds in each ammo pickup: a random amount from AmmoAmountMin to AmmoAmountMax, both included, whatever the class.
	 * Kills drop 18 to 36 (the user's rule); set both the same for a fixed amount. The player takes what fits under the
	 * carry limit and the rest stays on the ground.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Ammo", meta = (ClampMin = "1"))
	int32 AmmoAmountMin = LooterLoot::KillAmmoAmountMin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Ammo", meta = (ClampMin = "1"))
	int32 AmmoAmountMax = LooterLoot::KillAmmoAmountMax;

	/**
	 * Makes this a loot chest's table as far as ammo goes: every pickup holds exactly LooterLoot::ChestAmmoAmount rounds
	 * (the user's rule) rather than a kill's random amount. For the chests to come; a chest's table asset gets the same by
	 * setting both amounts to 36.
	 */
	void UseChestAmmoAmount();
};
