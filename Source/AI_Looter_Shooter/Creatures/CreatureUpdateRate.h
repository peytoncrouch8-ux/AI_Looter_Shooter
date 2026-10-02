#pragma once

#include "CoreMinimal.h"
#include "CreatureUpdateRate.generated.h"

/**
 * How often a creature updates (its brain, its movement and its pose) by how far it is from the nearest player or camera
 * (or how near it looks through a scope). Up close it updates every frame; farther away twenty, ten, then five times a
 * second. A dozen creatures ticking every frame cost a good part of the game thread's budget on Medium, and most of them
 * are far away or behind the player. The slower updates are safe: the brain's timers count real seconds, movement
 * sub-steps, and the bodies' springs and gaits take long steps in their stride. A creature busy with a player (hunting, attacking, alerted, just hurt) always
 * updates every frame, wherever it is (ACreatureBase::NeedsFullRate).
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FCreatureUpdateRate
{
	GENERATED_BODY()

	/** Off: the creature updates every frame wherever it is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate")
	bool bEnabled = true;

	/** Nearer than this (cm) to a player or the camera, it updates every frame. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "0", Units = "cm"))
	float NearDistance = 2500.f;

	/** From NearDistance up to this distance (cm), it updates every MidInterval. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "0", Units = "cm"))
	float MidDistance = 6000.f;

	/** From MidDistance up to this distance (cm), it updates every FarInterval; beyond it, every VeryFarInterval. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "0", Units = "cm"))
	float FarDistance = 12000.f;

	/** Seconds between updates in the mid band. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "0", ClampMax = "0.5", Units = "s"))
	float MidInterval = 0.05f;

	/** Seconds between updates in the far band. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "0", ClampMax = "0.5", Units = "s"))
	float FarInterval = 0.1f;

	/** Seconds between updates beyond FarDistance. The slime's springs sub-step up to MaxInterval. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "0", ClampMax = "0.5", Units = "s"))
	float VeryFarInterval = 0.2f;

	/** On screen and nearer than this (cm), it updates every frame whatever its band: choppy motion would show this close. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "0", Units = "cm"))
	float OnScreenFullRateDistance = 4000.f;

	/**
	 * Out of every player's view and at least this far (cm), it stops posing its body: it still senses, moves, fights back
	 * and respawns, and poses again as the view comes near it (ViewMargin).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "0", Units = "cm"))
	float FreezePoseDistance = 2500.f;

	/** Seconds between picking the rate again (getting busy with a player switches to every frame at once). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "0.05", Units = "s"))
	float CheckInterval = 0.25f;

	/** Seconds it updates every frame after being hurt, wherever it is: a sniped creature flinches and dies smoothly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "0", Units = "s"))
	float HurtFullRateTime = 8.f;

	/**
	 * Degrees beyond the screen's edges that still count as on screen. A frozen creature looks once an update, so it wakes
	 * before a turning view reaches it instead of being drawn in the pose it froze in.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "0", ClampMax = "90", Units = "Degrees"))
	float ViewMargin = 15.f;

	/**
	 * The field of view (degrees) the bands are set for: the game's default first-person one. Through a narrower view (a
	 * scope's zoom) a creature looks nearer, and it updates as if it were that near.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update Rate", meta = (ClampMin = "5", ClampMax = "170", Units = "Degrees"))
	float ReferenceFieldOfView = 90.f;

	/** The longest update the bodies' animation takes in one go (longer ones, hitches, move it on this much). */
	static constexpr float MaxInterval = 0.5f;

	/**
	 * Seconds between updates for a creature Distance (cm) from the nearest player or camera, 0 for every frame. An engaged
	 * creature (see ACreatureBase::NeedsFullRate) always updates every frame.
	 */
	float IntervalFor(float Distance, bool bEngaged, bool bOnScreen) const;

	/** Whether it can stop posing its body: off screen, not engaged, and FreezePoseDistance or farther. */
	bool ShouldFreezePose(float Distance, bool bEngaged, bool bOnScreen) const;

	/**
	 * How many times nearer things look through a camera with FieldOfView (degrees) than at ReferenceFieldOfView: 2 through
	 * a 2x sight. Never under 1, so a wide view never slows the bands down.
	 */
	float ZoomFor(float FieldOfView) const;

	/**
	 * Whether a body (a sphere Radius cm about Point) is in the view of a camera at ViewLocation looking along ViewDirection
	 * with a FieldOfView (degrees, across), MarginDegrees beyond the edges included: a cone through the corners of a 16:9
	 * screen. Walls and rocks don't count. A creature behind cover is still in view, since it can step out any moment.
	 */
	static bool IsInView(const FVector& ViewLocation, const FVector& ViewDirection, float FieldOfView, const FVector& Point,
		float Radius, float MarginDegrees);
};
