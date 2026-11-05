// ARespawnMarker's crows and bell: Hob's crows riding the fire's heat over a lit post, the post's own perched crows, and
// its bell's swing when it's rung (a find, a homecoming, a landing, a mark again).

#include "World/RespawnMarker.h"
#include "World/RoostCrows.h"
#include "World/RoostSettings.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

namespace
{
	/** The bell's swing on its loop, as the vigil stone's: how far at a full ring (degrees), how fast, how soon it settles (s). */
	constexpr float BellSwingDegrees = 24.f;
	constexpr float BellSwingHz = 1.2f;
	constexpr float BellSettleSeconds = 1.3f;
}

ARoostCrows* ARespawnMarker::GetCrows() const
{
	return Crows.Get();
}

ARoostCrows* ARespawnMarker::GetCirclingCrows() const
{
	return CirclingCrows.Get();
}

void ARespawnMarker::UpdateCirclingCrows()
{
	ARoostCrows* Circling = CirclingCrows.Get();
	const int32 Count = URoostSettings::Get().CirclingCrows;
	UWorld* World = GetWorld();
	const bool bWanted = bLit && bInPlay && Count > 0 && World;
	if (!bWanted)
	{
		if (Circling)
		{
			Circling->Destroy();
		}
		CirclingCrows.Reset();
		return;
	}
	if (Circling)
	{
		return;
	}
	// A few of Hob's crows riding the fire's heat over the basket, as long as it burns: with the column, a roost's form far off.
	const FTransform At(FRotator::ZeroRotator, GetBeaconLocation());
	Circling = World->SpawnActorDeferred<ARoostCrows>(ARoostCrows::StaticClass(), At, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Circling)
	{
		return;
	}
	Circling->MakeCircling(GetBeaconLocation(), Count);
	Circling->Seed = static_cast<int32>(GetTypeHash(GetMarkerId()) * 31u) | 1;
	Circling->FinishSpawning(At);
	CirclingCrows = Circling;
}

// ---------------------------------------------------------------------------
// The bell
// ---------------------------------------------------------------------------

void ARespawnMarker::RingBell(float Strength)
{
	// Rung again while it swings, it swings on from the harder of the two.
	BellStrength = FMath::Clamp(FMath::Max(Strength, BellStrength * FMath::Exp(-BellTime / BellSettleSeconds)), 0.f, 1.f);
	BellTime = 0.f;
	UpdateTicking();
}

void ARespawnMarker::SwingBell(float DeltaSeconds)
{
	if (BellStrength <= 0.f)
	{
		return;
	}
	BellTime += DeltaSeconds;
	const float Fade = FMath::Exp(-BellTime / BellSettleSeconds);
	if (Fade < 0.02f)
	{
		BellStrength = 0.f;
		Bell->SetRelativeRotation(BellRest);
		return;
	}
	// Side to side on its loop (about the socket's +X, as the vigil stone's bell), dying away.
	Bell->SetRelativeRotation(BellRest * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(GetBellSwing())));
}

float ARespawnMarker::GetBellSwing() const
{
	if (BellStrength <= 0.f)
	{
		return 0.f;
	}
	return BellSwingDegrees * BellStrength * FMath::Exp(-BellTime / BellSettleSeconds) * FMath::Sin(BellTime * UE_TWO_PI * BellSwingHz);
}

