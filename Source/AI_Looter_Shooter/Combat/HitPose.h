#pragma once

#include "CoreMinimal.h"

/**
 * The plain pieces each kind's additive hit pose is built from (HITS layer 3; no anim assets, no physics): a kick-and-spring
 * like the spider's hit jolt has always been, a hit envelope for short one-shot moves (a leg's buckle, a fang's splay), and
 * the wound stage a creature's health share is at (layer 4). Plain structs and functions: the kinds own their fields, the
 * tests drive them without a world.
 */

/**
 * One hit channel (a body's shove, a twist, a head's whip; any 3 numbers: cm or degrees): a hit sets a kick, which holds
 * Hold seconds and then fades, and Offset follows it on a critically damped spring stepped at 120 Hz (as the spider's jolt:
 * Omega 40, Fade 10: one hit peaks ~90 ms on at ~0.84 of its kick). Add keeps the stronger kick, so a blast's pellets don't
 * add up. Step only while !IsSettled() (it costs nothing at rest).
 */
struct AI_LOOTER_SHOOTER_API FHitSpring
{
	/** What it's pulled toward (fades after Hold), where it is, how fast it moves. */
	FVector Kick = FVector::ZeroVector;
	FVector Offset = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	/** Seconds the kick still holds before fading. */
	float Hold = 0.f;

	/** A hit: K is the new kick if at least as strong as the one there (its hold renewed); a weaker one is ignored. */
	void Add(const FVector& K, float HoldSeconds);

	/** Dt seconds on (in 1/120 s steps; at most half a second taken, so a hitch doesn't throw it). */
	void Step(float Dt, float Omega = 40.f, float Fade = 10.f);

	/** At rest: no kick, no hold, the offset and its speed all but none. */
	bool IsSettled() const;

	/** Back to rest at once (a respawn, a death that takes the pose over). */
	void Reset() { *this = FHitSpring(); }

	/** The spring's step (s): fixed, so it acts the same at any frame rate. */
	static constexpr float StepSeconds = 1.f / 120.f;
};

/**
 * A one-shot move's strength at T seconds after it began (0 to 1): eased up over In seconds, then eased back to 0 over Out
 * seconds; 0 before it began and after it's done.
 */
AI_LOOTER_SHOOTER_API float HitEnvelope(float T, float In, float Out);

/** How hurt it looks (HITS layer 4, by its health's share): 0 at half or more, 1 under half, 2 under a quarter. */
AI_LOOTER_SHOOTER_API int32 WoundStage(float HealthShare);
