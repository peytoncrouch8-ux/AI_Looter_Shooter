#include "Loot/LootTable.h"

ULootTable::ULootTable()
{
	for (const EAmmoType Type : LooterAmmo::AllTypes())
	{
		FAmmoLootEntry Entry;
		Entry.Type = Type;
		// Rockets less often than the rest: one launcher's ammo shouldn't crowd out six other guns'.
		Entry.Weight = Type == EAmmoType::Rocket ? LooterLoot::RocketAmmoWeight : 1.f;
		AmmoTypes.Add(Entry);
	}
}

void ULootTable::UseChestAmmoAmount()
{
	// No range: a chest always pays out the same, unlike a kill.
	AmmoAmountMin = LooterLoot::ChestAmmoAmount;
	AmmoAmountMax = LooterLoot::ChestAmmoAmount;
}
