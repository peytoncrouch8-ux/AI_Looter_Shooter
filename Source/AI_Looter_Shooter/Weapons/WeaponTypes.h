#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.generated.h"

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

	/** Max trace distance in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0"))
	float Range = 10000.f;

	/** Traces per shot. >1 for shotguns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	int32 PelletsPerShot = 1;

	float GetSecondsBetweenShots() const { return 60.f / FMath::Max(FireRate, 1.f); }
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponRarity Rarity = EWeaponRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 Seed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	FWeaponStats Stats;

	/** Rounds in the magazine when the weapon was stashed or dropped. -1 = fresh weapon (full magazine). Reserve ammo
	 * isn't stored per weapon: it lives in the holder's shared pool (UWeaponManagerComponent). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 SavedMagazine = -1;
};
