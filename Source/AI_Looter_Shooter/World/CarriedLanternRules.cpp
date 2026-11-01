// GivenToTheWind's names and CarriedLanternRules' math: Abel's light carried home, the freed souls' trail and their flight.

#include "World/CarriedLanternRules.h"

namespace GivenToTheWind
{
	FName Mission() { return FName(TEXT("Main7")); }
	FName SixSavedEvent() { return FName(TEXT("Abel.SixSaved")); }
	FName ReliquaryLanternTag() { return FName(TEXT("Reliquary_Lantern")); }
	FName SoulCoalTag() { return FName(TEXT("SoulCoal")); }
	FName DeliaChapelTag() { return FName(TEXT("Speaker_DeliaChapel")); }
	FName WestRoadCrewId() { return FName(TEXT("WestRoadCrew")); }
	FName WaitPlaceTag() { return FName(TEXT("Place_GravewindPoint")); }
	FName SaintAdaAwakeFlag() { return FName(TEXT("Main7.SaintAdaAwake")); }
	FName SoulCoalFlag(FName LumpId) { return FName(*FString::Printf(TEXT("SoulCoal.%s"), *LumpId.ToString())); }
	FName CoalWordFlag() { return FName(TEXT("Main7.CoalWord")); }
	FName LatchWordFlag() { return FName(TEXT("Main7.LatchWord")); }
	FName CoalLines() { return FName(TEXT("DA_Lines_Main7Coal")); }
	FName LatchLines() { return FName(TEXT("DA_Lines_Main7Latch")); }
	FName RoadLines() { return FName(TEXT("DA_Lines_Main7Road")); }
	FName ArriveLines() { return FName(TEXT("DA_Lines_Main7Arrive")); }
	FName WakesLines() { return FName(TEXT("DA_Lines_SaintAdaWakes")); }
}

namespace CarriedLanternRules
{
	float Drained(float Light, int32 Lines, float DeltaSeconds)
	{
		if (Lines <= 0)
		{
			return FMath::Clamp(Light, MinLight, 1.f);
		}
		return FMath::Max(MinLight, Light - DrainPerLine * static_cast<float>(Lines) * FMath::Max(DeltaSeconds, 0.f));
	}

	float Refilled(float Light, float SinceFree, float DeltaSeconds)
	{
		if (SinceFree < RefillDelay)
		{
			return Light;
		}
		return FMath::Min(1.f, Light + RefillPerSecond * FMath::Max(DeltaSeconds, 0.f));
	}

	FVector FollowerSlot(int32 Index, float Time)
	{
		// Back from the lantern in a loose line, alternating a little left and right, each one higher, all swaying slowly
		// out of step (a phase each), so they read as souls drifting after a light and not a formation.
		const float I = static_cast<float>(FMath::Max(Index, 0));
		const float Back = 140.f + 75.f * I;
		const float Side = ((Index % 2 == 0) ? 1.f : -1.f) * (25.f + 6.f * I) + 30.f * FMath::Sin(Time * 0.7f + I * 1.9f);
		const float Up = 95.f + 9.f * I + 18.f * FMath::Sin(Time * 1.1f + I * 2.3f);
		return FVector(-Back, Side, Up);
	}

	FVector WaitSlot(int32 Index, int32 Count, float Time)
	{
		const int32 Total = FMath::Max(Count, 1);
		const float Angle = UE_TWO_PI * static_cast<float>(Index) / static_cast<float>(Total) + Time * 0.15f;
		const float Radius = 160.f + 25.f * FMath::Sin(Time * 0.5f + static_cast<float>(Index));
		return FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 120.f + 30.f * FMath::Sin(Time * 0.8f + static_cast<float>(Index) * 1.3f));
	}

	FVector ReleasedAt(const FVector& From, float Seconds, int32 Index)
	{
		// The level's west is -Y (X north, Y east). Lifted first, then carried off ever faster on the wind toward the sunset,
		// a little south of west, each soul on its own slight line so they make a river and not a beam.
		const float T = FMath::Max(Seconds, 0.f);
		const float Spread = static_cast<float>((Index * 37) % 11) - 5.f;
		const float Rise = 900.f * (1.f - FMath::Exp(-T * 0.45f)) + 60.f * T;
		const float West = 120.f * T * T + 250.f * T;
		return From + FVector(-0.18f * West + Spread * 40.f * FMath::Min(T, 3.f), -West, Rise);
	}

	float ReleasedGlow(float Seconds)
	{
		const float In = FMath::SmoothStep(0.f, 1.2f, Seconds);
		const float Out = 1.f - FMath::SmoothStep(ReleaseSeconds * 0.6f, ReleaseSeconds, Seconds);
		return FMath::Clamp(In * Out, 0.f, 1.f);
	}
}
