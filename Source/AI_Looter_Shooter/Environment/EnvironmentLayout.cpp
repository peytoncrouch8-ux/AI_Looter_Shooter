#include "Environment/EnvironmentLayout.h"
#include "Environment/EnvironmentPalette.h"
#include "Environment/StylizedProp.h"
#include "AI_Looter_Shooter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"

AEnvironmentLayout::AEnvironmentLayout()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AEnvironmentLayout::BeginPlay()
{
	Super::BeginPlay();

	if (!LayoutData)
	{
		UE_LOG(LogLooter, Warning, TEXT("%s has no LayoutData; nothing to spawn."), *GetName());
		return;
	}

	for (const FPlacedObjectRecord& Record : LayoutData->Objects)
	{
		SpawnFromRecord(Record);
	}
}

AActor* AEnvironmentLayout::SpawnFromRecord(const FPlacedObjectRecord& Record) const
{
	const FEnvironmentPaletteEntry* Entry = Palette ? Palette->FindEntry(Record.EntryId) : nullptr;
	UWorld* World = GetWorld();
	if (!Entry || !World)
	{
		UE_LOG(LogLooter, Warning, TEXT("Layout: unknown palette entry '%s'"), *Record.EntryId.ToString());
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;

	if (UClass* ActorClass = Entry->ActorClass.LoadSynchronous())
	{
		return World->SpawnActor<AActor>(ActorClass, Record.Transform, Params);
	}

	if (UStaticMesh* StaticMesh = Entry->StaticMesh.LoadSynchronous())
	{
		Params.bDeferConstruction = true;
		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Record.Transform, Params);
		if (MeshActor)
		{
			MeshActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			MeshActor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
			MeshActor->FinishSpawning(Record.Transform);
		}
		return MeshActor;
	}

	// Configure before construction so the mesh is only generated once.
	AStylizedProp* Prop = World->SpawnActorDeferred<AStylizedProp>(AStylizedProp::StaticClass(), Record.Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Prop)
	{
		Prop->Shape = Entry->Shape;
		Prop->Seed = Record.Seed;
		Prop->PrimaryColor = Entry->PrimaryColor;
		Prop->SecondaryColor = Entry->SecondaryColor;
		Prop->FinishSpawning(Record.Transform);
	}
	return Prop;
}
