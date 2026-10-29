#include "Combat/HitPose.h"

namespace
{
	/** Below this (cm or degrees, and per second for the speed) a spring is at rest: nothing a player could see. */
	constexpr float SettledSize = 0.01f;
	/** The most of a frame a spring is stepped through (a hitch would otherwise fling it). */
	constexpr float MostStepped = 0.5f;

	/** 0 to 1 eased both ways (smoothstep). */
	float Smooth(float X)
	{
		const float T = FMath::Clamp(X, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}
}

void FHitSpring::Add(const FVector& K, float HoldSeconds)
{
	// The strongest kick stands (a blast's pellets land together; the last carries its whole share): they don't add up.
	if (K.SizeSquared() >= Kick.SizeSquared())
	{
		Kick = K;
		Hold = FMath::Max(Hold, HoldSeconds);
	}
}

void FHitSpring::Step(float Dt, float Omega, float Fade)
{
	const float Seconds = FMath::Min(Dt, MostStepped);
	if (Seconds <= 0.f)
	{
		return;
	}
	// Fixed steps, so a hit looks the same at 30 fps and at 144 (the spider's jolt has always been stepped this way).
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / StepSeconds - 0.01f));
	const float Step = Seconds / Steps;
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		Hold -= Step;
		if (Hold <= 0.f)
		{
			Hold = 0.f;
			Kick -= Kick * FMath::Min(Fade * Step, 1.f);
		}
		// Critically damped: it gets there as fast as it can without overshooting much, and settles.
		Velocity += (Omega * Omega * (Kick - Offset) - 2.f * Omega * Velocity) * Step;
		Offset += Velocity * Step;
	}
}

bool FHitSpring::IsSettled() const
{
	return Hold <= 0.f && Kick.IsNearlyZero(SettledSize) && Offset.IsNearlyZero(SettledSize) && Velocity.IsNearlyZero(SettledSize);
}

float HitEnvelope(float T, float In, float Out)
{
	if (T <= 0.f)
	{
		return 0.f;
	}
	const float Up = FMath::Max(In, KINDA_SMALL_NUMBER);
	if (T < Up)
	{
		return Smooth(T / Up);
	}
	const float Down = FMath::Max(Out, KINDA_SMALL_NUMBER);
	return T >= Up + Down ? 0.f : 1.f - Smooth((T - Up) / Down);
}

int32 WoundStage(float HealthShare)
{
	return HealthShare < 0.25f ? 2 : (HealthShare < 0.5f ? 1 : 0);
}
