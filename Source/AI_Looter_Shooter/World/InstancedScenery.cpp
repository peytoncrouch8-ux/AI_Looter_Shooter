#include "World/InstancedScenery.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"

AInstancedScenery::AInstancedScenery()
{
	PrimaryActorTick.bCanEverTick = false;
	Instances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Instances"));
	SetRootComponent(Instances);
	ApplySceneryFlags();
}

void AInstancedScenery::SetInstances(UStaticMesh* Mesh, const TArray<FTransform>& Transforms)
{
	ApplySceneryFlags();
	Instances->ClearInstances();
	Instances->SetStaticMesh(Mesh);
	if (Mesh && Transforms.Num() > 0)
	{
		// World space, and no navigation update: nothing past the boundary is walked.
		Instances->AddInstances(Transforms, /*bShouldReturnIndices=*/false, /*bWorldSpace=*/true, /*bUpdateNavigation=*/false);
	}
	Instances->MarkRenderStateDirty();
}

void AInstancedScenery::ApplySceneryFlags()
{
	Instances->SetMobility(EComponentMobility::Static);
	Instances->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Instances->SetGenerateOverlapEvents(false);
	Instances->SetCanEverAffectNavigation(false);
	Instances->SetCastShadow(false);
	Instances->bCastDynamicShadow = false;
	Instances->bCastStaticShadow = false;
	// Its bounds span the whole ring: a cull distance by size would drop every instance at once.
	Instances->bNeverDistanceCull = true;
	Instances->SetCullDistances(0, 0);
}
