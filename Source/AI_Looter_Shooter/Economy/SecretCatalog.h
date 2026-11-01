#pragma once

#include "CoreMinimal.h"
#include "Economy/SecretRules.h"
#include "Engine/DataAsset.h"
#include "Story/StoryCondition.h"
#include "SecretCatalog.generated.h"

class UMissionRunner;
struct FCampaignRecord;

/**
 * A level's secrets for sale (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 12; lanes/SECRETS.md): Mister Sexton's on
 * Ransom's Rest, /Game/Data/Secrets/DA_Secrets_RansomsRest, written by Tools/Unreal/create_secret_assets.py from
 * story_texts.SEXTON_SECRETS. Data only: USecretSubsystem finds it by the level's name (SecretRules::CatalogPathFor), the
 * broker (USecretBrokerComponent) opens it when OpenWhen holds, and the screen (USecretsWidget) lists it.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API USecretCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** The screen's title ("The Ferryman's Secrets") and its panel's tab ("Mister Sexton"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secrets")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secrets")
	FText Tab;

	/** When he trades at all (after Main 2: his turn-in says "Come back when you've silver"); empty: always. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secrets")
	FStoryCondition OpenWhen;

	/** Done, the prices are Discount less (Paid on Delivery: "his secrets cost a quarter less from now on"). None: never. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secrets")
	FName DiscountMission;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secrets", meta = (ClampMin = "0", ClampMax = "1"))
	float Discount = SecretRules::DefaultDiscount;

	/** His line over the list: the first whose condition holds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secrets")
	TArray<FSecretGreeting> Greetings;

	/** His line when nothing is left to sell. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secrets")
	FText SoldOut;

	/** Everything he sells here, in the list's order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secrets")
	TArray<FSecretOffer> Secrets;

	/** The offer with Id, or null. */
	const FSecretOffer* FindOffer(FName Id) const;

	/** He trades in Record's story now. */
	bool IsOpen(const FCampaignRecord& Record, const UMissionRunner* Runner = nullptr) const;

	/** The share off the prices in Record's story: Discount once DiscountMission is done, else 0. */
	float DiscountFor(const FCampaignRecord& Record) const;

	/** His line for Record's story (the first greeting that holds), or empty. */
	FText GreetingFor(const FCampaignRecord& Record, const UMissionRunner* Runner = nullptr) const;
};
