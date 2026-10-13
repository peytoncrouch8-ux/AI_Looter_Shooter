// UWeaponManagerComponent: the vendors' side of the weapon manager: a backpack gun sold, and the junk mark.

#include "Inventory/WeaponManagerComponent.h"

bool UWeaponManagerComponent::TakeFromBackpack(int32 BackpackIndex, FWeaponInstanceData& OutGun)
{
	if (!Backpack.IsValidIndex(BackpackIndex))
	{
		return false;
	}
	OutGun = Backpack[BackpackIndex];
	Backpack.RemoveAt(BackpackIndex);
	OnInventoryChanged.Broadcast();
	return true;
}

bool UWeaponManagerComponent::SetBackpackJunk(int32 BackpackIndex, bool bJunk)
{
	if (!Backpack.IsValidIndex(BackpackIndex) || !Backpack[BackpackIndex].Definition || Backpack[BackpackIndex].Named)
	{
		return false;
	}
	FWeaponInstanceData& Gun = Backpack[BackpackIndex];
	if (Gun.bJunk != bJunk)
	{
		Gun.bJunk = bJunk;
		OnInventoryChanged.Broadcast();
	}
	return true;
}
