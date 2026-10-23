#pragma once

#include "CoreMinimal.h"

class AActor;
class UObject;

/**
 * The part of an interactable the player may look at anywhere to use it, as a box: the chapel bell's whole hanging rope,
 * the Keeper's Lantern with the web round it, a few coins with room round them (IInteractable::GetInteractionBox). The
 * crosshair's line passing through it is looking straight at it; passing beside it, it's judged by the box's nearest point,
 * so a long rope looked at near its top, or a lantern hanging below the eyes, is still looked at (the 2026-10-23
 * play-check: the rope took focus only in a narrow band of pitch, the lantern from two spots of four).
 */
struct AI_LOOTER_SHOOTER_API FInteractionBox
{
	/** Its middle and turn in the world (no scale). */
	FTransform Transform = FTransform::Identity;

	/** Half its size along its own axes (cm). */
	FVector Extent = FVector::ZeroVector;

	FInteractionBox() = default;
	FInteractionBox(const FTransform& InTransform, const FVector& InExtent)
		: Transform(InTransform.GetRotation(), InTransform.GetLocation()), Extent(InExtent.GetAbs())
	{
	}

	/** World point Point is inside it (or on its face). */
	bool Contains(const FVector& Point) const
	{
		const FVector Local = Transform.InverseTransformPositionNoScale(Point);
		return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
	}
};

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

	/** The point looked at to use it: its middle, its box's point nearest the crosshair's line, or where the line meets it. */
	FVector Location = FVector::ZeroVector;

	/**
	 * Its own middle, when Location is its box's nearest point: the sight check tries it too before calling it hidden (the
	 * box's near corner can dip behind the ledge the coins lie on, or round the door frame the rope hangs behind).
	 */
	TOptional<FVector> Middle;

	/**
	 * Its box (IInteractable::GetInteractionBox), when it has one: whatever the sight line meets inside it is the thing's own
	 * surroundings (the brazier's basket round the fire bed, the gantry round the clamp, the post's arm over the lantern) and
	 * never hides it.
	 */
	TOptional<FInteractionBox> Box;

	FInteractionOptions Options;

	/** How far from the eyes it can be (cm). */
	float Reach = 0.f;

	/**
	 * How directly it must be looked at: the cosine of its cone's half angle (the component narrows it with distance for
	 * the level's interactables). Unset: the cone the focus is chosen with (loot's).
	 */
	TOptional<float> MinAim;

	/** It must be in plain sight of the eyes (not behind a wall). Loot never asked that, and keeps not asking. */
	bool bCheckSight = true;

	/** The player's component that offered it (IInteractionSource), or null for an interactable in the level. */
	UObject* Source = nullptr;
};
