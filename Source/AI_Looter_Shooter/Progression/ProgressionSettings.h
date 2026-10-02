#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Progression/LevelRules.h"
#include "Progression/XPCurve.h"
#include "ProgressionSettings.generated.h"

/**
 * The experience curve and what a level is worth (kill experience, enemy growth, the player's health), tunable without
 * code: Project Settings > Game > Progression, saved to Config/DefaultGame.ini under
 * [/Script/AI_Looter_Shooter.ProgressionSettings]. Changes apply from the next play session. Saved players keep their
 * level when these change (see FXPCurve::Clamp).
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

	/**
	 * A kill's experience grows by this factor with each level of the enemy: a level 1 creature gives its XPReward (10),
	 * and 1.08 makes a level 20 one worth 43. Below the level curve's growth, so kills per level rise slowly (10 at level
	 * 1, about 20 at level 20, about 60 at level 50); Looter.XP.Table prints them.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Kills", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	double KillXPGrowth = FLevelRules::DefaultKillXPGrowth;

	/** The share of a kill's experience lost for each level the enemy is below the player (0.15: 85% one level below). */
	UPROPERTY(Config, EditAnywhere, Category = "Kills", meta = (ClampMin = "0", ClampMax = "1"))
	double KillXPFalloff = FLevelRules::DefaultKillXPFalloff;

	/** The least share of a kill's experience, however far below the player the enemy is. */
	UPROPERTY(Config, EditAnywhere, Category = "Kills", meta = (ClampMin = "0", ClampMax = "1"))
	double KillXPFloor = FLevelRules::DefaultKillXPFloor;

	/**
	 * Enemy health and damage grow by this share of their level 1 values for each level (0.08: x1.72 at level 10). Keep it
	 * equal to the guns' DamagePerLevel (UWeaponDefinition), so a gun of an enemy's level always takes the same hits.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Levels", meta = (ClampMin = "0", ClampMax = "1"))
	float EnemyGrowthPerLevel = FLevelRules::DefaultEnemyGrowth;

	/**
	 * The first level-up reward: the player's most health grows by this share of their level 1 health for each level
	 * (0.08: +8% a level), keeping pace with the enemies' damage.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Levels", meta = (ClampMin = "0", ClampMax = "1"))
	float HealthPerLevel = FLevelRules::DefaultHealthPerLevel;

	/** The curve these settings describe. */
	FXPCurve GetCurve() const;

	/** What a level is worth, as these settings say. */
	FLevelRules GetLevelRules() const;
};
