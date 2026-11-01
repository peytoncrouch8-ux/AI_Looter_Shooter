#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/**
 * A scene's talking view of someone, chosen so they're seen whole: far enough off to hold them standing (hat and all) with
 * room round them, at their chest's height, and from a spot where nothing solid stands between the lens and them, kneeling
 * or standing (a deck's rail, a post, a board), over the ground rather than out over a drop. FreeAbel's view came from
 * Sully's eyes a metre or two off and aimed at his knelt head: the rail crossed him and his head left the top as he stood
 * (the 2026-11-01 play-check: m7_18).
 */
namespace SceneTalkView
{
	/** Who to frame. */
	struct FFigure
	{
		/** Where they stand (their feet, cm). */
		FVector Feet = FVector::ZeroVector;
		/** Their top standing (hat and all) and the lowest it goes in the shot (kneeling), over Feet (cm). */
		float StandTop = 180.f;
		float LowTop = 120.f;
		/** The way they face (degrees): the view is tried in front of them first. */
		float FacingYaw = 0.f;
		/** Left out of the sight checks (they themselves). */
		const AActor* Who = nullptr;
	};

	/** The lens's width the view is made for (degrees across a 16:9 frame). */
	inline constexpr float Fov = 70.f;

	/** How far off the lens must stand to hold Height (cm) top to bottom with a little room, at Fov across 16:9. */
	AI_LOOTER_SHOOTER_API float FramingDistance(float Height);

	/** The point the view looks at: a little over the middle of them standing. */
	AI_LOOTER_SHOOTER_API FVector LookPoint(const FFigure& Figure);

	/**
	 * The spots tried, best first: in front of them, then turning round them both ways a step at a time, at the framing
	 * distance (at least MinDistance), then a little farther; at the look point's height.
	 */
	AI_LOOTER_SHOOTER_API TArray<FVector> Spots(const FFigure& Figure, float MinDistance);

	/**
	 * The first spot (Spots) the lens sees them whole from (their feet, middle, knelt and standing tops all in plain sight
	 * of the level's solid things) over ground; failing that, the one that sees the most of them. Without a world, the first.
	 * Looks at LookPoint.
	 */
	AI_LOOTER_SHOOTER_API FTransform Find(const UWorld* World, const FFigure& Figure, float MinDistance);
}
