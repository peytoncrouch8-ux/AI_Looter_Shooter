#include "Loot/WeaponRack.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/AmmoPickup.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The weapon is dropped from a hand's width above the socket so it settles onto the table, not into it. */
	constexpr float DropHeight = 12.f;
	/** How far along the table from the weapon each ammo box sits (cm): clear of a rifle, still on a 2 m table. */
	constexpr float AmmoOffset = 75.f;
	/** Checking for a taken weapon a few times a second is plenty. */
	constexpr float CheckInterval = 0.5f;
}

AWeaponRack::AWeaponRack()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = CheckInterval;

	// World static, like placed scenery: loot settles only on static geometry, so a dynamic rack let its own rifle fall
	// through the table.
	Rack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rack"));
	Rack->SetMobility(EComponentMobility::Static);
	Rack->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Rack;
}

void AWeaponRack::BeginPlay()
{
	Super::BeginPlay();
	Restock();
}

void AWeaponRack::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Still lying here: nothing to do.
	if (Offered.IsValid() && Offered->IsPickup())
	{
		RestockTimer = 0.f;
		return;
	}
	// Taken. Restock only once the player has no weapon at all (dropped it somewhere, or lost it).
	if (PlayerHasWeapon())
	{
		RestockTimer = 0.f;
		return;
	}
	RestockTimer += DeltaSeconds;
	if (RestockTimer >= RestockSeconds)
	{
		RestockTimer = 0.f;
		Restock();
	}
}

void AWeaponRack::Restock()
{
	UWorld* World = GetWorld();
	if (!World || !Weapon)
	{
		UE_LOG(LogLooter, Warning, TEXT("%s has no weapon to offer."), *GetName());
		return;
	}

	// Lengthwise along the table, muzzle to the right as you face the rack (whose front is the actor's forward).
	const FVector Spot = Rack->DoesSocketExist(WeaponSocket) ? Rack->GetSocketLocation(WeaponSocket) : GetActorLocation();
	const FVector Along = -GetActorRightVector();
	const FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Weapon, Rarity, Level);
	const FTransform Where(Along.Rotation(), Spot + FVector::UpVector * DropHeight);
	if (AWeaponBase* Spawned = UWeaponRollLibrary::SpawnWeapon(this, Instance, Where))
	{
		// Loot like any other: it settles on the table, shows its label and can be picked up.
		Spawned->Toss(FVector::ZeroVector);
		Offered = Spawned;
	}

	// Ammo boxes at either end of the table, if the last ones were collected.
	AmmoBoxes.RemoveAll([](const TWeakObjectPtr<AAmmoPickup>& Box) { return !Box.IsValid(); });
	if (AmmoBoxes.IsEmpty() && AmmoMagazines > 0)
	{
		const int32 Rounds = Instance.Stats.MagazineSize * AmmoMagazines;
		for (const float Side : { -1.f, 1.f })
		{
			const FVector BoxSpot = Spot + Along * Side * AmmoOffset + FVector::UpVector * DropHeight;
			if (AAmmoPickup* Box = AAmmoPickup::SpawnAmmo(World, Weapon->AmmoType, FMath::Max(Rounds / 2, 1), BoxSpot))
			{
				Box->Toss(FVector::ZeroVector);
				// It stays until it's taken (loose loot boxes disappear after a while).
				Box->SetLifeSpan(0.f);
				AmmoBoxes.Add(Box);
			}
		}
	}
}

bool AWeaponRack::PlayerHasWeapon() const
{
	const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const UWeaponManagerComponent* Manager = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	return Manager && !Manager->GetWeapons().IsEmpty();
}
