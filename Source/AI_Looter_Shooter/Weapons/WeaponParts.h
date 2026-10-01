#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponParts.generated.h"

class UStaticMesh;
class UWeaponDefinition;

/** A part's change to one stat, in percent (+10 = 10% more): each gun rolls its own amount between Min and Max. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponStatRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	float Min = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	float Max = 0.f;

	bool IsZero() const { return Min == 0.f && Max == 0.f; }

	/** The amount at Roll (0-1) through the range. */
	float At(float Roll) const { return FMath::Lerp(Min, Max, Roll); }
};

/**
 * How a part changes a gun's stats. Every percentage is a range each gun rolls within, so two guns with the same parts
 * still differ a little; a gun's parts' percentages add up per stat and the total is capped (WeaponParts::Caps).
 * Magazines and sights set capacity and magnification outright: a 40-round magazine holds 40.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponPartStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FWeaponStatRange Damage;

	/** Higher is tighter: +40% accuracy shoots with the spread divided by 1.4. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FWeaponStatRange Accuracy;

	/** The distance it does full damage to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FWeaponStatRange Range;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FWeaponStatRange FireRate;

	/** Reload time: negative reloads faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FWeaponStatRange Reload;

	/** How hard each shot kicks: negative kicks less. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FWeaponStatRange Recoil;

	/** How quickly the gun is aimed and comes up after a swap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FWeaponStatRange Handling;

	/** Rounds it holds before rarity (magazines); 0 = it doesn't set the capacity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (ClampMin = "0"))
	int32 Magazine = 0;

	/** Magnification while aiming (sights); 0 = it doesn't set it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (ClampMin = "0"))
	float Zoom = 0.f;
};

/** A part that only fits with certain others: it needs the part in an earlier slot to be at least so long. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponPartRequirement
{
	GENERATED_BODY()

	/** The earlier slot whose part it depends on; none = it fits anything. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FName Slot;

	/** That part's Length must be at least this (cm), such as a long magazine tube that clamps to the barrel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (ClampMin = "0"))
	float MinLength = 0.f;
};

/** One way a part slot can look: its mesh, how it changes the gun's stats, and the word it can give the gun's name. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponPartOption
{
	GENERATED_BODY()

	/** Its name within its slot ("Marksman"): guns save their parts by it, so it must never change once guns drop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FName Key;

	/** What it's called on its own ("Marksman barrel"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FText DisplayName;

	/**
	 * Modeled in the gun's own space (origin at the back of the receiver, +X toward the muzzle), or in the space of the
	 * socket its slot hangs from.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	TObjectPtr<UStaticMesh> Mesh;

	/** Its chance against the slot's other options. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (ClampMin = "0"))
	float Weight = 1.f;

	/** Only guns of this rarity and better can have it: rarity unlocks the better parts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	EWeaponRarity MinRarity = EWeaponRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FWeaponPartStats Stats;

	/** Its length (cm), for parts others depend on (barrels). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (ClampMin = "0"))
	float Length = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FWeaponPartRequirement Requires;

	/** The word it puts before the gun's name ("Scoped"), if its NamePriority is the highest among the gun's parts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FText NamePrefix;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	int32 NamePriority = 0;
};

/** A place on the gun that one part fills: the roll's seed picks one of the options the gun's rarity allows. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponPartSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FName Name;

	/** Hangs from this socket on the part of an earlier slot (the latest one that has it); none = at the gun's origin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	FName Socket;

	/** A slot none of whose options the gun's rarity allows stays empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
	TArray<FWeaponPartOption> Options;
};

/** One color a paint can take. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponColorOption
{
	GENERATED_BODY()

	/** Linear. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paint")
	FLinearColor Color = FLinearColor::White;

	/** Its chance against the paint's other colors. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paint", meta = (ClampMin = "0"))
	float Weight = 1.f;
};

/** A material the gun's parts share, colored per gun: the roll's seed picks one of its colors. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWeaponPaint
{
	GENERATED_BODY()

	/** The parts' material slot it colors (the Blender material's name, such as GunPolymerSand). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paint")
	FName Slot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paint")
	TArray<FWeaponColorOption> Colors;
};

/** What a rolled gun is made of: the same definition, seed, rarity and saved parts always make the same gun. */
struct FWeaponLook
{
	/** One per part slot, in the definition's order; null where a slot stays empty. */
	TArray<const FWeaponPartOption*> Parts;
	/** Each paint's color, by material slot. */
	TMap<FName, FLinearColor> Colors;
};

