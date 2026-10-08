#include "Loot/LootDropComponent.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Loot/LootLibrary.h"
#include "Combat/HealthComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponCurseEffects.h"
#include "GameFramework/Actor.h"

void ULootDropComponent::BeginPlay()
{
	Super::BeginPlay();

	// Anything killable without its own table (every creature, the target dummy, future enemies) uses the default one.
	if (!LootTable)
	{
		LootTable = ULootLibrary::GetDefaultLootTable();
	}

	if (UHealthComponent* Health = GetOwner()->FindComponentByClass<UHealthComponent>())
	{
		Health->OnDeath.AddDynamic(this, &ULootDropComponent::HandleOwnerDeath);
	}
}

void ULootDropComponent::HandleOwnerDeath(AController* Killer)
{
	// Asked at the death rather than when play began, so turning the drop off after its owner has spawned still counts.
	if (!bDropOnDeath)
	{
		return;
	}
	// The ammo leans toward the class of the gun that landed the killing shot, so the gun in use keeps itself fed.
	const AWeaponBase* KillWeapon = AWeaponBase::FindKillWeapon(GetOwner());
	const TOptional<EAmmoType> KillAmmo = KillWeapon ? TOptional<EAmmoType>(KillWeapon->GetAmmoType()) : TOptional<EAmmoType>();
	// A practice area the player has already left once (Skyreach on a return visit) drops only ammo: practice, not a farm.
	const bool bWeapons = UAreaRulesSubsystem::DropsGunsAt(this);
	// A Greedy iron's kills roll their loot luckier.
	const float Luck = ExtraLuck + WeaponCurseEffects::KillLootLuck(KillWeapon);
	ULootLibrary::SpawnKillLoot(this, LootTable, GetOwner()->GetActorLocation(), Level, Luck, KillAmmo, bWeapons);
}

TArray<AActor*> ULootDropComponent::DropLoot()
{
	return ULootLibrary::SpawnLoot(this, LootTable, GetOwner()->GetActorLocation(), Level, ExtraLuck);
}
