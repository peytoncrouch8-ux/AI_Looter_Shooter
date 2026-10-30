#pragma once

#include "CoreMinimal.h"

/**
 * What a level is worth (Docs/Story.md, "What levels still mean"). An enemy's health and damage grow with its level,
 * compounding (+9% a level, the user's call 2026-10-29 night; was +8% of level 1's, linear), as a gun's damage grows with
 * the gun's (UWeaponDefinition::DamagePerLevel, FLevelRules::Compounded), so a gun of the enemy's level always
 * takes the same number of hits. A kill's experience grows with the enemy's level (a little faster than in step with it)
 * and falls off for enemies below the player, so farming old areas isn't the fast road. The player's own health grows with
 * their level: the first level-up reward. Plain numbers, no assets: the game reads them from UProgressionSettings, and
 * tests build their own.
 */
struct AI_LOOTER_SHOOTER_API FLevelRules
{
	/** The rules the game ships with (UProgressionSettings starts from these). */
	static constexpr double DefaultKillXPScale = 1.0;
	static constexpr double DefaultKillXPPower = 1.2;
	static constexpr double DefaultKillXPFalloff = 0.15;
	static constexpr double DefaultKillXPFloor = 0.1;
	static constexpr float DefaultEnemyGrowth = 0.09f;
	static constexpr float DefaultHealthPerLevel = 0.09f;

	/**
	 * The share of a creature's XPReward a kill gives at its level 1 (1: all of it; a Basic creature's 10 gives 10, as a
	 * Borderlands kill does early on). The user's call of 2026-10-13: kills give real numbers again and the levels take more
	 * (FXPCurve), the turn-ins carrying most of a level as in Borderlands (Docs/Progression.md). 0.1 for a day before that,
	 * 0.37 and 0.4 on the old curve.
	 */
	double KillXPScale = DefaultKillXPScale;

	/**
	 * A kill's experience grows as the enemy's level to this power: 1.2 makes a level 5 Basic creature worth 69, level 7
	 * 103, level 60 1,361 (XPReward 10). Slower than the curve's levels grow (about level^1.8), so the kills a level takes
	 * climb from about 110 at level 1 to 600 at 60 and the turn-ins carry more of it; Looter.XP.Table prints them.
	 */
	double KillXPPower = DefaultKillXPPower;

	/** The share of a kill's experience lost for each level the enemy is below the player (0.15: 15 points a level). */
	double KillXPFalloff = DefaultKillXPFalloff;

	/** The least share of a kill's experience, however far below the player the enemy is (0.1: 10%). */
	double KillXPFloor = DefaultKillXPFloor;

	/** An enemy's health and damage grow by this share of the level below's for each level (0.09: x1.68 at 7, x2.17 at 10). */
	float EnemyGrowth = DefaultEnemyGrowth;

	/**
	 * The player's most health grows by this share of the level below's for each level, compounding (0.09), so it keeps the
	 * pace of the enemies' damage (the 2026-10-02 rule, kept when the user made enemies compound +9%, 2026-10-29 night).
	 */
	float HealthPerLevel = DefaultHealthPerLevel;

	/** An enemy's health and damage at Level against its level 1 values: (1 + EnemyGrowth) ^ (Level - 1). */
	float EnemyScale(int32 Level) const;

	/**
	 * Growth compounded over the levels past 1: (1 + Growth) ^ (Level - 1), 1 at level 1 and below (the user, 2026-10-29
	 * night: "+9% a level, compounding", for gun damage, creature health and damage, and ward capacity alike, so a gun of an
	 * enemy's level always takes the same hits). A negative growth counts as none.
	 */
	static float Compounded(float Growth, int32 Level);

	/** The player's most health at Level against their level 1 health: (1 + HealthPerLevel) ^ (Level - 1). */
	float PlayerHealthScale(int32 Level) const;

	/**
	 * The share of its experience a kill gives: all of it for an enemy at or above the player's level, and KillXPFalloff
	 * less for each level below, never under KillXPFloor (with the defaults 85% one level below, 10% from six below).
	 */
	double KillXPShare(int32 EnemyLevel, int32 PlayerLevel) const;

	/**
	 * A kill's experience, rounded: BaseXP x KillXPScale x EnemyLevel^KillXPPower x KillXPShare. BaseXP is the
	 * creature's XPReward, which has its rank's multiplier in it already, so the rank counts once. At least 1 when BaseXP
	 * is; 0 for none (target dummies, practice).
	 */
	int64 KillXP(int32 BaseXP, int32 EnemyLevel, int32 PlayerLevel) const;
};
