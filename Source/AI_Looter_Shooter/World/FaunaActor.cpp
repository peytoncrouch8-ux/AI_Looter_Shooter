#include "World/FaunaActor.h"
#include "World/FaunaSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"

AFaunaActor::AFaunaActor()
{
	// The subsystem updates it; it never ticks by itself (a hundred small actors ticking would cost more than they draw).
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);
}

void AFaunaActor::BeginPlay()
{
	Super::BeginPlay();
	if (UFaunaSubsystem* Fauna = UFaunaSubsystem::Get(this))
	{
		Fauna->Register(this);
	}
}

void AFaunaActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFaunaSubsystem* Fauna = UFaunaSubsystem::Get(this))
	{
		Fauna->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AFaunaActor::UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context)
{
}

FVector AFaunaActor::GetFaunaCenter() const
{
	return GetActorLocation();
}

float AFaunaActor::GetFaunaRadius() const
{
	return 0.f;
}

float AFaunaActor::DistanceFrom(const FVector& Location) const
{
	return FMath::Max(0.f, static_cast<float>(FVector::Dist(GetFaunaCenter(), Location)) - GetFaunaRadius());
}

void AFaunaActor::SetFaunaShown(bool bShown)
{
	if (bShown == bFaunaShown)
	{
		return;
	}
	bFaunaShown = bShown;
	SetActorHiddenInGame(!bShown);
	OnFaunaShownChanged(bShown);
}

bool AFaunaActor::IsInItsLight(FName Lighting) const
{
	// A level without lighting states (None) is always day: something out only at dusk never shows there.
	if (ShownIn.IsNone())
	{
		return true;
	}
	const FName Now = Lighting.IsNone() ? FName(TEXT("Day")) : Lighting;
	return Now == ShownIn;
}

UInstancedStaticMeshComponent* AFaunaActor::MakeInstances(FName Name, UStaticMesh* Mesh, bool bCastShadows)
{
	UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(this, Name, RF_Transient);
	Instances->SetMobility(EComponentMobility::Movable);
	Instances->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Instances->SetGenerateOverlapEvents(false);
	Instances->SetCanEverAffectNavigation(false);
	Instances->SetCastShadow(bCastShadows);
	Instances->bCastDynamicShadow = bCastShadows;
	Instances->bReceivesDecals = false;
	Instances->bAffectDistanceFieldLighting = false;
	Instances->bVisibleInRayTracing = false;
	// Each instance goes by its own distance; the component's bounds cover the whole flock, or a whole valley of zones.
	Instances->bAllowCullDistanceVolume = false;
	if (CullDistance > 0.f)
	{
		Instances->SetCullDistances(static_cast<int32>(CullDistance * 0.9f), static_cast<int32>(CullDistance));
	}
	Instances->SetStaticMesh(Mesh);
	Instances->SetupAttachment(Root);
	Instances->RegisterComponent();
	AddInstanceComponent(Instances);
	return Instances;
}
