#include "World/MapRoad.h"
#include "Components/SceneComponent.h"

AMapRoad::AMapRoad()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	// Only data for the map's bake: nothing to see, hit or replicate.
	SetHidden(true);
	SetActorEnableCollision(false);
	SetCanBeDamaged(false);
}
