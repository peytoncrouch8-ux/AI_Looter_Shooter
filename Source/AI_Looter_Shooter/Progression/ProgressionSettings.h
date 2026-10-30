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

	/** The highest level a player can reach; experience stops counting there (65: the campaign's top, the user's call). */
	UPROPERTY(Config, EditAnywhere, Category = "Experience", meta = (ClampMin = "2", ClampMax = "500"))
	int32 MaxLevel = FXPCurve::DefaultMaxLevel;

	/**
	 * Reaching level L takes CurveScale x (L^CurvePower - 1) experience in all. Borderlands 2's curve is 60 x (L^2.8 - 1);
	 * 180 is three times it: 1,074 XP for level 2, about 21.4 million for level 65.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Experience", meta = (ClampMin = "1", ClampMax = "100000"))
	double CurveScale = FXPCurve::DefaultCurveScale;

	/**
	 * How steeply later levels grow (Borderlands 2's 2.8). Higher makes the late levels take far longer than the early
	 * ones; Looter.XP.Table prints the curve.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Experience", meta = (ClampMin = "1.0", ClampMax = "5.0"))
	double CurvePower = FXPCurve::DefaultCurvePower;

	/**
	 * The share of a creature's XPReward a kill gives at its level 1 (1: a level 1 Basic creature, XPReward 10, gives 10).
	 * Tuned with the missions' turn-in rewards so Ransom's Rest's story alone ends about level 5.5 and everything there
	 * about 6.8: Docs/Progression.md has the count, and Looter.Progression.Pacing checks it.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Kills", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	double KillXPScale = FLevelRules::DefaultKillXPScale;

	/**
	 * A kill's experience grows as the enemy's level to this power: 1.2 makes a level 5 Basic creature worth 6.9 times a
	 * level 1 one, level 60 136 times. Below the curve's own growth, so the kills a level takes rise slowly; Looter.XP.Table
	 * prints them.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Kills", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	double KillXPPower = FLevelRules::DefaultKillXPPower;

	/** The share of a kill's experience lost for each level the enemy is below the player (0.15: 85% one level below). */
	UPROPERTY(Config, EditAnywhere, Category = "Kills", meta = (ClampMin = "0", ClampMax = "1"))
	double KillXPFalloff = FLevelRules::DefaultKillXPFalloff;

	/** The least share of a kill's experience, however far below the player the enemy is. */
	UPROPERTY(Config, EditAnywhere, Category = "Kills", meta = (ClampMin = "0", ClampMax = "1"))
	double KillXPFloor = FLevelRules::DefaultKillXPFloor;

	/**
	 * Enemy health and damage grow by this share of the level below's for each level, compounding (0.09: x1.68 at 7, x2.17 at
	 * 10; the user, 2026-10-29 night). Keep it
	 * equal to the guns' DamagePerLevel (UWeaponDefinition), so a gun of an enemy's level always takes the same hits.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Levels", meta = (ClampMin = "0", ClampMax = "1"))
	float EnemyGrowthPerLevel = FLevelRules::DefaultEnemyGrowth;

	/**
	 * The first level-up reward: the player's most health grows by this share of the level below's for each level, compounding
	 * (0.06, the user's call 2026-10-30: was 0.09 as the enemies; now they slowly outscale the player, so gear and wards matter more).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Levels", meta = (ClampMin = "0", ClampMax = "1"))
	float HealthPerLevel = FLevelRules::DefaultHealthPerLevel;

	/** The curve these settings describe. */
	FXPCurve GetCurve() const;

	/** What a level is worth, as these settings say. */
	FLevelRules GetLevelRules() const;
};
