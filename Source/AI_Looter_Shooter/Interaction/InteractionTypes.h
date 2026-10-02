#pragma once

#include "CoreMinimal.h"

class AActor;
class UObject;

/**
 * How the Interact key uses something right now, as it tells the interaction component (IInteractable, or the player's
 * component that offered it, IInteractionSource). A thing takes a tap, a hold, or both: with both, letting go before
 * HoldSeconds is the tap (loot: a tap picks it up, a hold equips it). With only a tap, it acts as the key goes down.
 */
struct AI_LOOTER_SHOOTER_API FInteractionOptions
{
	/** It can be used now. One that can't (a lantern already lit, a bell still ringing) is never focused. */
	bool bUsable = true;

	/** A tap of the key uses it. */
	bool bTap = true;

	/** Holding the key for HoldSeconds uses it. */
	bool bHold = false;

	float HoldSeconds = 0.f;

	/** The prompt's words for the tap ("Open the door") and for the hold ("Ring the bell"); empty: none for it. */
	FText TapPrompt;
	FText HoldPrompt;

	/** How far from the player's eyes it can be used (cm); 0: the interaction component's own reach. */
	float Reach = 0.f;

	/** The key can do something with it: it's usable, and it takes a tap or a hold. */
	bool CanUse() const { return bUsable && (bTap || bHold); }

	/** Nothing: what's left when nothing is focused. */
	static FInteractionOptions None()
	{
		FInteractionOptions Options;
		Options.bUsable = false;
		Options.bTap = false;
		return Options;
	}
};

/** Where the player looks from, for choosing what they mean to use. */
struct AI_LOOTER_SHOOTER_API FInteractionView
{
	/** The crosshair's line: where it starts and which way it goes (in the front view, the eyes and the way they face). */
	FVector Location = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;

	/** Where reach is measured from: the character's eyes (a third-person camera sits a couple of meters behind them). */
	FVector ReachOrigin = FVector::ZeroVector;
};

/** Something the player might mean to use, as the interaction component weighs it. Lives only while it's weighed. */
struct AI_LOOTER_SHOOTER_API FInteractionCandidate
{
	AActor* Actor = nullptr;

	/** The point looked at to use it: its middle, or where the crosshair's line meets it. */
	FVector Location = FVector::ZeroVector;

	FInteractionOptions Options;

	/** How far from the eyes it can be (cm). */
	float Reach = 0.f;

	/** It must be in plain sight of the eyes (not behind a wall). Loot never asked that, and keeps not asking. */
	bool bCheckSight = true;

	/** The player's component that offered it (IInteractionSource), or null for an interactable in the level. */
	UObject* Source = nullptr;
};
