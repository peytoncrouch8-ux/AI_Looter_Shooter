#pragma once

#include "CoreMinimal.h"

/**
 * The ore lift's motion as plain math of time (Docs/Areas/RansomsRest_Expansion.md, section 6 "The ore lift"), apart from
 * the world so the tests can check it: how far along its cable the cage is, how it swings on its grip and bobs on the
 * cable, and when its grip clanks over a tower's sheaves. AOreLift places the cage from it, and the ride's camera rides
 * the cage.
 *
 * A run is the cage's trip between its two landings: the winch takes up the slack (the cage lifts off its landing), it
 * travels the cable easing up to speed and down from it, and it's set down on the far landing (a dip, the bounce dying
 * away, the swing stilled). Every number is a function of the seconds into the run, so a run skipped to its end, or
 * played in any frames, ends in the same place.
 */

/** A run's timing and the cable's shape as the run sees it, from its own start. */
struct AI_LOOTER_SHOOTER_API FOreLiftRunShape
{
	/** The winch taking up the slack: the cage lifts off its landing before it moves. */
	float TakeUpSeconds = 0.8f;

	/** Under way along the cable, easing up to speed over EaseSeconds and down from it over the last EaseSeconds. */
	float TravelSeconds = 21.f;
	float EaseSeconds = 3.5f;

	/** Set down on the far landing: the bounce dies away and the swing is stilled. */
	float SettleSeconds = 1.4f;

	/** The cable between the two landings' grips (cm). */
	float Length = 5700.f;

	/** Where the cable bends over a tower's sheaves, as distances from this run's start (cm), in order. */
	TArray<float> Bends;

	/**
	 * Each straight span's way, Bends.Num() + 1 of them in this run's order, as shares of a unit of travel: along the run's
	 * own heading over the ground (X) and across it (Y). A steep span has little of either; the bend between two spans is
	 * what kicks the cage's swing as the grip goes over a tower.
	 */
	TArray<FVector2D> Spans;

	float GetArrival() const { return TakeUpSeconds + TravelSeconds; }
	float GetDuration() const { return TakeUpSeconds + TravelSeconds + SettleSeconds; }

	/** The span holding Distance along the cable (0 to Bends.Num()). */
	int32 SpanAt(float Distance) const;

	/** Span's shares, or straight ahead when the shape gives none. */
	FVector2D SpanShares(int32 Span) const;
};

/** How the cage hangs and moves on its cable. Each a setting on AOreLift. */
struct AI_LOOTER_SHOOTER_API FOreLiftSwing
{
	/** From the grip down to the loaded cage's middle (cm): the pendulum it swings as (about 3.1 s a swing at 2.4 m). */
	float PendulumLength = 240.f;

	/** How fast a swing dies away (per second): a cage on a cable swings on a good while. */
	float Damping = 0.35f;

	/** The most it swings either way (degrees): a bigger swing rounds off short of it. */
	float MaxDegrees = 8.f;

	/** A breeze rocking it side to side while it travels (degrees, slowly). */
	float WindDegrees = 0.6f;

	/** The cage lifting off its landing as the winch takes the strain (cm), and dipping onto the far one as it's set down. */
	float TakeUpRise = 4.f;
	float LandDip = 3.5f;

	/** Each clank over a sheave: the cage bobs on the cable (cm), the rider's view jolts (degrees), and the grip's lurch
	 *  over the wheel kicks the swing forward (degrees per second). */
	float ClankBob = 2.2f;
	float ClankJolt = 0.6f;
	float ClankKick = 1.6f;
};

/** Where the cage is in its sway Seconds into a run. */
struct AI_LOOTER_SHOOTER_API FOreLiftSway
{
	/** Its swing about the grip (degrees): Along > 0 is its floor trailing behind the travel, Side > 0 its floor to the
	 *  travel's left. */
	float Along = 0.f;
	float Side = 0.f;

	/** How far it rides above the cable's line (cm): the lift-off, the clanks' bob, the dip onto the far landing. */
	float Bob = 0.f;

	/** The rider's view's jolt (degrees of roll), the clanks and the landing felt through the floor: the camera's alone. */
	float Jolt = 0.f;
};

/** The swing's state as the integrator carries it: the pendulum's angles (radians) and their rates, at a whole step. */
struct AI_LOOTER_SHOOTER_API FOreLiftSwingState
{
	int32 Step = 0;
	float Along = 0.f;
	float AlongRate = 0.f;
	float Side = 0.f;
	float SideRate = 0.f;
};

namespace OreLiftMotion
{
	/** The pendulum's steps (s): small, fixed and counted from the travel's start, so any frames give the same swing. */
	inline constexpr float StepSeconds = 1.f / 120.f;

	/** Gravity (cm/s^2). */
	inline constexpr float Gravity = 981.f;

	/** A tower's sheave train: this many wheels, this far apart along the cable (cm), each a clank as the grip goes over. */
	inline constexpr int32 SheavesPerTower = 3;
	inline constexpr float SheaveSpacing = 55.f;

	/** How far along Length (cm) the travel is Seconds into it: easing up to speed (the speed a smooth step) over Ease,
	 *  cruising, easing down over the last Ease; 0 before it starts, Length once it's done. */
	AI_LOOTER_SHOOTER_API float TravelDistance(float Seconds, float Travel, float Ease, float Length);

	/** Its speed (cm/s) and acceleration (cm/s^2) then. */
	AI_LOOTER_SHOOTER_API float TravelSpeed(float Seconds, float Travel, float Ease, float Length);
	AI_LOOTER_SHOOTER_API float TravelAcceleration(float Seconds, float Travel, float Ease, float Length);

	/** Seconds into the travel at which it has come Distance (cm). */
	AI_LOOTER_SHOOTER_API float TravelTimeAt(float Distance, float Travel, float Ease, float Length);

	/** How far along the cable (cm) the cage is Seconds into Run: 0 through the take-up, Length from its arrival on. */
	AI_LOOTER_SHOOTER_API float DistanceAt(const FOreLiftRunShape& Run, float Seconds);

	/** Seconds into Run as the grip goes over each sheave of each tower, in order: the clanks. */
	AI_LOOTER_SHOOTER_API TArray<float> ClankTimes(const FOreLiftRunShape& Run);

	/** Carries State on, step by step, to the last whole step at or before Seconds into Run (from the travel's start). */
	AI_LOOTER_SHOOTER_API void AdvanceSwing(const FOreLiftRunShape& Run, const FOreLiftSwing& Swing, float Seconds, FOreLiftSwingState& State);

	/**
	 * The sway Seconds into Run. State carries the swing between calls, so a frame's work is a few steps: it starts over
	 * from the travel's start whenever Seconds goes back before it (pass a fresh state to work it out from nothing).
	 */
	AI_LOOTER_SHOOTER_API FOreLiftSway SwayAt(const FOreLiftRunShape& Run, const FOreLiftSwing& Swing, float Seconds, FOreLiftSwingState& State);

	/** The same from nothing (the tests): every step from the travel's start. */
	AI_LOOTER_SHOOTER_API FOreLiftSway SwayAt(const FOreLiftRunShape& Run, const FOreLiftSwing& Swing, float Seconds);
}
