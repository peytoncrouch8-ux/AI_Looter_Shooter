#pragma once

#include "CoreMinimal.h"

/**
 * Every number of the part effects (Looter.PartEffect.* cvars, live in PIE: PartEffectTuning.cpp). The fields' values are
 * the cvars' defaults: a default-made tuning is what the tests read and a fresh game plays. Read Live() where a live change
 * matters; never keep it across frames.
 */
struct AI_LOOTER_SHOOTER_API FPartEffectTuning
{
	// Ricochet: a round that hits bounces once to the nearest creature within Radius, for Share of its damage.
	float RicochetRadius = 800.f;
	float RicochetShare = 0.6f;

	// Burst: Count rounds a pull, BurstGap times the gun's interval apart, PullGap times it between pulls.
	int32 BurstCount = 3;
	float BurstGap = 0.6f;
	float BurstPullGap = 1.8f;

	// Explosive: Direct of the round to what it hit, Splash of it round within Radius (EdgeShare of that at the edge).
	float ExplosiveRadius = 150.f;
	float ExplosiveDirect = 0.6f;
	float ExplosiveSplash = 0.4f;
	float ExplosiveEdgeShare = 0.3f;

	// Piercing: through this many creatures, keeping Share of its damage past each.
	int32 PiercingCount = 1;
	float PiercingShare = 0.85f;

	// Overcharge: a pull winds up for Seconds, then fires at Damage times, the gun cycling at FireRate of its rate.
	float OverchargeSeconds = 0.6f;
	float OverchargeDamage = 2.f;
	float OverchargeFireRate = 0.5f;
	/** A pull let go early fires at the charge it reached, at least this much of the way (0: a plain round). */
	float OverchargeMinCharge = 0.f;

	// Twin: two bullets a shot for one round, each Share of a round's damage, Angle degrees apart.
	float TwinShare = 0.65f;
	float TwinAngle = 0.8f;

	/** The tuning as the cvars hold it now. */
	static FPartEffectTuning Live();
};
