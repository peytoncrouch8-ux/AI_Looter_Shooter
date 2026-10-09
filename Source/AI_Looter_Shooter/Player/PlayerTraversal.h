#pragma once

#include "CoreMinimal.h"
#include "Player/TraversalCurve.h"
#include "Player/TraversalRules.h"

/** What a traversal is planned from: where the body and the eye are and how they move, the obstacle, and where it ends. */
struct FTraversalSetup
{
	ETraversalKind Kind = ETraversalKind::Mantle;
	/** The capsule's centre at the start (world), its way (flat, unit) and size (world cm). */
	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	float Radius = 35.f;
	float HalfHeight = 80.f;
	/** The body's velocity at the start, and its vertical acceleration (the gravity when falling, else 0). */
	FVector Velocity = FVector::ZeroVector;
	float GravityZ = 0.f;
	/** Along Direction from the start's axis: the obstacle's near face and far face (a mantle's top goes on: huge). */
	float NearFace = 0.f;
	float FarFace = 1.e6f;
	/** The obstacle's top (world Z). */
	float TopZ = 0.f;
	/** Where the move ends: along Direction from the start's axis, sideways (to its right), and the feet's height. */
	float EndAlong = 0.f;
	float EndSide = 0.f;
	float EndFeetZ = 0.f;
	/** The speed along Direction it ends at (the run going on after a vault; a step onto the top after a mantle). */
	float ExitSpeed = 0.f;
	float Duration = 0.4f;
	/** How far under the top the capsule's bottom may pass (LooterTraversal::VaultTuck, MantleTuck). */
	float Tuck = LooterTraversal::MantleTuck;
	/** The first-person eye (world): its height, speed and acceleration at the start. */
	float EyeZ = 0.f;
	float EyeSpeed = 0.f;
	float EyeAcceleration = 0.f;
	/** The eye's height over the feet where it ends (standing, less the dip). */
	float EyeEndHeight = 120.f;
	/** The eye's place along Direction from the axis (the first-person camera sits a little ahead of it). */
	float EyeForward = 0.f;
};

/**
 * A traversal's path, planned whole before it starts: the capsule's way along the ground (with any sideways drift eased
 * out), its height, and the first-person eye's own height, each a chain of quintics (FTraversalCurve) that sets off with
 * the speed and acceleration the body already had and ends with the speed it goes on at. The eye has its own gentler
 * curve rather than riding the capsule (the capsule may hop sharply; the view never does), and ends a little low to rise
 * after: the move's give. Planning checks the body's bottom against the obstacle's top corners and the eye against its
 * top, and retimes the move until both are clear. Pure, so it's unit tested; UPlayerLocomotionComponent moves the
 * character along it (PlayerLocomotionTraversal.cpp).
 */
struct FPlayerTraversal
{
	/** Plans the move. False, and nothing planned, when no timing keeps the body and the eye clear of the obstacle. */
	bool Plan(const FTraversalSetup& Setup);

	/** Moves on by DeltaTime, stopping exactly at the end; the move is over once the end is reached. */
	void Advance(float DeltaTime)
	{
		if (!bActive)
		{
			return;
		}
		Time = FMath::Min(Time + FMath::Max(static_cast<double>(DeltaTime), 0.0), Duration);
		if (Time >= Duration)
		{
			bActive = false;
		}
	}

	/** Jumps to the end (the player lost control mid-move: it finishes where it was always going to). */
	void Finish()
	{
		Time = Duration;
		bActive = false;
	}

	/** Ends it where it is (something else moved the player). */
	void Stop() { bActive = false; }

	bool IsActive() const { return bActive; }
	/** The move under way, or the last one. */
	ETraversalKind GetKind() const { return Kind; }
	float GetTime() const { return static_cast<float>(Time); }
	float GetDuration() const { return static_cast<float>(Duration); }
	float GetProgress() const { return Duration > 0.0 ? static_cast<float>(Time / Duration) : 1.f; }

