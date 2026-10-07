// AEncounterSpawner: its life in the level, its state, the story that switches it, its timer, and what the console and
// the tests ask of it. Its waves, where its creatures stand, spawning them and taking them away are in
// EncounterSpawnerWaves.cpp.

#include "Creatures/EncounterSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Core/LooterMenuGameMode.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/LineBatchComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

namespace
{
	/** Lines in a batch never expire on their own: the console command clears the batch. */
	constexpr float DrawForever = -1.f;

	/** A level circle of lines Radius round Middle, kept in BatchID. */
	void DrawRing(ULineBatchComponent& Lines, const FVector& Middle, float Radius, const FLinearColor& Tint, float Thickness, uint32 BatchID)
	{
		constexpr int32 Sides = 48;
		for (int32 Side = 0; Side < Sides; ++Side)
		{
			const double AngleFrom = 2.0 * UE_DOUBLE_PI * Side / Sides;
			const double AngleTo = 2.0 * UE_DOUBLE_PI * (Side + 1) / Sides;
			Lines.DrawLine(Middle + FVector(FMath::Cos(AngleFrom), FMath::Sin(AngleFrom), 0.0) * Radius,
				Middle + FVector(FMath::Cos(AngleTo), FMath::Sin(AngleTo), 0.0) * Radius, Tint, SDPG_World, Thickness, DrawForever, BatchID);
		}
	}
}

AEncounterSpawner::AEncounterSpawner()
{
	// It never ticks: a timer runs only while it's on (UpdateTimer).
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Anchor->SetMobility(EComponentMobility::Static);
	RootComponent = Anchor;

#if WITH_EDITORONLY_DATA
	// An encounter's story and caps matter wherever the player is: in a partitioned world it never streams out.
	bIsSpatiallyLoaded = false;
#endif
}

void AEncounterSpawner::BeginPlay()
{
	Super::BeginPlay();
	Rolls.GenerateNewSeed();
	// Behind the main menu no one plays: it stays off, whatever its story says.
	bMenuWorld = ALooterMenuGameMode::IsMenuWorld(GetWorld());
	if (Groups.IsEmpty())
	{
		UE_LOG(LogLooter, Warning, TEXT("Encounter %s has no groups: it will never spawn anything."), *GetSpawnerId().ToString());
	}
	if (UEncounterSubsystem* Encounters = GetEncounters())
	{
		Encounters->RegisterSpawner(this);
	}
	// A Legendary monster beaten too lately is away for this visit (the level beginning is an arrival): the encounter starts
	// cleared, whatever its story says.
	if (!LegendaryId.IsNone() && !bMenuWorld)
	{
		const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
		if (Sessions && !Sessions->IsLegendaryBack(GetWorld(), LegendaryId))
		{
			SendLegendaryAway();
		}
	}
	RefreshStory();
}

void AEncounterSpawner::SendLegendaryAway()
{
	bLegendaryAway = true;
	// Nothing of it this visit: nothing owed, and whatever it had out goes (the console's way; as the level begins there's
	// nothing yet).
	Owed.Reset();
	RemoveLiving(/*bOweThem*/ false);
	SetState(EEncounterState::Cleared);
	UpdateTimer();
	UE_LOG(LogLooter, Log, TEXT("Encounter %s: %s is away this visit (beaten too lately); cleared until the level loads again."),
		*GetSpawnerId().ToString(), *LegendaryId.ToString());
}

void AEncounterSpawner::HandleCreatureDeath(AController* Killer)
{
	if (LegendaryId.IsNone())
	{
		return;
	}
	// Beaten: it comes back on an arrival 20 minutes of play from now (USessionSubsystem keeps the time with the map).
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->NoteLegendaryDefeat(GetWorld(), LegendaryId);
	}
	UE_LOG(LogLooter, Log, TEXT("Encounter %s: %s is beaten."), *GetSpawnerId().ToString(), *LegendaryId.ToString());
}

void AEncounterSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CheckTimer);
	}
	if (UEncounterSubsystem* Encounters = GetEncounters())
	{
		Encounters->UnregisterSpawner(this);
	}
	// Removed from a level that goes on (a console spawner, a script): its creatures go with it rather than roam with no
	// spawner to answer to. A level that's ending takes everything with it anyway.
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		RemoveLiving(/*bOweThem*/ false);
	}
	Super::EndPlay(EndPlayReason);
}

