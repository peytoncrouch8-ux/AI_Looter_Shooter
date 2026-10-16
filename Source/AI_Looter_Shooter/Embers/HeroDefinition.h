#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HeroDefinition.generated.h"

class UEmberDefinition;

/**
 * A playable hero as data (`/Game/Data/Embers/DA_Hero_<Id>`): the three ember powers they own from the start
 * (Docs/Story.md, "The heroes": one equipped at a time). Ellis is the only hero built; the pick between five comes with
 * the hero-select screen, when the other four get their models.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UHeroDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** "Ellis", "Odessa", "Crane", "Gauge", "Pike". */
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	FName Id;

	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	FText DisplayName;

	/** The hero's three, in the order the powers page lists them; the first is equipped in a new game. */
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	TArray<TObjectPtr<UEmberDefinition>> Powers;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("Hero"), GetFName()); }
};
