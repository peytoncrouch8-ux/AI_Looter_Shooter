#pragma once

#include "CoreMinimal.h"

/**
 * A value that eases to its target on a minimum-jerk curve (a quintic): each time the target changes it sets off from
 * where it is, at the speed and acceleration it already has, and comes to rest on the target after the time it was given.
 * Its position, speed and acceleration never jump however the target changes, so a view moved by it never snaps or
 * jolts. (The stance blends it replaced were linear ramps through a smoothstep: a stance that changed back mid-way
 * reversed their speed in one frame.) A limit on its acceleration stretches a move that would need a harder turn (a slide
 * cancelled while the view is still dropping fast). Shift moves the value at once while keeping its speed, for a jump in
 * what the value is measured from. Pure, so it's unit tested.
 */
struct FViewEase
{
	/** At Value, at rest. */
	void Reset(float Value)
	{
		Target = Value;
		Coefficients[0] = Value;
		for (int32 Index = 1; Index < 6; ++Index)
		{
			Coefficients[Index] = 0.0;
		}
		Duration = 0.0;
		Time = 0.0;
	}

	/**
	 * Heads for NewTarget, arriving at rest Seconds from now, or later if getting there that fast would take more than
	 * MaxAcceleration (units per second squared; 0 = no limit). Asking for the target it already has changes nothing, so
	 * it can be asked every frame.
	 */
	void SetTarget(float NewTarget, float Seconds, float MaxAcceleration = 0.f)
	{
		if (NewTarget == Target)
		{
			return;
		}
		Plan(GetValue(), GetVelocity(), GetAcceleration(), NewTarget, Seconds, MaxAcceleration);
	}

	/**
	 * Moves the value by Delta at once, keeping its speed and its target, which it then eases on to within Seconds (or
	 * the time it had left, if that's longer).
	 */
	void Shift(float Delta, float Seconds, float MaxAcceleration = 0.f)
	{
		if (Delta == 0.f)
		{
			return;
		}
		const float Left = static_cast<float>(Duration - Time);
		Plan(GetValue() + Delta, GetVelocity(), GetAcceleration(), Target, FMath::Max(Seconds, Left), MaxAcceleration);
	}

	void Advance(float DeltaTime)
	{
		Time = FMath::Min(Time + FMath::Max(static_cast<double>(DeltaTime), 0.0), Duration);
	}

	float GetValue() const { return Evaluate(0); }
	/** Units per second. */
	float GetVelocity() const { return Evaluate(1); }
	/** Units per second squared. */
	float GetAcceleration() const { return Evaluate(2); }
	float GetTarget() const { return Target; }
	/** Arrived and at rest. */
	bool IsSettled() const { return Time >= Duration; }
	/** Seconds the move under way takes in all (after any stretching for the acceleration limit). */
	float GetDuration() const { return static_cast<float>(Duration); }

private:
	void Plan(float From, float Speed, float Acceleration, float To, float Seconds, float MaxAcceleration)
	{
		Target = To;
		Time = 0.0;
		double Length = FMath::Max(static_cast<double>(Seconds), 0.0);
		if (Length <= UE_KINDA_SMALL_NUMBER)
		{
			Reset(To);
			return;
		}
		// Stretch the move until its hardest turn is within the limit (a few tries). The acceleration it already has is
		// kept whatever it is, so the limit is never under that.
		const float Limit = MaxAcceleration > 0.f ? FMath::Max(MaxAcceleration, FMath::Abs(Acceleration) * 1.05f) : 0.f;
		Solve(From, Speed, Acceleration, To, Length);
		for (int32 Try = 0; Try < 8 && Limit > 0.f && PeakAcceleration() > Limit; ++Try)
		{
			Length *= 1.2;
			Solve(From, Speed, Acceleration, To, Length);
		}
	}

	/** The quintic from (From, Speed, Acceleration) to (To, at rest) over Length seconds. */
	void Solve(double From, double Speed, double Acceleration, double To, double Length)
	{
		const double T = Length;
		const double H = To - From;
		Coefficients[0] = From;
		Coefficients[1] = Speed;
		Coefficients[2] = 0.5 * Acceleration;
		Coefficients[3] = (20.0 * H - 12.0 * Speed * T - 3.0 * Acceleration * T * T) / (2.0 * T * T * T);
		Coefficients[4] = (-30.0 * H + 16.0 * Speed * T + 3.0 * Acceleration * T * T) / (2.0 * T * T * T * T);
		Coefficients[5] = (12.0 * H - 6.0 * Speed * T - Acceleration * T * T) / (2.0 * T * T * T * T * T);
		Duration = Length;
	}

	/** The hardest acceleration along the planned move, sampled. */
	float PeakAcceleration() const
	{
		double Peak = 0.0;
		constexpr int32 Samples = 16;
		for (int32 Index = 0; Index <= Samples; ++Index)
		{
			Peak = FMath::Max(Peak, FMath::Abs(AccelerationAt(Duration * Index / Samples)));
		}
		return static_cast<float>(Peak);
	}

	double AccelerationAt(double T) const
	{
		const double* C = Coefficients;
		return 2.0 * C[2] + T * (6.0 * C[3] + T * (12.0 * C[4] + T * 20.0 * C[5]));
	}

	float Evaluate(int32 Derivative) const
	{
		if (Time >= Duration)
		{
			return Derivative == 0 ? Target : 0.f;
		}
		const double* C = Coefficients;
		const double T = Time;
		switch (Derivative)
		{
		case 0: return static_cast<float>(C[0] + T * (C[1] + T * (C[2] + T * (C[3] + T * (C[4] + T * C[5])))));
		case 1: return static_cast<float>(C[1] + T * (2.0 * C[2] + T * (3.0 * C[3] + T * (4.0 * C[4] + T * 5.0 * C[5]))));
		default: return static_cast<float>(AccelerationAt(T));
		}
	}

	double Coefficients[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	float Target = 0.f;
	double Duration = 0.0;
	double Time = 0.0;
};
