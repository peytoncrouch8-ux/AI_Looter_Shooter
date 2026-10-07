#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Weapons/WeaponTypes.h"
#include "NamedWeaponDefinition.generated.h"

class UWeaponDefinition;

/**
 * A named gun (Heirloom now, the unique legendaries later): one gun of a kind, with fixed parts and a fixed rarity, its
 * own name in place of the one its parts would give it, and a flavor line its card and label show under the name.
 * One data asset per named gun in /Game/Data/Weapons, named DA_Named_<Id>; Tools/Unreal/create_named_weapons.py makes
 * them. Mission rewards (FMissionRewards::NamedGun) and Looter.GiveWeapon name one by its id.
 *
 * MakeInstance makes the gun at a level. Every copy is the same gun: the same parts, rarity and seed, and stats fixed at
 * StatQuality rather than rolled, so a reward that comes once is never a poor roll. Only its level follows the player.
 * The gun then saves and loads as any gun does (FWeaponInstanceData::Named points back here, so its name, line, stat
 * quality and wear follow this asset, as part numbers follow the parts).
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UNamedWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** "DA_Named_Heirloom" is the named gun "Heirloom". */
	static constexpr const TCHAR* AssetPrefix = TEXT("DA_Named_");

	/** Where the named guns are made and kept. */
	static constexpr const TCHAR* AssetFolder = TEXT("/Game/Data/Weapons");

	/** The kind of gun it is (DA_PumpShotgun): its slots and parts, ammo, rarity table and looks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Named Weapon")
	TObjectPtr<UWeaponDefinition> Weapon;

	/** Its name wherever a gun's name shows ("Heirloom"), in place of its parts' word and its kind's name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Named Weapon")
	FText DisplayName;

	/** The line its card and label show under its name ("Hold the door."). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Named Weapon")
	FText FlavorText;

	/** Every copy is this rare, and each of its parts must come on guns this rare (their MinRarity). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Named Weapon")
	EWeaponRarity Rarity = EWeaponRarity::Epic;

	/**
	 * Its parts: for every slot of its kind, the slot's name and the part's key (Body: Heritage), as the parts spreadsheet
	 * (Art/Models/Weapons/<Gun>.parts.csv) names them. FindProblems says when one is missing or doesn't fit.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Named Weapon")
	TMap<FName, FName> Parts;

	/**
	 * Where its stats sit within what its parts' ranges and its kind's variance allow, the same on every copy: 0 the worse
	 * end of each, 1 the better (for reload time, spread and recoil, where less is better, the low end), 0.5 the middle.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Named Weapon", meta = (ClampMin = "0", ClampMax = "1"))
	float StatQuality = 0.75f;

	/** How worn it looks, 0 (factory fresh) to 1 (battered), on every copy; below 0, rolled from its seed as any gun's. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Named Weapon", meta = (ClampMin = "-1", ClampMax = "1"))
	float Wear = -1.f;

	/** The seed every copy is made with: its paint colors, and its wear when Wear is below 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Named Weapon")
	int32 Seed = 0;

	/** Its id: the asset's name without DA_Named_ ("Heirloom"). */
	FName GetNamedId() const;

	/** Its parts' keys in its kind's slot order, None for a slot it names no part for: what its copies save. */
	TArray<FName> GetPartKeys() const;

	/** The gun at Level (at least 1): its kind, rarity, seed and parts, and its stats at StatQuality. */
	FWeaponInstanceData MakeInstance(int32 Level);

	/**
	 * What's wrong with it, one line each, empty when nothing is: no kind of gun or name, a slot with no part or a slot its
	 * kind doesn't have, a part its slot doesn't have, a part that doesn't come at its rarity, or one that doesn't fit the
	 * parts before it (the 8-shell tube on a barrel under 46 cm).
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Named")
	TArray<FString> FindProblems() const;

	/** The words name this gun: its id, its asset's name or its display name, ignoring case, spaces and punctuation. */
	bool IsNamed(const FString& Words) const;

	/** Every named gun asset in the project (loaded), by id. */
	static TArray<UNamedWeaponDefinition*> LoadAll();

	/** The named gun the words name (IsNamed), or null. */
	static UNamedWeaponDefinition* FindByName(const FString& Words);
};
