#pragma once

#include "CoreMinimal.h"

class AActor;
class APawn;
class UWorld;

/**
 * Loot coming to the player for its last metre or so, Borderlands' way (concept F, "pickups that come to you"): silver
 * flies in itself and is the player's on arrival (ASilverPickup); ammo counts the moment it's run past, as always, and a
 * copy of its bundle flies in after (UPickupPullSubsystem). Either only within reach and along a clear line, never
 * through a wall. The flight lifts in a small arc and gains speed, with a glint as it sets off and a faint trail, drawn
 * through the bullet subsystem's pooled effects (FWeaponFX's streaks: no draws of its own).
 */
namespace PickupPull
{
	/**
	 * Nothing solid between From (a pickup, lifted a little off the ground it lies on) and Player's middle: a visibility
	 * trace, the pickup and the player left out. Walls, rocks, fences and terrain block it; grass and triggers don't.
	 */
	AI_LOOTER_SHOOTER_API bool HasClearLine(const UWorld& World, const FVector& From, const APawn& Player, const AActor* Pickup);

	/** Within Range of the body's surface (cm; ASoulMotePickup::DistanceToBody) and along a clear line. */
	AI_LOOTER_SHOOTER_API bool CanPull(const UWorld& World, const FVector& From, const APawn& Player, float Range, const AActor* Pickup);

	/** Where a pulled pickup flies to: the body's middle (a character's capsule centre). */
	AI_LOOTER_SHOOTER_API FVector TargetOf(const APawn& Player);

	/** The glint's strength Age seconds after a pull set off: a quick flare, gone by GlintSeconds. */
	AI_LOOTER_SHOOTER_API float GlintAt(float Age);
	inline constexpr float GlintSeconds = 0.32f;

	/**
	 * One frame of a four-pointed glint at Where (a star turned to the camera, Size cm from tip to tip), Strength 0-1. Drawn
	 * by the bullet subsystem's effects; nothing without one.
	 */
	AI_LOOTER_SHOOTER_API void DrawGlint(UWorld& World, const FVector& Where, float Strength, float Size, const FLinearColor& Color);

	/** One frame of a faint streak from From to To (the flight's trail). */
	AI_LOOTER_SHOOTER_API void DrawTrail(UWorld& World, const FVector& From, const FVector& To, float Strength, const FLinearColor& Color);
}

/**
 * A pulled pickup's path as plain math (the tests check it): a point gaining speed straight for its target (which may
 * move: the player runs on), shown lifted in a low arc over that line that comes back down onto it at the end, so it
 * reads as tossed to the player rather than slid along the ground.
 */
struct AI_LOOTER_SHOOTER_API FPickupFlight
{
	/** Sets off from From toward Target at StartSpeed (cm/s). */
	void Start(const FVector& From, const FVector& Target, float StartSpeed);

	/** Moves on DeltaSeconds toward Target, gaining Acceleration up to MaxSpeed. True once it has got there. */
	bool Step(float DeltaSeconds, const FVector& Target, float Acceleration, float MaxSpeed);

	/** Where it shows: its point on the line, lifted by the arc. */
	FVector GetLocation() const;

	/** Its point on the straight line (what arrives). */
	const FVector& GetBase() const { return Base; }

	/** How far along it is, 0 to 1, by the distance left to the target. */
	float GetProgress() const { return Progress; }

	float GetAge() const { return Age; }
	float GetSpeed() const { return Speed; }
	bool IsStarted() const { return bStarted; }

	/** The arc's height for a flight this long (cm): a share of it, between these. */
	static constexpr float LiftShare = 0.12f;
	static constexpr float MinLift = 10.f;
	static constexpr float MaxLift = 38.f;

private:
	FVector Base = FVector::ZeroVector;
	float StartDistance = 0.f;
	float Speed = 0.f;
	float Lift = 0.f;
	float Progress = 0.f;
	float Age = 0.f;
	bool bStarted = false;
};
