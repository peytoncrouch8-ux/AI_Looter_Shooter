#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Shields/ShieldTypes.h"
#include "ShieldDefinition.generated.h"

class UStaticMesh;

/** How a rarity tier moves a ward's numbers, and how it looks. */
USTRUCT(BlueprintType)
struct FShieldRarityInfo
{
	GENERATED_BODY()

	/** Relative chance of the tier where a ward rolls its own (a gift; drops take the gun pick's rarity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float CapacityMultiplier = 1.f;

	/** Below 1: it wakes sooner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float DelayMultiplier = 1.f;

	/** On the refill rate as a share of the capacity: above 1 refills faster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float RateMultiplier = 1.f;

	/** The chance a ward of this tier rolls a trait (Rare and up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0", ClampMax = "1"))
	float TraitChance = 0.f;

	/** How strong its trait is (FShieldTuning's Trait.* numbers times this). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float TraitStrength = 1.f;

	/** The rarity's colour: the guns' own, so a Rare ward reads as a Rare gun does. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity")
	FLinearColor Color = FLinearColor::White;

	/** The ward box's glow in this tier (its coal window, WardGlow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rarity", meta = (ClampMin = "0"))
	float Glow = 2.f;
};

/**
 * A kind of ward (the player's recharging shield, the user's pick of 2026-10-22): its base numbers and how rarity, level
 * and build move them, its look on the ground. One kind for now, the Ward Box (/Game/Data/Shields/DA_Shield_WardBox, from
 * Tools/Unreal/create_shields.py): a belt-worn iron box holding a soul-coal behind glass, which throws a ward round its
 * wearer that soaks hits before their health does and fills again after a while unhurt.
 * The defaults below are the shipped numbers, so the game works before the asset exists (GetWardBox falls back to them).
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UShieldDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShieldDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** The ward box: the asset, or (before create_shields.py has made it) the class's defaults. Never null. */
	static UShieldDefinition* GetWardBox();

	/** Where the asset lives (create_shields.py writes it there). */
	static const TCHAR* WardBoxPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
	FText DisplayName;

	/** A Common ward's capacity at level 1: 40, against the player's 100 health. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield", meta = (ClampMin = "1"))
	float BaseCapacity = 40.f;

	/** Capacity grows by this share of its level-1 value each level: the player's health's 8%, so the two stay in step. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield", meta = (ClampMin = "0"))
	float CapacityPerLevel = 0.08f;

	/** Seconds unhurt before a Common ward refills. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield", meta = (ClampMin = "0", Units = "s"))
	float BaseRechargeDelay = 3.2f;

	/** A Common ward refills this share of its capacity a second (0.28: empty to full in about 3.6 s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield", meta = (ClampMin = "0.01"))
	float BaseRechargeShare = 0.28f;

	/** Each roll moves each number up to this share either way (seeded). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield", meta = (ClampMin = "0", ClampMax = "0.5"))
	float Variance = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
	TMap<EWeaponRarity, FShieldRarityInfo> RarityTable;

	/**
	 * The ward box's model per rarity (SM_WardBox_<Rarity> from Art/Models/Shields/WardBox.py: the Common's plain iron box
	 * gains brass corners, a gauge, an etched sigil and a saint's medal as it climbs). A missing one uses the Common's,
	 * then the engine's cube.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
	TMap<EWeaponRarity, TSoftObjectPtr<UStaticMesh>> Models;

	/** The model's material slot that glows in the rarity's colour (the coal window). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
	FName GlowSlot = TEXT("WardGlow");

	/** Its Inked icon (Art/Icons/InkedIcons.py; LooterUI::FindInkedIcon). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
	FName IconName = TEXT("WardBox");

	const FShieldRarityInfo& GetRarityInfo(EWeaponRarity Rarity) const;

	/** Rarity's model, or the Common's, or null (the pickup then shows the engine's cube). Loads it. */
	UStaticMesh* LoadModel(EWeaponRarity Rarity) const;
};
