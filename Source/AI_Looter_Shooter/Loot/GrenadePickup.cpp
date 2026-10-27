#include "Loot/GrenadePickup.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/GraveSaltGrenade.h"
#include "Combat/GraveSaltRules.h"
#include "Combat/GrenadeKinds.h"
#include "Combat/GrenadeKindsTuning.h"
#include "Loot/LootTossComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerThrowRules.h"
#include "Loot/LootBeamCrowd.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "World/LightBeam.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
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
	constexpr float TinTop = FGraveSaltRules::JarFuseTop * AGrenadePickup::ModelScale;

	/** The local player's level (1 without one: a test level), for the kind a dropped tin rolls. */
	int32 PlayerLevel(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const ULocalPlayer* Player = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
		const UPlayerProgressionSubsystem* Progression = Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
		return Progression ? FMath::Max(Progression->GetLevel(), 1) : 1;
	}
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
	if (!World)
	{
		return nullptr;
	}
	// The kind is rolled here, for whoever drops one (a kill, a chest, a bounty's haul), at the local player's level.
	const EGrenadeKind Kind = GrenadeKinds::RollDrop(PlayerLevel(World), FMath::FRand(), FMath::FRand(), FGrenadeKindTuning::Current());
	return SpawnKind(World, Count, Location, Kind);
}

AGrenadePickup* AGrenadePickup::SpawnKind(UWorld* World, int32 Count, const FVector& Location, EGrenadeKind InKind)
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
		Pickup->Kind = InKind;
		Pickup->FinishSpawning(Transform);
		if (GrenadeKinds::IsSpecial(InKind))
		{
			UE_LOG(LogLooter, Verbose, TEXT("%s: a %s dropped"), *Pickup->GetName(), *GrenadeKinds::Name(InKind).ToString());
		}
	}
	return Pickup;
}

void AGrenadePickup::BeginPlay()
{
	Super::BeginPlay();
	// Among other drops, one beam stands for them all (LootBeamCrowd); a lone tin keeps its own.
	ULootBeamCrowdSubsystem::Join(this, Beam);
	// Its kind's tin (the salt tin until that one's imported), or the engine's cylinder until any is.
	if (UStaticMesh* Mesh = AGraveSaltGrenade::FindJarMesh(Kind))
	{
		Model->SetStaticMesh(Mesh);
	}
	else
	{
		Model->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
		Model->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.2f) * ModelScale);
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

	for (AActor* Actor : Overlapping)
	{
		// Only a pawn a player is actually controlling collects.
		const APawn* Pawn = Cast<APawn>(Actor);
		UPlayerThrowComponent* Throw = Pawn && Pawn->IsPlayerControlled() ? UPlayerThrowComponent::Find(Pawn) : nullptr;
		if (!Throw)
		{
			continue;
		}
		CollectInto(*Throw);
		return;
	}
}

int32 AGrenadePickup::CollectInto(UPlayerThrowComponent& Throw)
{
	// As the ammo drops do (AAmmoPickup::CollectInto): whatever fits goes in and the pickup is spent either way, so a full
	// count clears the tin from the ground quietly. The counter's flash, the feed's "+N" line (both from the count changing)
	// and the pickup sound come only with grenades actually added.
	// A special tin swaps the kind carried even when the count is full: that's heard too (and the new kind's clasp over it).
	const FGrenadeCollect Made = Throw.CollectGrenades(Kind, Amount, EGrenadeChange::PickedUp);
	const int32 Taken = Made.Taken;
	if (Taken > 0 || Made.bSwapped)
	{
		LooterSound::PlayAt(this, ThrowCue::Pickup, GetActorLocation());
	}
	if (Made.bSwapped)
	{
		LooterSound::Play2D(this, GrenadeKindCue::Swap);
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s: %d %s collected, %d added (now %d of %s)"), *GetName(), Amount, *GrenadeKinds::Name(Kind).ToString(), Taken,
		Throw.GetGrenades(), *GrenadeKinds::Name(Throw.GetKind()).ToString());
	Amount = 0;
	Destroy();
	return Taken;
}
