#pragma once

#include "CoreMinimal.h"
#include "Elements/LooterElement.h"
#include "Weapons/WeaponTypes.h"
#include "ShieldTypes.generated.h"

class UShieldDefinition;

/**
 * A ward's trait (Rare and up roll one: ShieldRules::RollTrait). What each does is in Docs/Work/2026-10-22-Next/SHIELD.md
 * and ShieldText::TraitWords. Saved by name: append only, never rename.
 */
UENUM(BlueprintType)
enum class EShieldTrait : uint8
{
	None,
	/** Breaking looses a burst of the ward's element round the player, once a full charge. */
	LastBreath,
	/** A bite or claw that lands on a charged ward cuts the biter. */
	Barbwire,
	/** Some hits on a charged ward give rounds for the gun in hand. */
	Lodestone,
	/** A kill puts back a share of the ward and skips the rest of its wait. */
	Harvest,
	/** Broken, with health low: the ward flares back to a share of its charge, once a full charge. */
	SaintsGrace,
};

/** Every trait there is, None aside (tests and dumps walk it). */
inline constexpr EShieldTrait AllShieldTraits[] = { EShieldTrait::LastBreath, EShieldTrait::Barbwire, EShieldTrait::Lodestone,
	EShieldTrait::Harvest, EShieldTrait::SaintsGrace };

/**
 * The body a ward's seed picks: it leans the numbers (a Bulwark holds more and refills slower, a Quickset wakes sooner and
 * holds less) and names the ward. Saved by name: append only.
 */
UENUM(BlueprintType)
enum class EShieldBuild : uint8
{
	Standard,
	Bulwark,
	Quickset,
};

/** The numbers a ward works with, rolled from its definition, rarity, level, build and seed (ShieldRules::ComputeStats). */
USTRUCT(BlueprintType)
struct FShieldStats
{
	GENERATED_BODY()

	/** The most it holds (points of damage). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shield")
	float Capacity = 40.f;

	/** Seconds unhurt before it starts to refill. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shield")
	float RechargeDelay = 3.2f;

	/** Points it refills a second once it has started. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shield")
	float RechargeRate = 11.2f;

	/** Seconds from empty to full once refilling (the wait not counted). */
	float SecondsToFill() const { return Capacity / FMath::Max(RechargeRate, 0.01f); }
};

/**
 * Everything that makes one ward, as a session saves it and as it lies on the ground: re-rolling the same definition,
 * rarity, level, seed, build and trait gives the same numbers, so its stats are rebuilt whenever it's loaded (balance
 * changes reach every ward already found).
 */
USTRUCT(BlueprintType)
struct FShieldInstanceData
{
	GENERATED_BODY()

	/** What kind of ward (DA_Shield_WardBox); saved as the asset's path. Null: no ward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	TObjectPtr<UShieldDefinition> Definition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	EWeaponRarity Rarity = EWeaponRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield", meta = (ClampMin = "1"))
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	EShieldBuild Build = EShieldBuild::Standard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	EShieldTrait Trait = EShieldTrait::None;

	/** Last Breath's element (its burst's); None for every other trait. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield")
	ELooterElement Element = ELooterElement::None;

	/** Rebuilt from the rest whenever it's made or loaded (ShieldRules::ComputeStats); kept for the save's readability. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shield")
	FShieldStats Stats;

	bool IsSet() const { return Definition != nullptr; }
};

/** The wards the player carries, as a session saves them (inside FWeaponInventorySave; an older save reads it empty). */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FShieldInventorySave
{
	GENERATED_BODY()

	/** The ward worn (Definition null: none). It comes back full. */
	UPROPERTY()
	FShieldInstanceData Equipped;

	/**
	 * The spare wards carried, oldest first. Since 2026-10-31 they live in the backpack and share its room with the guns
	 * (FWeaponInventorySave::BackpackOrder says how they interleave); an older save's (three at most) all come back.
	 */
	UPROPERTY()
	TArray<FShieldInstanceData> Spares;

	/** The first gun's starting ward has been given (as the grenades' bUnlocked): a save without it gets one on load. */
	UPROPERTY()
	bool bStarterGiven = false;
};
