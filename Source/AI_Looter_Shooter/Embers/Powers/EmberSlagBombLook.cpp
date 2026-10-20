// UEmberSlagBomb: how the pool looks each frame (its glowing edge, the hot blobs crusting over, the burst's ring of fire
// racing out, sparks and smoke rising, its light). The lob, the burst and the burns are EmberSlagBomb.cpp.

#include "Embers/Powers/EmberSlagBomb.h"
#include "Embers/Powers/EmberPropActor.h"
#include "Weapons/WeaponFX.h"

namespace
{
	/** The slag's colours: white-hot, molten, cooling to a dark crust. */
	const FLinearColor WhiteHot(1.f, 0.85f, 0.55f);
	const FLinearColor Molten(1.f, 0.42f, 0.05f);
	const FLinearColor Cooling(0.75f, 0.12f, 0.02f);
	/** The burst's ring of fire runs out over this long. */
	constexpr float RingSeconds = 0.3f;
	/** Sparks and smoke rising off the pool, a second. */
	constexpr float PoolSparksPerSecond = 22.f;
	constexpr float PoolSmokePerSecond = 4.f;
	/**
	 * A low haze of molten light hanging over it (puffs in the slag's own colour, unlit, so they glow), a second: without it
	 * the pool read in daylight as a thin ring with a few dots in it (2026-10-20 play check), not a pool of slag.
	 */
	constexpr float PoolHazePerSecond = 9.f;
}

void UEmberSlagBomb::DrawPool(float DeltaSeconds)
{
	FWeaponFX* FX = Effects();
	if (!FX)
	{
		return;
	}
	const float Cool = FMath::Clamp(PoolAge / PoolSeconds, 0.f, 1.f);
	const FLinearColor Hot = FMath::Lerp(Molten, Cooling, Cool);
	const float Fade = FMath::Clamp(PoolLeft / 0.6f, 0.f, 1.f);
	// Its glowing edge, pulsing a little.
	EmberFX::DrawLine(*FX, Edge, true, 7.f, Hot, (18.f - 8.f * Cool) * (0.85f + 0.15f * FMath::Sin(PoolAge * 7.f)) * Fade);
	// Hot blobs lying on it, flickering; more of them crusted over (dark) as it cools.
	for (int32 Index = 0; Index < Blobs.Num(); ++Index)
	{
		const float Seed = FMath::Frac(Index * 0.618034f);
		if (Seed < Cool * 0.65f)
		{
			continue;
		}
		const float Angle = Seed * UE_TWO_PI;
		const FVector Way(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const float Size = 16.f + 24.f * Seed;
		const float Flicker = 0.7f + 0.3f * FMath::Sin(PoolAge * (5.f + 6.f * Seed) + Index);
		FX->AddStreak(Blobs[Index] - Way * Size, Blobs[Index] + Way * Size, Size * 0.9f, Seed > 0.8f ? WhiteHot : Hot, (28.f - 12.f * Cool) * Flicker * Fade);
	}
	// The molten haze, thinning as it crusts over.
	if (!Blobs.IsEmpty() && Random.FRand() < PoolHazePerSecond * DeltaSeconds * (1.f - 0.6f * Cool))
	{
		const FVector& Spot = Blobs[Random.RandRange(0, Blobs.Num() - 1)];
		FX->SpawnDustPuff(Spot + FVector(0.0, 0.0, 12.0), FVector(0.0, 0.0, Random.FRandRange(15.f, 35.f)) + Random.GetUnitVector() * 15.f, Hot * 0.9f,
			0.3f * Fade, Random.FRandRange(70.f, 110.f), Random.FRandRange(150.f, 210.f), Random.FRandRange(1.f, 1.5f));
	}
	// The first moments: the ring of fire racing out from the burst.
	if (RingAge >= 0.f)
	{
		RingAge += DeltaSeconds;
		const float U = FMath::Clamp(RingAge / RingSeconds, 0.f, 1.f);
		TArray<FVector> Racing;
		Racing.Reserve(Edge.Num());
		for (const FVector& Point : Edge)
		{
			Racing.Add(FMath::Lerp(BurstAt, Point, 1.f - FMath::Square(1.f - U)) + FVector(0.0, 0.0, 10.0));
		}
		EmberFX::DrawLine(*FX, Racing, true, 9.f, WhiteHot, 30.f * (1.f - U));
		if (U >= 1.f)
		{
			RingAge = -1.f;
		}
	}
	// Sparks and pale smoke rising off it.
	SmokeClock += DeltaSeconds;
	const int32 SparksNow = FMath::FloorToInt(PoolSparksPerSecond * DeltaSeconds + Random.FRand());
	for (int32 Index = 0; Index < SparksNow && !Blobs.IsEmpty(); ++Index)
	{
		const FVector& Spot = Blobs[Random.RandRange(0, Blobs.Num() - 1)];
		Sparks.Spawn(Spot, FVector(0.0, 0.0, Random.FRandRange(140.f, 300.f)) + Random.GetUnitVector() * 40.f, Random.FRandRange(0.5f, 0.9f), 1.1f, Hot,
			18.f * Fade, 250.f, 1.f);
	}
	if (SmokeClock >= 1.f / PoolSmokePerSecond && !Blobs.IsEmpty())
	{
		SmokeClock = 0.f;
		const FVector& Spot = Blobs[Random.RandRange(0, Blobs.Num() - 1)];
		FX->SpawnWisp(Spot + FVector(0.0, 0.0, 15.0), FVector(0.0, 0.0, Random.FRandRange(50.f, 90.f)), 0.2f, 35.f, 150.f, Random.FRandRange(1.6f, 2.4f));
	}
	if (AEmberPropActor* Here = Anchor.Get())
	{
		Here->SetLightIntensity(30.f * (1.f - 0.6f * Cool) * (0.85f + 0.15f * FMath::Sin(PoolAge * 11.f)) * Fade);
	}
}
