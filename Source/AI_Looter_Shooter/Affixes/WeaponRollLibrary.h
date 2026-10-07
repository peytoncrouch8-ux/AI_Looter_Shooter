#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponRollLibrary.generated.h"

class AWeaponBase;
class UWeaponDefinition;

UCLASS()
class AI_LOOTER_SHOOTER_API UWeaponRollLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Picks a rarity using the definition's weights. Luck shifts the odds toward
	 * higher tiers: 0 = default odds, 1 = each tier above Common has its weight doubled per tier.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Loot")
	static EWeaponRarity RollRarity(const UWeaponDefinition* Definition, float Luck = 0.f);

	/** RollRarity from a given random stream (seeded rolls, tests). */
	static EWeaponRarity RollRarityWith(const UWeaponDefinition* Definition, float Luck, FRandomStream& Random);

	/**
	 * Deterministically computes stats for a definition + rarity + level + seed: the base stats with random variance, the
	 * rarity's multipliers, the level's damage, and the changes of the parts the seed picks.
	 */
	UFUNCTION(BlueprintPure, Category = "Weapons|Loot")
	static FWeaponStats ComputeStats(const UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Level, int32 Seed);

	/**
	 * ComputeStats with the parts a gun was saved with (Parts, one key per slot; empty = picked from the seed):
	 *   stat = base (+-variance) x rarity x level (damage) x (1 + the parts' percentages, added and capped)
	 * Accuracy divides the spread instead; magazines and sights set capacity (then scaled by rarity) and zoom outright.
	 * With a FixedQuality (a named gun's) the variance and the parts' percentages aren't rolled but sit at that point of
	 * their ranges: 0 the worse end, 1 the better (the low end for reload time, spread and recoil), 0.5 the middle.
	 */
	static FWeaponStats ComputeStatsWithParts(const UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Level, int32 Seed,
		TConstArrayView<FName> Parts, TOptional<float> FixedQuality = {});

	/**
	 * A gun's stats as it is (AWeaponBase rebuilds them with this whenever it's made or loaded): a named gun's at its
	 * fixed quality (UNamedWeaponDefinition::StatQuality), so every copy is the same; any other's rolled from its seed.
	 */
	static FWeaponStats ComputeInstanceStats(const FWeaponInstanceData& Instance);

	/** Rolls a brand-new weapon instance with a random seed and rarity. */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Loot")
	static FWeaponInstanceData RollWeapon(UWeaponDefinition* Definition, int32 Level = 1, float Luck = 0.f);

	/** Same as RollWeapon but with a fixed rarity (quest rewards, testing). */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Loot")
	static FWeaponInstanceData RollWeaponWithRarity(UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Level = 1);

	/** Spawns the actor for a rolled instance. The weapon starts unowned and visible in the world. */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Loot", meta = (WorldContext = "WorldContextObject"))
	static AWeaponBase* SpawnWeapon(UObject* WorldContextObject, const FWeaponInstanceData& Instance, const FTransform& Transform);

	UFUNCTION(BlueprintPure, Category = "Weapons|Loot")
	static FLinearColor GetRarityColor(const UWeaponDefinition* Definition, EWeaponRarity Rarity);
};
