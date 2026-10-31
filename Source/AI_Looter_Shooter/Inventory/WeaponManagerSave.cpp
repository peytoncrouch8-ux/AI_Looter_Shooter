// UWeaponManagerComponent: what it carries, into a saved session and back.

#include "Inventory/WeaponManagerComponent.h"
#include "AI_Looter_Shooter.h"
#include "Inventory/WeaponInventorySave.h"
#include "Shields/PlayerShieldComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponPartSwap.h"

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
	// How the guns and the spare wards interleave, brought into step first (the wards are saved by the shield, below).
	FBackpackOrder Order = BackpackOrder;
	Order.Reconcile(Backpack.Num(), NumBackpackWards());
	OutSave.BackpackOrder = Order.Items;
	OutSave.Ammo.SetNum(LooterAmmo::NumTypes);
	for (int32 Type = 0; Type < LooterAmmo::NumTypes; ++Type)
	{
		OutSave.Ammo[Type] = AmmoPool[Type];
	}
	OutSave.PartsBox = PartsBox;
	// The wards ride with the guns (the session saves this struct as it is).
	if (const UPlayerShieldComponent* Shield = UPlayerShieldComponent::Find(GetOwner()))
	{
		Shield->SaveWards(OutSave.Shields);
	}
}

void UWeaponManagerComponent::RestoreInventory(const FWeaponInventorySave& Save)
{
	// Whatever it was given as play began (its starting weapons) makes way for what the session had.
	const int32 Before = NumCarriedGuns();
	EmptyInventory();

	// The wards before the guns: a save that had them keeps its own (and its starting ward given), so the first gun coming
	// back doesn't hand over another; an older save gets the starting ward as its guns come back.
	UPlayerShieldComponent* Shield = UPlayerShieldComponent::Find(GetOwner());
	if (Shield)
	{
		Shield->RestoreWards(Save.Shields);
	}

	// The backpack's order as saved (the wards' places too). A spare ward that couldn't come back (its kind gone) gives up
	// its place; the guns' are given up below as they come back or don't.
	BackpackOrder.Items = Save.BackpackOrder;
	if (Shield)
	{
		int32 KeptWards = 0;
		for (const FShieldInstanceData& Spare : Save.Shields.Spares)
		{
			if (Spare.IsSet())
			{
				++KeptWards;
			}
			else
			{
				BackpackOrder.RemoveAt(EBackpackItem::Ward, KeptWards);
			}
		}
	}

	// Guns whose kind no longer exists (their data asset gone) can't come back; the warning below counts them.
	for (const FWeaponInstanceData& Instance : Save.Equipped)
	{
		if (Instance.Definition && Weapons.Num() < MaxWeapons)
		{
			GiveWeapon(Instance);
		}
	}
	// The guns' own count as before: the spare wards never push a saved gun out (an old save's wards may take the backpack
	// over its room once, below).
	for (const FWeaponInstanceData& Instance : Save.Backpack)
	{
		if (Instance.Definition && Backpack.Num() < BackpackCapacity)
		{
			Backpack.Add(Instance);
		}
		else
		{
			BackpackOrder.RemoveAt(EBackpackItem::Gun, Backpack.Num());
		}
	}
	// An old save (no order) lists its guns, then its wards in their old order.
	BackpackOrder.Reconcile(Backpack.Num(), NumBackpackWards());
	if (GetBackpackUsed() > BackpackCapacity)
	{
		// The user's rule (2026-10-31): nothing is thrown away. The backpack holds more than its room this once; nothing more
		// goes in until enough comes out.
		UE_LOG(LogLooter, Warning, TEXT("Backpack: the save carries %d guns and %d spare wards, over the backpack's %d places (wards had their own "
			"room before 2026-10-31). All kept; nothing more goes in until %d come out."), Backpack.Num(), NumBackpackWards(), BackpackCapacity,
			GetBackpackUsed() - BackpackCapacity + 1);
	}
	const int32 Saved = Save.Equipped.Num() + Save.Backpack.Num();
	WarnGunsLost(Saved - NumCarriedGuns(), TEXT("RestoreInventory (a load)"),
		FString::Printf(TEXT("%d of the save's %d guns didn't come back (their kind is gone, or there was no room)"), Saved - NumCarriedGuns(), Saved));
	NoteGunsGone(Before, TEXT("a load whose save carries none: RestoreInventory"));
	for (int32 Type = 0; Type < LooterAmmo::NumTypes && Type < Save.Ammo.Num(); ++Type)
	{
		const EAmmoType AmmoType = static_cast<EAmmoType>(Type);
		AmmoPool[Type] = FMath::Clamp(Save.Ammo[Type], 0, GetMaxAmmo(AmmoType));
		OnAmmoChanged.Broadcast(AmmoType, AmmoPool[Type]);
	}
	// Nor can a part whose kind of gun, or whose own option, no longer exists (part keys are never renamed or reused, so
	// that only happens when a part is taken out of the game).
	for (const FBoxedWeaponPart& Part : Save.PartsBox)
	{
		if (PartsBox.Num() >= MaxBoxedParts)
		{
			break;
		}
		if (Part.Definition && WeaponPartSwap::FindOption(Part))
		{
			PartsBox.Add(Part);
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Parts box: the saved part %s %s no longer exists, so it's left out."), *Part.Slot.ToString(), *Part.Key.ToString());
		}
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
	const int32 Before = NumCarriedGuns();
	EmptyInventory();
	NoteGunsGone(Before, TEXT("emptied on request: ClearInventory"));
}

void UWeaponManagerComponent::EmptyInventory()
{
	StopFire();
	// A swap under way ends with the guns (the one in hand is put away just below, which lets go of its lock).
	StopSwap();
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
		ReleaseGun(Weapon);
	}
	Backpack.Reset();
	BackpackOrder.Reconcile(0, NumBackpackWards());
	PartsBox.Reset();
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
