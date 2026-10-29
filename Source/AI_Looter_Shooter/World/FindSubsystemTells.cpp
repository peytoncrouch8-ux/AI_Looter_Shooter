#include "World/FindSubsystem.h"
#include "Audio/LooterSound.h"
#include "World/FindGlints.h"
#include "World/FindSpot.h"
#include "World/RoostCrows.h"
#include "World/WorldQueries.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// UFindSubsystem's tells for finds not yet found (the user's pick, 2026-10-28): two crows circling low over each, and now and
// then a glint off its brass. None baked: made as play begins (after the legacy pass), gone with the find.

namespace
{
	/** Crows sent off by a find go once out of the player's sight, or this far off, after at least LeaveSeenFor (cm, s)... */
	constexpr double LeaveFar = 6000.0;
	constexpr float LeaveSeenFor = 1.5f;
	/** ...and at the latest after this long whatever the player looks at (s). */
	constexpr float LeaveAtLatest = 20.f;
	/** Out of sight: further from the view's middle than this (the cosine of the angle off it). */
	constexpr double OutOfSightCos = 0.35;
	/** A trace to a glint that stops this near it hit the find itself (its box, its lid): still a clear line (cm). */
	constexpr double GlintSelfMargin = 80.0;
}

void UFindSubsystem::StartTells()
{
	for (FSpotState& State : States)
	{
		UpdateTellsFor(State);
	}
}

void UFindSubsystem::UpdateTellsFor(FSpotState& State)
{
	UWorld* World = GetWorld();
	AFindSpot* Spot = State.Spot.Get();
	const bool bUnfound = Spot && !IsFound(Spot->FindId);
	if (!bUnfound || !bTellsEnabled)
	{
		State.GlintAge = -1.f;
	}
	const bool bWanted = World && bWatching && bTellsEnabled && bUnfound && Spot->Crows > 0;
	ARoostCrows* Birds = State.Crows.Get();
	if (!bWanted)
	{
		if (Birds)
		{
			Birds->Destroy();
		}
		State.Crows.Reset();
		return;
	}
	if (Birds)
	{
		return;
	}
	// Hob's crows' aerial flock, low and slow over the find, no beacon column under it: never read as a roost (high over
	// a red column), culled past the settings' CirclingCullDistance.
	const FVector Center = Spot->GetActorLocation() + FVector(0.0, 0.0, Spot->CrowHeight);
	const FTransform At(FRotator::ZeroRotator, Center);
	Birds = World->SpawnActorDeferred<ARoostCrows>(ARoostCrows::StaticClass(), At, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Birds)
	{
		return;
	}
	Birds->MakeCircling(Center, Spot->Crows);
	Birds->Seed = static_cast<int32>(static_cast<uint32>(FindRules::SeedFor(Spot->FindId)) * 31u) | 1;
	Birds->FinishSpawning(At);
	State.Crows = Birds;
}

void UFindSubsystem::SendCrowsOff(FSpotState& State)
{
	ARoostCrows* Birds = State.Crows.Get();
	State.Crows.Reset();
	if (!Birds)
	{
		return;
	}
	// The find made: the crows lift and go. They're let be until the player looks away, so none pops out of the sky in view.
	const AActor* Player = GetPlayer();
	Birds->Lift(Player ? Player->GetActorLocation() : Birds->GetActorLocation());
	if (ARoostCrows* Older = State.LeavingCrows.Get())
	{
		Older->Destroy();
	}
	State.LeavingCrows = Birds;
	State.LeavingAge = 0.f;
}

void UFindSubsystem::UpdateLeavingCrows(float DeltaSeconds)
{
	FVector Eye = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector;
	const bool bView = GetView(Eye, Forward);
	for (FSpotState& State : States)
	{
		ARoostCrows* Birds = State.LeavingCrows.Get();
		if (!Birds)
		{
			continue;
		}
		State.LeavingAge += DeltaSeconds;
		const FVector ToBirds = Birds->GetActorLocation() - Eye;
		const bool bFar = ToBirds.Size() > LeaveFar;
		const bool bUnseen = !bView || FVector::DotProduct(Forward.GetSafeNormal(), ToBirds.GetSafeNormal()) < OutOfSightCos;
		if (State.LeavingAge >= LeaveAtLatest || (State.LeavingAge >= LeaveSeenFor && (bFar || bUnseen)))
		{
			Birds->Destroy();
			State.LeavingCrows.Reset();
		}
	}
}

void UFindSubsystem::SetTellsEnabled(bool bEnabled)
{
	bTellsEnabled = bEnabled;
	for (FSpotState& State : States)
	{
		UpdateTellsFor(State);
	}
	DrawGlints();
}

ARoostCrows* UFindSubsystem::GetCrows(FName FindId) const
{
	const FSpotState* State = FindState(FindId);
	return State ? State->Crows.Get() : nullptr;
}

// ---------------------------------------------------------------------------
// Glints
// ---------------------------------------------------------------------------

