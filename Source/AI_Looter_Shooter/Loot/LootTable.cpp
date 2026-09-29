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
