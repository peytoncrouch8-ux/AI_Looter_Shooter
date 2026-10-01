#pragma once

#include "CoreMinimal.h"

/**
 * How much experience each level takes. Going from level L to L+1 takes round(BaseXP x Growth^(L-1)): 100 for the
 * first level-up, then each one Growth times the last (exponential, so later levels take far longer than early ones).
 * At MaxLevel the climb ends and experience stops counting. Plain numbers, no assets: the game reads them from
 * UProgressionSettings (Project Settings > Game > Progression), and tests build their own.
 */
struct AI_LOOTER_SHOOTER_API FXPCurve
{
	/** The rules the game ships with (UProgressionSettings starts from these). */
	static constexpr int32 DefaultMaxLevel = 70;
	static constexpr int64 DefaultBaseXP = 100;
	static constexpr double DefaultGrowth = 1.12;

	int32 MaxLevel = DefaultMaxLevel;
	int64 BaseXP = DefaultBaseXP;
	double Growth = DefaultGrowth;

	/** Experience needed to go from Level to Level + 1; 0 at MaxLevel and above (there is no next level). */
	int64 XPToNextLevel(int32 Level) const;

	/** Experience earned in total by the time a new player reaches Level (0 for level 1). */
	int64 TotalXPToReach(int32 Level) const;

	bool IsMaxLevel(int32 Level) const { return Level >= MaxLevel; }

	/** How far through Level the player is, 0 to 1. Always 1 at MaxLevel, where the bar shows full. */
	float LevelProgress(int32 Level, int64 XP) const;

	/**
	 * Adds Amount to a player at Level with XP into it, leveling up as many times as it covers. The remainder carries
	 * into the new level; at MaxLevel any excess is dropped and XP stays 0. Returns the number of levels gained.
	 */
	int32 ApplyXP(int32& Level, int64& XP, int64 Amount) const;

	/**
	 * Brings a saved level and XP inside the curve's rules, in case they changed since it was saved: the level within
	 * 1..MaxLevel, and the XP below what the level needs (the player keeps their level, never loses or gains one).
	 */
	void Clamp(int32& Level, int64& XP) const;
};
