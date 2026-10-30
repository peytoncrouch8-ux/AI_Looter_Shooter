#pragma once

#include "CoreMinimal.h"

/**
 * Every number of the unique legendaries (Looter.Legendary.<Name>.* cvars, live in PIE: LegendaryTuning.cpp). The fields'
 * values are the cvars' defaults: a default-made tuning is what the tests read and a fresh game plays. Read Live() where a
 * live change matters; never keep it across frames.
 */
struct AI_LOOTER_SHOOTER_API FLegendaryTuning
{
	// Gravesilk (the Gravemother's): each hit lays a stack of silk; Stacks of them root the target for RootSeconds, walking
	// at RootShare of its speed (the movement slow's floor), never a boss. Silk falls away SilkSeconds after the last hit,
	// and none takes for RootGap after a root ends, so steady fire can't hold one down for good.
	float GravesilkDropChance = 0.1f;
	int32 GravesilkStacks = 6;
	float GravesilkRootSeconds = 1.f;
	float GravesilkRootShare = 0.05f;
	float GravesilkSilkSeconds = 4.f;
	float GravesilkRootGap = 1.5f;

	// Company Scrip (the Foreman's): each kill in one burst adds FireRatePerStack to its fire rate, up to MaxStacks, all
	// gone when the trigger is let go (or no shot for BurstGap s: a reload, a swap); a reload pays RefundShare of the
	// magazine back to the pool (never more than it took).
	float CompanyScripDropChance = 0.1f;
	float CompanyScripFireRatePerStack = 0.04f;
	int32 CompanyScripMaxStacks = 8;
	float CompanyScripRefundShare = 0.2f;
	float CompanyScripBurstGap = 0.5f;

	// Lamplighter (Abel's): a kill with the sights up (the aim at least ScopedFrom of the way raised at the shot) leaves a
	// lantern on the ground that heals the player HealShare of their max health a second within Radius cm for Seconds; at
	// most MaxLanterns burn at once (the oldest goes out) and they never stack on one player.
	float LamplighterDropChance = 0.1f;
	float LamplighterHealShare = 0.02f;
	float LamplighterRadius = 400.f;
	float LamplighterSeconds = 5.f;
	float LamplighterScopedFrom = 0.6f;
	int32 LamplighterMaxLanterns = 3;

	/** The tuning as the cvars hold it now. */
	static FLegendaryTuning Live();
};
