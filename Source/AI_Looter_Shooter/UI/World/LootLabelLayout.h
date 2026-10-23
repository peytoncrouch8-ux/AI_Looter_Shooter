#pragma once

#include "CoreMinimal.h"

/**
 * One loot label on screen, for the layout (widget units: the viewport's DPI-scaled space). Labels hang from their
 * anchor (their widget component's pivot is 0.5, 1): the box runs from Anchor - (Size.X / 2, Size.Y) to Anchor + (Size.X / 2, 0).
 */
struct FLootLabelBox
{
	/** Where its component projects on screen: the label's bottom middle. */
	FVector2D Anchor = FVector2D::ZeroVector;
	FVector2D Size = FVector2D::ZeroVector;
	/** Looked at: it keeps its place and the others move round it. */
	bool bFocused = false;
	/** From the eyes (cm): nearer labels are placed before farther ones. */
	float Distance = 0.f;
	/** The shift it shows now (widget units, negative up), so a label keeps to the side it went to. */
	float Previous = 0.f;
};

/**
 * Keeps the loot labels lying near each other (guns' and wards' at an opened chest) from drawing over each other: a plain
 * rule (tests: Looter.Loot.Labels), played every frame by ULootLabelSubsystem.
 */
namespace LootLabelLayout
{
	/**
	 * Each label's vertical shift (widget units, negative up) so no two boxes overlap or come closer than Gap above or
	 * below each other. The focused label first (it never moves), then the nearest first: each stays where it is when it
	 * can, else goes just above or just below a label already placed, whichever is the smaller move, with KeepWeight times
	 * the distance from where it is now added (so it doesn't flip sides as the view moves).
	 */
	AI_LOOTER_SHOOTER_API TArray<float> Arrange(TConstArrayView<FLootLabelBox> Boxes, float Gap, float KeepWeight);

	/** Two placed boxes (min, max corners) are too close: they overlap across, and come within Gap of each other up or down. */
	AI_LOOTER_SHOOTER_API bool Crowds(const FBox2D& A, const FBox2D& B, float Gap);

	/**
	 * A gun's own label is already naming it: looked at (its plate shows) with its label up. Its name flashing in the
	 * pickup feed beside the plate then says the same name twice (play-check 2026-10-23: a strongbox's Epic rifle, its
	 * plate and the feed's flash a hand apart), so the feed skips the flash for it.
	 */
	AI_LOOTER_SHOOTER_API bool PlateNamesGun(bool bLookedAt, bool bLabelShowing);
}
