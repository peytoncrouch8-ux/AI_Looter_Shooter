#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HeroLineTable.generated.h"

/**
 * One of Ellis's texts and what the other heroes read in its place (Story/HeroLines). Source is the key: Ellis's words
 * exactly as the data asset, the level or the LOCTEXT has them. A row exists only where the words name Ellis's family or
 * Ellis's own life, or where a hero has a voice of their own; every other text reads the same for all five.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FHeroLineVariant
{
	GENERATED_BODY()

	/** hero_lines.json's id for the row ("lines.SextonDeal.5"): for the log and the tests only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	FName Id;

	/** Ellis's words, exactly as the data or the LOCTEXT has them: the key. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line", meta = (MultiLine = true))
	FString Source;

	/**
	 * Who says them in Ellis's story ("Ellis", "Grandma Delia"). Set, a caption line said by someone else keeps its words
	 * (a short line like "Who?" is Ellis's only in Ellis's mouth). Empty: whoever says it, and texts that aren't lines.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	FString Speaker;

	/** What every hero but Ellis reads. Empty: Source (a row only for some heroes' own voices). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line", meta = (MultiLine = true))
	FText Other;

	/** A hero's own words over Other, by UHeroDefinition::Id ("Odessa", "Crane", "Gauge", "Pike"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	TMap<FName, FText> Heroes;

	/** Ellis's alone: another hero hears nothing here unless Heroes has their own words. Python: drop_for_others. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	bool bDropForOthers = false;
};

/**
 * Every hero variant of the story's words, as one data asset: /Game/Data/Story/DA_HeroLines, written whole by
 * Tools/Unreal/create_hero_lines.py from Tools/Unreal/hero_lines.json (the one place the variants are written). Only data:
 * HeroLines::Resolve reads it where words reach the screen.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UHeroLineTable : public UDataAsset
{
	GENERATED_BODY()

public:
	/** "/Game/Data/Story/DA_HeroLines.DA_HeroLines". */
	static const TCHAR* AssetPath();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lines")
	TArray<FHeroLineVariant> Lines;

#if WITH_EDITOR
	/** Rows changed (the script, or by hand): HeroLines looks them up afresh. */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
