// FLostLootStore: Lost Property's shelves as plain data: putting a gun on them (the oldest out when full), claiming one,
// finding one, and the order guns handed in together go up in.

#include "Loot/LostLootStore.h"
#include "Loot/LostLootRules.h"

int32 FLostLootStore::GetCapacity() const
{
	return LostLootRules::Capacity(Upgrades);
}

int32 FLostLootStore::FindNextToGo() const
{
	// A named gun is one of a kind (Heirloom is Pa's): the oldest of the others makes room before it ever would.
	const int32 OldestUnnamed = Entries.IndexOfByPredicate([](const FLostLootEntry& Entry) { return Entry.Weapon.Named == nullptr; });
	if (OldestUnnamed != INDEX_NONE)
	{
		return OldestUnnamed;
	}
	return Entries.IsEmpty() ? INDEX_NONE : 0;
}

bool FLostLootStore::Add(const FWeaponInstanceData& Gun, ELostLootReason Reason, const FString& Map, double HandedInAt,
	FLostLootEntry* OutGone, bool* bOutGone)
{
	if (bOutGone)
	{
		*bOutGone = false;
	}
	if (!Gun.Definition)
	{
		return false;
	}
	// Full: the next to go makes room (more than full can only be a save from a bigger store; it comes down by one).
	if (Entries.Num() >= GetCapacity())
	{
		const int32 Going = FindNextToGo();
		if (Entries.IsValidIndex(Going))
		{
			if (OutGone)
			{
				*OutGone = Entries[Going];
			}
			if (bOutGone)
			{
				*bOutGone = true;
			}
			Entries.RemoveAt(Going);
			++GivenAway;
		}
	}
	FLostLootEntry& Entry = Entries.AddDefaulted_GetRef();
	// As it was left: its magazine, its notches, a junk mark the player gave it.
	Entry.Weapon = Gun;
	Entry.Ticket = FMath::Max(NextTicket, 1);
	Entry.Reason = Reason;
	Entry.Map = Map;
	Entry.HandedInAt = HandedInAt;
	NextTicket = Entry.Ticket + 1;
	return true;
}

bool FLostLootStore::RemoveAt(int32 Index, FWeaponInstanceData* OutGun)
{
	if (!Entries.IsValidIndex(Index))
	{
		return false;
	}
	if (OutGun)
	{
		*OutGun = Entries[Index].Weapon;
	}
	Entries.RemoveAt(Index);
	return true;
}

int32 FLostLootStore::FindTicket(int32 Ticket) const
{
	return Entries.IndexOfByPredicate([Ticket](const FLostLootEntry& Entry) { return Entry.Ticket == Ticket; });
}

int32 FLostLootStore::FindNamed(const UNamedWeaponDefinition* Named) const
{
	if (!Named)
	{
		return INDEX_NONE;
	}
	return Entries.IndexOfByPredicate([Named](const FLostLootEntry& Entry) { return Entry.Weapon.Named == Named; });
}

int32 FLostLootStore::Clear()
{
	const int32 Gone = Entries.Num();
	Entries.Reset();
	Unannounced = 0;
	UnannouncedGone = 0;
	return Gone;
}

void FLostLootStore::SortOldestFirst(TArray<FLostLootArrival>& Arrivals)
{
	// Less of its five minutes left means it lay longer. A save from before the life was kept has none (0): those count as
	// the oldest, in the order the save kept them.
	Arrivals.StableSort([](const FLostLootArrival& A, const FLostLootArrival& B) { return A.LifeLeft < B.LifeLeft; });
}

FString FLostLootStore::TicketText(int32 Ticket)
{
	return FString::Printf(TEXT("%04d"), FMath::Max(Ticket, 0));
}