UEncounterSubsystem* AEncounterSpawner::GetEncounters() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UEncounterSubsystem>() : nullptr;
}

FName AEncounterSpawner::GetSpawnerId() const
{
	return SpawnerId.IsNone() ? GetFName() : SpawnerId;
}

void AEncounterSpawner::SetRandomSeed(int32 Seed)
{
	Rolls.Initialize(Seed);
}

// ---------------------------------------------------------------------------
// Its state and the story
// ---------------------------------------------------------------------------

void AEncounterSpawner::SetState(EEncounterState NewState)
{
	if (State != NewState)
	{
		UE_LOG(LogLooter, Verbose, TEXT("Encounter %s: %s -> %s"), *GetSpawnerId().ToString(), *GetStateName(State), *GetStateName(NewState));
	}
	State = NewState;
}

void AEncounterSpawner::RefreshStory()
{
	const UEncounterSubsystem* Encounters = GetEncounters();
	const bool bStoryHolds = Encounters ? Encounters->IsStoryMet(ActiveWhen, FromStep) : ActiveWhen.IsEmpty() && FromStep <= 0;
	const bool bWanted = !bMenuWorld && (bForced || bStoryHolds);
	const bool bFirst = !bStoryApplied;
	if (!bFirst && bWanted == bStoryActive)
	{
		return;
	}
	bStoryApplied = true;
	bStoryActive = bWanted;
	// A cleared encounter stays cleared for this visit, whatever the story says now.
	if (State != EEncounterState::Cleared)
	{
		if (bStoryActive && State == EEncounterState::Off)
		{
			SetState(EEncounterState::Waiting);
		}
		else if (!bStoryActive && State != EEncounterState::Off)
		{
			GoOff();
		}
	}
	const FString When = FromStep > 0 ? FString::Printf(TEXT("%s, from step %d"), *ActiveWhen.Describe(), FromStep) : ActiveWhen.Describe();
	if (bFirst)
	{
		UE_LOG(LogLooter, Verbose, TEXT("Encounter %s: %s as play begins (%s)."), *GetSpawnerId().ToString(), bStoryActive ? TEXT("on") : TEXT("off"), *When);
	}
	else
	{
		UE_LOG(LogLooter, Log, TEXT("Encounter %s: %s by the story (%s)."), *GetSpawnerId().ToString(), bStoryActive ? TEXT("on") : TEXT("off"), *When);
	}
	UpdateTimer();
}

void AEncounterSpawner::GoOff()
{
	SetState(EEncounterState::Off);
	// A fresh encounter if its story comes round again; nothing is owed any more.
	Owed.Reset();
	WavesStarted = 0;
	TotalQueued = 0;
	Killed = 0;
	WaveClock = 0.f;
	// What's out goes now, unless someone sees it or it's fighting: then as soon as that's over (UpdateEncounter).
	if (!IsAnyInView() && !IsAnyFighting())
	{
		RemoveLiving(/*bOweThem*/ false);
	}
}

bool AEncounterSpawner::TriggerWave(bool bForce)
{
	if (bForce)
	{
		// The console's way to try an encounter: on whatever its story says, and a cleared one starts over (a Legendary
		// monster away this visit comes back).
		bForced = true;
		bLegendaryAway = false;
		if (State == EEncounterState::Cleared)
		{
			Owed.Reset();
			WavesStarted = 0;
			TotalQueued = 0;
			Killed = 0;
			WaveClock = 0.f;
			SetState(bStoryActive ? EEncounterState::Waiting : EEncounterState::Off);
		}
		RefreshStory();
	}
	const FString Label = GetSpawnerId().ToString();
	if (State == EEncounterState::Off)
	{
		const FString When = ActiveWhen.Describe();
		UE_LOG(LogLooter, Log, TEXT("Encounter %s: no wave, its story condition doesn't hold (%s%s)."), *Label, *When,
			FromStep > 0 ? *FString::Printf(TEXT(", from step %d"), FromStep) : TEXT(""));
		return false;
	}
	if (State == EEncounterState::Cleared)
	{
		UE_LOG(LogLooter, Log, TEXT("Encounter %s: no wave, it's cleared for this visit."), *Label);
		return false;
	}
	if (!HasWavesLeft())
	{
		UE_LOG(LogLooter, Log, TEXT("Encounter %s: no wave, all %d have come."), *Label, WavesStarted);
		return false;
	}
	SetState(EEncounterState::Engaged);
	StartWave();
	UpdateTimer();
	return true;
}

