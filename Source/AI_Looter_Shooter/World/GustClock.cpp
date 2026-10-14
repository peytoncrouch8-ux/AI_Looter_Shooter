#include "World/GustClock.h"

void FGustClock::Reset(int32 Seed, float InWindYaw)
{
	Random.Initialize(Seed);
	WindYaw = InWindYaw;
	bBlowing = false;
	Age = 0.f;
	Count = 0;
	Current = FGustEvent();
	// The first comes sooner than the rest: a player who has just arrived sees the wind's life early on.
	Until = Random.FRandRange(FirstIntervalMin, FirstIntervalMax);
}

FGustEvent FGustClock::Roll()
{
	FGustEvent Gust;
	Gust.Direction = FRotator(0.f, WindYaw + Random.FRandRange(-DirectionVariance, DirectionVariance), 0.f).Vector();
	Gust.Strength = Random.FRandRange(StrengthMin, StrengthMax);
	Gust.Duration = Random.FRandRange(DurationMin, DurationMax);
	return Gust;
}

bool FGustClock::Advance(float DeltaSeconds)
{
	const float Step = FMath::Max(DeltaSeconds, 0.f);
	if (bBlowing)
	{
		Age += Step;
		if (Age >= Current.Duration)
		{
			bBlowing = false;
		}
	}
	Until -= Step;
	if (Until > 0.f)
	{
		return false;
	}
	// Due: it begins now, already as far in as the step overshot, and the next is counted from its start.
	const float Overshoot = -Until;
	Current = Roll();
	bBlowing = Overshoot < Current.Duration;
	Age = Overshoot;
	Until = Random.FRandRange(IntervalMin, IntervalMax) - Overshoot;
	++Count;
	return true;
}

bool FGustClock::StartNow(const FGustEvent& Gust)
{
	if (bBlowing)
	{
		return false;
	}
	Current = Gust;
	Current.Direction = Gust.Direction.GetSafeNormal2D().IsNearlyZero() ? FRotator(0.f, WindYaw, 0.f).Vector() : Gust.Direction.GetSafeNormal2D();
	Current.Strength = FMath::Clamp(Gust.Strength, 0.f, 1.f);
	Current.Duration = FMath::Max(Gust.Duration, 0.1f);
	bBlowing = true;
	Age = 0.f;
	++Count;
	// The one due next stays due, but never on top of this one.
	Until = FMath::Max(Until, Current.Duration + 1.f);
	return true;
}

float FGustClock::GetStrengthNow() const
{
	return bBlowing ? Current.Strength * Shape(Age, Current.Duration) : 0.f;
}

float FGustClock::Shape(float GustAge, float GustDuration)
{
	if (GustDuration <= 0.f || GustAge < 0.f || GustAge >= GustDuration)
	{
		return 0.f;
	}
	// It comes on quickly and dies away slowly, as a real gust does.
	const float In = FMath::SmoothStep(0.f, 0.3f * GustDuration, GustAge);
	const float Out = 1.f - FMath::SmoothStep(0.5f * GustDuration, GustDuration, GustAge);
	return In * Out;
}
