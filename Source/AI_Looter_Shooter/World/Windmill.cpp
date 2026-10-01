#include "World/Windmill.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

AWindmill::AWindmill()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// World static like placed scenery, so traces that look for the ground and solid things (the minimap, loot) see it.
	Tower = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tower"));
	Tower->SetMobility(EComponentMobility::Static);
	Tower->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Tower;

	// The fan moves every frame: without collision it costs no physics updates, and it spins high above anyone's head.
	Fan = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fan"));
	Fan->SetMobility(EComponentMobility::Movable);
	Fan->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Fan->SetupAttachment(Tower);
}

void AWindmill::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Fan->AttachToComponent(Tower, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FanSocket);
	// Each windmill starts at its own point in the gusts, so two never turn in step.
	WindTime = static_cast<float>(GetTypeHash(GetFName()) % 1000);
}

void AWindmill::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	WindTime += DeltaSeconds;
	const float Gust = FMath::PerlinNoise1D(WindTime / GustSeconds) * 2.f;
	const float DegreesPerSecond = TurnsPerMinute * 6.f * FMath::Max(0.f, 1.f + Gustiness * Gust);
	Angle = FMath::Fmod(Angle + DegreesPerSecond * DeltaSeconds, 360.f);

	// Moving a component costs a render update; skip it while nobody sees the fan (it picks up where the wind left it).
	if (Fan->WasRecentlyRendered(0.25f))
	{
		Fan->SetRelativeRotation(FRotator(0.f, 0.f, Angle));
	}
}
