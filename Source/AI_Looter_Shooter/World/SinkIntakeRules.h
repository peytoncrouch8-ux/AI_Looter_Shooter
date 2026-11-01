#pragma once

#include "CoreMinimal.h"

/** A hose's shape: a cubic curve from its top (the barge's drum) to its end (the intake's mouth, or wherever it lies). */
struct FIntakeHoseShape
{
	FVector P0 = FVector::ZeroVector;
	FVector P1 = FVector::ZeroVector;
	FVector P2 = FVector::ZeroVector;
	FVector P3 = FVector::ZeroVector;

	/** Its point at T (0 the top, 1 the end). */
	FVector At(float T) const;
};

/**
 * The barge intake's rules (ASinkIntake, Main 5 "Choke Point"), plain math so the tests check them: the draw on the player
 * near its mouth, the hose's rest, buck, swing and limp shapes, and the second hose's sag.
 */
namespace SinkIntakeRules
{
	/** At the draw's edge the pull is this share of the player's own walk, at the drain's edge this; capped (fused: doubled). */
	inline constexpr float EdgeDraw = 0.22f;
	inline constexpr float MouthDraw = 0.55f;
	inline constexpr float MostDraw = 0.85f;

	/**
	 * The draw's pull at Distance from the mouth (a share of the player's own walking input toward it): nothing past
	 * DrawRadius, EdgeDraw at it rising to MouthDraw at DrainRadius and inside, times Scale (2 while the fuse burns), at most
	 * MostDraw (never more than the player's own walk: they can always lean out of it).
	 */
	AI_LOOTER_SHOOTER_API float DrawInput(float Distance, float DrawRadius, float DrainRadius, float Scale);

	/** The hose as Landfall lays it (its four control points: build_area_landfall's curve). */
	AI_LOOTER_SHOOTER_API FIntakeHoseShape RestShape(const FVector& Top, const FVector& C1, const FVector& C2, const FVector& Bottom);

	/** Choked and spent: hanging from the drum and lying along the floor to Rest. */
	AI_LOOTER_SHOOTER_API FIntakeHoseShape LimpShape(const FVector& Top, const FVector& Rest);

	/** The buck as the charges blow: the rest shape, its lower half thrashing side to side (Time s into BuckSeconds). */
	AI_LOOTER_SHOOTER_API FIntakeHoseShape BuckShape(const FIntakeHoseShape& Rest, const FVector& Side, float Time, float BuckSeconds);

	/** The swing's angle off the hanging line (degrees) Time s into it: kicked out, swinging back and over, dying away. */
	AI_LOOTER_SHOOTER_API float SwingAngle(float Time);

	/**
	 * The swing: the end torn off the mouth, swung across the floor about the hose's middle (along Side, the floor's way
	 * across the barge), kept FloorZ + 40 cm up at least, settling into Limp over the swing's last share. Time in Seconds.
	 */
	AI_LOOTER_SHOOTER_API FIntakeHoseShape SwingShape(const FIntakeHoseShape& Rest, const FIntakeHoseShape& Limp, const FVector& Side,
		float FloorZ, float Time, float Seconds);

	/** The second hose from the barge to the air shaft: a long sag, deepest a little past its middle. */
	AI_LOOTER_SHOOTER_API FIntakeHoseShape SecondShape(const FVector& From, const FVector& To);
}
