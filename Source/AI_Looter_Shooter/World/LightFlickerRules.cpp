// LightFlickerRules: a flame's breath, gutter and carry, as numbers (ULightFlickerSubsystem writes them on lights).

#include "World/LightFlickerRules.h"

namespace
{
	/**
	 * The breath's three layers: how fast each runs (per second) and how much of it there is. Perlin noise rarely reaches
	 * its ends, so the sum is raised (Lift) and clamped: most of the time the light moves 4-6%, now and then the full 8%.
	 */
	constexpr float SlowRate = 1.3f;
	constexpr float MidRate = 3.1f;
	constexpr float QuickRate = 7.7f;
	constexpr float SlowShare = 0.5f;
	constexpr float MidShare = 0.32f;
	constexpr float QuickShare = 0.18f;
	constexpr float Lift = 1.7f;

	/** A fresh flame's seed lies in [0, SeedRange): far enough apart that two lamps side by side never breathe together. */
	constexpr float SeedRange = 512.f;
}

float LightFlickerRules::BreathAt(float Seconds, float Seed)
{
	const float Sum = SlowShare * FMath::PerlinNoise1D(Seconds * SlowRate + Seed)
		+ MidShare * FMath::PerlinNoise1D(Seconds * MidRate + Seed * 1.7f + 17.3f)
		+ QuickShare * FMath::PerlinNoise1D(Seconds * QuickRate + Seed * 2.3f + 41.9f);
	return FMath::Clamp(Sum * Lift, -1.f, 1.f);
}

float LightFlickerRules::GutterAt(float Since)
{
	if (Since < 0.f || Since > GutterSeconds)
	{
		return 0.f;
	}
	// Down and back up in one smooth dip.
	return FMath::Square(FMath::Sin(UE_PI * Since / GutterSeconds));
}

float LightFlickerRules::CarrySeconds(float Hold)
{
	return CarryOutSeconds + FMath::Max(Hold, 0.f) + CarryBackSeconds;
}

float LightFlickerRules::CarryAt(float Since, float Low, float Hold)
{
	if (Since <= 0.f)
	{
		return 1.f;
	}
	if (Since < CarryOutSeconds)
	{
		return FMath::Lerp(1.f, Low, FMath::SmoothStep(0.f, 1.f, Since / CarryOutSeconds));
	}
	Since -= CarryOutSeconds;
	if (Since < Hold)
	{
		return Low;
	}
	Since -= Hold;
	if (Since < CarryBackSeconds)
	{
		return FMath::Lerp(Low, 1.f, FMath::SmoothStep(0.f, 1.f, Since / CarryBackSeconds));
	}
	return 1.f;
}

// ---------------------------------------------------------------------------
// One flame
// ---------------------------------------------------------------------------

void FFlameFlicker::Start(FRandomStream& Random, bool bInCarried)
{
	using namespace LightFlickerRules;
	Seed = Random.FRandRange(0.f, SeedRange);
	Clock = 0.f;
	bCarries = bInCarried;
	// The first gutter and carry may come sooner than the later ones, so a short visit can see one.
	NextGutter = Random.FRandRange(GutterGapMin * 0.25f, GutterGapMax);
	GutterStart = -1.f;
	NextCarry = Random.FRandRange(CarryGapMin * 0.3f, CarryGapMax);
	CarryStart = -1.f;
	CarryLow = 1.f;
	CarryHold = 0.f;
}

void FFlameFlicker::Advance(float DeltaSeconds, FRandomStream& Random)
{
	using namespace LightFlickerRules;
	Clock += FMath::Max(DeltaSeconds, 0.f);
	if (GutterStart >= 0.f && Clock - GutterStart > GutterSeconds)
	{
		GutterStart = -1.f;
	}
	if (GutterStart < 0.f && Clock >= NextGutter)
	{
		GutterStart = Clock;
		NextGutter = Clock + GutterSeconds + Random.FRandRange(GutterGapMin, GutterGapMax);
	}
	if (!bCarries)
	{
		return;
	}
	if (CarryStart >= 0.f && Clock - CarryStart > CarrySeconds(CarryHold))
	{
		CarryStart = -1.f;
	}
	if (CarryStart < 0.f && Clock >= NextCarry)
	{
		CarryStart = Clock;
		CarryLow = Random.FRandRange(CarryLowMin, CarryLowMax);
		CarryHold = Random.FRandRange(CarryHoldMin, CarryHoldMax);
		NextCarry = Clock + CarrySeconds(CarryHold) + Random.FRandRange(CarryGapMin, CarryGapMax);
	}
}

bool FFlameFlicker::IsGuttering() const
{
	return GutterStart >= 0.f && Clock - GutterStart <= LightFlickerRules::GutterSeconds;
}

bool FFlameFlicker::IsCarried() const
{
	return bCarries && CarryStart >= 0.f && Clock - CarryStart <= LightFlickerRules::CarrySeconds(CarryHold);
}

float FFlameFlicker::GetFlame() const
{
	using namespace LightFlickerRules;
	const float Dip = GutterStart >= 0.f ? GutterAt(Clock - GutterStart) : 0.f;
	return 1.f + Breath * BreathAt(Clock, Seed) - GutterDepth * Dip;
}

float FFlameFlicker::GetCarry() const
{
	return bCarries && CarryStart >= 0.f ? LightFlickerRules::CarryAt(Clock - CarryStart, CarryLow, CarryHold) : 1.f;
}

float FFlameFlicker::GetGlassFactor() const
{
	return (1.f + LightFlickerRules::GlassShare * (GetFlame() - 1.f)) * GetCarry();
}