int32 UFindSubsystem::GetGlintsShowing() const
{
	int32 Showing = 0;
	for (const FSpotState& State : States)
	{
		Showing += State.GlintAge >= 0.f ? 1 : 0;
	}
	return Showing;
}

bool UFindSubsystem::HasLiveGlints() const
{
	return States.ContainsByPredicate([](const FSpotState& State) { return State.GlintAge >= 0.f; });
}

void UFindSubsystem::PollGlints(float DeltaSeconds)
{
	FVector Eye;
	FVector Forward;
	if (!bWatching || !bTellsEnabled || !GetView(Eye, Forward))
	{
		return;
	}
	int32 Showing = GetGlintsShowing();
	bool bStarted = false;
	for (FSpotState& State : States)
	{
		const AFindSpot* Spot = State.Spot.Get();
		if (!Spot || !Spot->bGlint || State.GlintAge >= 0.f)
		{
			continue;
		}
		State.NextGlint -= DeltaSeconds;
		if (State.NextGlint > 0.f || IsFound(Spot->FindId))
		{
			continue;
		}
		// Its own rhythm, whether this star shows or not: the next one is a seeded wait away.
		State.NextGlint = FindRules::NextGlintDelay(State.GlintRandom);
		if (Showing >= FindRules::MaxGlints)
		{
			continue;
		}
		const FVector At = Spot->GetGlintLocation();
		const double Distance = FVector::Dist(Eye, At);
		// The cheap test first; the trace only for one in range.
		if (Distance < FindRules::GlintNear || Distance > FindRules::GlintFar
			|| !FindRules::CanGlint(Distance, HasClearLine(Eye, At), IsSunSideOf(At, Eye)))
		{
			continue;
		}
		State.GlintAge = 0.f;
		++Showing;
		bStarted = true;
		if (!State.bGlintHeard && Distance <= FindRules::GlintSoundRange)
		{
			LooterSound::PlayAt(this, FindCue::Glint, At);
			State.bGlintHeard = true;
		}
	}
	if (bStarted)
	{
		DrawGlints();
	}
}

void UFindSubsystem::UpdateGlints(float DeltaSeconds)
{
	for (FSpotState& State : States)
	{
		if (State.GlintAge < 0.f)
		{
			continue;
		}
		State.GlintAge += DeltaSeconds;
		if (State.GlintAge > FindRules::GlintSeconds)
		{
			State.GlintAge = -1.f;
		}
	}
	DrawGlints();
}

void UFindSubsystem::DrawGlints()
{
	TArray<FFindGlintStar> Stars;
	for (const FSpotState& State : States)
	{
		const AFindSpot* Spot = State.Spot.Get();
		if (!Spot || State.GlintAge < 0.f)
		{
			continue;
		}
		FFindGlintStar& Star = Stars.AddDefaulted_GetRef();
		Star.Location = Spot->GetGlintLocation();
		Star.Strength = FindRules::GlintEnvelope(State.GlintAge) * FindRules::GlintStrength;
		Star.Seed = static_cast<float>(static_cast<uint32>(FindRules::SeedFor(Spot->FindId)) % 1000u) / 1000.f;
	}
	UWorld* World = GetWorld();
	if (Stars.IsEmpty() && !Glints)
	{
		return;
	}
	if (!Glints && World)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Glints = World->SpawnActor<AFindGlints>(AFindGlints::StaticClass(), FTransform::Identity, Params);
	}
	if (!Glints)
	{
		return;
	}
	FVector Eye = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector;
	GetView(Eye, Forward);
	Glints->SetStars(Stars, Eye);
}

bool UFindSubsystem::IsSunSideOf(const FVector& Spot, const FVector& Player) const
{
	if (!bSunLooked && GetWorld())
	{
		// The sun is the light the sky atmosphere sets by; any directional light will do in a level without one.
		bSunLooked = true;
		const UDirectionalLightComponent* Fallback = nullptr;
		for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
		{
			const UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(It->GetLightComponent());
			if (!Light || !Light->IsVisible())
			{
				continue;
			}
			if (Light->bAtmosphereSunLight)
			{
				Sun = Light;
				break;
			}
			Fallback = Fallback ? Fallback : Light;
		}
		if (!Sun.IsValid())
		{
			Sun = Fallback;
		}
	}
	const UDirectionalLightComponent* Light = Sun.Get();
	// No sun (a test level, an indoor map): every side counts as lit.
	return !Light || FindRules::IsSunSide(Player - Spot, -Light->GetForwardVector());
}

bool UFindSubsystem::HasClearLine(const FVector& Eyes, const FVector& Spot) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	FHitResult Hit;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("FindGlint"), GetPlayer(), /*bTraceComplex*/ false);
	if (!World->LineTraceSingleByChannel(Hit, Eyes, Spot, ECC_Visibility, Params))
	{
		return true;
	}
	return Hit.Distance >= FVector::Dist(Eyes, Spot) - GlintSelfMargin;
}
