#pragma once

#include "CoreMinimal.h"
#include "Elements/LooterElement.h"

class AActor;
class UCreatureTraitComponent;

/** What the crosshair's hit marker shows for a hit, weakest first: a stronger one takes over a blast's marker, a kill's red beats all. */
enum class EHudHitMark : uint8
{
	/** A plain hit: white, the tick. */
	Body,
	/** A Warded creature's ward took it: the ward's colour (its element's), the ward's glassy tick. */
	Ward,
	/** A critical hit on its body: orange, the crit ping. */
	Crit,
	/** The hit broke its ward: the ward's colour, bigger and longer, the break's crack. */
	WardBreak,
	/** It died of it: red, bigger and longer (the creature's voice plays the kill's confirm). */
	Kill
};

/**
 * The hit marker's rules (the gun feel pass, GF-IMPACT 2026-10-22; UPlayerHUDWidget draws it): which mark a hit shows (a
 * ward's hit and break tell from a crit and a wound), how big (a hit's share of the target's health; a blast by its pellets
 * landed), its sound (one a blast; a stronger mark later in it sounds again), its colour and time. Tests:
 * Looter.UI.HUD.HitMarker.*.
 */
namespace HudHitMarker
{
	/** The feel numbers, each a Looter.UI.HitMarker.* cvar; the fields' values are the defaults. */
	struct AI_LOOTER_SHOOTER_API FTuning
	{
		/** .SizeGain / .SizeShare: the marker is 1 + SizeGain x min(the hit's share of the target's most health / SizeShare, 1) its size. */
		float SizeGain = 0.4f;
		float SizeShare = 0.25f;
		/** .BlastWindow (s): a pellet gun's hits this soon after its blast's first are that blast's (one marker, one sound). */
		float BlastWindow = 0.08f;
		/** .Seconds / .KillSeconds / .BreakSeconds: how long each shows; .Pop / .KillPop / .BreakPop: how much bigger it pops in. */
		float Seconds = 0.18f;
		float KillSeconds = 0.34f;
		float BreakSeconds = 0.3f;
		float Pop = 0.3f;
		float KillPop = 0.75f;
		float BreakPop = 0.6f;

		static FTuning Live();
	};

	/**
	 * The mark: a kill whatever else; on a Warded creature, its ward broken by it (up before, none after) or taking it (still
	 * up after); else a crit or a plain hit.
	 */
	AI_LOOTER_SHOOTER_API EHudHitMark Decide(bool bKill, bool bCritical, bool bWarded, float WardBefore, float WardAfter);

	/** The marker's size for a hit taking DamageShare of the target's most health. */
	AI_LOOTER_SHOOTER_API float Size(float DamageShare, const FTuning& Tuning);

	/** A blast's marker's size: Landed of its Pellets on bodies (all of them as big as a quarter of a body's health). */
	AI_LOOTER_SHOOTER_API float BlastSize(int32 Landed, int32 Pellets, const FTuning& Tuning);

	/** Whether Next takes over a marker showing Shown in the same blast (only a stronger one). */
	inline bool Outranks(EHudHitMark Next, EHudHitMark Shown) { return static_cast<uint8>(Next) > static_cast<uint8>(Shown); }

	/** How long a mark shows, and how much bigger it pops in. */
	AI_LOOTER_SHOOTER_API float SecondsOf(EHudHitMark Mark, const FTuning& Tuning);
	AI_LOOTER_SHOOTER_API float PopOf(EHudHitMark Mark, const FTuning& Tuning);

	/** The marker's colour: white, the ward's cyan, the crit's orange, the kill's red (LooterUI::Color). */
	AI_LOOTER_SHOOTER_API FLinearColor Tint(EHudHitMark Mark);

	/**
	 * The same for a hit on a typed ward (2026-10-29): a ward's tick and its break in the ward's element's colour
	 * (ElementRules::UiColor; the break a touch whiter), so the player sees which element the ward is; the kit's cyan for a
	 * ward with no element. Every other mark as Tint(Mark).
	 */
	AI_LOOTER_SHOOTER_API FLinearColor Tint(EHudHitMark Mark, ELooterElement WardElement);

	/**
	 * The sound the HUD plays for a mark (GunfeelSound::HitConfirmCue's: the ward's tick, the crit's ping, the plain tick; a
	 * break's crack); none for a kill (its confirm is the creature's voice's, UI.Kill / UI.Kill.Crit).
	 */
	AI_LOOTER_SHOOTER_API FName Cue(EHudHitMark Mark);

	/** Whether a hit of this mark sounds within a blast that has already sounded Sounded (only a stronger sound plays again). */
	AI_LOOTER_SHOOTER_API bool SoundsAgain(EHudHitMark Mark, EHudHitMark Sounded);
}

/**
 * The wards of the last few creatures the player hit, as last seen, so a hit can tell it broke one (the HUD hears of a hit
 * after it was dealt). Read again every frame (Refresh): a burn or another's shot may wear a ward between the player's hits.
 */
struct AI_LOOTER_SHOOTER_API FHudWardMemory
{
	static constexpr int32 Kept = 8;

	/** The ward Target held as last seen (a creature not seen yet: a full one, as every ward starts). */
	float Before(const AActor* Target) const;

	/**
	 * Notes Target's ward now (Traits its component, read again by Refresh), and its element (None keeps the one last seen:
	 * a broken ward may no longer name it, and its break still shows in its colour).
	 */
	void Note(const AActor* Target, const UCreatureTraitComponent* Traits, float Fraction, ELooterElement Element = ELooterElement::None);

	/** The element of Target's ward as last seen (None: not seen, or a ward with no element). */
	ELooterElement ElementOf(const AActor* Target) const;

	/** Reads every remembered creature's ward again; forgets the gone. */
	void Refresh();

	int32 Num() const { return Entries.Num(); }

private:
	struct FEntry
	{
		TWeakObjectPtr<const AActor> Actor;
		TWeakObjectPtr<const UCreatureTraitComponent> Traits;
		float Fraction = 1.f;
		ELooterElement Element = ELooterElement::None;
	};
	TArray<FEntry, TInlineAllocator<Kept>> Entries;
};
