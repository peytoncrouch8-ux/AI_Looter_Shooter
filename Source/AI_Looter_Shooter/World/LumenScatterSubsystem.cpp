#include "World/LumenScatterSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

bool ULumenScatterSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void ULumenScatterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ULumenScatterSubsystem::HandleLevelAdded);
}

void ULumenScatterSubsystem::Deinitialize()
{
	FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);
	LevelAddedHandle.Reset();
	Super::Deinitialize();
}

void ULumenScatterSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	int32 Parts = 0;
	for (TActorIterator<AActor> It(&InWorld); It; ++It)
	{
		Parts += LeaveOutOf(**It);
	}
	UE_LOG(LogLooter, Log, TEXT("Lumen scatter: %d small instanced parts left out of Lumen and the distance fields in %s."), Parts, *InWorld.GetName());
}

void ULumenScatterSubsystem::HandleLevelAdded(ULevel* Level, UWorld* World)
{
	// Streaming is a world-wide event: only this world's levels, and only once play has begun (OnWorldBeginPlay does the rest).
	if (!Level || World != GetWorld() || !World->HasBegunPlay())
	{
		return;
	}
	for (AActor* Actor : Level->Actors)
	{
		if (Actor)
		{
			LeaveOutOf(*Actor);
		}
	}
}

bool ULumenScatterSubsystem::IsSmallScatter(const UInstancedStaticMeshComponent& Part)
{
	const UStaticMesh* Mesh = Part.GetStaticMesh();
	return Mesh && Mesh->GetBounds().SphereRadius < SmallRadius;
}

int32 ULumenScatterSubsystem::LeaveOutOf(const AActor& Actor)
{
	int32 Changed = 0;
	TInlineComponentArray<UInstancedStaticMeshComponent*> Instanced(&Actor);
	for (UInstancedStaticMeshComponent* Part : Instanced)
	{
		if (!Part || !IsSmallScatter(*Part) || (!Part->bAffectDistanceFieldLighting && !Part->bAffectDynamicIndirectLighting))
		{
			continue;
		}
		Part->bAffectDistanceFieldLighting = false;
		Part->bAffectDynamicIndirectLighting = false;
		// The renderer reads both when the part's scene proxy is made: make it again.
		Part->MarkRenderStateDirty();
		++Changed;
	}
	return Changed;
}
