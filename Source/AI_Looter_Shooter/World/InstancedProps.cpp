#include "World/InstancedProps.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "World/MinimapSubsystem.h"

AInstancedProps::AInstancedProps()
{
	PrimaryActorTick.bCanEverTick = false;
	Instances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Instances"));
	SetRootComponent(Instances);
	ApplyPropFlags();
}

void AInstancedProps::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Only in play: the editor's scatter would keep its layers out of these map-wide bounds (see the class comment).
	// Components are initialized before anything begins play, so the minimap's bake and the creatures see the tag. A
	// passable set is never an obstacle: the player and the creatures go through it.
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && bSolid)
	{
		Tags.AddUnique(MinimapTags::Obstacle);
	}
}

void AInstancedProps::SetInstances(UStaticMesh* Mesh, const TArray<FTransform>& Transforms)
{
	ApplyPropFlags();
	Instances->ClearInstances();
	Instances->SetStaticMesh(Mesh);
	if (Mesh && Transforms.Num() > 0)
	{
		// World space, and no navigation update: nothing in the game walks a navmesh.
		Instances->AddInstances(Transforms, /*bShouldReturnIndices=*/false, /*bWorldSpace=*/true, /*bUpdateNavigation=*/false);
	}
	Instances->MarkRenderStateDirty();
}

void AInstancedProps::ApplyPropFlags()
{
	Instances->SetMobility(EComponentMobility::Static);
	if (bSolid)
	{
		// Solid as a placed static mesh actor is (BlockAll), each instance with its mesh's own hulls.
		Instances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Instances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		// Passable: no query or physics at all, so shots, pawns and the minimap's traces all go through.
		Instances->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	Instances->SetGenerateOverlapEvents(false);
	Instances->SetCanEverAffectNavigation(false);
	Instances->SetCastShadow(bCastShadows);
	Instances->bCastDynamicShadow = bCastShadows;
	// Each instance is culled by its own distance. A cull distance volume judges the whole component by its bounds,
	// which span every instance of the mesh (or, for a mesh used once, would cull it by a different rule from the rest).
	Instances->bNeverDistanceCull = false;
	Instances->bAllowCullDistanceVolume = false;
	Instances->SetCullDistances(0, FMath::Max(0, FMath::RoundToInt(CullDistance)));
}
