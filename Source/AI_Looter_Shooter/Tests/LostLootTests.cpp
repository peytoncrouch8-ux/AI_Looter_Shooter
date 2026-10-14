#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/EconomyRules.h"
#include "Loot/LostLootRules.h"
#include "Loot/LostLootStore.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "UObject/Package.h"

// Lost Property's rules and shelves alone, without a level (LostLootPlayTests.cpp has them in test levels, with the
// session's saves, the ground's clock, claiming and the screen): the five minutes, the shelves' size and their upgrades'
// prices, the oldest out when full (never a named gun while another is there), the tickets, the order guns go up in.

namespace
{
	/** A gun of a stand-in kind: the shelves only ask that it has one. */
	FWeaponInstanceData MakeGun(UWeaponDefinition* Kind, int32 Seed, UNamedWeaponDefinition* Named = nullptr)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = Kind;
		Gun.Named = Named;
		Gun.Seed = Seed;
		Gun.Level = 3;
		Gun.Rarity = EWeaponRarity::Uncommon;
		return Gun;
	}

	TArray<int32> Seeds(const FLostLootStore& Store)
	{
		TArray<int32> Found;
		for (const FLostLootEntry& Entry : Store.Entries)
		{
			Found.Add(Entry.Weapon.Seed);
		}
		return Found;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostLootRulesTest, "Looter.LostLoot.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLostLootRulesTest::RunTest(const FString& Parameters)
{
	// The user's numbers: five minutes on the ground, 30 on the shelves, five upgrades of 10 each.
	TestEqual(TEXT("Five minutes on the ground"), LostLootRules::GroundLifeSeconds, 300.f);
	TestEqual(TEXT("30 to start"), LostLootRules::Capacity(0), 30);
	TestEqual(TEXT("40 after one"), LostLootRules::Capacity(1), 40);
	TestEqual(TEXT("80 at most"), LostLootRules::Capacity(LostLootRules::MaxUpgrades), 80);
	TestEqual(TEXT("...and never more"), LostLootRules::Capacity(9), 80);
	TestEqual(TEXT("...or less"), LostLootRules::Capacity(-2), 30);
	TestEqual(TEXT("Five upgrades"), LostLootRules::MaxUpgrades, 5);

	// Prices rise, and grow with the player's level as the vendors' do; nothing after the fifth.
	const int32 Bases[] = { 300, 600, 1000, 1600, 2400 };
	int32 Last = 0;
	for (int32 Upgrades = 0; Upgrades < LostLootRules::MaxUpgrades; ++Upgrades)
	{
		const int32 Number = Upgrades + 1;
		TestEqual(FString::Printf(TEXT("Upgrade %d's base price"), Number), LostLootRules::UpgradeBasePrice(Upgrades), Bases[Upgrades]);
		TestEqual(FString::Printf(TEXT("Upgrade %d at level 1"), Number), LostLootRules::UpgradePrice(Upgrades, 1), Bases[Upgrades]);
		TestEqual(FString::Printf(TEXT("Upgrade %d at level 4, grown as a vendor's price grows"), Number), LostLootRules::UpgradePrice(Upgrades, 4),
			LooterEconomy::AtLevel(Bases[Upgrades], 4));
		TestTrue(FString::Printf(TEXT("Upgrade %d dearer than the one before"), Number), Bases[Upgrades] > Last);
		TestFalse(FString::Printf(TEXT("Upgrade %d has a name"), Number), LostLootRules::UpgradeName(Upgrades).IsEmpty());
		Last = Bases[Upgrades];
	}
	TestEqual(TEXT("At level 4 the first costs about two guns of the day"), LostLootRules::UpgradePrice(0, 4), 372);
	TestEqual(TEXT("None after the fifth"), LostLootRules::UpgradePrice(LostLootRules::MaxUpgrades, 4), 0);
	TestTrue(TEXT("...and no name"), LostLootRules::UpgradeName(LostLootRules::MaxUpgrades).IsEmpty());

	// When a gun on the ground goes: its time up, unless the player stands by it, and then once its grace is up too.
	TestFalse(TEXT("Time left: it stays"), LostLootRules::IsDue(0.5f, false));
	TestTrue(TEXT("Time up, the player away: it goes"), LostLootRules::IsDue(0.f, false));
	TestFalse(TEXT("Time up, the player by it: it waits"), LostLootRules::IsDue(-10.f, true));
	TestTrue(TEXT("...but not for good"), LostLootRules::IsDue(-LostLootRules::NearGraceSeconds, true));

	// The HUD's line, counted and plural; the oldest gone said when there were.
	TestEqual(TEXT("One gun"), LostLootRules::SentLine(1, 0).ToString(), FString(TEXT("1 gun sent to Lost Property at the depot")));
	TestEqual(TEXT("Three guns"), LostLootRules::SentLine(3, 0).ToString(), FString(TEXT("3 guns sent to Lost Property at the depot")));
	TestTrue(TEXT("The oldest gone to make room"), LostLootRules::SentLine(40, 10).ToString().Contains(TEXT("10 oldest")));
	TestEqual(TEXT("A ticket's number"), FLostLootStore::TicketText(42), FString(TEXT("0042")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostLootShelvesTest, "Looter.LostLoot.Shelves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLostLootShelvesTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Kind = NewObject<UWeaponDefinition>(GetTransientPackage());
	UNamedWeaponDefinition* Heirloom = NewObject<UNamedWeaponDefinition>(GetTransientPackage());
	const FString Map(TEXT("/Game/Maps/Lvl_RansomsRest"));

	// Thirty fill it, each with the next ticket.
	FLostLootStore Store;
	for (int32 Seed = 1; Seed <= 30; ++Seed)
	{
		Store.Add(MakeGun(Kind, Seed), ELostLootReason::LeftLying, Map, Seed * 10.0);
	}
	TestTrue(TEXT("Full at thirty"), Store.Num() == 30 && Store.IsFull());
	TestTrue(TEXT("Tickets count up from 1"), Store.Num() == 30 && Store.Entries[0].Ticket == 1 && Store.Entries[29].Ticket == 30
		&& Store.NextTicket == 31);
	TestEqual(TEXT("The first in is the next to go"), Store.FindNextToGo(), 0);
	TestFalse(TEXT("A gun with no kind isn't taken"), Store.Add(FWeaponInstanceData(), ELostLootReason::LeftLying, Map, 0.0));

	// The thirty-first: the oldest goes out to make room.
	FLostLootEntry Gone;
	bool bGone = false;
	TestTrue(TEXT("The thirty-first goes on"), Store.Add(MakeGun(Kind, 31), ELostLootReason::LeftArea, Map, 310.0, &Gone, &bGone));
	TestTrue(TEXT("...the oldest out"), bGone && Gone.Weapon.Seed == 1 && Gone.Ticket == 1);
	TestTrue(TEXT("...still thirty, newest last"), Store.Num() == 30 && Store.Entries.Last().Weapon.Seed == 31 && Store.Entries.Last().Ticket == 31);
	TestEqual(TEXT("...one given away"), Store.GivenAway, 1);
	TestFalse(TEXT("...and the gone one's nowhere on the shelves"), Seeds(Store).Contains(1));

	// A named gun is one of a kind: while another is there, it never goes to make room.
	FLostLootStore Kept;
	Kept.Add(MakeGun(Kind, 100, Heirloom), ELostLootReason::LeftLying, Map, 0.0);
	for (int32 Seed = 101; Seed < 130; ++Seed)
	{
		Kept.Add(MakeGun(Kind, Seed), ELostLootReason::LeftLying, Map, 0.0);
	}
	TestEqual(TEXT("The named gun is the oldest, but the next to go is the oldest of the rest"), Kept.FindNextToGo(), 1);
	Kept.Add(MakeGun(Kind, 130), ELostLootReason::LeftLying, Map, 0.0);
	TestTrue(TEXT("Heirloom stays when the shelves make room"), Kept.FindNamed(Heirloom) == 0 && !Seeds(Kept).Contains(101));
	FLostLootStore AllNamed;
	AllNamed.Add(MakeGun(Kind, 1, Heirloom), ELostLootReason::LeftLying, Map, 0.0);
	TestEqual(TEXT("Only named guns: the oldest of them"), AllNamed.FindNextToGo(), 0);
	TestEqual(TEXT("Empty: nothing to go"), FLostLootStore().FindNextToGo(), static_cast<int32>(INDEX_NONE));

	// Bigger shelves take more before any goes.
	Store.Upgrades = 1;
	TestTrue(TEXT("Room for forty"), Store.GetCapacity() == 40 && !Store.IsFull());

	// Claimed off the shelves by ticket; emptied by the console, the shelving and the counter kept.
	const int32 At = Store.FindTicket(15);
	FWeaponInstanceData Claimed;
	TestTrue(TEXT("Ticket 15 found and taken"), Store.RemoveAt(At, &Claimed) && Claimed.Seed == 15 && Store.FindTicket(15) == INDEX_NONE);
	TestFalse(TEXT("Nothing at a place past the end"), Store.RemoveAt(99));
	TestEqual(TEXT("Cleared: every gun"), Store.Clear(), 29);
	TestTrue(TEXT("...the shelving and the counter kept"), Store.Num() == 0 && Store.Upgrades == 1 && Store.NextTicket == 32);

	// Guns handed in together go up the longest lain first; an older save's (no life kept) count as the oldest.
	TArray<FLostLootArrival> Arrivals;
	Arrivals.Add({ MakeGun(Kind, 1), 250.f });
	Arrivals.Add({ MakeGun(Kind, 2), 0.f });
	Arrivals.Add({ MakeGun(Kind, 3), 12.f });
	Arrivals.Add({ MakeGun(Kind, 4), 0.f });
	Arrivals.Add({ MakeGun(Kind, 5), -20.f });
	FLostLootStore::SortOldestFirst(Arrivals);
	TArray<int32> Order;
	for (const FLostLootArrival& Arrival : Arrivals)
	{
		Order.Add(Arrival.Weapon.Seed);
	}
	TestTrue(TEXT("Overdue, then the old save's in their order, then the least time left"), Order == TArray<int32>{ 5, 2, 4, 3, 1 });
	return true;
}

#endif
