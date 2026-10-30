#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Elements/LooterElement.h"
#include "Shields/ShieldTypes.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponTypes.h"
#include "LootLibrary.generated.h"

class ULootTable;
class UWeaponDefinition;

USTRUCT(BlueprintType)
struct FAmmoDrop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	EAmmoType Type = EAmmoType::AssaultRifle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "1"))
	int32 Amount = 1;
};

/** Everything one roll of a loot table produced, before anything is spawned. */
USTRUCT(BlueprintType)
struct FLootRoll
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TArray<FWeaponInstanceData> Weapons;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TArray<FAmmoDrop> Ammo;

	/** The wards it drops (a share of its gun picks, when the table drops wards: ULootTable::bDropsShields). */
	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TArray<FShieldInstanceData> Shields;
};

/** One gun a roll drops, before its stats are rolled: which kind and how rare. */
struct FLootWeaponPick
{
	UWeaponDefinition* Definition = nullptr;
	EWeaponRarity Rarity = EWeaponRarity::Common;
};

UCLASS()
class AI_LOOTER_SHOOTER_API ULootLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Rolls a loot table and spawns the results (weapons and ammo) tossed out around Location. */
	UFUNCTION(BlueprintCallable, Category = "Loot", meta = (WorldContext = "WorldContextObject"))
	static TArray<AActor*> SpawnLoot(UObject* WorldContextObject, const ULootTable* LootTable, FVector Location, int32 Level = 1, float ExtraLuck = 0.f);

	/**
	 * SpawnLoot for a kill: the ammo drops lean toward KillAmmo, the ammo class of the gun that made it, when set. Without
	 * bWeapons only the ammo drops (a practice area's creatures once the player has left it).
	 */
	static TArray<AActor*> SpawnKillLoot(UObject* WorldContextObject, const ULootTable* LootTable, FVector Location, int32 Level,
		float ExtraLuck, TOptional<EAmmoType> KillAmmo, bool bWeapons = true, ELooterElement Lean = ELooterElement::None);

	/**
	 * Decides what one kill drops, without spawning anything. What drops (counts, weapons, rarities, ammo classes and
	 * amounts) comes from Random, so a seeded stream repeats it; each weapon still gets its own fresh stat seed (and its
	 * curse, if any, from that seed: RollDroppedWeapon). KillAmmo,
	 * when set, is the ammo class of the gun that made the kill: each box leans toward it (the table's KillWeaponAmmoBias).
	 * Without bWeapons the guns aren't drawn at all, so the ammo is what the same stream would have dropped with them.
	 * Level also caps a legendary in the early areas (LegendaryCapFor).
	 */
	static FLootRoll RollLoot(const ULootTable* LootTable, int32 Level, float ExtraLuck, FRandomStream& Random,
		TOptional<EAmmoType> KillAmmo = {}, bool bWeapons = true, ELooterElement Lean = ELooterElement::None);

	/**
	 * One gun that drops (a kill's or a chest's): rolled with a fresh seed at the rarity and level, and cursed when its
	 * seed says so (WeaponCurses::Roll: some Rare, Epic and Legendary guns), its stats with the curse in them. Lean: the
	 * element a Touched killer leans the gun's parts toward (WeaponParts::Pick; None: no lean). SpawnKillLoot and RollLoot
	 * pass theirs on to every gun they drop.
	 */
	static FWeaponInstanceData RollDroppedWeapon(UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Level,
		ELooterElement Lean = ELooterElement::None);

	/** RollLoot's first half: the ammo pickups one kill drops (how many, each one's class and rounds). */
	static TArray<FAmmoDrop> RollAmmo(const ULootTable* LootTable, FRandomStream& Random, TOptional<EAmmoType> KillAmmo = {});

	/**
	 * RollLoot's second half, drawn right after RollAmmo: the guns one kill drops (whether any, how many, which and how
	 * rare), without rolling their stats. Cheap enough to simulate a million kills (LootOdds). Level: the guns' level, which
	 * caps a legendary in the early areas (LegendaryCapFor); NoLevel leaves the odds as the table has them.
	 */
	static TArray<FLootWeaponPick> RollWeaponPicks(const ULootTable* LootTable, float ExtraLuck, FRandomStream& Random, int32 Level = 0);

	/** A Level for RollWeaponPicks and LootOdds that names no area: the table's own odds (the tests' tables, the old odds). */
	static constexpr int32 NoLevel = 0;

	/**
	 * The most a legendary's share of each gun may be, from LootTable at Level (1: no cap). The user's call (2026-10-27,
	 * "Rarer, bosses still drop"): guns of level 1 to Looter.Loot.EarlyLegendary.MaxLevel (8: Ransom's Rest's creatures, 1-6
	 * before their ranks' levels and 8 at most with them, and its chests, kept in its band; the Lily starts at 9) are legendary
	 * at most Looter.Loot.EarlyLegendary.Share of the time (1 in 300: a Regular kill 0.1%, a Strongbox 0.67%); a table that
	 * keeps its odds (ULootTable::bKeepsLegendaryOdds: Soulfed, bosses, bounties) and every later level are never capped.
	 */
	static float LegendaryCapFor(const ULootTable* LootTable, int32 Level);

	/**
	 * Table itself when it already keeps its legendary odds, else a transient copy of it that does (a bounty's mark carries
	 * its rank's table: the board's fight swaps it for this one so a mark pays as a boss does). Outer: the copy's owner.
	 */
	static ULootTable* KeepingLegendaryOdds(ULootTable* Table, UObject* Outer);

	/**
	 * Rounds in one ammo pickup from the table: a random amount from its AmmoAmountMin to AmmoAmountMax, both included,
	 * in AR rounds (a kill's 18 to 36 without a table; RollAmmoAmountOf scales it to a class). Never below 1; a Max below Min
	 * drops exactly Min.
	 */
	static int32 RollAmmoAmount(const ULootTable* LootTable, FRandomStream& Random);

	/**
	 * What one ammo pickup of Type from the table holds: an even roll over the class's own range, the table's AmmoAmountMin to
	 * AmmoAmountMax scaled by LooterLoot::AmmoAmountFor (a kill's shells 4 to 8, every amount as likely; the user's by-class
	 * pickups, 2026-10-29 night). Rockets: RollAmmoAmount read as rockets, as before.
	 */
	static int32 RollAmmoAmountOf(const ULootTable* LootTable, EAmmoType Type, FRandomStream& Random);

	/** Picks one weapon definition from the table by weight. */
	static UWeaponDefinition* PickWeaponWith(const ULootTable* LootTable, FRandomStream& Random);

	/**
	 * Picks one ammo class from the table by weight (every class, equally, if the table lists none). KillAmmo's weight
	 * is multiplied by the table's KillWeaponAmmoBias; a class the table never drops stays out.
	 */
	static EAmmoType PickAmmoType(const ULootTable* LootTable, FRandomStream& Random, TOptional<EAmmoType> KillAmmo = {},
		bool bNoRockets = false);

	/**
	 * No rockets while nobody carries a launcher (the overnight playtest, 2026-10-28: +3 rockets at level 1): each rocket
	 * pickup in Roll becomes another class from the table, with a fresh amount, unless bLauncher (one is carried) or Roll
	 * itself holds a launcher. Spawning code passes AnyPlayerCarriesLauncher.
	 */
	static void KeepRocketsForLaunchers(FLootRoll& Roll, const ULootTable* LootTable, FRandomStream& Random, bool bLauncher);

	/**
	 * Whether any player carries a launcher, in hand, holstered or in the backpack. True when no player has a weapon manager
	 * (nothing to judge by: the roll is left as it is).
	 */
	static bool AnyPlayerCarriesLauncher(const UObject* WorldContextObject);

	/** The loot table creatures, dummies and anything else with a loot drop component use unless given their own. */
	static ULootTable* GetDefaultLootTable();
};
