#pragma once

#include "CoreMinimal.h"

/**
 * The gang's skiff's course in the cold open (Docs/Story.md: Cold open, 1): out of the evening cloud in the west, low over
 * the plains, down into Gravewind Canyon and up it toward the Mooring Ledge under Ransom's Point.
 *
 * A smooth curve through the set's points (Catmull-Rom), travelled at a speed that starts at the packet skiff's drift, so
 * the cloud parts on a skiff moving as the player's was when it went into its own, holds it while the white thins, picks
 * up over the plains to whatever covers the course in its time, and eases off into the canyon. The skiff's bow follows
 * the curve; it leans into its turns, lifts its nose a little on a climb and heaves slowly, as the packet skiff did.
 *
 * Plain math of time, apart from the world, so the tests check it and a skip lands where the course ends.
 */
class AI_LOOTER_SHOOTER_API FColdOpenCourse
{
public:
	/** The drift it starts at (cm/s): the packet skiff's cruise (SkiffRide::CruiseSpeed). */
	static constexpr float StartSpeed = 650.f;

	/** It holds the drift this long (the white thins over it), picks up over RampSeconds, and slows over the last SlowSeconds. */
	static constexpr float HoldSeconds = 4.5f;
	static constexpr float RampSeconds = 5.f;
	static constexpr float SlowSeconds = 6.f;

	/** How fast it comes in at the end (cm/s). */
	static constexpr float EndSpeed = 900.f;

	/** How far it leans into a turn and lifts its nose on a climb, at most (degrees). */
	static constexpr float MaxBankDegrees = 8.f;
	static constexpr float MaxPitchDegrees = 6.f;

	/** Builds the course through InPoints (world, cm), taking Seconds from the first to the last. False with fewer than two. */
	bool Build(const TArray<FVector>& InPoints, float Seconds);

	bool IsBuilt() const { return Table.Num() >= 2; }

	float GetDuration() const { return Duration; }

	/** Its length along the curve (cm). */
	float GetLength() const;

	/** The speed over the plains that covers the course in its time (cm/s). */
	float GetCruiseSpeed() const { return Cruise; }

	/** How far along the curve it is Seconds in (cm), and how fast it goes then (cm/s). */
	float DistanceAt(float Seconds) const;
	float SpeedAt(float Seconds) const;

	/** The point Distance along the curve, and the way the curve heads there (a unit vector). */
	FVector PointAt(float Distance) const;
	FVector HeadingAt(float Distance) const;

	/** Where the skiff's keel is Seconds in and how it's turned: its bow along the curve, leaning into the turns, heaving. */
	FTransform PoseAt(float Seconds) const;

private:
	/** The curve's point on the segment from Points[Segment] to the next, Alpha (0-1) along it. */
	FVector Evaluate(int32 Segment, float Alpha) const;

	/** The speed's phases, fitted to the duration: held, ramping up, cruising, slowing. */
	float Hold = HoldSeconds;
	float Ramp = RampSeconds;
	float Slow = SlowSeconds;

	TArray<FVector> Points;

	/** Samples along the curve: where each lies and how far along the curve it is, in order. */
	struct FSample
	{
		FVector Location;
		float Distance;
	};
	TArray<FSample> Table;

	float Duration = 0.f;
	float Cruise = StartSpeed;
};
