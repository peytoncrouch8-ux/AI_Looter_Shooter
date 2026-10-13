#pragma once

#include "CoreMinimal.h"

/**
 * How much experience each level takes: Borderlands 2's curve, steeper. Reaching level L takes CurveScale x
 * (L^CurvePower - 1) experience in all, rounded; Borderlands 2's is 60 x (L^2.8 - 1), ours three times that (the user's
 * call of 2026-10-13: levelling takes more, so a campaign of many areas reaches the top level only near its end). Each
 * level asks for more than the last (1,074 for level 2, 18,967 from 7 to 8, about 812,000 from 60 to 61), and the top is
 * level 65, about 21.4 million in all. At MaxLevel the climb ends and experience stops counting. Plain numbers, no
 * assets: the game reads them from UProgressionSettings (Project Settings > Game > Progression), and tests build their own.
 * Docs/Progression.md has the table and the reasoning.
 */
struct AI_LOOTER_SHOOTER_API FXPCurve
{
	/** The rules the game ships with (UProgressionSettings starts from these). */
	static constexpr int32 DefaultMaxLevel = 65;
	static constexpr double DefaultCurveScale = 180.0;
	static constexpr double DefaultCurvePower = 2.8;

	int32 MaxLevel = DefaultMaxLevel;
	/** The experience to reach level L is CurveScale x (L^CurvePower - 1). */
	double CurveScale = DefaultCurveScale;
	double CurvePower = DefaultCurvePower;

	/** Experience needed to go from Level to Level + 1; 0 at MaxLevel and above (there is no next level). */
	int64 XPToNextLevel(int32 Level) const;

	/** Experience earned in total by the time a new player reaches Level (0 for level 1; MaxLevel's above it). */
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