/** A gun's parts' changes put together: percent totals per stat (capped), and the capacity and zoom its parts set. */
struct FWeaponPartTotals
{
	float Damage = 0.f;
	float Accuracy = 0.f;
	float Range = 0.f;
	float FireRate = 0.f;
	float Reload = 0.f;
	float Recoil = 0.f;
	float Handling = 0.f;
	/** 0 = no part sets it. */
	int32 Magazine = 0;
	float Zoom = 0.f;
};

namespace WeaponParts
{
	/** How far a gun's parts can move each stat, as limits on their summed percentages. */
	struct FCap
	{
		float Min;
		float Max;
	};
	inline constexpr FCap DamageCap{ -20.f, 30.f };
	inline constexpr FCap AccuracyCap{ -30.f, 60.f };
	inline constexpr FCap RangeCap{ -30.f, 60.f };
	inline constexpr FCap FireRateCap{ -20.f, 30.f };
	inline constexpr FCap ReloadCap{ -50.f, 60.f };
	inline constexpr FCap RecoilCap{ -50.f, 40.f };
	inline constexpr FCap HandlingCap{ -40.f, 40.f };

	/**
	 * A rolled gun's parts and paint. With SavedParts (one key per slot, as the gun was saved) the gun keeps those parts,
	 * whatever has been added to the lists since; a saved part that no longer exists is picked again from the seed. Without
	 * them the seed picks among the options the rarity allows that fit the parts already picked (a part's Requires names an
	 * earlier slot). Every slot and paint takes one draw whatever it allows, so a slot's pick never shifts the ones after it.
	 */
	AI_LOOTER_SHOOTER_API FWeaponLook Pick(const UWeaponDefinition& Definition, int32 Seed, EWeaponRarity Rarity,
		TConstArrayView<FName> SavedParts = {});

	/** Pick for a rolled gun, with its saved parts. */
	AI_LOOTER_SHOOTER_API FWeaponLook Pick(const FWeaponInstanceData& Instance);

	/** The look's parts by key, one per slot (none for an empty slot): what a gun saves. */
	AI_LOOTER_SHOOTER_API TArray<FName> PartKeys(const FWeaponLook& Look);

	/**
	 * The parts' changes added up: each part's percentages rolled within their ranges from the gun's seed (each part from
	 * a draw of its own, so changing one part never changes another's roll), summed per stat and capped.
	 */
	AI_LOOTER_SHOOTER_API FWeaponPartTotals CombinedStats(const FWeaponLook& Look, int32 Seed);

	/** The word the gun's parts put before its name (empty when none does). */
	AI_LOOTER_SHOOTER_API FText NamePrefix(const FWeaponLook& Look);

	/**
	 * How worn a gun looks, 0 (factory fresh) to 1 (battered), rolled from its seed: commons come scuffed and grimy,
	 * legendaries nearly clean. The gun master (M_Gun) reads it from the parts' custom primitive data WearDataIndex.
	 */
	AI_LOOTER_SHOOTER_API float Wear(const FWeaponInstanceData& Instance);
	inline constexpr int32 WearDataIndex = 0;

	/** How strongly a gun's rarity parts glow: commons faintly. */
	AI_LOOTER_SHOOTER_API float RarityGlow(EWeaponRarity Rarity);
}
