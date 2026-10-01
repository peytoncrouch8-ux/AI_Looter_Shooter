#include "Loot/AmmoPickup.h"
#include "Loot/LootTossComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "AI_Looter_Shooter.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float RetryInterval = 0.25f;
	const FLinearColor LightColor(1.f, 0.9f, 0.75f);

	/** The bundle of rounds of each ammo type, in EAmmoType order. */
	TArray<UStaticMesh*> FindTypeModels()
	{
		TArray<UStaticMesh*> Models;
		for (const EAmmoType Type : LooterAmmo::AllTypes())
		{
			const FString Name = StaticEnum<EAmmoType>()->GetNameStringByValue(static_cast<int64>(Type));
			const ConstructorHelpers::FObjectFinder<UStaticMesh> Model(*FString::Printf(TEXT("/Game/Art/Loot/SM_Ammo%s.SM_Ammo%s"), *Name, *Name));
			Models.Add(Model.Object);
		}
		return Models;
	}
}

AAmmoPickup::AAmmoPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root: a small sphere that only bounces off world geometry (not other loot) while the pickup is tossed.
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
	static const TArray<UStaticMesh*> Models = FindTypeModels();
	TypeModels.Append(Models);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Collision);
	// High enough above the cartridges not to blow them out; mostly it paints a pool of the class color on the ground.
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(3.f);
	Glow->SetAttenuationRadius(130.f);
	Glow->SetCastShadows(false);

	TossMovement = CreateDefaultSubobject<ULootTossComponent>(TEXT("TossMovement"));
	TossMovement->SetUpdatedComponent(Collision);

	Spin = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("Spin"));
	Spin->RotationRate = FRotator(0.f, 90.f, 0.f);
	Spin->SetUpdatedComponent(Model);
}

AAmmoPickup* AAmmoPickup::SpawnAmmo(UWorld* World, EAmmoType Type, int32 Amount, const FVector& Location)
{
	if (!World || !LooterAmmo::IsValid(Type) || Amount <= 0)
	{
		return nullptr;
	}
	const FTransform Transform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Location);
	AAmmoPickup* Pickup = World->SpawnActorDeferred<AAmmoPickup>(AAmmoPickup::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Pickup)
	{
		Pickup->AmmoType = Type;
		Pickup->Amount = Amount;
		Pickup->FinishSpawning(Transform);
	}
	return Pickup;
}

void AAmmoPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const int32 Type = static_cast<int32>(AmmoType);
	Model->SetStaticMesh(TypeModels.IsValidIndex(Type) ? TypeModels[Type] : nullptr);
}

void AAmmoPickup::BeginPlay()
{
	Super::BeginPlay();
	SpawnTime = GetWorld()->GetTimeSeconds();
	SetLifeSpan(LifeSeconds);

	Glow->SetLightColor(LightColor);

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AAmmoPickup::HandleTriggerOverlap);
	TossMovement->OnProjectileStop.AddDynamic(this, &AAmmoPickup::HandleTossStopped);
	GetWorldTimerManager().SetTimer(RetryTimer, this, &AAmmoPickup::TryCollect, RetryInterval, true, CollectDelay);
}

void AAmmoPickup::Toss(const FVector& Velocity)
{
	TossMovement->Throw(Velocity);
}

void AAmmoPickup::HandleTossStopped(const FHitResult& ImpactResult)
{
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
}

void AAmmoPickup::HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryCollect();
}

void AAmmoPickup::TryCollect()
{
	if (GetWorld()->GetTimeSeconds() - SpawnTime < CollectDelay)
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

	const LooterAmmo::FInfo& Info = LooterAmmo::GetInfo(AmmoType);
	for (AActor* Actor : Overlapping)
	{
		// Only a pawn a player is actually controlling collects.
		const APawn* Pawn = Cast<APawn>(Actor);
		UWeaponManagerComponent* Inventory = Pawn && Pawn->IsPlayerControlled() ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
		if (!Inventory)
		{
			continue;
		}

		const int32 Taken = Inventory->AddAmmo(AmmoType, Amount);
		if (Taken > 0)
		{
			Amount -= Taken;
			// Shown in the HUD's pickup feed, not its message plate.
			Inventory->OnAmmoPickedUp.Broadcast(AmmoType, Taken);
			UE_LOG(LogLooter, Verbose, TEXT("%s picked up %d %s (now %d / %d, %d left on the ground)"), *Pawn->GetName(), Taken, Info.Name,
				Inventory->GetAmmo(AmmoType), Inventory->GetMaxAmmo(AmmoType), Amount);
		}
		else if (!ToldFull.Contains(Actor))
		{
			ToldFull.Add(Actor);
			Inventory->OnAmmoPickedUp.Broadcast(AmmoType, 0);
		}

		if (Amount <= 0)
		{
			Destroy();
			return;
		}
	}
}
