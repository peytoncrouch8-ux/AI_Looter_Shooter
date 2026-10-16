#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EmberDefinition.generated.h"

class UEmberPower;

/**
 * One ember power as data (`/Game/Data/Embers/DA_Ember_<Id>`; Tools/Unreal/create_ember_assets.py writes them): its
 * words, its look on the HUD and the powers page, its clock and its numbers. What it does is C++: PowerClass
 * (Embers/Powers/*). The numbers are the demo's starting points (Docs/Handoffs/CloudIslandConcepts_2026-10-05.md,
 * "Ember powers" and "Five heroes"), tuned in play; damage grows with the player's level the way a gun's does.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UEmberDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Never changes once saved (the session saves the equipped power by it): "DustDevil", "Sundown", "BorrowedTime". */
	UPROPERTY(EditDefaultsOnly, Category = "Ember")
	FName Id;

	UPROPERTY(EditDefaultsOnly, Category = "Ember")
	FText DisplayName;

	/** Its nature, one word, as the powers page shows it: Wind, Dusk, Debt, Fortune, Forge, Sight... */
	UPROPERTY(EditDefaultsOnly, Category = "Ember")
	FText Nature;

	/** What it does, for the powers page (two or three plain sentences with its numbers). */
	UPROPERTY(EditDefaultsOnly, Category = "Ember", meta = (MultiLine = true))
	FText Description;

	/** Its colour on the HUD (the socket's active arc and glow) and its FX (LooterUI::Color has each power's). */
	UPROPERTY(EditDefaultsOnly, Category = "Ember")
	FLinearColor Color = FLinearColor::White;

	/** The Inked icon drawn in its socket and on its page: InkedIconData's function name ("DustDevil"). */
	UPROPERTY(EditDefaultsOnly, Category = "Ember")
	FName IconName;

	/** Seconds from a use until it can be used again (counted from the use, not from the end of its duration). */
	UPROPERTY(EditDefaultsOnly, Category = "Ember|Clock", meta = (ClampMin = "0"))
	float CooldownSeconds = 10.f;

	/** How long it runs (0: instant, over the moment it's used). */
	UPROPERTY(EditDefaultsOnly, Category = "Ember|Clock", meta = (ClampMin = "0"))
	float DurationSeconds = 0.f;

	/** The power's own numbers; each PowerClass documents what it reads. Against a level 1 rifle's 14 a shot. */
	UPROPERTY(EditDefaultsOnly, Category = "Ember|Numbers")
	float Damage = 0.f;

	/** A reach (cm): a dash's length, a lob's range, a mark's radius. */
	UPROPERTY(EditDefaultsOnly, Category = "Ember|Numbers")
	float Range = 0.f;

	/** An area (cm): what the power touches round its path or point. */
	UPROPERTY(EditDefaultsOnly, Category = "Ember|Numbers")
	float Radius = 0.f;

	/** A share or a rate: a slow's strength, a dilation, a debt's share struck off per kill. */
	UPROPERTY(EditDefaultsOnly, Category = "Ember|Numbers")
	float Strength = 0.f;

	/** Anything else a power needs, by name ("Dizzy" seconds, "Fling" cm); read with Extra(Name, Default). */
	UPROPERTY(EditDefaultsOnly, Category = "Ember|Numbers")
	TMap<FName, float> Extras;

	/** What it does (Embers/Powers/*): made once per component, per power. */
	UPROPERTY(EditDefaultsOnly, Category = "Ember")
	TSubclassOf<UEmberPower> PowerClass;

	float Extra(FName Name, float Default) const
	{
		const float* Found = Extras.Find(Name);
		return Found ? *Found : Default;
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("Ember"), GetFName()); }
};
