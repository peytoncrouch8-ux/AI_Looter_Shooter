#include "Loot/AmmoPickup.h"
#include "Loot/LootTossComponent.h"
#include "Environment/StylizedMeshKit.h"
#include "Environment/StylizedSurface.h"
#include "Inventory/WeaponManagerComponent.h"
#include "AI_Looter_Shooter.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "TimerManager.h"
#include "UDynamicMesh.h"

namespace
{
	constexpr float PickupRadius = 110.f;
	constexpr float ModelScale = 1.6f;
	constexpr float RetryInterval = 0.25f;

	// Material slots of the box model.
	namespace AmmoBoxSlot
	{
		constexpr int32 Body = 0;
		constexpr int32 Band = 1;
		constexpr int32 Brass = 2;
		constexpr int32 Tip = 3;
		constexpr int32 Hull = 4;
	}

	// Every class shares one look, an olive can with a stenciled band, so no box reads as a rarity color.
	// Only the rounds on top tell the classes apart.
	const FLinearColor LightColor(1.f, 0.9f, 0.75f);
}

AAmmoPickup::AAmmoPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root: a small sphere that only bounces off world geometry (not other loot) while the box is tossed.
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
	Trigger->InitSphereRadius(PickupRadius);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetGenerateOverlapEvents(true);

	Model = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("Model"));
	Model->SetupAttachment(Collision);
	Model->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Model->SetRelativeScale3D(FVector(ModelScale));
	Model->SetRelativeLocation(FVector(0.f, 0.f, -8.f));

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

void AAmmoPickup::BeginPlay()
{
	Super::BeginPlay();
	SpawnTime = GetWorld()->GetTimeSeconds();
	SetLifeSpan(LifeSeconds);

	BuildModel();
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
			Inventory->OnMessage.Broadcast(FText::FromString(FString::Printf(TEXT("+%d %s"), Taken, Info.Name)));
			UE_LOG(LogLooter, Verbose, TEXT("%s picked up %d %s (now %d / %d, %d left in the box)"), *Pawn->GetName(), Taken, Info.Name,
				Inventory->GetAmmo(AmmoType), Inventory->GetMaxAmmo(AmmoType), Amount);
		}
		else if (!ToldFull.Contains(Actor))
		{
			ToldFull.Add(Actor);
			Inventory->OnMessage.Broadcast(FText::FromString(FString::Printf(TEXT("%s full"), Info.Name)));
		}

		if (Amount <= 0)
		{
			Destroy();
			return;
		}
	}
}

void AAmmoPickup::BuildModel()
{
	using namespace StylizedMesh;
	const LooterAmmo::FInfo& Info = LooterAmmo::GetInfo(AmmoType);

	UDynamicMesh* Scratch = NewObject<UDynamicMesh>(this, NAME_None, RF_Transient);

	// A small ammo can: body, a stenciled band around the middle, lid, and the class's cartridges standing on top.
	Box(Scratch, AmmoBoxSlot::Body, FVector(0.f, 0.f, 6.f), FVector(20.f, 13.f, 12.f));
	Box(Scratch, AmmoBoxSlot::Band, FVector(0.f, 0.f, 6.5f), FVector(20.6f, 13.6f, 3.2f));
	Box(Scratch, AmmoBoxSlot::Body, FVector(0.f, 0.f, 12.4f), FVector(21.f, 14.f, 1.2f));
	Box(Scratch, AmmoBoxSlot::Body, FVector(0.f, 0.f, 13.6f), FVector(8.f, 2.f, 1.2f)); // handle

	const int32 Count = FMath::Max(Info.CartridgeCount, 1);
	const float Spacing = FMath::Min(Info.CartridgeRadius * 2.6f, 16.f / FMath::Max(Count - 1, 1));
	const float CaseHeight = Info.CartridgeHeight * 0.65f;
	const bool bShells = AmmoType == EAmmoType::Shotgun;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float X = (Index - (Count - 1) * 0.5f) * Spacing;
		const FVector Base(X, 3.2f, 13.f);
		// Shotgun shells: red plastic hull on a brass base. Everything else: brass case with a copper tip.
		Cylinder(Scratch, AmmoBoxSlot::Brass, FTransform(Base), Info.CartridgeRadius, bShells ? Info.CartridgeHeight * 0.25f : CaseHeight, 8);
		if (bShells)
		{
			Cylinder(Scratch, AmmoBoxSlot::Hull, FTransform(Base + FVector(0.f, 0.f, Info.CartridgeHeight * 0.25f)), Info.CartridgeRadius * 0.95f,
				Info.CartridgeHeight * 0.75f, 8);
		}
		else
		{
			Cone(Scratch, AmmoBoxSlot::Tip, FTransform(Base + FVector(0.f, 0.f, CaseHeight)), Info.CartridgeRadius, Info.CartridgeRadius * 0.3f,
				Info.CartridgeHeight - CaseHeight, 8);
		}
	}
	FinishNormals(Scratch, 0.f);

	UE::Geometry::FDynamicMesh3 Built;
	Scratch->ProcessMesh([&Built](const UE::Geometry::FDynamicMesh3& Source) { Built = Source; });
	Model->SetMesh(MoveTemp(Built));

	TArray<FStylizedSurface> Surfaces;
	Surfaces.Add(FStylizedSurface::Solid(StylizedColors::Hex(0x4d5a38), 0.06f)); // olive drab
	FStylizedSurface Band = FStylizedSurface::Solid(StylizedColors::Hex(0xe6dcc0), 0.f);
	Band.Glow = 0.35f; // a faint glow so boxes catch the eye in the grass
	Surfaces.Add(Band);
	Surfaces.Add(FStylizedSurface::Solid(StylizedColors::Hex(0xd6a64c), 0.05f));
	Surfaces.Add(FStylizedSurface::Solid(StylizedColors::Hex(0xb8683a), 0.05f));
	Surfaces.Add(FStylizedSurface::Solid(StylizedColors::Hex(0xb8392c), 0.05f));
	StylizedSurfaces::Apply(Model, Surfaces);
}
