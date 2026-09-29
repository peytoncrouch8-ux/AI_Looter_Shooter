#include "Environment/EnvironmentLayout.h"
#include "Environment/EnvironmentPalette.h"
#include "Environment/StylizedProp.h"
#include "Environment/LayoutPlaceable.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "AI_Looter_Shooter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

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
		UE_LOG(LogLooter, Warning, TEXT("%s has no LayoutData; Build Mode changes can't be saved."), *GetName());
		return;
	}

	for (const FPlacedObjectRecord& Record : LayoutData->Objects)
	{
		PlaceObject(Record);
	}
}

const FEnvironmentPaletteEntry* AEnvironmentLayout::FindEntry(FName Id) const
{
	return Palette ? Palette->FindEntry(Id) : nullptr;
}

AActor* AEnvironmentLayout::SpawnFromRecord(const FPlacedObjectRecord& Record, bool bPreview) const
{
	const FEnvironmentPaletteEntry* Entry = FindEntry(Record.EntryId);
	UWorld* World = GetWorld();
	if (!Entry || !World)
	{
		UE_LOG(LogLooter, Warning, TEXT("Layout: unknown palette entry '%s'"), *Record.EntryId.ToString());
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AActor* Spawned = nullptr;

	if (UClass* ActorClass = Entry->ActorClass.LoadSynchronous())
	{
		Spawned = World->SpawnActor<AActor>(ActorClass, Record.Transform, Params);
	}
	else if (UStaticMesh* StaticMesh = Entry->StaticMesh.LoadSynchronous())
	{
		Params.bDeferConstruction = true;
		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Record.Transform, Params);
		if (MeshActor)
		{
			MeshActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			MeshActor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
			MeshActor->FinishSpawning(Record.Transform);
		}
		Spawned = MeshActor;
	}
	else
	{
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
		Spawned = Prop;
	}

	if (Spawned && bPreview)
	{
		Spawned->SetActorEnableCollision(false);
		Spawned->SetCanBeDamaged(false);

		// Previews are just a picture: no AI brain, no movement, no gameplay ticking (creatures would walk off).
		if (APawn* Pawn = Cast<APawn>(Spawned))
		{
			if (AController* Controller = Pawn->GetController())
			{
				Controller->UnPossess();
				Controller->Destroy();
			}
		}
		Spawned->SetActorTickEnabled(false);
		for (UActorComponent* Component : Spawned->GetComponents())
		{
			Component->SetComponentTickEnabled(false);
		}
	}
	return Spawned;
}

AActor* AEnvironmentLayout::PlaceObject(const FPlacedObjectRecord& Record)
{
	AActor* Spawned = SpawnFromRecord(Record, false);
	if (Spawned)
	{
		Objects.Add(Spawned, Record);
	}
	return Spawned;
}

AActor* AEnvironmentLayout::SpawnPreview(const FPlacedObjectRecord& Record)
{
	return SpawnFromRecord(Record, true);
}

bool AEnvironmentLayout::IsLayoutObject(const AActor* Actor) const
{
	return Actor && Objects.Contains(Actor);
}

bool AEnvironmentLayout::GetRecord(const AActor* Object, FPlacedObjectRecord& OutRecord) const
{
	const FPlacedObjectRecord* Found = Objects.Find(Object);
	if (!Found)
	{
		return false;
	}
	OutRecord = *Found;
	OutRecord.Transform = GetLayoutTransform(Object);
	return true;
}

bool AEnvironmentLayout::RemoveObject(AActor* Object, FPlacedObjectRecord& OutRecord)
{
	if (!GetRecord(Object, OutRecord))
	{
		return false;
	}
	Objects.Remove(Object);
	Object->Destroy();
	return true;
}

FTransform AEnvironmentLayout::GetLayoutTransform(const AActor* Object)
{
	if (const ILayoutPlaceable* Placeable = Cast<ILayoutPlaceable>(Object))
	{
		return Placeable->GetLayoutTransform();
	}
	return Object->GetActorTransform();
}

void AEnvironmentLayout::FindObjects(FName EntryId, const FVector& Center, float Radius, TArray<AActor*>& OutActors) const
{
	for (const TPair<TWeakObjectPtr<AActor>, FPlacedObjectRecord>& Pair : Objects)
	{
		AActor* Actor = Pair.Key.Get();
		if (Actor && Pair.Value.EntryId == EntryId && FVector::DistSquared2D(Actor->GetActorLocation(), Center) <= Radius * Radius)
		{
			OutActors.Add(Actor);
		}
	}
}

void AEnvironmentLayout::ClearAll()
{
	for (const TPair<TWeakObjectPtr<AActor>, FPlacedObjectRecord>& Pair : Objects)
	{
		if (AActor* Actor = Pair.Key.Get())
		{
			Actor->Destroy();
		}
	}
	Objects.Reset();
}

bool AEnvironmentLayout::SaveLayout(FString& OutMessage)
{
	if (!LayoutData)
	{
		OutMessage = TEXT("No LayoutData asset assigned to the EnvironmentLayout actor.");
		return false;
	}

	// Transforms are read from the live actors, so moves/rotations/scales are captured.
	LayoutData->Objects.Reset();
	for (const TPair<TWeakObjectPtr<AActor>, FPlacedObjectRecord>& Pair : Objects)
	{
		if (const AActor* Actor = Pair.Key.Get())
		{
			FPlacedObjectRecord Record = Pair.Value;
			Record.Transform = GetLayoutTransform(Actor);
			LayoutData->Objects.Add(Record);
		}
	}

#if WITH_EDITOR
	UPackage* Package = LayoutData->GetOutermost();
	Package->MarkPackageDirty();
	const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	if (!UPackage::SavePackage(Package, LayoutData, *Filename, SaveArgs))
	{
		OutMessage = FString::Printf(TEXT("Failed to write %s"), *Filename);
		return false;
	}
	OutMessage = FString::Printf(TEXT("Saved %d objects"), LayoutData->Objects.Num());
	return true;
#else
	OutMessage = TEXT("Layout updated for this session (saving to disk needs the editor).");
	return true;
#endif
}
