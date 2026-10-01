#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Progression/XPCurve.h"
#include "ProgressionSettings.generated.h"

/**
 * The experience curve, tunable without code: Project Settings > Game > Progression, saved to Config/DefaultGame.ini
 * under [/Script/AI_Looter_Shooter.ProgressionSettings]. Changes apply from the next play session. Saved players keep
 * their level when these change (see FXPCurve::Clamp).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Progression"))
class AI_LOOTER_SHOOTER_API UProgressionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UProgressionSettings();

	/** The highest level a player can reach; experience stops counting there. */
	UPROPERTY(Config, EditAnywhere, Category = "Experience", meta = (ClampMin = "2", ClampMax = "500"))
	int32 MaxLevel = FXPCurve::DefaultMaxLevel;

	/** Experience to go from level 1 to level 2. */
	UPROPERTY(Config, EditAnywhere, Category = "Experience", meta = (ClampMin = "1"))
	int64 BaseXP = FXPCurve::DefaultBaseXP;

	/**
	 * Each level-up takes this many times the experience of the one before (1.12 = 12% more). With 100 base XP and 70
	 * levels, 1.12 makes the whole climb about 2.07 million XP; 1.10 about 0.72 million, 1.15 about 10.3 million.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Experience", meta = (ClampMin = "1.01", ClampMax = "2.0"))
	double Growth = FXPCurve::DefaultGrowth;

	/** The curve these settings describe. */
	FXPCurve GetCurve() const;
};
