#include "World/FindSubsystem.h"
#include "Audio/LooterSound.h"
#include "World/FindGlints.h"
#include "World/FindSpot.h"
#include "World/RoostCrows.h"
#include "World/SwimWaterSubsystem.h"
#include "World/WorldQueries.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// UFindSubsystem's tells for finds not yet found (the user's pick, 2026-10-28): two crows circling low over each, and now and
// then a glint off its brass. None baked: made as play begins (after the legacy pass), gone with the find. Since the
// 2026-10-29 play-check the crows wheel 4 m up on a tight circle (they were specks 10-19 m up), a third sits on a chest
// under open sky until the player comes near, and a glint under water glows steadily (the tarn's box was lost in the murk).

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
	/** A crow sits on a find only with this much open sky over its seat (cm): room to get up and away. */
	constexpr double PerchHeadroom = 400.0;
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
		if (State.bGlowing)
		{
			// A steady glow stays drawn until the pool is drawn again without it.
			State.bGlowing = false;
			DrawGlints();
		}
	}
	const bool bWanted = World && bWatching && bTellsEnabled && bUnfound && Spot->Crows > 0;
	if (!bWanted)
	{
		for (TWeakObjectPtr<ARoostCrows>* Flock : { &State.Crows, &State.Percher })
		{
			if (ARoostCrows* Birds = Flock->Get())
			{
				Birds->Destroy();
			}
			Flock->Reset();
		}
		return;
	}
	if (!State.Crows.IsValid())
	{
		// Hob's crows' aerial flock, low and slow over the find, no beacon column under it: never read as a roost (high over
		// a red column), culled past the settings' CirclingCullDistance.
		const FVector Over = Spot->GetActorLocation();
		const FTransform At(FRotator::ZeroRotator, Over + FVector(0.0, 0.0, Spot->CrowHeight));
		ARoostCrows* Birds = World->SpawnActorDeferred<ARoostCrows>(ARoostCrows::StaticClass(), At, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Birds)
		{
			Birds->MakeFindCircling(Over, Spot->Crows, Spot->CrowHeight);
			Birds->Seed = static_cast<int32>(static_cast<uint32>(FindRules::SeedFor(Spot->FindId)) * 31u) | 1;
			Birds->FinishSpawning(At);
			State.Crows = Birds;
		}
	}
	FTransform Perch;
	if (!State.Percher.IsValid() && FindPerch(*Spot, Perch))
	{
		// And one sitting on the box itself: seen from further than the wheel over it, and it goes up as the player comes.
		ARoostCrows* Sitter = World->SpawnActorDeferred<ARoostCrows>(ARoostCrows::StaticClass(), Perch, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Sitter)
		{
			Sitter->MakeFindPerch(Perch);
			Sitter->Seed = static_cast<int32>(static_cast<uint32>(FindRules::SeedFor(Spot->FindId)) * 57u) | 1;
			Sitter->FinishSpawning(Perch);
			State.Percher = Sitter;
		}
	}
}

bool UFindSubsystem::FindPerch(const AFindSpot& Spot, FTransform& OutPerch) const
{
	const UWorld* World = GetWorld();
	const AActor* Source = Spot.GetSourceActor();
	if (!World || !Source)
	{
		return false;
	}
	// The top of what blocks on it (a chest's body and lid; a grave's buried box has none and gets no crow).
	FVector Origin;
	FVector Extent;
	Source->GetActorBounds(/*bOnlyCollidingComponents*/ true, Origin, Extent);
	if (Extent.IsNearlyZero(1.0))
	{
		return false;
	}
	const FVector Seat(Origin.X, Origin.Y, Origin.Z + Extent.Z);
	// Not on the tarn's bed, and with open sky over it (the bolt-hole's box sits under a slab): the crow must get off it.
	if (const USwimWaterSubsystem* Waters = USwimWaterSubsystem::Get(this))
	{
		if (Waters->FindAt(Seat, USwimWaterSubsystem::DeepReachBelow, 0.f))
		{
			return false;
		}
	}
	FHitResult Hit;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("FindPerch"), Source, /*bTraceComplex*/ false);
	if (World->LineTraceSingleByChannel(Hit, Seat + FVector(0.0, 0.0, 20.0), Seat + FVector(0.0, 0.0, PerchHeadroom), ECC_Visibility, Params))
	{
		return false;
	}
	// Facing its own way (seeded), so the finds' crows don't all sit alike.
	const float Yaw = static_cast<float>(static_cast<uint32>(FindRules::SeedFor(Spot.FindId)) % 360u);
	OutPerch = FTransform(FRotator(0.0, Yaw, 0.0), Seat);
	return true;
}

void UFindSubsystem::SendCrowsOff(FSpotState& State)
{
	// The find made: its glow goes out with its other tells (drawn again without it: a steady glow would stay up).
	if (State.bGlowing)
	{
		State.bGlowing = false;
		DrawGlints();
	}
	ARoostCrows* Birds = State.Crows.Get();
	ARoostCrows* Sitter = State.Percher.Get();
	State.Crows.Reset();
	State.Percher.Reset();
	if (!Birds && !Sitter)
	{
		return;
	}
	// The crows lift and go. They're let be until the player looks away, so none pops out of the sky in view.
	const AActor* Player = GetPlayer();
	auto Send = [Player](ARoostCrows* Flock, TWeakObjectPtr<ARoostCrows>& Leaving)
	{
		if (ARoostCrows* Older = Leaving.Get())
		{
			Older->Destroy();
		}
		Leaving.Reset();
		if (Flock)
		{
			Flock->Lift(Player ? Player->GetActorLocation() : Flock->GetActorLocation());
			Leaving = Flock;
		}
	};
	Send(Birds, State.LeavingCrows);
	Send(Sitter, State.LeavingPercher);
	State.LeavingAge = 0.f;
}

