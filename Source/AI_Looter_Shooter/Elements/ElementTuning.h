#pragma once

#include "CoreMinimal.h"

/**
 * Every number of the elements (Looter.Element.* cvars, live in PIE: ElementTuning.cpp). The fields' values are the cvars'
 * defaults, so a default-made tuning is what the rules' tests read and what a fresh game plays. Main bakes the user's
 * tuned values in here later. Read Live() where a live change matters; never keep it across frames.
 */
struct AI_LOOTER_SHOOTER_API FElementTuning
{
	// --- The weakness table (ElementRules::Affinity): times an effect's damage and chance, and an element gun's hit (HitScale) ---

	/** A creature weak to the element (2, was 1.75: the user's wider gap, 2026-10-29). An element gun's own hit too. */
	float Weak = 2.f;
	/** A creature that resists it (0.33, was 0.5). */
	float Resist = 0.33f;
	/** An element gun's hit on a creature immune to it, times (it sets no status at all). */
	float ImmuneHit = 0.1f;

	// --- Typed wards (UCreatureTraitComponent): a hit into a ward, times the larger of these that applies (never both) ---

	/** The ward's own element breaks it (a shock ward to Thunder). */
	float WardMatch = 2.f;
	/** Wardbreak on any other ward (the name). */
	float WardBreakAny = 1.5f;

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

	// --- Thunder: a jolt (the user's pick, 2026-10-29) ---

	/** The jolt's burst: this share of the hit that set it, ThunderDelay seconds later. */
	float ThunderShare = 0.35f;
	float ThunderDelay = 0.2f;
	/** The burst staggers the creature this long (Basic rank; never a boss or a Soulfed). */
	float ThunderStaggerSeconds = 0.5f;
	/** It arcs to the nearest living creature within this (cm), for ChainShare of its burst (damage only, no status). */
	float ThunderChainRadius = 350.f;
	float ThunderChainShare = 0.5f;
	/** Jolts waiting on one creature; one past it joins the latest. */
	int32 ThunderMaxPending = 4;

	// --- Rot: a rot (the user's pick, 2026-10-29) ---

	/** The rot deals this share of the hit that set it, over RotSeconds, its rate growing from half the mean to one and a half. */
	float RotShare = 0.5f;
	float RotSeconds = 6.f;
	/** When a rotting creature dies, every living creature within this (cm) is set rotting... */
	float RotSpreadRadius = 300.f;
	/** ...as by a hit of this share of the strongest hit that rotted the dead one. */
	float RotSpreadShare = 0.6f;

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
