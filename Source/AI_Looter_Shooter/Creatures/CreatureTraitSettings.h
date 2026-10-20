#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Creatures/CreatureTraitTuning.h"
#include "CreatureTraitSettings.generated.h"

/**
 * The creature traits' numbers, tunable without code: Project Settings > Game > Creature Traits, saved to
 * Config/DefaultGame.ini under [/Script/AI_Looter_Shooter.CreatureTraitSettings]. The defaults (CreatureTraitTuning.h) are
 * the approved design's; how many a creature carries is its tier's (FCreatureRankInfo::TraitCount).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Creature Traits"))
class AI_LOOTER_SHOOTER_API UCreatureTraitSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCreatureTraitSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Traits")
	FFleetTraitTuning Fleet;

	UPROPERTY(Config, EditAnywhere, Category = "Traits")
	FMendingTraitTuning Mending;

	UPROPERTY(Config, EditAnywhere, Category = "Traits")
	FWardedTraitTuning Warded;

	UPROPERTY(Config, EditAnywhere, Category = "Traits", meta = (DisplayName = "Grave-burst"))
	FGraveBurstTraitTuning GraveBurst;

	UPROPERTY(Config, EditAnywhere, Category = "Traits")
	FRallyingTraitTuning Rallying;

	UPROPERTY(Config, EditAnywhere, Category = "Traits")
	FVengefulTraitTuning Vengeful;

	UPROPERTY(Config, EditAnywhere, Category = "Traits")
	FPhasingTraitTuning Phasing;

	/** A trait's looks are drawn only with a player this near (cm): past it nobody could read them. */
	UPROPERTY(Config, EditAnywhere, Category = "Look", meta = (ClampMin = "0", Units = "cm"))
	float LookRange = 9000.f;

	/** The project's settings. */
	static const UCreatureTraitSettings& Get();
};
