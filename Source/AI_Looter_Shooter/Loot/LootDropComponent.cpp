#include "Loot/LootDropComponent.h"
#include "Loot/LootLibrary.h"
#include "Combat/HealthComponent.h"
#include "Weapons/WeaponBase.h"
#include "GameFramework/Actor.h"

void ULootDropComponent::BeginPlay()
{
	Super::BeginPlay();

	// Anything killable without its own table (every creature, the target dummy, future enemies) uses the default one.
	if (!LootTable)
	{
		LootTable = ULootLibrary::GetDefaultLootTable();
	}

	if (bDropOnDeath)
	{
		if (UHealthComponent* Health = GetOwner()->FindComponentByClass<UHealthComponent>())
		{
			Health->OnDeath.AddDynamic(this, &ULootDropComponent::HandleOwnerDeath);
		}
	}
}

void ULootDropComponent::HandleOwnerDeath(AController* Killer)
{
	// The ammo leans toward the class of the gun that landed the killing shot, so the gun in use keeps itself fed.
	const UHealthComponent* Health = GetOwner()->FindComponentByClass<UHealthComponent>();
	const AWeaponBase* KillWeapon = Health ? Cast<AWeaponBase>(Health->GetLastDamageCauser()) : nullptr;
	const TOptional<EAmmoType> KillAmmo = KillWeapon ? TOptional<EAmmoType>(KillWeapon->GetAmmoType()) : TOptional<EAmmoType>();
	ULootLibrary::SpawnKillLoot(this, LootTable, GetOwner()->GetActorLocation(), Level, ExtraLuck, KillAmmo);
}

TArray<AActor*> ULootDropComponent::DropLoot()
{
	return ULootLibrary::SpawnLoot(this, LootTable, GetOwner()->GetActorLocation(), Level, ExtraLuck);
}
