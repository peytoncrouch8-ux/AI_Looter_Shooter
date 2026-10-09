#include "World/Windmill.h"
#include "Audio/AmbientEmitterComponent.h"
#include "Audio/LooterSoundCues.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

namespace
{
	/** The pace the fan's loop was made at (Art/Sounds/recipes/world.py: eighteen turns a minute). */
	constexpr float SoundTurnsPerMinute = 18.f;
}

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

	// Its sounds hang at the hub on the tower's socket (not on the spinning fan, which would move them every frame).
	FanSound = CreateDefaultSubobject<UAmbientEmitterComponent>(TEXT("FanSound"));
	FanSound->SetupAttachment(Tower, FanSocket);
	FanSound->LoopCue = LooterSoundCue::WindmillFan;
	CreakSound = CreateDefaultSubobject<UAmbientEmitterComponent>(TEXT("CreakSound"));
	CreakSound->SetupAttachment(Tower, FanSocket);
	CreakSound->OneShotCues = { FName(LooterSoundCue::WindmillCreak) };
	CreakSound->OneShotGap = FVector2D(5.0, 13.0);
	CreakSound->Scatter = 0.f;
}

void AWindmill::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Fan->AttachToComponent(Tower, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FanSocket);
	// The sounds follow the socket too, which exists only once the tower has its model.
	FanSound->AttachToComponent(Tower, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FanSocket);
	CreakSound->AttachToComponent(Tower, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FanSocket);
	// Each windmill starts at its own point in the gusts, so two never turn in step.
	WindTime = static_cast<float>(GetTypeHash(GetFName()) % 1000);
}

void AWindmill::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	WindTime += DeltaSeconds;
	const float Gust = FMath::PerlinNoise1D(WindTime / GustSeconds) * 2.f;
	SpeedShare = FMath::Max(0.f, 1.f + Gustiness * Gust);
	const float DegreesPerSecond = TurnsPerMinute * 6.f * SpeedShare;
	Angle = FMath::Fmod(Angle + DegreesPerSecond * DeltaSeconds, 360.f);

	// The fan's sound runs at its pace (the blades' swish comes faster in a gust) and swells with it.
	const float Pace = TurnsPerMinute / SoundTurnsPerMinute * SpeedShare;
	FanSound->SetModulation(FMath::Clamp(0.55f + 0.45f * SpeedShare, 0.3f, 1.4f), FMath::Clamp(Pace, 0.6f, 1.5f));

	// Moving a component costs a render update; skip it while nobody sees the fan (it picks up where the wind left it).
	if (Fan->WasRecentlyRendered(0.25f))
	{
		Fan->SetRelativeRotation(FRotator(0.f, 0.f, Angle));
	}
}
