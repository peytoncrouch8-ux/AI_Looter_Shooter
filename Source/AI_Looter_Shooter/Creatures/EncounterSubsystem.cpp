#include "Creatures/EncounterSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureUpdateRate.h"
#include "Creatures/EncounterSpawner.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/StoryCondition.h"
#include "World/SafeGround.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Subsystems/SubsystemCollection.h"

UEncounterSubsystem* UEncounterSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject && GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UEncounterSubsystem>() : nullptr;
}

bool UEncounterSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UEncounterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Creatures spawned in play join the count as they appear: spawners', a boss's adds, egg sacs', the console's.
	if (UWorld* World = GetWorld())
	{
		SpawnHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UEncounterSubsystem::HandleActorSpawned));
	}
}

void UEncounterSubsystem::PostInitialize()
{
	Super::PostInitialize();
	// The story switches spawners and zones: they look again whenever a mission starts, steps on or finishes.
	UMissionRunner* Runner = GetWorld() ? GetWorld()->GetSubsystem<UMissionRunner>() : nullptr;
	if (Runner)
	{
		BoundRunner = Runner;
		MissionsHandle = Runner->OnChanged.AddUObject(this, &UEncounterSubsystem::HandleMissionsChanged);
		// A mission's events start the waves listening for them, like a boss's call does (SendEvent).
		MissionEventHandle = Runner->OnEvent.AddUObject(this, &UEncounterSubsystem::HandleMissionEvent);
	}
}

void UEncounterSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(SpawnHandle);
	}
	SpawnHandle.Reset();
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsHandle);
		Runner->OnEvent.Remove(MissionEventHandle);
	}
	BoundRunner.Reset();
	MissionsHandle.Reset();
	MissionEventHandle.Reset();
	Creatures.Reset();
	Spawners.Reset();
	SafeZones.Reset();
	Super::Deinitialize();
}

void UEncounterSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// The level's placed creatures, once; spawned ones join as they appear (HandleActorSpawned).
	for (TActorIterator<ACreatureBase> It(&InWorld); It; ++It)
	{
		TrackCreature(*It);
	}
}

// ---------------------------------------------------------------------------
// Spawners and safe zones
// ---------------------------------------------------------------------------

void UEncounterSubsystem::RegisterSpawner(AEncounterSpawner* Spawner)
{
	if (!Spawner)
	{
		return;
	}
	Spawners.RemoveAll([](const TWeakObjectPtr<AEncounterSpawner>& Each) { return !Each.IsValid(); });
	Spawners.AddUnique(Spawner);
}

void UEncounterSubsystem::UnregisterSpawner(AEncounterSpawner* Spawner)
{
	Spawners.RemoveAll([Spawner](const TWeakObjectPtr<AEncounterSpawner>& Each) { return !Each.IsValid() || Each.Get() == Spawner; });
}

void UEncounterSubsystem::RegisterSafeZone(ASafeGround* Zone)
{
	if (!Zone)
	{
		return;
	}
	SafeZones.RemoveAll([](const TWeakObjectPtr<ASafeGround>& Each) { return !Each.IsValid(); });
	SafeZones.AddUnique(Zone);
}

void UEncounterSubsystem::UnregisterSafeZone(ASafeGround* Zone)
{
	SafeZones.RemoveAll([Zone](const TWeakObjectPtr<ASafeGround>& Each) { return !Each.IsValid() || Each.Get() == Zone; });
}

TArray<AEncounterSpawner*> UEncounterSubsystem::GetSpawners() const
{
	TArray<AEncounterSpawner*> Found;
	for (const TWeakObjectPtr<AEncounterSpawner>& Each : Spawners)
	{
		if (AEncounterSpawner* Spawner = Each.Get())
		{
			Found.Add(Spawner);
		}
	}
	return Found;
}

TArray<ASafeGround*> UEncounterSubsystem::GetSafeZones() const
{
	TArray<ASafeGround*> Found;
	for (const TWeakObjectPtr<ASafeGround>& Each : SafeZones)
	{
		if (ASafeGround* Zone = Each.Get())
		{
			Found.Add(Zone);
		}
	}
	return Found;
}

AEncounterSpawner* UEncounterSubsystem::FindSpawner(FName SpawnerId) const
{
	for (AEncounterSpawner* Spawner : GetSpawners())
	{
		// Names compare without regard to case.
		if (Spawner->GetSpawnerId() == SpawnerId)
		{
			return Spawner;
		}
	}
	return nullptr;
}

bool UEncounterSubsystem::IsInSafeZone(const FVector& Point) const
{
	for (const TWeakObjectPtr<ASafeGround>& Each : SafeZones)
	{
		const ASafeGround* Zone = Each.Get();
		if (Zone && Zone->IsActive() && Zone->Contains(Point))
		{
			return true;
		}
	}
	return false;
}

bool UEncounterSubsystem::IsSheltered(const UObject* WorldContextObject, const FVector& Point)
{
	// Every creature asks this a few times a second while it hunts or looks around: straight to the level, no lookups.
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const UEncounterSubsystem* Encounters = World ? World->GetSubsystem<UEncounterSubsystem>() : nullptr;
	return Encounters && Encounters->IsInSafeZone(Point);
}

// ---------------------------------------------------------------------------
// The story
// ---------------------------------------------------------------------------

