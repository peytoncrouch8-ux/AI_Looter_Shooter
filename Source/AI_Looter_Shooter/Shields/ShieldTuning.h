#pragma once

#include "CoreMinimal.h"

/**
 * Every feel number of the player's ward (SHIELD, 2026-10-22; Docs/Work/2026-10-22-Next/SHIELD.md): how it soaks, breaks and
 * refills, how its traits hit, how often one drops, how the HUD shows it. Each is a Looter.Shield.* console variable so the
 * user tunes it live in PIE; the fields' own values are the cvars' defaults. The rules (ShieldRules) take a tuning, so tests
 * drive them with a default-made one, never with what the user has set; play reads Live() where it uses them.
 */
struct AI_LOOTER_SHOOTER_API FShieldTuning
{
	/** Looter.Shield.Enable: 0 and no ward takes anything (every hit goes to health), to feel the game without one. */
	int32 Enable = 1;

	// --- The ward's numbers, on top of every ward's own (live: the stats a ward rolled stay as rolled) ---
	/** Looter.Shield.CapacityScale: every ward's capacity, times. */
	float CapacityScale = 1.f;
	/** Looter.Shield.DelayScale: every ward's wait before it refills, times. */
	float DelayScale = 1.f;
	/** Looter.Shield.RateScale: every ward's refill rate, times. */
	float RateScale = 1.f;

	// --- Breaking ---
	/**
	 * Looter.Shield.BreakSpill: the share of the breaking hit's overflow that reaches health (0.5: a 30 bite on 10 of ward left
	 * takes 10 from health, not 20). A break is a warning, not a second hit: the user's "harder fights feel fair".
	 */
	float BreakSpill = 0.5f;
	/**
	 * Looter.Shield.HealthWaits: 1 and the wounds that close (FWoundsClose, 7 s, 7%/s as tuned for 12.2) hold off their ramp
	 * until the ward is full again: the ward refills first. Their wait still counts while it refills, so health comes back
	 * as before whenever the ward fills inside the 7 s (every ward but the slowest Bulwarks).
	 */
	int32 HealthWaits = 1;

	// --- Drops and the starting ward ---
	/**
	 * Looter.Shield.Drop.Share: of the guns a table that drops wards rolls (its bDropsShields), this share come as a ward of
	 * the same rarity instead: the weapon-drop share split, so a kill's loot rate is unchanged.
	 */
	float DropShare = 0.2f;
	/** Looter.Shield.Starter: 1 and the first gun in the player's hands brings a Common ward (as it brings two grenades). */
	int32 Starter = 1;

	// --- Traits (each times the ward's rarity's TraitStrength: Rare 1, Epic 1.25, Legendary 1.6) ---
	/** Looter.Shield.Nova.Damage: Last Breath's burst at its heart, at level 1 (grows with the level as enemies do). */
	float NovaDamage = 260.f;
	/** Looter.Shield.Nova.Radius: its reach (cm, to a body's nearest side). */
	float NovaRadius = 450.f;
	/** Looter.Shield.Nova.Core: full damage within this (cm). */
	float NovaCore = 150.f;
	/** Looter.Shield.Nova.Edge: the share it still deals at its reach's edge. */
	float NovaEdge = 0.35f;
	/** Looter.Shield.Nova.BossCap: a boss takes at most this share of its health from one burst. */
	float NovaBossCap = 0.05f;
	/** Looter.Shield.Barb.Share: Barbwire cuts a biter for this share of the ward's capacity, each bite on a charged ward. */
	float BarbShare = 0.8f;
	/** Looter.Shield.Lodestone.Chance: the chance a hit on a charged ward gives rounds. */
	float LodestoneChance = 0.4f;
	/** Looter.Shield.Lodestone.Share: the rounds given, as a share of the gun in hand's magazine (at least 1). */
	float LodestoneShare = 0.1f;
	/** Looter.Shield.Harvest.Share: a kill puts back this share of the ward's capacity (and starts it refilling at once). */
	float HarvestShare = 0.2f;
	/** Looter.Shield.Grace.Below: Saint's Grace answers once health falls under this share with the ward broken. */
	float GraceBelow = 0.3f;
	/** Looter.Shield.Grace.Refill: it flares the ward back to this share of its capacity. */
	float GraceRefill = 0.6f;

	// --- Feel ---
	/** Looter.Shield.HitSoundGap: Shield.Hit plays at most this often (s): a shotgun's pellets crackle once. */
	float HitSoundGap = 0.08f;
	/** Looter.Shield.Hud.ChipHold / ChipDrain: what a hit took lingers pale this long (s), then drains over this long. */
	float HudChipHold = 0.3f;
	float HudChipDrain = 0.4f;
	/** Looter.Shield.Hud.FlashSeconds: the bar's white flash as it breaks or fills. */
	float HudFlashSeconds = 0.35f;
	/** Looter.Shield.Hud.ShimmerSeconds: one sweep of the shimmer along the bar while it refills. */
	float HudShimmerSeconds = 0.9f;

	/** The cvars' values now. */
	static FShieldTuning Live();
};
