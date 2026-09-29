#pragma once

#include "CoreMinimal.h"

/** Game-wide combat rules. One place, so every weapon and every target follows the same numbers. */
namespace LooterCombat
{
	/** A hit on any killable target's critical spot deals this multiple of what the hit would otherwise deal. */
	inline constexpr float CriticalHitMultiplier = 1.5f;

	/**
	 * Every hit rolls its damage uniformly within +/- this fraction of the listed damage (0.1 = 90%..110%), so
	 * repeated hits read as a spread of numbers instead of the same one. The listed damage is the average.
	 */
	inline constexpr float DamageVariance = 0.1f;

	/** Lowest and highest damage one non-critical hit can deal, for stat cards. */
	inline float MinHitDamage(float BaseDamage) { return BaseDamage * (1.f - DamageVariance); }
	inline float MaxHitDamage(float BaseDamage) { return BaseDamage * (1.f + DamageVariance); }

	/**
	 * Final damage of one pellet/bullet/bite. Roll in [0, 1] picks the spot in the damage range (0 = lowest,
	 * 0.5 = listed damage, 1 = highest); a critical hit multiplies that same roll by CriticalHitMultiplier.
	 */
	inline float HitDamage(float BaseDamage, bool bCritical, float Roll)
	{
		const float Scale = FMath::Lerp(1.f - DamageVariance, 1.f + DamageVariance, FMath::Clamp(Roll, 0.f, 1.f));
		return BaseDamage * Scale * (bCritical ? CriticalHitMultiplier : 1.f);
	}

	/** HitDamage with a fresh random roll. */
	inline float RollHitDamage(float BaseDamage, bool bCritical)
	{
		return HitDamage(BaseDamage, bCritical, FMath::FRand());
	}
}
