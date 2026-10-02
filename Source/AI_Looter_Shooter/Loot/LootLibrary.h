#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
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
};

UCLASS()
class AI_LOOTER_SHOOTER_API ULootLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Rolls a loot table and spawns the results (weapons and ammo) tossed out around Location. */
	UFUNCTION(BlueprintCallable, Category = "Loot", meta = (WorldContext = "WorldContextObject"))
	static TArray<AActor*> SpawnLoot(UObject* WorldContextObject, const ULootTable* LootTable, FVector Location, int32 Level = 1, float ExtraLuck = 0.f);

	/** SpawnLoot for a kill: the ammo drops lean toward KillAmmo, the ammo class of the gun that made it, when set. */
	static TArray<AActor*> SpawnKillLoot(UObject* WorldContextObject, const ULootTable* LootTable, FVector Location, int32 Level,
		float ExtraLuck, TOptional<EAmmoType> KillAmmo);

	/**
	 * Decides what one kill drops, without spawning anything. What drops (counts, weapons, rarities, ammo classes and
	 * amounts) comes from Random, so a seeded stream repeats it; each weapon still gets its own fresh stat seed. KillAmmo,
	 * when set, is the ammo class of the gun that made the kill: each box leans toward it (the table's KillWeaponAmmoBias).
	 */
	static FLootRoll RollLoot(const ULootTable* LootTable, int32 Level, float ExtraLuck, FRandomStream& Random,
		TOptional<EAmmoType> KillAmmo = {});

	/**
	 * Rounds in one ammo pickup from the table: a random amount from its AmmoAmountMin to AmmoAmountMax, both included,
	 * whatever the class (a kill's 18 to 36 without a table). Never below 1; a Max below Min drops exactly Min.
	 */
	static int32 RollAmmoAmount(const ULootTable* LootTable, FRandomStream& Random);

	/** Picks one weapon definition from the table by weight. */
	static UWeaponDefinition* PickWeaponWith(const ULootTable* LootTable, FRandomStream& Random);

	/**
	 * Picks one ammo class from the table by weight (every class, equally, if the table lists none). KillAmmo's weight
	 * is multiplied by the table's KillWeaponAmmoBias; a class the table never drops stays out.
	 */
	static EAmmoType PickAmmoType(const ULootTable* LootTable, FRandomStream& Random, TOptional<EAmmoType> KillAmmo = {});

	/** The loot table creatures, dummies and anything else with a loot drop component use unless given their own. */
	static ULootTable* GetDefaultLootTable();
};
