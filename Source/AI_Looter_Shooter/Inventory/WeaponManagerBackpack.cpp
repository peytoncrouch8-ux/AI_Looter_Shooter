// UWeaponManagerComponent: the backpack's shared room. Guns and the player's spare wards take the same places (the user,
// 2026-10-31: "they don't get a seperate storage amount than guns"), and they are listed in the order they went in
// (FBackpackOrder). The guns are this component's Backpack; the wards stay the shield component's Spares, which tells this
// one when they change (NoteWardsChanged).

#include "Inventory/WeaponManagerComponent.h"
#include "Shields/PlayerShieldComponent.h"

int32 UWeaponManagerComponent::NumBackpackWards() const
{
	const UPlayerShieldComponent* Shield = UPlayerShieldComponent::Find(GetOwner());
	return Shield ? Shield->GetSpares().Num() : 0;
}

int32 UWeaponManagerComponent::GetBackpackUsed() const
{
	return Backpack.Num() + NumBackpackWards();
}

bool UWeaponManagerComponent::HasBackpackRoom() const
{
	return GetBackpackUsed() < BackpackCapacity;
}

TArray<FBackpackItem> UWeaponManagerComponent::GetBackpackItems() const
{
	// A copy brought into step, so a path that changed a list without a word still lists everything carried.
	FBackpackOrder Order = BackpackOrder;
	Order.Reconcile(Backpack.Num(), NumBackpackWards());
	return Order.Entries();
}

void UWeaponManagerComponent::NoteWardsChanged(int32 RemovedSpare)
{
	if (RemovedSpare != INDEX_NONE)
	{
		BackpackOrder.RemoveAt(EBackpackItem::Ward, RemovedSpare);
	}
	// A ward that went in takes the backpack's last place; one emptied out takes its place with it.
	BackpackOrder.Reconcile(Backpack.Num(), NumBackpackWards());
	OnInventoryChanged.Broadcast();
}

void UWeaponManagerComponent::SyncBackpackOrder()
{
	BackpackOrder.Reconcile(Backpack.Num(), NumBackpackWards());
}

void UWeaponManagerComponent::BackpackAdd(const FWeaponInstanceData& Instance)
{
	SyncBackpackOrder();
	Backpack.Add(Instance);
	BackpackOrder.Add(EBackpackItem::Gun);
}

int32 UWeaponManagerComponent::BackpackRemoveAt(int32 Index)
{
	if (!Backpack.IsValidIndex(Index))
	{
		return INDEX_NONE;
	}
	SyncBackpackOrder();
	Backpack.RemoveAt(Index);
	return BackpackOrder.RemoveAt(EBackpackItem::Gun, Index);
}

void UWeaponManagerComponent::BackpackInsert(const FWeaponInstanceData& Instance, int32 Index, int32 Position)
{
	Index = FMath::Clamp(Index, 0, Backpack.Num());
	// Not synced first: a swap's insert follows its own remove, whose place it reuses.
	Backpack.Insert(Instance, Index);
	BackpackOrder.Insert(EBackpackItem::Gun, Index, Position);
	SyncBackpackOrder();
}
