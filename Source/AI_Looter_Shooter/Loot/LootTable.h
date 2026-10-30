#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponTypes.h"
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
 * How many rounds one dropped ammo pickup holds (the user's rules). A table's amounts are AR rounds; each class scales them
 * by its kill box (LooterAmmo::FInfo::KillBox; the user, 2026-10-29 night: AR 18-36, SMG 24-48, pistol 12-24, shotgun 4-8,
 * sniper 3-6, rockets one to three as before). Each class's carry limit still caps what the player takes from it.
 */
namespace LooterLoot
{
	/** A kill's ammo pickups hold a random amount from Min to Max AR rounds, both included (other classes scaled). */
	inline constexpr int32 KillAmmoAmountMin = 18;
	inline constexpr int32 KillAmmoAmountMax = 36;

	/**
	 * A loot chest's ammo pickups always hold this many AR rounds: 1.5 of a class's box (the user, 2026-10-29 night; was 36,
	 * a kill's most): AR 54, SMG 72, pistol 36, shotgun 12, sniper 9, rockets three.
	 */
	inline constexpr int32 ChestAmmoAmount = 54;

	/**
	 * Rockets come few (the contract, 2026-10-21: one to three a pickup): the rounds a pickup rolled, read as rockets, the
	 * low third of a kill's range one, the middle two, the top three (a chest's: three).
	 */
	inline int32 RocketsFor(int32 Rounds)
	{
		return Rounds <= 24 ? 1 : (Rounds <= 30 ? 2 : 3);
	}

	/**
	 * What a pickup of Type that rolled Rounds (AR rounds) holds: Rounds x the class's KillBox / 36, rounded, at least one
	 * (AR: the rounds themselves), or rockets for rockets.
	 */
	inline int32 AmmoAmountFor(EAmmoType Type, int32 Rounds)
	{
		if (Type == EAmmoType::Rocket)
		{
			return RocketsFor(Rounds);
		}
		const int32 Box = LooterAmmo::GetInfo(Type).KillBox;
		return FMath::Max(1, FMath::RoundToInt32(static_cast<float>(Rounds) * static_cast<float>(Box) / static_cast<float>(KillAmmoAmountMax)));
	}

	/** How likely rockets are against each other class in a table that lists every class (a launcher drops less often too). */
	inline constexpr float RocketAmmoWeight = 0.5f;
}

/**
 * What an enemy, chest or boss drops when it dies. Create via Content Browser > Miscellaneous > Data Asset > LootTable.
 *
 * Every kill rolls two things independently:
 *  - Ammo: a few pickups, each of a random ammo class (weighted by AmmoTypes), leaning toward the class of the gun
 *    that made the kill (KillWeaponAmmoBias), each holding AmmoAmountMin to AmmoAmountMax rounds.
 *  - Weapons: only WeaponDropChance of kills drop one. Its rarity comes from the weapon's own rarity table
 *    (Common most often, Legendary least), shifted up by Luck; in the early areas a legendary is capped
 *    (bKeepsLegendaryOdds).
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

	/**
	 * Keeps its guns' own legendary odds in the early areas: a Soulfed monster's and a boss's table (create_rank_assets.py
	 * sets it), a bounty's. Any other table rolled at an early level (Ransom's Rest's) gives a legendary at most
	 * Looter.Loot.EarlyLegendary.Share of its guns (ULootLibrary::LegendaryCapFor; the user, 2026-10-27: "Rarer, bosses still
	 * drop"). Later areas give every table its full odds.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapons")
	bool bKeepsLegendaryOdds = false;

	/**
	 * The least rarity its guns come at (a better loot chest's floor; the user, 2026-10-27: tier 3 Uncommon, tier 4 Rare,
	 * tier 5 Epic): the rarities under it never roll, the rest keep their odds against each other (luck, and the early
	 * legendary cap on what's left). Common: no floor, every roll as before.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapons")
	EWeaponRarity MinWeaponRarity = EWeaponRarity::Common;

	/**
	 * Some of the guns it drops come as wards instead (SHIELD, 2026-10-22: the weapon-drop share split): each gun it rolls is
	 * a ward of the same rarity with Looter.Shield.Drop.Share's chance (0.2), so a kill's loot rate stays as it was. Off for
	 * tables made in code (the tests' guns stay guns); create_shields.py turns it on for the game's kill tables (the chests
	 * copy the default table's). The boss table keeps it off until the boss loot shower throws wards.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapons")
	bool bDropsShields = false;

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
	 * Rounds in each ammo pickup: a random amount from AmmoAmountMin to AmmoAmountMax, both included, in AR rounds; the other
	 * classes scale it by their kill box (LooterLoot::AmmoAmountFor).
	 * Kills drop 18 to 36 (the user's rule); set both the same for a fixed amount. The player takes what fits under the
	 * carry limit and the rest is lost: the pickup is collected either way (AAmmoPickup::CollectInto).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Ammo", meta = (ClampMin = "1"))
	int32 AmmoAmountMin = LooterLoot::KillAmmoAmountMin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Ammo", meta = (ClampMin = "1"))
	int32 AmmoAmountMax = LooterLoot::KillAmmoAmountMax;

	/**
	 * Makes this a loot chest's table as far as ammo goes: every pickup holds exactly LooterLoot::ChestAmmoAmount rounds
	 * (the user's rule) rather than a kill's random amount. For the chests to come; a chest's table asset gets the same by
	 * setting both amounts to LooterLoot::ChestAmmoAmount (54).
	 */
	void UseChestAmmoAmount();
};
