#include "World/DuskScenery.h"
#include "World/LightingStateSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"

ADuskScenery::ADuskScenery()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	Instances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Instances"));
	SetRootComponent(Instances);
	ApplySceneryFlags();
}

void ADuskScenery::SetInstances(UStaticMesh* Mesh, const TArray<FTransform>& Transforms)
{
	ApplySceneryFlags();
	Instances->ClearInstances();
	Instances->SetStaticMesh(Mesh);
	if (Mesh && Transforms.Num() > 0)
	{
		Instances->AddInstances(Transforms, /*bShouldReturnIndices=*/false, /*bWorldSpace=*/true, /*bUpdateNavigation=*/false);
	}
	Instances->MarkRenderStateDirty();
}

void ADuskScenery::ApplySceneryFlags()
{
	Instances->SetMobility(EComponentMobility::Static);
	Instances->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Instances->SetGenerateOverlapEvents(false);
	Instances->SetCanEverAffectNavigation(false);
	Instances->SetCastShadow(false);
	Instances->bCastDynamicShadow = false;
	Instances->bCastStaticShadow = false;
	Instances->bReceivesDecals = false;
	// Each wisp and fog bank goes by itself at the art's distance; the component's bounds (the whole Rim) never cull it all.
	Instances->SetCullDistances(static_cast<int32>(CullDistance * 0.85f), static_cast<int32>(CullDistance));
}

void ADuskScenery::BeginPlay()
{
	Super::BeginPlay();
	ApplySceneryFlags();
	ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(this);
	if (Lighting)
	{
		Listening = Lighting->OnChanged.AddUObject(this, &ADuskScenery::OnLightingChanged);
	}
	// As the level begins: shown only if it starts in its state (a level without states is day).
	ApplyState(Lighting ? Lighting->GetState() : NAME_None);
}

void ADuskScenery::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(this))
	{
		Lighting->OnChanged.Remove(Listening);
	}
	Listening.Reset();
	Super::EndPlay(EndPlayReason);
}

void ADuskScenery::OnLightingChanged(const FLightingStateChange& Change)
{
	ApplyState(Change.To);
}

void ADuskScenery::ApplyState(FName State)
{
	// FName equality ignores case, as the console's state names do.
	bShownNow = !State.IsNone() && State == ShownState;
	SetActorHiddenInGame(!bShownNow);
}
