#include "Loot/AmmoPickup.h"
#include "Loot/LootTossComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "World/LightBeam.h"
#include "AI_Looter_Shooter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"

namespace
{
	constexpr float RetryInterval = 0.25f;

	// The beam over every ammo drop: about two thirds the height and width of the smallest gun beam (an Uncommon's, 350
	// by 6) and fainter, so a field of ammo reads from afar without outshining the guns, whose beams carry rarity.
	constexpr float AmmoBeamHeight = 230.f;
	constexpr float AmmoBeamRadius = 4.f;
	constexpr float AmmoBeamGlow = 1.2f;

	/**
	 * Every ammo beam looks the same, so they all share one material instance rather than each drop making its own (a long
	 * fight leaves dozens of drops). It sits in the transient package, kept alive by the beams that use it, and is made
	 * again after the last of them is gone and collected. Transient so a pickup saved in a level never writes it out:
	 * building the pickup sets it again.
	 */
	UMaterialInterface* SharedAmmoBeamMaterial()
	{
		static TWeakObjectPtr<UMaterialInstanceDynamic> SharedInstance;
		if (!SharedInstance.IsValid())
		{
			UMaterialInstanceDynamic* Instance = LightBeams::CreateMaterial(GetTransientPackage(), FLinearColor::White, AmmoBeamGlow, AmmoBeamHeight);
			if (Instance)
			{
				Instance->SetFlags(RF_Transient);
			}
			SharedInstance = Instance;
		}
		return SharedInstance.Get();
	}

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

	// No light of its own (the user's call): the spin, the ink line and the brass highlights carry it, and a light per
	// drop painted pale pools on the ground and cost a light each. The beam is a glowing mesh, not a light, so it lights
	// nothing around it. It hangs off the root rather than the model so it stays upright and still while the bundle spins;
	// its look is set when the pickup is built (OnConstruction), where it takes the material every ammo beam shares.
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

	// White for every class: colors on loot mean rarity. The beam rises from the top of the bundle rather than through
	// it, where its glow would wash out the rounds and their ink line.
	LightBeams::Setup(Beam, SharedAmmoBeamMaterial(), AmmoBeamHeight, AmmoBeamRadius);
	const UStaticMesh* Mesh = Model->GetStaticMesh();
	const double BundleTop = Model->GetRelativeLocation().Z + (Mesh ? Mesh->GetBounds().GetBox().Max.Z : 0.0);
	Beam->SetRelativeLocation(FVector(0.0, 0.0, BundleTop + AmmoBeamHeight * 0.5));
}

void AAmmoPickup::BeginPlay()
{
	Super::BeginPlay();
	SpawnTime = GetWorld()->GetTimeSeconds();
	SetLifeSpan(LifeSeconds);

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
