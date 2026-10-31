#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Economy/VendorTrade.h"
#include "Inventory/BackpackOrder.h"
#include "Inventory/WeaponInventorySave.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LostLootRules.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldPickupRules.h"
#include "Shields/ShieldRules.h"
#include "Tests/AutomationCommon.h"
#include "Tests/RecoveryTestWorld.h"
#include "UI/Inventory/LoadoutBackpackRules.h"
#include "Weapons/WeaponDefinition.h"
#include "GameFramework/Character.h"

// Guns and wards share the backpack (the user, 2026-10-31: "they don't get a seperate storage amount than guns, guns and
// wards take up the same backpack space"): the order they went in (FBackpackOrder), the shared count every "is there room"
// asks, the take rules of a ward on the ground, the save's round trip, and an old save's spares coming in after its guns.

namespace
{
	constexpr EAutomationTestFlags WardBackpackTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	FShieldInstanceData BackpackTestWard(UShieldDefinition* Definition, EWeaponRarity Rarity, int32 Seed)
	{
		return ShieldRules::MakeWith(Definition, Rarity, 1, Seed, EShieldBuild::Standard, EShieldTrait::None);
	}

	FWeaponInstanceData BackpackTestGun(UWeaponDefinition* Definition, int32 Seed)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = Definition;
		Gun.Rarity = EWeaponRarity::Common;
		Gun.Level = 1;
		Gun.Seed = Seed;
		return Gun;
	}

	/** "GWG": the backpack's order in kinds, for a readable test. */
	FString Kinds(const TArray<FBackpackItem>& Items)
	{
		FString Out;
		for (const FBackpackItem& Item : Items)
		{
			Out += Item.Kind == EBackpackItem::Gun ? TEXT("G") : TEXT("W");
		}
		return Out;
	}

	/** A player stand-in with a ward component and a weapon manager of Capacity places. */
	bool MakeHolder(UWorld* World, const FVector& Where, int32 Capacity, ACharacter*& OutPlayer, UPlayerShieldComponent*& OutShield, UWeaponManagerComponent*& OutInventory)
	{
		UHealthComponent* Health = nullptr;
		OutPlayer = RecoveryTestWorld::SpawnPlayer(World, Where, Health);
		if (!OutPlayer)
		{
			return false;
		}
		OutShield = UPlayerShieldComponent::FindOrAdd(*OutPlayer);
		OutInventory = NewObject<UWeaponManagerComponent>(OutPlayer);
		OutInventory->RegisterComponent();
		OutInventory->MaxWeapons = 1;
		OutInventory->BackpackCapacity = Capacity;
		return OutShield != nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBackpackOrderTest, "Looter.Inventory.Backpack.Order", WardBackpackTestFlags)

