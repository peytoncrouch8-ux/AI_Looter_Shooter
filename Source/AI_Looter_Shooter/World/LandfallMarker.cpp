// ALandfallMarker: a tagged point a Reach objective goes to, carried by what it marks (the Lamplighter's prow).

#include "World/LandfallMarker.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ALandfallMarker::ALandfallMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Point = CreateDefaultSubobject<USceneComponent>(TEXT("Point"));
	Point->SetMobility(EComponentMobility::Movable);
	RootComponent = Point;
	SetCanBeDamaged(false);
}

ALandfallMarker* ALandfallMarker::SpawnTagged(UWorld* World, FName Tag, const FVector& Where, USceneComponent* Carrier)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	ALandfallMarker* Marker = World->SpawnActor<ALandfallMarker>(Where, FRotator::ZeroRotator, Params);
	if (!Marker)
	{
		return nullptr;
	}
	if (!Tag.IsNone())
	{
		Marker->Tags.AddUnique(Tag);
	}
	if (Carrier)
	{
		Marker->AttachToComponent(Carrier, FAttachmentTransformRules::KeepWorldTransform);
	}
	return Marker;
}
