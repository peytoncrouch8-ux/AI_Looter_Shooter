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
	/** How far along the table from the weapon each ammo pickup sits (cm): clear of a rifle, still on a 2 m table. */
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

	// Ammo at either end of the table, if the last was collected.
	AmmoPickups.RemoveAll([](const TWeakObjectPtr<AAmmoPickup>& Ammo) { return !Ammo.IsValid(); });
	if (AmmoPickups.IsEmpty() && AmmoMagazines > 0)
	{
		const int32 Rounds = Instance.Stats.MagazineSize * AmmoMagazines;
		for (const float Side : { -1.f, 1.f })
		{
			const FVector AmmoSpot = Spot + Along * Side * AmmoOffset + FVector::UpVector * DropHeight;
			if (AAmmoPickup* Ammo = AAmmoPickup::SpawnAmmo(World, Weapon->AmmoType, FMath::Max(Rounds / 2, 1), AmmoSpot))
			{
				Ammo->Toss(FVector::ZeroVector);
				// It stays until it's taken (loose loot disappears after a while).
				Ammo->SetLifeSpan(0.f);
				AmmoPickups.Add(Ammo);
			}
		}
	}
}

bool AWeaponRack::IsWeaponOffered() const
{
	return Offered.IsValid() && Offered->IsPickup();
}

int32 AWeaponRack::GetAmmoPickupsLeft() const
{
	return AmmoPickups.FilterByPredicate([](const TWeakObjectPtr<AAmmoPickup>& Ammo) { return Ammo.IsValid() && Ammo->GetAmount() > 0; }).Num();
}

bool AWeaponRack::Offers(const AActor* Loot) const
{
	if (!Loot)
	{
		return false;
	}
	return (Offered.Get() == Loot && IsWeaponOffered())
		|| AmmoPickups.ContainsByPredicate([Loot](const TWeakObjectPtr<AAmmoPickup>& Ammo) { return Ammo.Get() == Loot; });
}

void AWeaponRack::RestoreOffer(bool bWeaponOffered, int32 AmmoPickupsLeft)
{
	if (!bWeaponOffered && Offered.IsValid() && Offered->IsPickup())
	{
		Offered->Destroy();
		Offered.Reset();
	}
	AmmoPickups.RemoveAll([](const TWeakObjectPtr<AAmmoPickup>& Ammo) { return !Ammo.IsValid(); });
	while (AmmoPickups.Num() > FMath::Max(AmmoPickupsLeft, 0))
	{
		AmmoPickups.Pop()->Destroy();
	}
	// The restock wait starts over, as if the weapon had just been taken.
	RestockTimer = 0.f;
}

bool AWeaponRack::PlayerHasWeapon() const
{
	const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const UWeaponManagerComponent* Manager = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	return Manager && !Manager->GetWeapons().IsEmpty();
}
