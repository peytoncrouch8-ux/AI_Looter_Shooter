#include "Embers/Powers/EmberPropActor.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Scalability.h"

AEmberPropActor::AEmberPropActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	// Seen, never touched: no creature, player or shot meets it, and it casts its small shadow only.
	Model = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Model"));
	Model->SetupAttachment(Root);
	Model->SetMobility(EComponentMobility::Movable);
	Model->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Model->SetCanEverAffectNavigation(false);
	Model->SetGenerateOverlapEvents(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetCastShadows(false);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetVisibility(false);
}

UStaticMesh* AEmberPropActor::FindMesh(const TCHAR* Path)
{
	if (!Path || !*Path)
	{
		return nullptr;
	}
	const FString ObjectPath(Path);
	if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(ObjectPath)))
	{
		return nullptr;
	}
	return LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

AEmberPropActor* AEmberPropActor::Place(UWorld* World, const TCHAR* MeshPath, const FVector& Ground, float Yaw, const FVector& Scale)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEmberPropActor* Prop = World->SpawnActor<AEmberPropActor>(AEmberPropActor::StaticClass(), Ground, FRotator(0.f, Yaw, 0.f), Params);
	if (!Prop)
	{
		return nullptr;
	}
	if (UStaticMesh* Mesh = FindMesh(MeshPath))
	{
		Prop->Model->SetStaticMesh(Mesh);
		Prop->Model->SetRelativeScale3D(Scale);
		// Its lowest point on the ground, wherever the model keeps its pivot (the mine's lantern hangs from its glass).
		const FBox Bounds = Mesh->GetBoundingBox();
		Prop->Model->SetRelativeLocation(FVector(0.0, 0.0, -Bounds.Min.Z * Scale.Z));
	}
	return Prop;
}

bool AEmberPropActor::HasModel() const
{
	return Model && Model->GetStaticMesh() != nullptr;
}

FVector AEmberPropActor::GetTop() const
{
	const UStaticMesh* Mesh = Model ? Model->GetStaticMesh() : nullptr;
	if (!Mesh)
	{
		return GetActorLocation() + FVector(0.0, 0.0, 40.0);
	}
	const FBox Bounds = Mesh->GetBoundingBox();
	const FVector Scale = Model->GetRelativeScale3D();
	return GetActorLocation() + FVector(0.0, 0.0, (Bounds.Max.Z - Bounds.Min.Z) * Scale.Z);
}

void AEmberPropActor::SetLight(const FLinearColor& Color, float Candela, float Radius, const FVector& Offset)
{
	Light->SetRelativeLocation(Offset);
	Light->SetLightColor(Color);
	Light->SetAttenuationRadius(Radius);
	SetLightIntensity(Candela);
}

bool AEmberPropActor::LightsAllowed()
{
	// A power's light is a whole extra light while it lasts: Grave Ward's lantern measured +0.3 ms on Medium at the Rest's
	// spawn (GPU lights pass and the render thread's visibility wait; 2026-10-20), and new additions pay their own way on
	// Medium (the user's rule). High and Epic light the ground; Low and Medium keep the glass's and the pool's own glow.
	return Scalability::GetQualityLevels().ShadowQuality >= 2;
}

void AEmberPropActor::SetLightIntensity(float Candela)
{
	const bool bShow = Candela > 0.f && LightsAllowed();
	Light->SetIntensity(bShow ? Candela : 0.f);
	Light->SetVisibility(bShow);
}

void AEmberPropActor::Remove()
{
	Destroy();
}