	/** The capsule's centre, its velocity, and the eye's height, speed and acceleration (world), now. */
	FVector GetCenter() const { return CenterAt(Time); }
	FVector GetVelocity() const { return VelocityAt(Time); }
	float GetEyeZ() const { return static_cast<float>(Eye.Value(Time)); }
	float GetEyeSpeed() const { return static_cast<float>(Eye.Speed(Time)); }
	float GetEyeAcceleration() const { return static_cast<float>(Eye.Acceleration(Time)); }

	/** The same at a time into the move (the tests sample it). */
	FVector CenterAt(double At) const
	{
		return Origin + Direction * Along.Value(At) + Right * Side.Value(At) + FVector(0.0, 0.0, Feet.Value(At) + HalfHeight);
	}
	FVector VelocityAt(double At) const
	{
		return Direction * Along.Speed(At) + Right * Side.Speed(At) + FVector(0.0, 0.0, Feet.Speed(At));
	}
	double FeetAt(double At) const { return Feet.Value(At); }
	double AlongAt(double At) const { return Along.Value(At); }
	double AlongSpeedAt(double At) const { return Along.Speed(At); }
	double EyeAt(double At) const { return Eye.Value(At); }

	/**
	 * How low the capsule's bottom may come at a point along the move (S, from the start's axis): the top less the tuck
	 * while the body's axis is over the top, rounding off over the body's radius past either face (its lower hemisphere
	 * clearing the corner), no limit further out.
	 */
	static double RequiredFeet(const FTraversalSetup& Setup, double S);

	/**
	 * The planned path's worst misses, sampled finely (cm, 0 when clear): the capsule's bottom under the corners (beyond
	 * the tuck), the eye under the top's clearance while over the obstacle, and the eye leaving the body: too low over the
	 * feet, or over the capsule's top. And the eye's hardest acceleration (cm/s^2).
	 */
	void Measure(const FTraversalSetup& Setup, double Length, float& OutCorner, float& OutEyeOver, float& OutEyeLow, float& OutEyeHigh,
		float& OutEyeAcceleration) const;

	/** The longest a move is stretched to while retiming it (s). */
	static constexpr double MaxDuration = 0.75;
	/**
	 * The eye's acceleration at most (cm/s^2): 1.4 cm of change in a frame's step at 60 fps, under the slide's jolt limit.
	 * A move that needs harder (a ledge caught while falling fast: the view turns from falling to rising) is stretched; one
	 * stretched as far as it goes is taken anyway, a little sharper.
	 */
	static constexpr double MaxEyeAcceleration = 5000.0;
	/** The eye never comes lower over the feet than this through a move (cm): about kneeling. */
	static constexpr double MinEyeOverFeet = 35.0;
	/** A vault's hop turns over at its top with this much downward acceleration (cm/s^2): the body's, and the eye's. */
	static constexpr double HopTurn = 1200.0;
	static constexpr double EyeHopTurn = 600.0;

private:
	bool Commit(ETraversalKind InKind, double InDuration);
	/** Any sideways drift the body had, eased out as it goes (a strafe into the ledge doesn't stop dead). */
	void BuildSide(const FTraversalSetup& Setup, double Length);
	void BuildEye(const FTraversalSetup& Setup, double Until, double Length);
	void BuildMantle(const FTraversalSetup& Setup, double Length, double Hold, double Rise, double EyeShare);
	void BuildVault(const FTraversalSetup& Setup, double Length, double Lift, double EyeLift);
	void BuildGlide(const FTraversalSetup& Setup, double Length);

	FTraversalCurve Along;
	FTraversalCurve Side;
	FTraversalCurve Feet;
	FTraversalCurve Eye;
	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	FVector Right = FVector::RightVector;
	double HalfHeight = 0.0;
	ETraversalKind Kind = ETraversalKind::None;
	double Duration = 0.0;
	double Time = 0.0;
	bool bActive = false;
};
