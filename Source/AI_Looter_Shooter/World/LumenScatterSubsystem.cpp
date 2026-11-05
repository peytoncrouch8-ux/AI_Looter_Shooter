#include "World/LumenScatterSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** Every game world's ShadowHighOnly scatter to the variable's new value (a preset picked while playing). */
	void ApplyHighShadowsEverywhere(IConsoleVariable* Variable)
	{
		if (!GEngine)
		{
			return;
		}
		const bool bShadows = Variable && Variable->GetInt() != 0;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || !World->IsGameWorld())
			{
				continue;
			}
			int32 Parts = 0;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				Parts += ULumenScatterSubsystem::ApplyHighShadows(**It, bShadows);
			}
			if (Parts > 0)
			{
				UE_LOG(LogLooter, Log, TEXT("Scatter shadows: %d shrub parts %s shadows in %s."), Parts, bShadows ? TEXT("cast") : TEXT("cast no"), *World->GetName());
			}
		}
	}

	// Set by the graphics preset (UGraphicsSettingsSubsystem's QualityVariables: 0 on Low and Medium); 1 elsewhere.
	TAutoConsoleVariable<int32> CVarScatterHighShadows(
		TEXT("Looter.Scatter.HighShadows"),
		1,
		TEXT("0: the scatter tagged ShadowHighOnly (sagebrush, rabbitbrush, bushes) casts no shadow (Low and Medium, set by the graphics preset)."),
		FConsoleVariableDelegate::CreateStatic(&ApplyHighShadowsEverywhere),
		ECVF_Default);
}

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
	int32 Shrubs = 0;
	const bool bShadows = HighShadowsAllowed();
	for (TActorIterator<AActor> It(&InWorld); It; ++It)
	{
		Parts += LeaveOutOf(**It);
		Shrubs += ApplyHighShadows(**It, bShadows);
	}
	UE_LOG(LogLooter, Log, TEXT("Lumen scatter: %d small instanced parts left out of Lumen and the distance fields in %s; %d shrub parts %s shadows."),
		Parts, *InWorld.GetName(), Shrubs, bShadows ? TEXT("cast") : TEXT("cast no"));
}

void ULumenScatterSubsystem::HandleLevelAdded(ULevel* Level, UWorld* World)
{
	// Streaming is a world-wide event: only this world's levels, and only once play has begun (OnWorldBeginPlay does the rest).
	if (!Level || World != GetWorld() || !World->HasBegunPlay())
	{
		return;
	}
	const bool bShadows = HighShadowsAllowed();
	for (AActor* Actor : Level->Actors)
	{
		if (Actor)
		{
			LeaveOutOf(*Actor);
			ApplyHighShadows(*Actor, bShadows);
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

FName ULumenScatterSubsystem::ShadowHighOnlyTag()
{
	static const FName Tag(TEXT("ShadowHighOnly"));
	return Tag;
}

bool ULumenScatterSubsystem::HighShadowsAllowed()
{
	return CVarScatterHighShadows.GetValueOnGameThread() != 0;
}

int32 ULumenScatterSubsystem::ApplyHighShadows(const AActor& Actor, bool bShadows)
{
	// Any size: a bush can be over SmallRadius, so this rule doesn't sit behind IsSmallScatter.
	int32 Changed = 0;
	TInlineComponentArray<UInstancedStaticMeshComponent*> Instanced(&Actor);
	for (UInstancedStaticMeshComponent* Part : Instanced)
	{
		if (!Part || !Part->ComponentHasTag(ShadowHighOnlyTag()) || static_cast<bool>(Part->CastShadow) == bShadows)
		{
			continue;
		}
		// Marks the render state dirty itself (the part's proxy is made again, once, as a preset changes).
		Part->SetCastShadow(bShadows);
		++Changed;
	}
	return Changed;
}
