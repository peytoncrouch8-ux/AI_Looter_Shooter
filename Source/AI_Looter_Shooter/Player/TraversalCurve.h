#pragma once

#include "CoreMinimal.h"

/**
 * One coordinate moved on quintic curves in a row, each picking up exactly where the last ended: its place, speed and
 * acceleration never jump, so neither does anything riding it (FPlayerTraversal's body and eye). Each piece arrives at
 * its own place, speed and acceleration, so a move can end still or carry on at a run. Past its end it carries on from
 * the state it ended in. Pure, so it's unit tested.
 */
struct FTraversalCurve
{
	/** At Value at time 0, moving at Speed and accelerating at Acceleration. */
	void Begin(double Value, double Speed, double Acceleration)
	{
		Count = 0;
		EndTime = 0.0;
		EndValue = Value;
		EndSpeed = Speed;
		EndAcceleration = Acceleration;
	}

	/** On to Value, arriving at time Until with Speed and Acceleration. A time not after the last end adds nothing. */
	void Add(double Until, double Value, double Speed = 0.0, double Acceleration = 0.0)
	{
		if (Count >= MaxSegments || Until <= EndTime + 1.e-4)
		{
			return;
		}
		// The quintic with the start's and the end's place, speed and acceleration (the minimum-jerk curve FViewEase uses,
		// with an end that needn't be at rest).
		FSegment& Segment = Segments[Count++];
		const double T = Until - EndTime;
		const double H = Value - EndValue;
		const double V0 = EndSpeed;
		const double A0 = EndAcceleration;
		Segment.Start = EndTime;
		Segment.Length = T;
		Segment.C[0] = EndValue;
		Segment.C[1] = V0;
		Segment.C[2] = 0.5 * A0;
		Segment.C[3] = (20.0 * H - (8.0 * Speed + 12.0 * V0) * T - (3.0 * A0 - Acceleration) * T * T) / (2.0 * T * T * T);
		Segment.C[4] = (-30.0 * H + (14.0 * Speed + 16.0 * V0) * T + (3.0 * A0 - 2.0 * Acceleration) * T * T) / (2.0 * T * T * T * T);
		Segment.C[5] = (12.0 * H - 6.0 * (Speed + V0) * T + (Acceleration - A0) * T * T) / (2.0 * T * T * T * T * T);
		EndTime = Until;
		EndValue = Value;
		EndSpeed = Speed;
		EndAcceleration = Acceleration;
	}

	double Value(double Time) const { return Evaluate(Time, 0); }
	double Speed(double Time) const { return Evaluate(Time, 1); }
	double Acceleration(double Time) const { return Evaluate(Time, 2); }
	double GetEndTime() const { return EndTime; }

private:
	static constexpr int32 MaxSegments = 4;

	struct FSegment
	{
		double Start = 0.0;
		double Length = 0.0;
		double C[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	};

	double Evaluate(double Time, int32 Derivative) const
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FSegment& Segment = Segments[Index];
			if (Time <= Segment.Start + Segment.Length)
			{
				const double* C = Segment.C;
				const double T = FMath::Max(Time - Segment.Start, 0.0);
				switch (Derivative)
				{
				case 0: return C[0] + T * (C[1] + T * (C[2] + T * (C[3] + T * (C[4] + T * C[5]))));
				case 1: return C[1] + T * (2.0 * C[2] + T * (3.0 * C[3] + T * (4.0 * C[4] + T * 5.0 * C[5])));
				default: return 2.0 * C[2] + T * (6.0 * C[3] + T * (12.0 * C[4] + T * 20.0 * C[5]));
				}
			}
		}
		// Past the end (or nothing added): on from the end state.
		const double After = FMath::Max(Time - EndTime, 0.0);
		switch (Derivative)
		{
		case 0: return EndValue + EndSpeed * After + 0.5 * EndAcceleration * After * After;
		case 1: return EndSpeed + EndAcceleration * After;
		default: return EndAcceleration;
		}
	}

	FSegment Segments[MaxSegments];
	int32 Count = 0;
	double EndTime = 0.0;
	double EndValue = 0.0;
	double EndSpeed = 0.0;
	double EndAcceleration = 0.0;
};
