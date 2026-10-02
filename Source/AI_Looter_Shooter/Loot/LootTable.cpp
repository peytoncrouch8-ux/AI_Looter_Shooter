#include "Loot/LootTable.h"

ULootTable::ULootTable()
{
	for (const EAmmoType Type : LooterAmmo::AllTypes())
	{
		FAmmoLootEntry Entry;
		Entry.Type = Type;
		AmmoTypes.Add(Entry);
	}
}

void ULootTable::UseChestAmmoAmount()
{
	// No range: a chest always pays out the same, unlike a kill.
	AmmoAmountMin = LooterLoot::ChestAmmoAmount;
	AmmoAmountMax = LooterLoot::ChestAmmoAmount;
}
