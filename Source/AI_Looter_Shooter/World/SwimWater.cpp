#include "World/SwimWater.h"
#include "World/SwimWaterRules.h"
#include "World/SwimWaterSubsystem.h"
#include "Components/SceneComponent.h"

ASwimWater::ASwimWater()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;
	SetCanBeDamaged(false);
}

void ASwimWater::RefreshBounds()
{
	Bounds = SwimShore::BoundsOf(Shore);
	DeepBounds = SwimShore::BoundsOf(DeepZone);
}

bool ASwimWater::ContainsXY(const FVector2D& Point) const
{
	return Bounds.bIsValid && Bounds.IsInside(Point) && SwimShore::Contains(Shore, Point);
}

bool ASwimWater::IsDeepXY(const FVector2D& Point) const
{
	return DeepBounds.bIsValid && DeepBounds.IsInside(Point) && SwimShore::Contains(DeepZone, Point);
}

void ASwimWater::BeginPlay()
{
	Super::BeginPlay();
	RefreshBounds();
	if (USwimWaterSubsystem* Waters = USwimWaterSubsystem::Get(this))
	{
		Waters->Register(this);
	}
}

void ASwimWater::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USwimWaterSubsystem* Waters = USwimWaterSubsystem::Get(this))
	{
		Waters->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}
