// UWeaponManagerComponent: what it carries, into a saved session and back.

#include "Inventory/WeaponManagerComponent.h"
#include "Inventory/WeaponInventorySave.h"
#include "Weapons/WeaponBase.h"

void UWeaponManagerComponent::SaveInventory(FWeaponInventorySave& OutSave) const
{
	OutSave.Equipped.Reset();
	OutSave.ActiveSlot = INDEX_NONE;
	for (const AWeaponBase* Weapon : Weapons)
	{
		if (IsValid(Weapon))
		{
			if (Weapon == GetActiveWeapon())
			{
				OutSave.ActiveSlot = OutSave.Equipped.Num();
			}
			OutSave.Equipped.Add(Weapon->GetInstanceForStorage());
		}
	}
	OutSave.Backpack = Backpack;
	OutSave.Ammo.SetNum(LooterAmmo::NumTypes);
	for (int32 Type = 0; Type < LooterAmmo::NumTypes; ++Type)
	{
		OutSave.Ammo[Type] = AmmoPool[Type];
	}
}

void UWeaponManagerComponent::RestoreInventory(const FWeaponInventorySave& Save)
{
	// Whatever it was given as play began (its starting weapons) makes way for what the session had.
	ClearInventory();

	// Guns whose kind no longer exists (their data asset gone) can't come back.
	for (const FWeaponInstanceData& Instance : Save.Equipped)
	{
		if (Instance.Definition && Weapons.Num() < MaxWeapons)
		{
			GiveWeapon(Instance);
		}
	}
	for (const FWeaponInstanceData& Instance : Save.Backpack)
	{
		if (Instance.Definition && Backpack.Num() < BackpackCapacity)
		{
			Backpack.Add(Instance);
		}
	}
	for (int32 Type = 0; Type < LooterAmmo::NumTypes && Type < Save.Ammo.Num(); ++Type)
	{
		const EAmmoType AmmoType = static_cast<EAmmoType>(Type);
		AmmoPool[Type] = FMath::Clamp(Save.Ammo[Type], 0, GetMaxAmmo(AmmoType));
		OnAmmoChanged.Broadcast(AmmoType, AmmoPool[Type]);
	}
	// The first gun given went into hand; the one that was in hand goes back there.
	if (Weapons.IsValidIndex(Save.ActiveSlot))
	{
		EquipSlot(Save.ActiveSlot);
	}
	OnInventoryChanged.Broadcast();
}

void UWeaponManagerComponent::ClearInventory()
{
	StopFire();
	AWeaponBase* InHand = GetActiveWeapon();
	if (InHand)
	{
		InHand->OnHolstered();
	}
	TArray<TObjectPtr<AWeaponBase>> Gone = MoveTemp(Weapons);
	Weapons.Reset();
	ActiveSlot = INDEX_NONE;
	// Told before the guns go, so whatever listens can let go of the one in hand.
	if (InHand)
	{
		OnActiveWeaponChanged.Broadcast(nullptr, InHand);
	}
	for (AWeaponBase* Weapon : Gone)
	{
		if (IsValid(Weapon))
		{
			Weapon->Destroy();
		}
	}
	Backpack.Reset();
	for (int32 Type = 0; Type < LooterAmmo::NumTypes; ++Type)
	{
		if (AmmoPool[Type] != 0)
		{
			AmmoPool[Type] = 0;
			OnAmmoChanged.Broadcast(static_cast<EAmmoType>(Type), 0);
		}
	}
	OnInventoryChanged.Broadcast();
}
