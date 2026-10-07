#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.generated.h"

class UNamedWeaponDefinition;
class UWeaponDefinition;

UENUM(BlueprintType)
enum class EWeaponRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	SemiAuto,
	FullAuto,
	Burst
};

/** What kind of gun it is: picks its icons. */
UENUM(BlueprintType)
enum class EWeaponKind : uint8
{
	None,
	Rifle,
	Shotgun
};

/** The part of a gun a reload visibly works on. */
UENUM(BlueprintType)
enum class EWeaponReloadPart : uint8
{
	None,
	/** Rifles: the old magazine slides out and drops away, a fresh one goes in, then the charging handle. */
	Magazine,
	/** Shotguns: shells are pushed in one at a time, then the pump is racked. */
	Pump,
};

/** The final numbers a weapon fires with. Produced by rolling a UWeaponDefinition. */
USTRUCT(BlueprintType)
struct FWeaponStats
{
	GENERATED_BODY()

	/** Damage per pellet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0"))
	float Damage = 20.f;

	/** Rounds per minute. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	float FireRate = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	int32 MagazineSize = 30;

	/** Seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0"))
	float ReloadTime = 2.f;

	/** Half-angle of the spread cone, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0"))
	float Spread = 1.f;

	/**
	 * How far (cm) it does full damage. Past it the damage falls off, to RangeFalloffFloor at RangeFalloffEnd times the
	 * range, and bullets are gone at MaxRangeFactor times the range.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0"))
	float Range = 4000.f;

	/** Traces per shot. >1 for shotguns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	int32 PelletsPerShot = 1;

	/** How hard each shot kicks: 1 = the weapon's recoil profile as it is, below 1 kicks less. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0"))
	float Recoil = 1.f;

	/** How quickly it's aimed and comes up after a swap: 1 = normal, above 1 quicker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.1"))
	float Handling = 1.f;

	/** Magnification while aiming down the sights (1 = none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	float Zoom = 1.f;

	float GetSecondsBetweenShots() const { return 60.f / FMath::Max(FireRate, 1.f); }

	/** Bullets fly up to this many times the range. */
	static constexpr float MaxRangeFactor = 3.f;
	/** Damage falls off linearly from the range to this many times the range, down to the floor. */
	static constexpr float RangeFalloffEnd = 2.f;
	static constexpr float RangeFalloffFloor = 0.5f;

	/** The share of its damage a bullet still does after flying Distance cm. */
	float DamageAtDistance(float Distance) const
	{
		const float Over = (Distance - Range) / FMath::Max(Range * (RangeFalloffEnd - 1.f), 1.f);
		return FMath::Lerp(1.f, RangeFalloffFloor, FMath::Clamp(Over, 0.f, 1.f));
	}
};

/** How a rarity tier modifies the base stats. Multipliers are applied after random variance. */
USTRUCT(BlueprintType)
struct FWeaponRarityInfo
{
	GENERATED_BODY()

	/** Relative chance of rolling this tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float FireRateMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float MagazineMultiplier = 1.f;

	/** <1 means faster reloads. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float ReloadTimeMultiplier = 1.f;

	/** <1 means tighter spread. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float SpreadMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
	FLinearColor Color = FLinearColor::White;
};

/**
 * Everything needed to recreate a specific rolled weapon. Save this to persist loot:
 * re-rolling the same definition with the same seed, rarity and level gives identical stats.
 */
USTRUCT(BlueprintType)
struct FWeaponInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> Definition = nullptr;

	/**
	 * The named gun it is (Heirloom), or null for a rolled one: its name and flavor line replace the name its parts would
	 * give it, and its stats sit at the named gun's fixed quality instead of rolling from the seed. Saved as the asset's
	 * path, like Definition; a gun saved before named guns existed reads back as null, a rolled gun as before.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<UNamedWeaponDefinition> Named = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponRarity Rarity = EWeaponRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 Seed = 0;

	/**
	 * The parts it was built with, by key, one per part slot (none for an empty slot). Kept with the gun so parts added to
	 * the lists later never change guns already found; their numbers are still read from the parts, so balance changes
	 * reach every gun. Empty: picked from the seed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<FName> Parts;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	FWeaponStats Stats;

	/** Rounds in the magazine when the weapon was stashed or dropped. -1 = fresh weapon (full magazine). Reserve ammo
	 * isn't stored per weapon: it lives in the holder's shared pool (UWeaponManagerComponent). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 SavedMagazine = -1;
};