// ---------------------------------------------------------------------------
// The timer: what it looks at while it's on
// ---------------------------------------------------------------------------

void AEncounterSpawner::UpdateEncounter(float DeltaSeconds)
{
	PruneLiving();
	switch (State)
	{
	case EEncounterState::Off:
		// Its story ended with creatures out: they go as soon as nobody sees them go.
		if (NumAlive() > 0 && !IsAnyInView() && !IsAnyFighting())
		{
			RemoveLiving(/*bOweThem*/ false);
		}
		break;

	case EEncounterState::Waiting:
		if (WantsApproach() && IsPlayerWithin(ActivationRadius))
		{
			Engage();
		}
		break;

	case EEncounterState::Engaged:
	{
		WaveClock += DeltaSeconds;
		SpawnOwed();
		const int32 Remaining = NumAlive() + Owed.Num();
		// Waiting for a clear, the clock counts from the moment the last of them is gone.
		if (bWaitForClear && Remaining > 0)
		{
			WaveClock = 0.f;
		}
		const bool bWavesLeft = HasWavesLeft();
		if (EncounterRules::IsWaveDue(bWavesLeft, WaveClock, WaveInterval, bWaitForClear, Remaining))
		{
			StartWave();
		}
		else if (Remaining == 0 && !bWavesLeft)
		{
			SetState(EEncounterState::Cleared);
			UE_LOG(LogLooter, Log, TEXT("Encounter %s: cleared (%d killed)."), *GetSpawnerId().ToString(), Killed);
		}
		else if (Remaining == 0 && !HasTimedWaves())
		{
			// Its next wave comes only on a trigger: nothing to look after until then.
			SetState(EEncounterState::Waiting);
		}
		else if (ShouldDespawn())
		{
			Despawn();
		}
		break;
	}

	default:
		break;
	}
	UpdateTimer();
}

void AEncounterSpawner::HandleCheckTimer()
{
	UpdateEncounter(FMath::Max(CheckSeconds, 0.1f));
}

bool AEncounterSpawner::WantsChecks() const
{
	switch (State)
	{
	case EEncounterState::Off:
		// Only to take away what's still out once nobody sees it.
		return NumAlive() > 0;
	case EEncounterState::Waiting:
		return WantsApproach();
	case EEncounterState::Engaged:
		return true;
	default:
		return false;
	}
}

bool AEncounterSpawner::WantsApproach() const
{
	// Creatures taken away come back with the player; the first wave comes on approach if it's meant to, and timed waves
	// after it go on once the player is back. Waves that come only on a trigger don't need the player near.
	if (Owed.Num() > 0)
	{
		return true;
	}
	return WavesStarted == 0 ? bSpawnOnApproach : (HasWavesLeft() && HasTimedWaves());
}

void AEncounterSpawner::UpdateTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (!WantsChecks())
	{
		Timers.ClearTimer(CheckTimer);
		return;
	}
	if (!Timers.IsTimerActive(CheckTimer))
	{
		const float Every = FMath::Max(CheckSeconds, 0.1f);
		// From a random point of its interval, so the level's spawners don't all look on the same frame.
		const float FirstIn = static_cast<float>(Rolls.FRandRange(0.05, static_cast<double>(Every)));
		Timers.SetTimer(CheckTimer, this, &AEncounterSpawner::HandleCheckTimer, Every, /*bLoop*/ true, FirstIn);
	}
}

bool AEncounterSpawner::IsPlayerWithin(float Distance) const
{
	const UEncounterSubsystem* Encounters = GetEncounters();
	const APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	return Player && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) <= FMath::Square(static_cast<double>(Distance));
}

// ---------------------------------------------------------------------------
// What's going on
// ---------------------------------------------------------------------------

int32 AEncounterSpawner::NumAlive() const
{
	int32 Alive = 0;
	for (const FLivingCreature& Each : Living)
	{
		const ACreatureBase* Creature = Each.Creature.Get();
		Alive += Creature && !Creature->IsDead() ? 1 : 0;
	}
	return Alive;
}

TArray<ACreatureBase*> AEncounterSpawner::GetAliveCreatures() const
{
	TArray<ACreatureBase*> Alive;
	for (const FLivingCreature& Each : Living)
	{
		ACreatureBase* Creature = Each.Creature.Get();
		if (Creature && !Creature->IsDead())
		{
			Alive.Add(Creature);
		}
	}
	return Alive;
}

