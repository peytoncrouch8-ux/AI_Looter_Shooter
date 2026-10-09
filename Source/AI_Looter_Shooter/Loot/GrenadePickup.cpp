#include "Loot/GrenadePickup.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/GraveSaltGrenade.h"
#include "Loot/LootTossComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "World/LightBeam.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "TimerManager.h"

namespace
{
	constexpr float RetryInterval = 0.25f;
	/** The beam: an ammo drop's (white, 1.5 m, thin and faint): grenades are ammo of a kind, and colours mean rarity. */
	constexpr float BeamHeight = 150.f;
	constexpr float BeamRadius = 4.f;
	constexpr float BeamGlow = 1.2f;
	/** The tin's height at the pickup's size, where the beam starts (the model's own is about 12 cm). */
	constexpr float TinTop = 12.f * AGrenadePickup::ModelScale;
}

AGrenadePickup::AGrenadePickup()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root: a small sphere that only bounces off world geometry while the pickup is tossed (as the ammo pickups do).
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(10.f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetRootComponent(Collision);

	// Walk-over pickup radius: overlaps pawns only.
	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Collision);
	Trigger->InitSphereRadius(CollectRadius);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetGenerateOverlapEvents(true);

	Model = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Model"));
	Model->SetupAttachment(Collision);
	Model->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Model->SetRelativeLocation(FVector(0.f, 0.f, -8.f));
	Model->SetRelativeScale3D(FVector(ModelScale));

	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Collision);
	Beam->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Beam->SetCastShadow(false);
	Beam->SetCanEverAffectNavigation(false);

	TossMovement = CreateDefaultSubobject<ULootTossComponent>(TEXT("TossMovement"));
	TossMovement->SetUpdatedComponent(Collision);

	Spin = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("Spin"));
	Spin->RotationRate = FRotator(0.f, 90.f, 0.f);
	Spin->SetUpdatedComponent(Model);
}

AGrenadePickup* AGrenadePickup::SpawnGrenades(UWorld* World, int32 Count, const FVector& Location)
{
	if (!World || Count <= 0)
	{
		return nullptr;
	}
	const FTransform Transform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Location);
	AGrenadePickup* Pickup = World->SpawnActorDeferred<AGrenadePickup>(AGrenadePickup::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Pickup)
	{
		Pickup->Amount = Count;
		Pickup->FinishSpawning(Transform);
	}
	return Pickup;
}

void AGrenadePickup::BeginPlay()
{
	Super::BeginPlay();
	// The tin, or the engine's cylinder until the model's been imported.
	if (UStaticMesh* Mesh = AGraveSaltGrenade::FindJarMesh())
	{
		Model->SetStaticMesh(Mesh);
	}
	else
	{
		Model->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
		Model->SetRelativeScale3D(FVector(0.07f, 0.07f, 0.1f) * ModelScale);
	}
	LightBeams::Setup(Beam, FLinearColor::White, BeamGlow, BeamHeight, BeamRadius);
	Beam->SetRelativeLocation(FVector(0.0, 0.0, Model->GetRelativeLocation().Z + TinTop + BeamHeight * 0.5));

	SpawnTime = GetWorld()->GetTimeSeconds();
	SetLifeSpan(LifeSeconds);
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGrenadePickup::HandleTriggerOverlap);
	TossMovement->OnProjectileStop.AddDynamic(this, &AGrenadePickup::HandleTossStopped);
	GetWorldTimerManager().SetTimer(RetryTimer, this, &AGrenadePickup::TryCollect, RetryInterval, true, CollectDelay);
}

void AGrenadePickup::Toss(const FVector& Velocity)
{
	TossMovement->Throw(Velocity);
}

void AGrenadePickup::HandleTossStopped(const FHitResult& ImpactResult)
{
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
}

void AGrenadePickup::HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryCollect();
}

void AGrenadePickup::TryCollect()
{
	const UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() - SpawnTime < CollectDelay)
	{
		return;
	}
	TArray<AActor*> Overlapping;
	Trigger->GetOverlappingActors(Overlapping, APawn::StaticClass());

	// Forget players who walked away, so they hear "full" again next time.
	for (auto It = ToldFull.CreateIterator(); It; ++It)
	{
		if (!It->IsValid() || !Overlapping.Contains(It->Get()))
		{
			It.RemoveCurrent();
		}
	}

	for (AActor* Actor : Overlapping)
	{
		// Only a pawn a player is actually controlling collects.
		const APawn* Pawn = Cast<APawn>(Actor);
		UPlayerThrowComponent* Throw = Pawn && Pawn->IsPlayerControlled() ? UPlayerThrowComponent::Find(Pawn) : nullptr;
		if (!Throw)
		{
			continue;
		}
		const int32 Taken = Throw->AddGrenades(Amount, EGrenadeChange::PickedUp);
		if (Taken > 0)
		{
			Amount -= Taken;
			LooterSound::PlayAt(this, ThrowCue::Pickup, GetActorLocation());
			UE_LOG(LogLooter, Verbose, TEXT("%s picked up %d grave salt (now %d, %d left on the ground)"), *Pawn->GetName(), Taken,
				Throw->GetGrenades(), Amount);
		}
		else if (!ToldFull.Contains(Actor))
		{
			ToldFull.Add(Actor);
			// Full: the feed's once-a-visit word (a pickup that gave none), heard as a soft refusal.
			Throw->OnGrenadesChanged.Broadcast(Throw->GetGrenades(), 0, EGrenadeChange::PickedUp);
			LooterSound::Play2D(this, LooterSoundCue::Denied, 0.5f);
		}
		if (Amount <= 0)
		{
			Destroy();
			return;
		}
	}
}
