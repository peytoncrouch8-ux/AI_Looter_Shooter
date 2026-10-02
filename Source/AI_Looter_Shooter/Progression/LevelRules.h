#pragma once

#include "CoreMinimal.h"

/**
 * What a level is worth (Docs/Story.md, "What levels still mean"). An enemy's health and damage grow linearly with its
 * level, as a gun's damage grows with the gun's (UWeaponDefinition::DamagePerLevel), so a gun of the enemy's level always
 * takes the same number of hits. A kill's experience grows 8% a level and falls off for enemies below the player, so
 * farming old areas isn't the fast road. The player's own health grows with their level: the first level-up reward.
 * Plain numbers, no assets: the game reads them from UProgressionSettings, and tests build their own.
 */
struct AI_LOOTER_SHOOTER_API FLevelRules
{
	/** The rules the game ships with (UProgressionSettings starts from these). */
	static constexpr double DefaultKillXPGrowth = 1.08;
	static constexpr double DefaultKillXPFalloff = 0.15;
	static constexpr double DefaultKillXPFloor = 0.1;
	static constexpr float DefaultEnemyGrowth = 0.08f;
	static constexpr float DefaultHealthPerLevel = 0.08f;

	/** A kill's experience is this many times the last level's for each level of the enemy (1.08: 8% a level). */
	double KillXPGrowth = DefaultKillXPGrowth;

	/** The share of a kill's experience lost for each level the enemy is below the player (0.15: 15 points a level). */
	double KillXPFalloff = DefaultKillXPFalloff;

	/** The least share of a kill's experience, however far below the player the enemy is (0.1: 10%). */
	double KillXPFloor = DefaultKillXPFloor;

	/** An enemy's health and damage grow by this share of their level 1 values for each level (0.08: x1.72 at level 10). */
	float EnemyGrowth = DefaultEnemyGrowth;

	/** The player's most health grows by this share of their level 1 health for each level (0.08: +8% a level). */
	float HealthPerLevel = DefaultHealthPerLevel;

	/** An enemy's health and damage at Level against its level 1 values: 1 + EnemyGrowth x (Level - 1). */
	float EnemyScale(int32 Level) const;

	/** The player's most health at Level against their level 1 health: 1 + HealthPerLevel x (Level - 1). */
	float PlayerHealthScale(int32 Level) const;

	/**
	 * The share of its experience a kill gives: all of it for an enemy at or above the player's level, and KillXPFalloff
	 * less for each level below, never under KillXPFloor (with the defaults 85% one level below, 10% from six below).
	 */
	double KillXPShare(int32 EnemyLevel, int32 PlayerLevel) const;

	/**
	 * A kill's experience, rounded: BaseXP x KillXPGrowth^(EnemyLevel - 1) x KillXPShare. BaseXP is the creature's
	 * XPReward, which has its rank's multiplier in it already, so the rank counts once. At least 1 when BaseXP is; 0 for
	 * none (target dummies, practice).
	 */
	int64 KillXP(int32 BaseXP, int32 EnemyLevel, int32 PlayerLevel) const;
};
