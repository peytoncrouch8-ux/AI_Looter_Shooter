#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/Inventory/LoadoutStage.h"
#include "UI/Inventory/LoadoutRules.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventorySwapsTest, "Looter.Inventory.Swaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInventorySwapsTest::RunTest(const FString& Parameters)
{
	// The inventory's select-and-swap moves, on a real weapon manager holding real weapons.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun"));
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestNotNull(TEXT("Shotgun loads"), Shotgun))
	{
		return false;
	}

	APawn* Holder = World->SpawnActor<APawn>();
	UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>(Holder);
	Inventory->RegisterComponent();
	Inventory->MaxWeapons = 3;

	auto Give = [Inventory](UWeaponDefinition* Definition, EWeaponRarity Rarity)
	{
		return Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Definition, Rarity, 1)) != nullptr;
	};
	auto SlotIs = [Inventory](int32 Slot, const UWeaponDefinition* Definition, EWeaponRarity Rarity)
	{
		const TArray<AWeaponBase*> Weapons = Inventory->GetWeapons();
		return Weapons.IsValidIndex(Slot) && Weapons[Slot] && Weapons[Slot]->GetInstance().Definition == Definition && Weapons[Slot]->GetRarity() == Rarity;
	};
	auto PackIs = [Inventory](int32 Index, const UWeaponDefinition* Definition, EWeaponRarity Rarity)
	{
		const TArray<FWeaponInstanceData>& Pack = Inventory->GetBackpack();
		return Pack.IsValidIndex(Index) && Pack[Index].Definition == Definition && Pack[Index].Rarity == Rarity;
	};
	auto InHand = [Inventory](EWeaponRarity Rarity)
	{
		const AWeaponBase* Active = Inventory->GetActiveWeapon();
		return Active && Active->GetRarity() == Rarity;
	};

	TestTrue(TEXT("Given three weapons"), Give(Rifle, EWeaponRarity::Common) && Give(Shotgun, EWeaponRarity::Common) && Give(Rifle, EWeaponRarity::Rare));
	TestEqual(TEXT("First weapon in hand"), Inventory->GetActiveSlot(), 0);

	// Slot 3 into the backpack, a new weapon into the freed slot: [Common rifle, Common shotgun, Legendary rifle], pack [Rare rifle].
	TestTrue(TEXT("Slot 3 stashed"), Inventory->StashSlot(2));
	TestTrue(TEXT("Rare rifle in the backpack"), PackIs(0, Rifle, EWeaponRarity::Rare));
	TestTrue(TEXT("Legendary given"), Give(Rifle, EWeaponRarity::Legendary));

	// Reordering slots: the weapon in hand moves with its slot and stays in hand.
	TestTrue(TEXT("Slots 1 and 2 swapped"), Inventory->SwapSlots(0, 1));
	TestTrue(TEXT("Shotgun first"), SlotIs(0, Shotgun, EWeaponRarity::Common));
	TestEqual(TEXT("In-hand slot followed the rifle"), Inventory->GetActiveSlot(), 1);
	TestTrue(TEXT("Still the common rifle in hand"), InHand(EWeaponRarity::Common));

	// An equipped weapon and a backpack item trade places.
	TestTrue(TEXT("Slot 3 swapped with backpack 1"), Inventory->SwapSlotWithBackpack(2, 0));
	TestTrue(TEXT("Rare rifle in slot 3"), SlotIs(2, Rifle, EWeaponRarity::Rare));
	TestTrue(TEXT("Legendary rifle in the backpack"), PackIs(0, Rifle, EWeaponRarity::Legendary));
	TestEqual(TEXT("Still three weapons"), Inventory->GetWeapons().Num(), 3);

	// Swapping out the weapon in hand puts the new one in hand.
	TestTrue(TEXT("Weapon in hand swapped"), Inventory->SwapSlotWithBackpack(1, 0));
	TestTrue(TEXT("Legendary rifle in hand"), InHand(EWeaponRarity::Legendary));
	TestTrue(TEXT("Common rifle in the backpack"), PackIs(0, Rifle, EWeaponRarity::Common));

	// Backpack into a free slot, only when there is one; the weapon in hand doesn't change.
	TestFalse(TEXT("No free slot yet"), Inventory->MoveBackpackToSlot(0));
	TestTrue(TEXT("Slot 1 dropped"), Inventory->DropSlot(0));
	TestTrue(TEXT("Backpack item moved into the free slot"), Inventory->MoveBackpackToSlot(0));
	TestEqual(TEXT("Backpack empty"), Inventory->GetBackpack().Num(), 0);
	TestTrue(TEXT("Common rifle in the last slot"), SlotIs(2, Rifle, EWeaponRarity::Common));
	TestTrue(TEXT("Legendary still in hand"), InHand(EWeaponRarity::Legendary));

	// Moving a weapon past the last one puts it last, the others shift up, and it stays in hand:
	// [Legendary, Rare, Common] -> [Rare, Common, Legendary].
	TestTrue(TEXT("Slot 1 moved to the end"), Inventory->MoveSlot(0, 5));
	TestTrue(TEXT("Rare rifle now first"), SlotIs(0, Rifle, EWeaponRarity::Rare));
	TestTrue(TEXT("Common rifle now second"), SlotIs(1, Rifle, EWeaponRarity::Common));
	TestTrue(TEXT("Legendary now last"), SlotIs(2, Rifle, EWeaponRarity::Legendary));
	TestEqual(TEXT("In-hand slot followed the legendary"), Inventory->GetActiveSlot(), 2);
	TestTrue(TEXT("Legendary still in hand after the move"), InHand(EWeaponRarity::Legendary));
	TestFalse(TEXT("Moving the last weapon to the end changes nothing"), Inventory->MoveSlot(2, 5));

	// Stashing the weapon in hand hands over to one still equipped: stash the legendary (in hand) and the rare.
	TestTrue(TEXT("Two stashed"), Inventory->StashSlot(2) && Inventory->StashSlot(0));
	TestTrue(TEXT("Legendary first, rare second"), PackIs(0, Rifle, EWeaponRarity::Legendary) && PackIs(1, Rifle, EWeaponRarity::Rare));
	TestTrue(TEXT("Common rifle in hand"), InHand(EWeaponRarity::Common));
	TestFalse(TEXT("Out of range moves are refused"), Inventory->SwapSlots(0, 3) || Inventory->SwapSlotWithBackpack(0, 5) || Inventory->MoveBackpackToSlot(4));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryLoadoutTest, "Looter.Inventory.Loadout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInventoryLoadoutTest::RunTest(const FString& Parameters)
{
	// Where the loadout screen shows each slot's gun: the one in hand is held, the others go on the back, then at the hip.
	using C = ELoadoutCarry;
	auto Carries = [](int32 NumWeapons, int32 ActiveSlot)
	{
		TArray<C> Result;
		for (int32 Slot = 0; Slot < 3; ++Slot)
		{
			Result.Add(LoadoutCarry::ForSlot(Slot, NumWeapons, ActiveSlot));
		}
		return Result;
	};
	TestTrue(TEXT("Slot 1 in hand"), Carries(3, 0) == TArray<C>{ C::InHand, C::Back, C::Hip });
	TestTrue(TEXT("Slot 2 in hand"), Carries(3, 1) == TArray<C>{ C::Back, C::InHand, C::Hip });
	TestTrue(TEXT("Slot 3 in hand"), Carries(3, 2) == TArray<C>{ C::Back, C::Hip, C::InHand });
	TestTrue(TEXT("Two guns, the third slot empty"), Carries(2, 0) == TArray<C>{ C::InHand, C::Back, C::None });
	TestTrue(TEXT("Nothing in hand"), Carries(2, INDEX_NONE) == TArray<C>{ C::Back, C::Hip, C::None });

	// The swap list shows the chosen slot's kind of gun first, each group in backpack order.
	UWeaponDefinition* RifleKind = NewObject<UWeaponDefinition>();
	UWeaponDefinition* ShotgunKind = NewObject<UWeaponDefinition>();
	auto Gun = [](UWeaponDefinition* Kind, float Damage, float FireRate)
	{
		FWeaponInstanceData Item;
		Item.Definition = Kind;
		Item.Stats.Damage = Damage;
		Item.Stats.FireRate = FireRate;
		return Item;
	};
	const TArray<FWeaponInstanceData> Backpack = { Gun(RifleKind, 20.f, 600.f), Gun(ShotgunKind, 10.f, 80.f), Gun(RifleKind, 30.f, 600.f), Gun(ShotgunKind, 12.f, 80.f) };
	TArray<int32> Order;
	TestEqual(TEXT("Two rifles in the backpack"), LoadoutRules::SortForSwap(Backpack, RifleKind, Order), 2);
	TestTrue(TEXT("Rifles first"), Order == TArray<int32>{ 0, 2, 1, 3 });
	TestEqual(TEXT("No kind for an empty slot"), LoadoutRules::SortForSwap(Backpack, nullptr, Order), 0);
	TestTrue(TEXT("Then it's backpack order"), Order == TArray<int32>{ 0, 1, 2, 3 });

	// Verdicts compare damage per second, and only between guns of the same kind.
	const FWeaponInstanceData Current = Gun(RifleKind, 25.f, 600.f);
	TestTrue(TEXT("Upgrade"), LoadoutRules::Compare(Backpack[2], &Current) == LoadoutRules::EVerdict::Upgrade);
	TestTrue(TEXT("Weaker"), LoadoutRules::Compare(Backpack[0], &Current) == LoadoutRules::EVerdict::Weaker);
	TestTrue(TEXT("Similar"), LoadoutRules::Compare(Gun(RifleKind, 25.5f, 600.f), &Current) == LoadoutRules::EVerdict::Similar);
	TestTrue(TEXT("Other kinds aren't compared"), LoadoutRules::Compare(Backpack[1], &Current) == LoadoutRules::EVerdict::None);
	TestTrue(TEXT("Nothing to compare with"), LoadoutRules::Compare(Backpack[2], nullptr) == LoadoutRules::EVerdict::None);
	return true;
}

#endif
