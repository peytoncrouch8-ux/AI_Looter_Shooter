#pragma once

#include "CoreMinimal.h"
#include "Combat/GraveSaltTuning.h"

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
 *  - The flight: a 9 cm ball (the jar, twice its first size since 2026-10-27, is 15 cm across) under the world's
 *    gravity. Off the ground and walls it bounces, keeping 30% of its speed into the surface and half its speed along it
 *    (a heavy tin of salt on dirt), so a throw clinks down two or three times and rolls a few metres to a stop: a level
 *    throw from the eye first lands about 11.5 m out and lies still about 4 m on, just as its fuse runs out. Slower than
 *    60 cm/s on the ground it lies still. The fuse burns 1.5 s from the hand (a long lob bursts in the air); a creature
 *    (anything that can be hurt that isn't a player) bursts it on contact.
 *  - The burst (its numbers are FGraveSaltTuning's, the Looter.Grenade.* cvars): 900 at its heart at full strength at
 *    level 1 (a Common rifle's magazine is 30 rounds of 20: one and a half magazines), full within a metre, falling
 *    straight to a fifth at 4 m, nothing past. It grows with the player's level as enemies and guns do
 *    (FLevelRules::EnemyScale), and on top of that it starts weak (the user's playtest, 2026-10-27: "too strong to start
 *    with"): 10% of its strength through level 3 (90 at the heart, 135 on the Unpaid: a Basic Unpaid there is left
 *    about a sixth to a third for a follow-up shot), then rising on a curve to full by level 12 (EarlyShare). At full strength a pack of Basic creatures goes and a
 *    Gravebound spider is left at half. +/-10% like every hit, never critical. Measured to the body's nearest side, so a
 *    big body is caught by its edge.
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
	/**
	 * The ball the flight sweeps (cm): between the jar's half width (7.5) and its middle's height over its foot (10), so a
	 * jar lying on its side or standing barely floats or sinks.
	 */
	static constexpr float CollisionRadius = 9.f;

	// --- The jar (Art/Models/Throwables/SaltGrenade.py, twice its first size since 2026-10-27: "projectiles are too small") ---
	/** Its middle above its foot, the model's pivot (cm): it tumbles, and sits in the hand, about its middle. */
	static constexpr float JarMiddle = 10.f;
	/** Its tin's radius (R_BODY) and the fuse's top above its foot (cm). */
	static constexpr float JarRadius = 7.5f;
	static constexpr float JarFuseTop = 24.f;
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

	// --- The burst (its numbers: Tuning, FGraveSaltTuning::Current in play, a default-made one in the tests) ---

	/**
	 * The share of the burst's damage at Distance (cm) from its heart to a body's side: 1 in the core, the tuning's
	 * EdgeShare at its radius, 0 past it.
	 */
	static float Falloff(float Distance, const FGraveSaltTuning& Tuning);

	/**
	 * One body's damage: BaseDamage times Falloff(Distance) times LevelScale, at Roll (0-1) in the +/-10% range, the
	 * tuning's UnpaidScale for the Unpaid. 0 past the radius. LevelScale is the thrower's (ThrowerScale).
	 */
	static float BurstDamage(float Distance, float LevelScale, float Roll, bool bUnpaid, const FGraveSaltTuning& Tuning);

	/** A boss's share of Damage: at most the tuning's BossShareCap of its MaxHealth. */
	static float CapForBoss(float Damage, float MaxHealth, const FGraveSaltTuning& Tuning);

	/**
	 * How strong the burst is for a player of Level, before the level growth: the tuning's EarlyShare through its
	 * EarlyLevels, then rising as the square of the way on to 1 at its FullStrengthLevel, and staying there.
	 */
	static float EarlyShare(int32 Level, const FGraveSaltTuning& Tuning);

	/** The burst's whole scale for a thrower of Level whose enemies grow by EnemyScale: EnemyScale x EarlyShare(Level). */
	static float ThrowerScale(int32 Level, float EnemyScale, const FGraveSaltTuning& Tuning);

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
