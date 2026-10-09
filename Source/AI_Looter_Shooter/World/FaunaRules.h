#pragma once

#include "CoreMinimal.h"
#include "World/FaunaTypes.h"

/**
 * The ambient fauna's rules as plain functions (the birds, insects, tumbleweeds and cloth of World/Fauna*), so the tests
 * check them without a level: what scares an animal, how often an actor updates for its distance and the view, when it
 * hides, how many insects a swarm may have out, the seeded randomness that keeps every run the same, a wing's turn,
 * the curves birds fly along and a tumbleweed's bounce.
 */
namespace FaunaRules
{
	/** A shown actor hides only past this share of its cull distance (and shows again under it): no flicker at the edge. */
	constexpr float CullHysteresis = 1.08f;

	/** Nearer than this (cm) and on screen, an actor updates every frame. */
	constexpr float FullRateDistance = 3000.f;

	/** Up to this (cm) on screen, a busy actor (birds in the air, a tumbleweed rolling) still updates every frame. */
	constexpr float BusyFullRateDistance = 6000.f;

	/** A player's speed (cm/s) at which a bird's flee radius is as set; still they scare less, sprinting more. */
	constexpr float WalkSpeed = 500.f;
	constexpr float SprintSpeed = 780.f;

	/**
	 * How far off a threat scares an animal, as a share of its flee radius: a player standing still 0.8, walking 1, sprinting
	 * 1.35 (in between by speed), crouched 0.6 of that; a hostile creature 0.7, whatever it does (animals know the dead
	 * walk here, but they don't fear them as they fear a man with a gun).
	 */
	AI_LOOTER_SHOOTER_API float FearOf(bool bPlayer, bool bCrouched, float Speed);

	/**
	 * Whether a threat scares an animal at Where: nearer than FleeRadius times its fear, height counting 0.6 (a crow on a
	 * roof lets the player walk closer underneath than across open ground, but not under it).
	 */
	AI_LOOTER_SHOOTER_API bool IsThreatened(const FVector& Where, const FFaunaThreat& Threat, float FleeRadius);

	AI_LOOTER_SHOOTER_API bool IsThreatenedByAny(const FVector& Where, TConstArrayView<FFaunaThreat> Threats, float FleeRadius);

	/** The nearest threat to Where (INDEX_NONE without any) and how far it is (cm). */
	AI_LOOTER_SHOOTER_API int32 NearestThreat(const FVector& Where, TConstArrayView<FFaunaThreat> Threats, float& OutDistance);

	/**
	 * Whether an animal at Where hears a noise that came after Since (world seconds): a gunshot within GunfireRadius, a
	 * bullet's strike within ImpactRadius, anything else within the noise's own radius.
	 */
	AI_LOOTER_SHOOTER_API bool Hears(const FVector& Where, const FFaunaNoise& Noise, double Since, float GunfireRadius, float ImpactRadius);

	/** The first noise in Noises an animal at Where hears (INDEX_NONE: none). */
	AI_LOOTER_SHOOTER_API int32 FirstHeard(const FVector& Where, TConstArrayView<FFaunaNoise> Noises, double Since, float GunfireRadius,
		float ImpactRadius);

	/**
	 * Seconds between an actor's updates (0: every frame): on screen, every frame nearer than FullRateDistance (a busy
	 * one up to BusyFullRateDistance), else 30 or 15 times a second; off screen 10 times a second while busy and 4 at
	 * rest, which keeps its timers and its threats without moving anything the player can see.
	 */
	AI_LOOTER_SHOOTER_API float UpdateInterval(float Distance, bool bOnScreen, bool bBusy);

	/** Whether an actor at Distance (cm) from the view is drawn, with hysteresis about CullDistance (0: never culled). */
	AI_LOOTER_SHOOTER_API bool ShouldShow(float Distance, float CullDistance, bool bShownNow);

	/** How many more a swarm may bring out: what is Wanted, no more than its Cap leaves room for. */
	AI_LOOTER_SHOOTER_API int32 SwarmRoom(int32 Cap, int32 Active, int32 Wanted);

	/** A number in [0, 1) from a seed and an index, the same on every run and machine. */
	AI_LOOTER_SHOOTER_API float Random01(uint32 Seed, uint32 Index);

	/** A number in [Min, Max) from a seed and an index (Random01). */
	AI_LOOTER_SHOOTER_API float RandomRange(uint32 Seed, uint32 Index, float Min, float Max);

	/**
	 * A wing's turn at its shoulder, in the bird's frame (X forward, Y right, Z up): UpDegrees raises its tip, BackDegrees
	 * sweeps it back. Side is -1 for the left wing (along -Y) and +1 for the right (along +Y).
	 */
	AI_LOOTER_SHOOTER_API FQuat WingTurn(int32 Side, float UpDegrees, float BackDegrees);

	/**
	 * A point Alpha (0..1) along the cubic from P0 leaving at V0 to P1 arriving at V1, flown in Duration seconds; with
	 * OutVelocity, its velocity there (cm/s). Birds fly these: smooth, and exactly where they're meant to end.
	 */
	AI_LOOTER_SHOOTER_API FVector Hermite(const FVector& P0, const FVector& V0, const FVector& P1, const FVector& V1, float Duration,
		float Alpha, FVector* OutVelocity = nullptr);

	/** How long (s) to fly Distance (cm) at Speed (cm/s), within Min and Max. */
	AI_LOOTER_SHOOTER_API float FlightSeconds(float Distance, float Speed, float Min, float Max);

	/**
	 * A velocity after meeting a surface with Normal: the part into it turned back at Restitution (0 stops dead, 1 bounces
	 * fully), the part along it kept. A velocity already leaving the surface is unchanged.
	 */
	AI_LOOTER_SHOOTER_API FVector Bounce(const FVector& Velocity, const FVector& Normal, float Restitution);

	/** How far (degrees) a flier banks turning at TurnRate (radians a second) at Speed (cm/s), within MaxDegrees. */
	AI_LOOTER_SHOOTER_API float BankDegrees(float Speed, float TurnRate, float MaxDegrees);

	/** The ground under a ground perch Offset (cm, XY) from it, on its slope (its Normal). */
	AI_LOOTER_SHOOTER_API FVector GroundNear(const FFaunaPerch& Perch, const FVector2D& Offset);

	/** Turns From toward To (degrees, the short way) by at most MaxStep. */
	AI_LOOTER_SHOOTER_API float StepAngle(float From, float To, float MaxStep);
}
