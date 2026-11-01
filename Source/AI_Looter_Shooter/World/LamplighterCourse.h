#pragma once

#include "CoreMinimal.h"

/**
 * The Lamplighter's course in, as plain math (the tests drive it): a smooth curve through the world points WORLD's build
 * script measures (ALamplighterShip::Course: from ~350 m west over Gravewind Canyon, low over the plains, into the canyon
 * toward the Mooring Ledge), walked by distance. Built once (samples along each span, so a pose is a lookup and no
 * per-frame allocation), it gives where she is and which way she heads at any distance along it, how she banks into its
 * turns and pitches with its climb, her gas-bag's slow bob, and how fast she sails: cruising, slowing to a creep as she
 * nears the grapple stop, creeping at most CreepLimit past it, then holding.
 */
class AI_LOOTER_SHOOTER_API FLamplighterCourse
{
public:
	/** Cruise and creep (cm/s): a frontier airship's glide, and the creep Holloway holds her to while the Lantern looks. */
	static constexpr float CruiseSpeed = 900.f;
	static constexpr float CreepSpeed = 160.f;
	/** How hard she speeds up and slows (cm/s²): a gas-bag's lazy pull. */
	static constexpr float Acceleration = 120.f;
	/** How far past the grapple stop she creeps before she holds (cm). */
	static constexpr float CreepLimit = 3000.f;
	/** Her bank into a turn at most, and her pitch with the climb at most (degrees). */
	static constexpr float MaxBankDegrees = 6.f;
	static constexpr float MaxPitchDegrees = 4.f;
	/** The bob at full sway: heave (cm), roll and pitch (degrees), each at its own pace (Hz) so they never fall into step. */
	static constexpr float HeaveCm = 14.f;
	static constexpr float HeaveHz = 0.19f;
	static constexpr float RollDegrees = 0.9f;
	static constexpr float RollHz = 0.13f;
	static constexpr float PitchDegrees = 0.5f;
	static constexpr float PitchHz = 0.27f;

	/** Builds the curve through Points (two or more; a point repeated is dropped). False, and empty, with fewer. */
	bool Build(const TArray<FVector>& Points);

	bool IsValid() const { return Samples.Num() >= 2; }
	void Reset();

	/** Its length along the curve (cm). */
	float GetLength() const { return Lengths.IsEmpty() ? 0.f : Lengths.Last(); }

	/** How far along the curve its point PointIndex is (clamped to its points). */
	float DistanceAtPoint(int32 PointIndex) const;

	/** Where on the curve Distance along it is (clamped to its ends). */
	FVector LocationAt(float Distance) const;

	/** Which way it goes there, flat (unit). */
	FVector HeadingAt(float Distance) const;

	/** How fast the heading turns there (radians per cm; positive turning toward +Y, the way yaw grows). */
	float TurnRateAt(float Distance) const;

	/** How steeply it climbs there (rise per run). */
	float SlopeAt(float Distance) const;

	/**
	 * Her pose at Distance, at Speed (cm/s) and Clock seconds into the flight: on the curve, heading along it, banked into the
	 * turn by how fast she takes it and pitched with the climb, and bobbing by Sway (0 still, 1 the full bob). Scale is the
	 * actor's own.
	 */
	FTransform PoseAt(float Distance, float Speed, float Clock, float Sway, const FVector& Scale = FVector::OneVector) const;

	/** The same pose with no bank, pitch or bob: where she is and which way she heads (what the tender keeps station on). */
	FTransform CalmPoseAt(float Distance, const FVector& Scale = FVector::OneVector) const;

	/**
	 * The speed she sails for at Distance with the grapple stop at StopDistance: cruise, braking smoothly into the creep as
	 * she nears it, the creep past it, and nothing once CreepLimit past it (or at the curve's end).
	 */
	float TargetSpeed(float Distance, float StopDistance) const;

	/** Where she holds with the grapple stop at StopDistance: CreepLimit past it, or the curve's end. */
	float HoldDistance(float StopDistance) const { return FMath::Min(StopDistance + CreepLimit, GetLength()); }

	/** Speed moved toward Target by at most Acceleration over DeltaSeconds. */
	static float SpeedToward(float Speed, float Target, float DeltaSeconds);

	/** The bob's heave (cm), roll and pitch (degrees) Clock seconds in, at Sway. */
	static void BobAt(float Clock, float Sway, float& OutHeave, float& OutRoll, float& OutPitch);

private:
	/** Index of the sample at or before Distance, and how far on to the next (0-1). */
	void Locate(float Distance, int32& OutIndex, float& OutAlpha) const;

	/** Samples along each span between two points: smooth turns, a small table. */
	static constexpr int32 SamplesPerSpan = 16;

	TArray<FVector> Samples;
	TArray<float> Lengths;
	TArray<int32> PointSamples;
};