bool UEncounterSubsystem::IsStoryMet(const FStoryCondition& When, int32 FromStep) const
{
	const bool bAsksStep = FromStep > 0 && !When.DuringMission.IsNone();
	if (When.IsEmpty() && !bAsksStep)
	{
		return true;
	}
	// The session's record (a test's in tests). Without a mission runner there's no story here to follow.
	const UWorld* World = GetWorld();
	const UMissionRunner* Runner = World ? World->GetSubsystem<UMissionRunner>() : nullptr;
	if (!Runner)
	{
		return false;
	}
	const FCampaignRecord& Campaign = Runner->GetCampaign();
	if (!When.IsMet(Campaign, Runner))
	{
		return false;
	}
	if (!bAsksStep)
	{
		return true;
	}
	// Its mission's step, counted from 1: the step it's on running here, or the campaign's main mission's wherever it is.
	int32 Step = Runner->GetStep(When.DuringMission);
	if (Step == INDEX_NONE && Campaign.ActiveMission == When.DuringMission)
	{
		Step = Campaign.ActiveMissionStep;
	}
	return Step != INDEX_NONE && Step + 1 >= FromStep;
}

void UEncounterSubsystem::RefreshStory()
{
	// Copies: a spawner going off takes its creatures away, and nothing here may change under the loop.
	for (AEncounterSpawner* Spawner : GetSpawners())
	{
		Spawner->RefreshStory();
	}
	for (ASafeGround* Zone : GetSafeZones())
	{
		Zone->RefreshStory();
	}
}

void UEncounterSubsystem::HandleMissionsChanged()
{
	RefreshStory();
}

void UEncounterSubsystem::HandleMissionEvent(const FMissionEvent& Event)
{
	// The missions hear every kill and pickup: only an event some spawner waits for goes on (and into the log).
	for (const TWeakObjectPtr<AEncounterSpawner>& Spawner : Spawners)
	{
		if (Spawner.IsValid() && Spawner->WaveEvent == Event.Name)
		{
			SendEvent(Event.Name);
			return;
		}
	}
}

// ---------------------------------------------------------------------------
// Encounter events
// ---------------------------------------------------------------------------

int32 UEncounterSubsystem::SendEvent(FName Event)
{
	if (Event.IsNone())
	{
		return 0;
	}
	int32 Started = 0;
	int32 Listening = 0;
	for (AEncounterSpawner* Spawner : GetSpawners())
	{
		if (Spawner->WaveEvent == Event)
		{
			++Listening;
			Started += Spawner->TriggerWave() ? 1 : 0;
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Encounters: event %s started %d waves (%d spawners listen for it)."), *Event.ToString(), Started, Listening);
	return Started;
}

int32 UEncounterSubsystem::SendEventAt(const UObject* WorldContextObject, FName Event)
{
	UEncounterSubsystem* Encounters = Get(WorldContextObject);
	return Encounters ? Encounters->SendEvent(Event) : 0;
}

// ---------------------------------------------------------------------------
// The level's creatures
// ---------------------------------------------------------------------------

void UEncounterSubsystem::HandleActorSpawned(AActor* Actor)
{
	if (ACreatureBase* Creature = Cast<ACreatureBase>(Actor))
	{
		TrackCreature(Creature);
	}
}

void UEncounterSubsystem::TrackCreature(ACreatureBase* Creature)
{
	if (!Creature || Creature->IsActorBeingDestroyed())
	{
		return;
	}
	// Ones gone since go as others join, so the list stays the level's creatures, living and dead.
	Creatures.RemoveAll([](const TWeakObjectPtr<ACreatureBase>& Each) { return !Each.IsValid(); });
	Creatures.AddUnique(Creature);
}

int32 UEncounterSubsystem::CountAlive(const UClass* Class) const
{
	int32 Alive = 0;
	for (const TWeakObjectPtr<ACreatureBase>& Each : Creatures)
	{
		const ACreatureBase* Creature = Each.Get();
		if (Creature && !Creature->IsDead() && !Creature->IsActorBeingDestroyed() && (!Class || Creature->IsA(Class)))
		{
			++Alive;
		}
	}
	return Alive;
}

int32 UEncounterSubsystem::CountAliveNear(const FVector& Point, float Radius) const
{
	const double ReachSquared = FMath::Square(static_cast<double>(Radius));
	int32 Alive = 0;
	for (const TWeakObjectPtr<ACreatureBase>& Each : Creatures)
	{
		const ACreatureBase* Creature = Each.Get();
		if (Creature && !Creature->IsDead() && !Creature->IsActorBeingDestroyed()
			&& FVector::DistSquared(Creature->GetActorLocation(), Point) <= ReachSquared)
		{
			++Alive;
		}
	}
	return Alive;
}

// ---------------------------------------------------------------------------
// The player
// ---------------------------------------------------------------------------

APawn* UEncounterSubsystem::GetPlayer() const
{
	if (APawn* StandIn = TestPlayer.Get())
	{
		return StandIn;
	}
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	return Controller ? Controller->GetPawn() : nullptr;
}

void UEncounterSubsystem::SetTestPlayer(APawn* StandIn)
{
	TestPlayer = StandIn;
}

bool UEncounterSubsystem::IsInPlayersView(const FVector& Point, float Radius) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	// The creatures' own measure: a view as wide as the game's first-person one at least, and a margin past the screen's
	// edges, so a turning view never catches one being taken away.
	const FCreatureUpdateRate Measure;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Player = It->Get();
		const APlayerCameraManager* Camera = Player && Player->IsLocalController() ? Player->PlayerCameraManager.Get() : nullptr;
		if (Camera && FCreatureUpdateRate::IsInView(Camera->GetCameraLocation(), Camera->GetCameraRotation().Vector(),
			FMath::Max(Camera->GetFOVAngle(), Measure.ReferenceFieldOfView), Point, Radius, Measure.ViewMargin))
		{
			return true;
		}
	}
	return false;
}
