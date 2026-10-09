#pragma once

#include "CoreMinimal.h"

/** A body as the burst sees it: an upright capsule (a creature's own, or the box round anything else). */
struct FGraveSaltBody
{
	FVector Center = FVector::ZeroVector;
	float HalfHeight = 50.f;
	float Radius = 30.f;
};

/**
 * The grave-salt grenade's flight and burst (Docs/Polish/BorderlandsComparison.md, item 16), as plain numbers with no
 * engine state: AGraveSaltGrenade and GraveSaltBurst play them, and the tests drive them. The throw's own numbers are
 * FThrowRules' (Player/PlayerThrowRules.h).
 *
 *  - The flight: a 6 cm ball under the world's gravity. Off the ground and walls it bounces, keeping 30% of its speed
 *    into the surface and half its speed along it (a heavy tin of salt on dirt), so a throw clinks down two or three
 *    times and rolls a few metres to a stop: a level throw from the eye first lands about 11.5 m out and lies still
 *    about 4 m on, just as its fuse runs out. Slower than 60 cm/s on the ground it lies still. The fuse burns 1.5 s
 *    from the hand (a long lob bursts in the air); a creature (anything that can be hurt that isn't a player) bursts it
 *    on contact.
 *  - The burst: 900 at its heart at level 1 (a Common rifle's magazine is 30 rounds of 20: one and a half magazines),
 *    full within a metre, falling straight to a fifth at 4 m, nothing past. It grows with the player's level as enemies
 *    and guns do (FLevelRules::EnemyScale), so it takes the same share at equal levels: a pack of Basic creatures goes,
 *    a Gravebound spider (1620 at level 1) is left at half. +/-10% like every hit, never critical. Measured to the body's
 *    nearest side, so a big body is caught by its edge.
 *  - Salt burns the dead: the Unpaid take half as much again and are staggered (the melee strike's heavy stagger).
 *  - A boss takes at most 8% of its health from one burst, so three grenades never take a quarter of its bar.
 *  - What a wall shields isn't hurt (the burst must see some of the body: middle, top or chest), and the thrower and
 *    every other player never are (fun first).
 *  - Bodies within 60% of the radius are knocked away from it by the melee strike's hop (never over an edge).
 *  - The view kicks by distance: full within 2 m, falling to nothing at 15 m.
 */
struct AI_LOOTER_SHOOTER_API FGraveSaltRules
{
	// --- The flight ---
	static constexpr float FuseSeconds = 1.5f;
	static constexpr float CollisionRadius = 6.f;
	/** The share of the speed into a surface it bounces back with, and the share of its speed along it it keeps. */
	static constexpr float Restitution = 0.3f;
	static constexpr float SlideKeep = 0.5f;
	/** A bounce off ground this slow (cm/s) leaves it lying still. */
	static constexpr float RestSpeed = 60.f;
	/** A surface facing up more than this is ground (it can roll and lie on it). */
	static constexpr float GroundMinUp = 0.7f;
	/** Off the ground, a hop back up slower than this (cm/s) is no hop: it rolls along instead, slowed by RollFriction. */
	static constexpr float RollSpeed = 90.f;
	static constexpr float RollFriction = 1600.f;
	/** A bounce is heard when it meets the surface at least this fast (cm/s); a roll's skitter isn't. */
	static constexpr float AudibleSpeed = 80.f;
	/** How fast it tumbles in flight (degrees a second at the throw's speed); it slows with each bounce. */
	static constexpr float TumbleDegrees = 720.f;

	/** One step of free flight: Dt seconds under GravityZ (cm/s/s, negative down), exactly as a thrown thing falls. */
	static void Fly(FVector& Location, FVector& Velocity, float Dt, float GravityZ);

	/** The velocity off a surface facing Normal: the speed into it reversed at Restitution, along it kept at SlideKeep. */
	static FVector Bounce(const FVector& Velocity, const FVector& Normal);

	/**
	 * Meeting a surface facing Normal at Velocity: a bounce (Bounce), except on ground when the hop back up would be slower
	 * than RollSpeed: then it rolls along it (no speed off it, its way along kept), slowed by RollFriction over Dt seconds.
	 * bOutRolled says which.
	 */
	static FVector Contact(const FVector& Velocity, const FVector& Normal, float Dt, bool& bOutRolled);

	/** After bouncing off a surface facing Normal at Velocity (the speed it leaves with), whether it now lies still. */
	static bool ShouldRest(const FVector& Velocity, const FVector& Normal);

	// --- The burst ---
	static constexpr float BaseDamage = 900.f;
	static constexpr float Radius = 400.f;
	static constexpr float CoreRadius = 100.f;
	static constexpr float EdgeShare = 0.2f;
	static constexpr float UnpaidScale = 1.5f;
	static constexpr float BossShareCap = 0.08f;
	/** Bodies within this share of the radius are knocked away. */
	static constexpr float KnockShare = 0.6f;

	/** The share of the burst's damage at Distance (cm) from its heart to a body's side: 1 in the core, EdgeShare at the radius, 0 past it. */
	static float Falloff(float Distance);

	/**
	 * One body's damage: BaseDamage times Falloff(Distance) times LevelScale, at Roll (0-1) in the +/-10% range, half
	 * again for the Unpaid. 0 past the radius.
	 */
	static float BurstDamage(float Distance, float LevelScale, float Roll, bool bUnpaid);

	/** A boss's share of Damage: at most BossShareCap of its MaxHealth. */
	static float CapForBoss(float Damage, float MaxHealth);

	/** From Point to the nearest side of Body (cm; 0 inside it). */
	static float DistanceToBody(const FVector& Point, const FGraveSaltBody& Body);

	/** The point on Body nearest Point (Point itself inside it). */
	static FVector NearestPoint(const FVector& Point, const FGraveSaltBody& Body);

	// --- The view ---
	static constexpr float KickFullDistance = 200.f;
	static constexpr float KickRadius = 1500.f;

	/** How much of the burst's jolt a player Distance (cm) from it feels: 1 close, falling smoothly to 0 at KickRadius. */
	static float KickShare(float Distance);
};
