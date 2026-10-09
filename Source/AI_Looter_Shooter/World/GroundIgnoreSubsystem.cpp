#include "World/GroundIgnoreSubsystem.h"
#include "World/PlayableArea.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Volume.h"

bool UGroundIgnoreSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UGroundIgnoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UWorld* World = GetWorld())
	{
		SpawnHandle = World->AddOnActorPreSpawnInitialization(FOnActorSpawned::FDelegate::CreateUObject(this, &UGroundIgnoreSubsystem::HandleActorSpawned));
		DestroyHandle = World->AddOnActorDestroyedHandler(FOnActorDestroyed::FDelegate::CreateUObject(this, &UGroundIgnoreSubsystem::HandleActorDestroyed));
	}
	// Streaming levels in and out is a world-wide event: the handlers ignore other worlds' levels.
	LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UGroundIgnoreSubsystem::HandleLevelChanged);
	LevelRemovedHandle = FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &UGroundIgnoreSubsystem::HandleLevelChanged);
}

void UGroundIgnoreSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorPreSpawnInitialization(SpawnHandle);
		World->RemoveOnActorDestroyedHandler(DestroyHandle);
	}
	FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);
	FWorldDelegates::LevelRemovedFromWorld.Remove(LevelRemovedHandle);
	SpawnHandle.Reset();
	DestroyHandle.Reset();
	LevelAddedHandle.Reset();
	LevelRemovedHandle.Reset();
	Ignored.Reset();
	Super::Deinitialize();
}

void UGroundIgnoreSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// A call before the level's actors were all set up may have found only some of them.
	bDirty = true;
}

bool UGroundIgnoreSubsystem::IsIgnoredKind(const AActor* Actor)
{
	return Actor && (Actor->IsA<AVolume>() || Actor->IsA<APlayableArea>());
}

void UGroundIgnoreSubsystem::HandleActorSpawned(AActor* Actor)
{
	// Hooked to the spawn's first step, which already has the actor in its level, so no call can slip in between.
	bDirty |= IsIgnoredKind(Actor);
}

void UGroundIgnoreSubsystem::HandleActorDestroyed(AActor* Actor)
{
	// A gone actor's id can be handed to a new object later, so a stale entry must not stay in the list.
	bDirty |= IsIgnoredKind(Actor);
}

void UGroundIgnoreSubsystem::HandleLevelChanged(ULevel* Level, UWorld* World)
{
	// A null world is a whole map being replaced, so this one's levels are going too.
	bDirty |= !World || World == GetWorld();
}

void UGroundIgnoreSubsystem::FindIgnored(const UWorld& World, TArray<TWeakObjectPtr<const AActor>>& OutFound)
{
	for (TActorIterator<AVolume> It(&World); It; ++It)
	{
		OutFound.Emplace(*It);
	}
	// The playable area's invisible walls are world static too, but they only stop walking pawns.
	for (TActorIterator<APlayableArea> It(&World); It; ++It)
	{
		OutFound.Emplace(*It);
	}
}

void UGroundIgnoreSubsystem::AddIgnoredByWalking(const UWorld& World, FCollisionQueryParams& Params)
{
	TArray<TWeakObjectPtr<const AActor>> Found;
	FindIgnored(World, Found);
	Params.AddIgnoredActors(Found);
}

void UGroundIgnoreSubsystem::AddIgnoredTo(FCollisionQueryParams& Params)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (bDirty || LookedUpLevels != World->GetNumLevels())
	{
		Ignored.Reset();
		FindIgnored(*World, Ignored);
		LookedUpLevels = World->GetNumLevels();
		bDirty = false;
		++LookupCount;
	}
	// Weak pointers: each is checked still alive as it's added.
	Params.AddIgnoredActors(Ignored);
}