bool FBackpackOrderTest::RunTest(const FString& Parameters)
{
	// The order the backpack's guns and wards went in, as plain rules.
	FBackpackOrder Order;
	Order.Add(EBackpackItem::Gun);
	Order.Add(EBackpackItem::Ward);
	Order.Add(EBackpackItem::Gun);
	Order.Add(EBackpackItem::Ward);
	TestEqual(TEXT("Added in order"), Kinds(Order.Entries()), FString(TEXT("GWGW")));
	TestTrue(TEXT("...each with its index in its own list"), Order.Entries()[2] == FBackpackItem{ EBackpackItem::Gun, 1 } && Order.Entries()[3] == FBackpackItem{ EBackpackItem::Ward, 1 });
	TestEqual(TEXT("The second gun stands third"), Order.PositionOf(EBackpackItem::Gun, 1), 2);

	// A gun swapped out and another put back where it was (EquipFromBackpack with the slots full).
	const int32 Place = Order.RemoveAt(EBackpackItem::Gun, 0);
	TestEqual(TEXT("The first gun came out of the front"), Place, 0);
	Order.Insert(EBackpackItem::Gun, 0, Place);
	TestEqual(TEXT("...and the swapped one went back there"), Kinds(Order.Entries()), FString(TEXT("GWGW")));
	// A place that no longer fits the count is not used: the gun goes in before the one that was its index.
	Order.Insert(EBackpackItem::Gun, 1, 0);
	TestEqual(TEXT("A wrong place: before the gun that was second"), Kinds(Order.Entries()), FString(TEXT("GWGGW")));
	Order.RemoveAt(EBackpackItem::Gun, 1);

	// Out of step (a load, a list changed without a word): brought back, a missing one at the end, an extra from the end.
	TestTrue(TEXT("One more ward than listed: added at the end"), Order.Reconcile(2, 3) && Kinds(Order.Entries()) == TEXT("GWGWW"));
	TestTrue(TEXT("One gun fewer: the last gun goes"), Order.Reconcile(1, 3) && Kinds(Order.Entries()) == TEXT("GWWW"));
	TestFalse(TEXT("In step: nothing changes"), Order.Reconcile(1, 3));
	FBackpackOrder Old;
	Old.Reconcile(2, 2);
	TestEqual(TEXT("An old save (no order): its guns, then its wards"), Kinds(Old.Entries()), FString(TEXT("GGWW")));

	// The loadout's list codes: a ward's never reads as a gun's index, nor as none.
	TestTrue(TEXT("Ward codes stay below INDEX_NONE"), LoadoutBackpack::WardCode(0) < INDEX_NONE && LoadoutBackpack::WardOf(LoadoutBackpack::WardCode(3)) == 3);
	TestEqual(TEXT("A gun's entry is no ward"), LoadoutBackpack::WardOf(2), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("...nor is none"), LoadoutBackpack::WardOf(INDEX_NONE), static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBackpackSharedRoomTest, "Looter.Inventory.Backpack.Shared", WardBackpackTestFlags)

bool FBackpackSharedRoomTest::RunTest(const FString& Parameters)
{
	// One backpack: guns and spare wards take the same places, in the order they went in, and every "is there room" counts
	// both (stowing a gun, taking a ward, a vendor's gun, Lost Property, a ward's tap).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ACharacter* Player = nullptr;
	UPlayerShieldComponent* Shield = nullptr;
	UWeaponManagerComponent* Inventory = nullptr;
	if (!TestTrue(TEXT("A holder"), MakeHolder(WorldWrapper.GetTestWorld(), FVector::ZeroVector, 3, Player, Shield, Inventory)))
	{
		return false;
	}
	UWeaponDefinition* GunKind = NewObject<UWeaponDefinition>();
	UShieldDefinition* WardKind = NewObject<UShieldDefinition>();
	Shield->Equip(BackpackTestWard(WardKind, EWeaponRarity::Common, 1), true);
	TestTrue(TEXT("A ward worn takes no place"), Inventory->GetBackpackUsed() == 0 && Inventory->HasBackpackRoom());
	TestTrue(TEXT("...a ward's tap then puts it in the backpack"), ShieldPickupRules::TapFor(Shield) == ShieldPickupRules::ETap::ToBackpack);

	TestTrue(TEXT("A gun in"), Inventory->AddToBackpack(BackpackTestGun(GunKind, 10)));
	TestTrue(TEXT("A ward in"), Shield->Take(BackpackTestWard(WardKind, EWeaponRarity::Rare, 2)));
	TestTrue(TEXT("Another gun in"), Inventory->AddToBackpack(BackpackTestGun(GunKind, 11)));
	TestEqual(TEXT("Listed in the order they went in"), Kinds(Inventory->GetBackpackItems()), FString(TEXT("GWG")));
	TestEqual(TEXT("Three places taken of three"), Inventory->GetBackpackUsed(), 3);
	TestFalse(TEXT("Full: no room"), Inventory->HasBackpackRoom());
	TestFalse(TEXT("...a gun isn't stowed"), Inventory->AddToBackpack(BackpackTestGun(GunKind, 12)));
	TestFalse(TEXT("...a ward isn't taken in"), Shield->Take(BackpackTestWard(WardKind, EWeaponRarity::Common, 3)) || Shield->StowWard());
	TestTrue(TEXT("...a ward's tap swaps with the worn one"), ShieldPickupRules::TapFor(Shield) == ShieldPickupRules::ETap::Swap);
	TestTrue(TEXT("...a vendor's gun only fits the free slot"), VendorTrade::HasRoomForGun(*Inventory)
		&& LostLootRules::WhereItGoes(*Inventory) == LostLootRules::ETake::InSlot);

	// A gun sold: the ward moves up a place, the room is back.
	FWeaponInstanceData Sold;
	TestTrue(TEXT("The first gun sold"), Inventory->TakeFromBackpack(0, Sold) && Sold.Seed == 10);
	TestEqual(TEXT("...the ward first now"), Kinds(Inventory->GetBackpackItems()), FString(TEXT("WG")));
	TestTrue(TEXT("...room again"), Inventory->HasBackpackRoom() && LostLootRules::WhereItGoes(*Inventory) == LostLootRules::ETake::InBackpack);

	// Wearing the spare: the worn one takes its place in the order.
	TestTrue(TEXT("The spare worn"), Shield->EquipSpare(0) && Shield->GetWard().Seed == 2 && Shield->GetSpares()[0].Seed == 1);
	TestEqual(TEXT("...the old one where the spare was"), Kinds(Inventory->GetBackpackItems()), FString(TEXT("WG")));
	TestTrue(TEXT("The worn one stowed: last in"), Shield->StowWard() && Kinds(Inventory->GetBackpackItems()) == TEXT("WGW"));
	TestTrue(TEXT("A spare dropped: its place goes"), Shield->DropSpare(0) && Kinds(Inventory->GetBackpackItems()) == TEXT("GW"));
	TestEqual(TEXT("...the one left is the stowed one"), Shield->GetSpares().Num() == 1 ? Shield->GetSpares()[0].Seed : -1, 2);

	// None worn: a ward's tap wears it whatever the room.
	TestTrue(TEXT("None worn: a tap wears"), ShieldPickupRules::TapFor(Shield) == ShieldPickupRules::ETap::Wear
		&& ShieldPickupRules::TapFor(false, false) == ShieldPickupRules::ETap::Wear);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBackpackSaveTest, "Looter.Inventory.Backpack.Save", WardBackpackTestFlags)

bool FBackpackSaveTest::RunTest(const FString& Parameters)
{
	// The save keeps the backpack's order; an old save's spares (three, in their own room before 2026-10-31) come in after
	// its guns, all of them, even past the backpack's room, and the log says so.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = nullptr;
	UPlayerShieldComponent* Shield = nullptr;
	UWeaponManagerComponent* Inventory = nullptr;
	ACharacter* Loaded = nullptr;
	UPlayerShieldComponent* LoadedShield = nullptr;
	UWeaponManagerComponent* LoadedInventory = nullptr;
	if (!TestTrue(TEXT("Two holders"), MakeHolder(World, FVector::ZeroVector, 12, Player, Shield, Inventory) && MakeHolder(World, FVector(1000.f, 0.f, 0.f), 12, Loaded, LoadedShield, LoadedInventory)))
	{
		return false;
	}
	UWeaponDefinition* GunKind = NewObject<UWeaponDefinition>();
	UShieldDefinition* WardKind = NewObject<UShieldDefinition>();

	// A mixed backpack, saved and loaded.
	Shield->Equip(BackpackTestWard(WardKind, EWeaponRarity::Common, 1), true);
	Inventory->AddToBackpack(BackpackTestGun(GunKind, 10));
	Shield->Take(BackpackTestWard(WardKind, EWeaponRarity::Epic, 2));
	Inventory->AddToBackpack(BackpackTestGun(GunKind, 11));
	FWeaponInventorySave Save;
	Inventory->SaveInventory(Save);
	TestTrue(TEXT("Saved: the order"), Save.BackpackOrder == TArray<EBackpackItem>({ EBackpackItem::Gun, EBackpackItem::Ward, EBackpackItem::Gun }));
	LoadedInventory->RestoreInventory(Save);
	TestEqual(TEXT("Loaded: the same order"), Kinds(LoadedInventory->GetBackpackItems()), FString(TEXT("GWG")));
	TestTrue(TEXT("...the same things"), LoadedInventory->GetBackpack().Num() == 2 && LoadedShield->GetSpares().Num() == 1
		&& LoadedShield->GetSpares()[0].Seed == 2 && LoadedShield->HasWard() && LoadedShield->GetWard().Seed == 1);

	// An old save: eleven guns, three spares, no order.
	FWeaponInventorySave Old;
	for (int32 Index = 0; Index < 11; ++Index)
	{
		Old.Backpack.Add(BackpackTestGun(GunKind, 100 + Index));
	}
	Old.Shields.Equipped = BackpackTestWard(WardKind, EWeaponRarity::Common, 50);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Old.Shields.Spares.Add(BackpackTestWard(WardKind, EWeaponRarity::Rare, 60 + Index));
	}
	Old.Shields.bStarterGiven = true;
	AddExpectedMessagePlain(TEXT("over the backpack's 12 places"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	LoadedInventory->RestoreInventory(Old);
	TestEqual(TEXT("Every gun back"), LoadedInventory->GetBackpack().Num(), 11);
	TestEqual(TEXT("Every spare back"), LoadedShield->GetSpares().Num(), 3);
	TestEqual(TEXT("The guns first, then the wards"), Kinds(LoadedInventory->GetBackpackItems()), FString(TEXT("GGGGGGGGGGGWWW")));
	TestTrue(TEXT("...the wards in their old order"), LoadedShield->GetSpares()[0].Seed == 60 && LoadedShield->GetSpares()[2].Seed == 62);
	TestEqual(TEXT("Over the room this once: 14 of 12"), LoadedInventory->GetBackpackUsed(), 14);
	TestFalse(TEXT("...nothing more goes in"), LoadedInventory->HasBackpackRoom() || LoadedShield->Take(BackpackTestWard(WardKind, EWeaponRarity::Common, 70)));
	FWeaponInventorySave Again;
	LoadedInventory->SaveInventory(Again);
	TestEqual(TEXT("Saved again with its order"), Again.BackpackOrder.Num(), 14);
	return true;
}

#endif
