#pragma once

#include "CoreMinimal.h"

class UHealthComponent;
struct FRecoverySettings;

/**
 * "Wounds that close", the revenant's own trait (FRecoverySettings has the numbers): after RegenDelaySeconds without being
 * hurt, health comes back, the rate rising from nothing to RegenRatePerSecond of the most health over RegenRampSeconds,
 * until it is full. Any hurt (damage that landed, or a cursed iron's drain) stops it and starts the wait over; a scene or
 * a death only holds it. One of these per player, ticked by UPlayerVitalsSubsystem. It is plain
 * data and plain functions, so the tests run it without a world (Step) or with a bare health component (Tick).
 */
struct FWoundsClose
{
	/** Seconds spent waiting since the last hurt, up to the delay (it holds at the delay while healing, and at full health). */
	float QuietSeconds = 0.f;

	/** Seconds the current run has been healing: the ramp's clock. Zero when nothing is healing. */
	float RunSeconds = 0.f;

	/** The health component's hurt count as last seen (UHealthComponent::GetHurtCount), once it has been seen. */
	uint32 SeenHurtCount = 0;
	bool bSeenHurtCount = false;

	/** Health is coming back right now (after the delay, below full, not held). */
	bool IsRegenerating() const { return RunSeconds > 0.f; }

	/** Forgets the wait and the run (a death, a respawn): the next hurt-free stretch waits the whole delay. */
	void Reset();

	/**
	 * Moves the rule on DeltaSeconds and returns the health to give now (never past the most): nothing while it waits.
	 * bHurt: something hurt the player since the last step (restarts the wait). bHeld: dead or in a scene (nothing
	 * heals, the ramp starts over when it lets go, the wait is kept).
	 */
	float Step(const FRecoverySettings& Settings, float DeltaSeconds, float Health, float MaxHealth, bool bHurt, bool bHeld);

	/**
	 * Step for one body: reads its hurt count and health, and heals it. Returns what it healed. A dead body resets the rule
	 * and gets nothing.
	 */
	float Tick(const FRecoverySettings& Settings, UHealthComponent& Health, float DeltaSeconds, bool bHeld);
};
