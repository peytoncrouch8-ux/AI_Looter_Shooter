#pragma once

#include "CoreMinimal.h"

/**
 * Every number of the elements (Looter.Element.* cvars, live in PIE: ElementTuning.cpp). The fields' values are the cvars'
 * defaults, so a default-made tuning is what the rules' tests read and what a fresh game plays. Main bakes the user's
 * tuned values in here later. Read Live() where a live change matters; never keep it across frames.
 */
struct AI_LOOTER_SHOOTER_API FElementTuning
{
	// --- The weakness table's two sides (ElementRules::Affinity): times an effect's damage and its chance ---

	/** A creature weak to the element. */
	float Weak = 1.75f;
	/** A creature that resists it. */
	float Resist = 0.5f;

	// --- How often a hit sets its status, by the gun's kind (per bullet: a shotgun's per pellet) ---

	float ChanceRifle = 0.2f;
	float ChanceShotgun = 0.07f;
	float ChanceRevolver = 0.28f;
	float ChanceSMG = 0.15f;
	float ChanceSniper = 0.35f;
	float ChanceLMG = 0.17f;
	float ChanceLauncher = 0.5f;
	/** Added for each rarity step above Uncommon (a Legendary's +3 steps). */
	float ChanceRarityStep = 0.02f;
	/** The most any hit's chance can be, after the weakness. */
	float ChanceMax = 0.9f;

	// --- Ember: a burn ---

	/** The burn deals this share of the hit that set it, over EmberSeconds. */
	float EmberShare = 0.4f;
	float EmberSeconds = 3.f;
	/** Burns at once on one creature; another past it replaces the weakest one left, if it's stronger. */
	int32 EmberMaxStacks = 3;

	// --- Soul-light: a smite ---

	/** The smite's burst: this share of the hit that set it, SoulLightDelay seconds later. */
	float SoulLightShare = 0.6f;
	float SoulLightDelay = 1.f;
	/** It leaps to the nearest of the dead within this (cm), for ChainShare of its burst. */
	float SoulLightChainRadius = 400.f;
	float SoulLightChainShare = 0.5f;
	/** Smites waiting on one creature; one past it adds to the latest. */
	int32 SoulLightMaxPending = 4;

	// --- Grave-salt: salted ---

	/** The salt deals this share of the hit that set it, over GraveSaltSeconds. */
	float GraveSaltShare = 0.25f;
	float GraveSaltSeconds = 4.f;
	/** A salted creature walks at this share of its speed (0.7: a 30% slow; never a boss). */
	float GraveSaltSlow = 0.7f;
	/** A bullet hit on a salted creature's standing ward counts this many times (salted wards break faster). */
	float GraveSaltWardBonus = 1.5f;

	// --- Everyone ---

	/** Bosses carry a status this share of its time (and a smite's burst this share of its damage). */
	float BossDuration = 0.5f;
	/** Damage over time comes off in pieces this often (s). */
	float TickSeconds = 0.5f;
	/** An element gun's tracer is mixed this far toward its element's colour (0: the gun's own). */
	float TracerTint = 0.55f;
	/** The statuses' and hits' looks, times (0 hides them). */
	float LookScale = 1.f;

	/** The tuning as the cvars hold it now. */
	static FElementTuning Live();
};