void UFindSubsystem::UpdateLeavingCrows(float DeltaSeconds)
{
	FVector Eye = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector;
	const bool bView = GetView(Eye, Forward);
	for (FSpotState& State : States)
	{
		if (!State.LeavingCrows.IsValid() && !State.LeavingPercher.IsValid())
		{
			continue;
		}
		State.LeavingAge += DeltaSeconds;
		for (TWeakObjectPtr<ARoostCrows>* Leaving : { &State.LeavingCrows, &State.LeavingPercher })
		{
			ARoostCrows* Birds = Leaving->Get();
			if (!Birds)
			{
				continue;
			}
			const FVector ToBirds = Birds->GetActorLocation() - Eye;
			const bool bFar = ToBirds.Size() > LeaveFar;
			const bool bUnseen = !bView || FVector::DotProduct(Forward.GetSafeNormal(), ToBirds.GetSafeNormal()) < OutOfSightCos;
			if (State.LeavingAge >= LeaveAtLatest || (State.LeavingAge >= LeaveSeenFor && (bFar || bUnseen)))
			{
				Birds->Destroy();
				Leaving->Reset();
			}
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

ARoostCrows* UFindSubsystem::GetPercher(FName FindId) const
{
	const FSpotState* State = FindState(FindId);
	return State ? State->Percher.Get() : nullptr;
}

bool UFindSubsystem::IsGlowing(FName FindId) const
{
	const FSpotState* State = FindState(FindId);
	return State && State->bGlowing;
}

// ---------------------------------------------------------------------------
// Glints
// ---------------------------------------------------------------------------

int32 UFindSubsystem::GetGlintsShowing() const
{
	int32 Showing = 0;
	for (const FSpotState& State : States)
	{
		Showing += State.GlintAge >= 0.f || State.bGlowing ? 1 : 0;
	}
	return Showing;
}

bool UFindSubsystem::HasLiveGlints() const
{
	return States.ContainsByPredicate([](const FSpotState& State) { return State.GlintAge >= 0.f || State.bGlowing; });
}

bool UFindSubsystem::IsUnderwaterGlint(FSpotState& State) const
{
	const AFindSpot* Spot = State.Spot.Get();
	if (!Spot)
	{
		return false;
	}
	// The waters are all in by the legacy pass (each registers as play begins); a spot doesn't move, so once is enough.
	if (!State.bUnderLooked)
	{
		const USwimWaterSubsystem* Waters = USwimWaterSubsystem::Get(this);
		State.bUnderwater = Waters && Waters->FindAt(Spot->GetGlintLocation(), USwimWaterSubsystem::DeepReachBelow, 0.f) != nullptr;
		State.bUnderLooked = true;
	}
	return State.bUnderwater;
}

void UFindSubsystem::PollGlints(float DeltaSeconds)
{
	FVector Eye;
	FVector Forward;
	if (!bWatching || !bTellsEnabled || !GetView(Eye, Forward))
	{
		return;
	}
	bool bStarted = false;
	// Under water no sun catches the brass: the glow shows while the player is near, whatever the sun (and it takes its
	// place in the pool first, so a flash elsewhere never puts it out).
	for (FSpotState& State : States)
	{
		const AFindSpot* Spot = State.Spot.Get();
		if (!Spot || !Spot->bGlint || !IsUnderwaterGlint(State))
		{
			continue;
		}
		const bool bGlow = !IsFound(Spot->FindId) && FVector::Dist(Eye, Spot->GetGlintLocation()) <= FindRules::UnderGlowRange;
		// Drawn again when it comes on and when it goes off (nothing else redraws a pool with no star left in it).
		bStarted |= bGlow != State.bGlowing;
		State.bGlowing = bGlow;
	}
	int32 Showing = GetGlintsShowing();
	for (FSpotState& State : States)
	{
		const AFindSpot* Spot = State.Spot.Get();
		if (!Spot || !Spot->bGlint || State.GlintAge >= 0.f || State.bUnderwater)
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
	FVector Eye = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector;
	GetView(Eye, Forward);
	TArray<FFindGlintStar> Stars;
	// The underwater glows first: the pool's places go to them before a passing flash.
	for (const bool bGlows : { true, false })
	{
		for (const FSpotState& State : States)
		{
			const AFindSpot* Spot = State.Spot.Get();
			if (!Spot || (bGlows ? !State.bGlowing : State.GlintAge < 0.f))
			{
				continue;
			}
			FFindGlintStar& Star = Stars.AddDefaulted_GetRef();
			Star.Location = Spot->GetGlintLocation();
			Star.Seed = static_cast<float>(static_cast<uint32>(FindRules::SeedFor(Spot->FindId)) % 1000u) / 1000.f;
			if (bGlows)
			{
				Star.Strength = FindRules::UnderGlowPulse(Clock, Star.Seed) * FindRules::UnderGlowStrength;
				Star.Scale = FindRules::UnderGlowScale(FVector::Dist(Eye, Star.Location));
				Star.bUnderwater = true;
			}
			else
			{
				Star.Strength = FindRules::GlintEnvelope(State.GlintAge) * FindRules::GlintStrength;
			}
		}
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
