#pragma once

#include "CoreMinimal.h"

class APawn;
class UHealthComponent;
class UPlayerShieldComponent;

/**
 * How the HUD tells a hit the ward took from a wound (SHIELD-POLISH, 2026-10-22; Borderlands tints a shield hit): a hit
 * the ward soaked whole tints the screen's edges cyan and glints the portrait's window cyan, with no red and no flinch; a
 * hit that breaks it does both (the break is the "get out" cue); a hit on health (no ward, or an empty one) is red and
 * flinches as before. UHudScreenEdgeWidget and UHudPortraitWidget each watch the local pawn with an FHudWardHitWatch.
 */
namespace HudWardHit
{
	/** One frame's look at the player: what changed since the last. */
	struct FSeen
	{
		/** Hits the ward was handed (UPlayerShieldComponent::GetHitCount's rise), and breaks (GetBreakCount's). */
		uint32 WardHits = 0;
		uint32 WardBreaks = 0;
		/** The ward's charge last frame (0: empty or none). */
		float ChargeBefore = 0.f;
		/** Health points fallen since last frame (0 when it held or rose). */
		float HealthLost = 0.f;
	};

	struct FReaction
	{
		/** The ward took something: cyan edges, the portrait's cyan glint. */
		bool bWard = false;
		/** Health was hurt, or the ward broke: red edges, the portrait's flinch. */
		bool bWound = false;

		bool Any() const { return bWard || bWound; }
	};

	/** Health falls by more than this (points) for a wound, as the edges have always read it. */
	inline constexpr float HealthStep = 0.01f;

	/**
	 * The rule: a break is both; health lost is a wound; a ward hit is the ward's when it held charge or health didn't fall
	 * (the frame its refill began, it held a sliver it didn't have the frame before). An empty ward handed a hit is no ward
	 * hit: health takes it, red.
	 */
	AI_LOOTER_SHOOTER_API FReaction Decide(const FSeen& Seen);

	/** The feel numbers, each a Looter.Shield.Hud.* cvar; the fields' values are the defaults. */
	struct AI_LOOTER_SHOOTER_API FTuning
	{
		/** Looter.Shield.Hud.EdgeSeconds: the cyan edges fade over this long (the red's is 0.55 s). */
		float EdgeSeconds = 0.4f;
		/** Looter.Shield.Hud.EdgePeak: their strength at the hit, against the red's 0.85 (a soaked hit is lighter news). */
		float EdgePeak = 0.7f;
		/** Looter.Shield.Hud.GlintSeconds: the portrait window's cyan glint fades over this long. */
		float GlintSeconds = 0.35f;
		/** Looter.Shield.Hud.GlintPeak: its strength at the hit (the red flash's is 0.55). */
		float GlintPeak = 0.45f;

		static FTuning Live();
	};
}

/**
 * Watches one pawn's health and ward from frame to frame for a HUD widget (no binding: the widgets already poll each
 * tick). A new pawn (a respawn) or a ward newly found starts it fresh, with no reaction that frame.
 */
struct AI_LOOTER_SHOOTER_API FHudWardHitWatch
{
	/** This frame's reaction to Pawn (null: none, and the watch forgets what it saw). */
	HudWardHit::FReaction Look(APawn* Pawn);

	/** The health and ward watched (either may be null). */
	UHealthComponent* GetHealth() const;
	UPlayerShieldComponent* GetShield() const;

private:
	TWeakObjectPtr<APawn> Watched;
	TWeakObjectPtr<UHealthComponent> Health;
	TWeakObjectPtr<UPlayerShieldComponent> Shield;
	float LastHealth = -1.f;
	float LastCharge = 0.f;
	uint32 LastHits = 0;
	uint32 LastBreaks = 0;
	bool bShieldSeen = false;
};