int32 AEncounterSpawner::GetPlannedWaves() const
{
	return EncounterRules::PlannedWaves(NumWaves, MaxTotal);
}

FHuntingGround AEncounterSpawner::MakeHuntingGround() const
{
	return FHuntingGround::MakeAround(GetActorLocation(), GiveUpRadius, GroundCorners, GroundMargin, GroundMaxRise);
}

FString AEncounterSpawner::GetStateName(EEncounterState InState)
{
	switch (InState)
	{
	case EEncounterState::Waiting:
		return TEXT("Waiting");
	case EEncounterState::Engaged:
		return TEXT("Engaged");
	case EEncounterState::Cleared:
		return TEXT("Cleared");
	default:
		return TEXT("Off");
	}
}

FString AEncounterSpawner::Describe() const
{
	const int32 Planned = GetPlannedWaves();
	const FString Waves = NumWaves <= 0 && MaxTotal > 0
		? FString::Printf(TEXT("%d (until %d in all)"), WavesStarted, MaxTotal)
		: FString::Printf(TEXT("%d/%d"), WavesStarted, Planned);
	const FString When = FromStep > 0 ? FString::Printf(TEXT("%s, from step %d"), *ActiveWhen.Describe(), FromStep) : ActiveWhen.Describe();
	const FString Away = bLegendaryAway ? FString::Printf(TEXT(", %s away this visit"), *LegendaryId.ToString()) : FString();
	return FString::Printf(TEXT("%s, %s (%s): waves %s, %d alive, %d owed, %d killed, %d spawned%s"), *GetStateName(State),
		bStoryActive ? TEXT("on") : TEXT("off"), *When, *Waves, NumAlive(), Owed.Num(), Killed, TotalQueued, *Away);
}

void AEncounterSpawner::DrawEncounter(ULineBatchComponent& Lines, uint32 BatchID) const
{
	const FVector Here = GetActorLocation();
	const FVector Middle = Here + FVector(0.0, 0.0, 50.0);
	const FLinearColor GroundTint = LooterUI::Color::Accent();
	const FLinearColor SpotTint = LooterUI::Color::SegmentOn();
	const FLinearColor ReachTint = LooterUI::Color::TextDim();

	// Its hunting ground: the polygon with a post at each corner, or the give-up radius.
	if (GroundCorners.Num() >= 3)
	{
		for (int32 Index = 0; Index < GroundCorners.Num(); ++Index)
		{
			const FVector& Corner = GroundCorners[Index];
			const FVector& NextCorner = GroundCorners[(Index + 1) % GroundCorners.Num()];
			Lines.DrawLine(Corner + FVector(0.0, 0.0, 50.0), NextCorner + FVector(0.0, 0.0, 50.0), GroundTint, SDPG_World, 6.f, DrawForever, BatchID);
			Lines.DrawLine(Corner, Corner + FVector(0.0, 0.0, 300.0), GroundTint, SDPG_World, 6.f, DrawForever, BatchID);
		}
	}
	else if (GiveUpRadius > 0.f)
	{
		DrawRing(Lines, Middle, GiveUpRadius, GroundTint, 6.f, BatchID);
	}

	// Where its creatures appear: its points as crosses, or its radius; and how near the player comes before they do.
	if (SpawnPoints.Num() > 0)
	{
		for (const FVector& Local : SpawnPoints)
		{
			const FVector Point = GetActorTransform().TransformPositionNoScale(Local) + FVector(0.0, 0.0, 50.0);
			Lines.DrawLine(Point - FVector(60.0, 0.0, 0.0), Point + FVector(60.0, 0.0, 0.0), SpotTint, SDPG_World, 4.f, DrawForever, BatchID);
			Lines.DrawLine(Point - FVector(0.0, 60.0, 0.0), Point + FVector(0.0, 60.0, 0.0), SpotTint, SDPG_World, 4.f, DrawForever, BatchID);
		}
	}
	else
	{
		DrawRing(Lines, Middle, SpawnRadius, SpotTint, 4.f, BatchID);
	}
	if (bSpawnOnApproach && ActivationRadius > 0.f)
	{
		DrawRing(Lines, Middle, ActivationRadius, ReachTint, 2.f, BatchID);
	}
	// A post at the spawner itself.
	Lines.DrawLine(Here, Here + FVector(0.0, 0.0, 400.0), SpotTint, SDPG_World, 8.f, DrawForever, BatchID);
}
