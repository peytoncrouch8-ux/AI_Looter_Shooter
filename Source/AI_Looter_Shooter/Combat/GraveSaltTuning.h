#pragma once

#include "CoreMinimal.h"

/**
 * The grave-salt grenade's tuning: the burst's damage and reach, how weak it starts, how many the first gun brings, and
 * how big its explosion reads. The Looter.Grenade.* console variables set them (FGraveSaltTuning::Current, read each
 * time: a live change counts at the next throw or burst); a default-made one holds the defaults, which the tests read.
 * The flight's numbers stay FGraveSaltRules' constants (the arc and the bounce are physics, not feel).
 *
 * The user's playtest (2026-10-27, note 7): "Grenades are too strong to start with, projectiles are too small and lacks
 * explosion visuals." So the burst starts at EarlyShare of its strength at level 1 and grows to full by FullStrengthLevel
 * (on top of the level growth every enemy and gun has), the first gun brings one jar, not two, and the burst is a real
 * explosion (fire, a shock ring, a smoke column, debris, a scorch) sized to the radius.
 */
struct AI_LOOTER_SHOOTER_API FGraveSaltTuning
{
	// --- The burst (Looter.Grenade.*) ---

	/** Looter.Grenade.Damage: the burst's heart at full strength at level 1 (a Common rifle's magazine is 30 x 20 = 600). */
	float BaseDamage = 900.f;
	/** Looter.Grenade.Radius / .CoreRadius (cm): nothing past the radius; full within the core. */
	float Radius = 400.f;
	float CoreRadius = 100.f;
	/** Looter.Grenade.EdgeShare: the share left at the radius (straight down from the core's edge). */
	float EdgeShare = 0.2f;
	/** Looter.Grenade.UnpaidScale: salt burns the dead (half as much again). */
	float UnpaidScale = 1.5f;
	/** Looter.Grenade.BossCap: a boss takes at most this share of its health from one burst. */
	float BossShareCap = 0.08f;
	/** Looter.Grenade.KnockShare: bodies within this share of the radius are knocked away. */
	float KnockShare = 0.6f;

	/**
	 * Looter.Grenade.EarlyShare / .FullLevel: the burst's strength at the player's level 1, growing straight to full by
	 * FullStrengthLevel. 0.45 at level 1 puts the heart at 405, about two thirds of a Common rifle magazine: a Basic spider
	 * (300) at its heart still goes, a pack at its edge doesn't. Ransom's Rest (levels 1-7) sees 45-75% of it.
	 */
	float EarlyShare = 0.45f;
	int32 FullStrengthLevel = 12;

	/** Looter.Grenade.Starting: the jars the first gun brings (three can be carried, FThrowRules::MaxGrenades). */
	int32 StartingGrenades = 1;

	// --- The explosion (Looter.Grenade.Look.*) ---

	/** Looter.Grenade.Look.Scale: the explosion's size against the burst's radius (1: its fire, ring and dust fill it). */
	float LookScale = 1.f;
	/** Looter.Grenade.Look.Ring / .RingLife / .RingWidth: the shock ring racing over the ground to the radius (0: none). */
	float RingGlow = 7.f;
	float RingLife = 0.45f;
	float RingWidth = 0.07f;
	/** Looter.Grenade.Look.Salt: the white salt and embers flung on top of the fire (the salt's own look; 0 none). */
	float SaltLayer = 1.f;
	/** Looter.Grenade.Look.Scorch: the scorch left on the ground, its size a share of the radius (0: none). */
	float ScorchShare = 0.6f;
	/** Looter.Grenade.Look.GroundReach (cm): a burst this near the ground (under it) is on it: ring, debris, scorch. */
	float GroundReach = 90.f;
	/** Looter.Grenade.Look.Flash / .FlashRadius / .FlashTime: the burst's point light (no shadows), its reach and fade. */
	float FlashCandelas = 320.f;
	float FlashRadius = 1100.f;
	float FlashSeconds = 0.25f;

	/** The tuning as the cvars hold it now. */
	static FGraveSaltTuning Current();
};
