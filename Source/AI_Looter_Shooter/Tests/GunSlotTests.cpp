#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/GunSlotRules.h"
#include "Inventory/WeaponInventorySave.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LostLootRules.h"
#include "Missions/MissionDefinition.h"
#include "Session/CampaignRecord.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The equip slots opening with the player's progress (the user, 2026-10-31: "2 slots available at the start of the game, 3rd
// slot available at level 3, and fourth slot available after finishing all of Ransom's Rest's main missions"; Looter.GunSlots.*):
// the rule, the weapon manager keeping guns out of shut slots, old saves, the moment one opens, key 4. The HUD's column, the
// loadout's shut rows and the banner: GunSlotUITests.cpp.

namespace
{
	FGunSlotProgress SlotTestProgress(int32 Level, bool bStoryDone)
	{
		FGunSlotProgress Progress;
		Progress.Level = Level;
		Progress.bStoryDone = bStoryDone;
		return Progress;
	}

	UMissionDefinition* SlotTestMission(FName Id, EMissionKind Kind, FName Area)
	{
		UMissionDefinition* Mission = NewObject<UMissionDefinition>(GetTransientPackage());
		Mission->Id = Id;
		Mission->Kind = Kind;
		Mission->Area = Area;
		return Mission;
	}

	/** A holder with every slot (MaxWeapons 4) whose slots open as a player's at Progress. */
	UWeaponManagerComponent* SlotTestHolder(UWorld* World, const FGunSlotProgress& Progress)
	{
		APawn* Holder = World->SpawnActor<APawn>();
		UWeaponManagerComponent* Inventory = Holder ? NewObject<UWeaponManagerComponent>(Holder) : nullptr;
		if (Inventory)
		{
			Inventory->RegisterComponent();
			Inventory->MaxWeapons = GunSlotRules::NumSlots;
			Inventory->BackpackCapacity = 12;
			Inventory->SetSlotProgressForTesting(Progress);
		}
		return Inventory;
	}

	UWeaponDefinition* SlotTestRifle()
	{
		return LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunSlotRuleTest, "Looter.GunSlots.Rule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunSlotRuleTest::RunTest(const FString& Parameters)
{
	// The user's rule: two from the start, the third at level 3, the fourth with Ransom's Rest's story done.
	TestEqual(TEXT("Four slots in all"), GunSlotRules::NumSlots, 4);
	TestEqual(TEXT("A new game: two"), GunSlotRules::OpenSlots(SlotTestProgress(1, false)), 2);
	TestEqual(TEXT("Level 2: still two"), GunSlotRules::OpenSlots(SlotTestProgress(2, false)), 2);
	TestEqual(TEXT("Level 3: three"), GunSlotRules::OpenSlots(SlotTestProgress(3, false)), 3);
	TestEqual(TEXT("Level 40, story not done: three"), GunSlotRules::OpenSlots(SlotTestProgress(40, false)), 3);
	TestEqual(TEXT("Level 6, story done: four"), GunSlotRules::OpenSlots(SlotTestProgress(6, true)), 4);
	TestTrue(TEXT("At level 1: slots 1-2 open, 3 waits for the level, 4 for the story"),
		GunSlotRules::LockOf(0, SlotTestProgress(1, false)) == EGunSlotLock::Open && GunSlotRules::LockOf(1, SlotTestProgress(1, false)) == EGunSlotLock::Open
		&& GunSlotRules::LockOf(2, SlotTestProgress(1, false)) == EGunSlotLock::Level && GunSlotRules::LockOf(3, SlotTestProgress(1, false)) == EGunSlotLock::Story);
	TestTrue(TEXT("At level 3: only slot 4 shut, by the story"), GunSlotRules::LockOf(2, SlotTestProgress(3, false)) == EGunSlotLock::Open
		&& GunSlotRules::LockOf(3, SlotTestProgress(3, false)) == EGunSlotLock::Story);
	TestTrue(TEXT("No slot past the fourth to shut"), GunSlotRules::LockOf(4, SlotTestProgress(1, false)) == EGunSlotLock::Open);
	TestEqual(TEXT("The shut rows' words"), GunSlotRules::LockText(EGunSlotLock::Level).ToString(), FString(TEXT("Unlocks at level 3")));
	TestEqual(TEXT("...and the story's"), GunSlotRules::LockText(EGunSlotLock::Story).ToString(), FString(TEXT("Unlocks after Ransom's Rest's story")));
	TestEqual(TEXT("A shut slot's key says so"), GunSlotRules::LockedKeyMessage(3, EGunSlotLock::Story).ToString(),
		FString(TEXT("Slot 4 unlocks after Ransom's Rest's story")));

	// The story is every main mission of the area, read from the missions' data: side missions and other areas don't count.
	const FName Area = GunSlotRules::StoryArea();
	TArray<TObjectPtr<UMissionDefinition>> Missions;
	Missions.Add(SlotTestMission(TEXT("SlotTestMainA"), EMissionKind::Main, Area));
	Missions.Add(SlotTestMission(TEXT("SlotTestMainB"), EMissionKind::Main, Area));
	Missions.Add(SlotTestMission(TEXT("SlotTestSide"), EMissionKind::Side, Area));
	Missions.Add(SlotTestMission(TEXT("SlotTestElsewhere"), EMissionKind::Main, TEXT("GildedLily")));
	TestTrue(TEXT("The story missions: the area's two main ones"),
		GunSlotRules::StoryMissions(Missions, Area) == TArray<FName>({ TEXT("SlotTestMainA"), TEXT("SlotTestMainB") }));
	FCampaignRecord Campaign;
	TestFalse(TEXT("None finished: not done"), GunSlotRules::IsStoryDone(Missions, Campaign, Area));
	Campaign.CompletedMissions.Add(TEXT("SlotTestMainA"));
	Campaign.CompletedMissions.Add(TEXT("SlotTestSide"));
	TestFalse(TEXT("One main left: not done"), GunSlotRules::IsStoryDone(Missions, Campaign, Area));
	Campaign.CompletedMissions.Add(TEXT("SlotTestMainB"));
	TestTrue(TEXT("Both main ones: done (the other area's main mission doesn't count)"), GunSlotRules::IsStoryDone(Missions, Campaign, Area));
	TestFalse(TEXT("...not done just before the last one ended"), GunSlotRules::IsStoryDone(Missions, Campaign, Area, TEXT("SlotTestMainB")));
	TestFalse(TEXT("No missions known: never done by accident"), GunSlotRules::IsStoryDone({}, Campaign, Area));

	// The project's missions: Ransom's Rest's main ones are its story (Main 1 to 7, Dead Lift and Back Pay since 2026-10-27).
	TArray<TObjectPtr<UMissionDefinition>> Project;
	for (UMissionDefinition* Mission : UMissionDefinition::LoadAll())
	{
		Project.Add(Mission);
	}
	const TArray<FName> Story = GunSlotRules::StoryMissions(Project, Area);
	if (Story.IsEmpty())
	{
		AddWarning(TEXT("No mission assets here (/Game/Data/Missions): the project's story wasn't checked."));
		return true;
	}
	for (const TCHAR* Expected : { TEXT("Main1"), TEXT("Main2"), TEXT("Main3"), TEXT("Main4"), TEXT("Main5"), TEXT("DeadLift"),
		TEXT("BackPay"), TEXT("Main6"), TEXT("Main7") })
	{
		TestTrue(FString::Printf(TEXT("%s is in Ransom's Rest's story"), Expected), Story.Contains(FName(Expected)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunSlotPickupsTest, "Looter.GunSlots.Pickups",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunSlotPickupsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = SlotTestRifle();
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponManagerComponent* Inventory = SlotTestHolder(World, SlotTestProgress(1, false));
	if (!TestNotNull(TEXT("A holder"), Inventory))
	{
		return false;
	}
	auto Gun = [Rifle](int32 Seed)
	{
		FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1);
		Instance.Seed = Seed;
		return Instance;
	};
	auto Loot = [World, &Gun](int32 Seed)
	{
		AWeaponBase* Dropped = UWeaponRollLibrary::SpawnWeapon(World, Gun(Seed), FTransform(FVector(0.0, 300.0, 50.0)));
		if (Dropped)
		{
			Dropped->Toss(FVector::ZeroVector);
		}
		return Dropped;
	};

	// A new game: two open. Two guns fill them; a third looted goes to the backpack, never into slot 3.
	TestEqual(TEXT("Two slots open"), Inventory->GetUnlockedSlots(), 2);
	TestTrue(TEXT("Two guns go in"), Inventory->GiveWeapon(Gun(1)) && Inventory->GiveWeapon(Gun(2)));
	TestFalse(TEXT("...and the open slots are full"), Inventory->HasFreeSlot());
	AWeaponBase* Third = Loot(3);
	TestTrue(TEXT("A tap on a third: into the backpack"), Third && Inventory->TryPickup(Third));
	TestTrue(TEXT("...two in the slots, one in the backpack"), Inventory->GetWeapons().Num() == 2 && Inventory->GetBackpack().Num() == 1);
	AWeaponBase* Fourth = Loot(4);
	TestTrue(TEXT("A hold on a fourth: in hand in place of the gun in hand"), Fourth && Inventory->EquipPickup(Fourth));
	TestTrue(TEXT("...still two in the slots, the old one in the backpack"), Inventory->GetWeapons().Num() == 2 && Inventory->GetBackpack().Num() == 2
		&& Inventory->GetActiveWeapon() == Fourth);
	TestFalse(TEXT("A backpack gun can't go into a shut slot"), Inventory->MoveBackpackToSlot(0));
	TestTrue(TEXT("Equipping one from the backpack swaps with the gun in hand"), Inventory->EquipFromBackpack(0) && Inventory->GetWeapons().Num() == 2);
	TestTrue(TEXT("Slot 3 waits for the level, slot 4 for the story"),
		Inventory->GetSlotLock(2) == EGunSlotLock::Level && Inventory->GetSlotLock(3) == EGunSlotLock::Story);

	// The wheel goes round the guns carried: two, so 1 -> 2 -> 1.
	Inventory->EquipSlot(0);
	Inventory->NextWeapon();
	TestEqual(TEXT("The wheel: slot 2"), Inventory->GetActiveSlot(), 1);
	Inventory->NextWeapon();
	TestEqual(TEXT("...and round to slot 1, past the shut ones"), Inventory->GetActiveSlot(), 0);

	// Lost Property and the vendor ask the same: with the backpack full, a shut slot is no room.
	Inventory->BackpackCapacity = 0;
	TestTrue(TEXT("Backpack full, open slots full: no room"), LostLootRules::WhereItGoes(*Inventory) == LostLootRules::ETake::NoRoom);

	// Level 3: the third opens and takes a gun; the fourth still waits for the story.
	Inventory->SetSlotProgressForTesting(SlotTestProgress(3, false));
	TestEqual(TEXT("Level 3: three open"), Inventory->GetUnlockedSlots(), 3);
	TestTrue(TEXT("...a free slot: room"), LostLootRules::WhereItGoes(*Inventory) == LostLootRules::ETake::InSlot);
	AWeaponBase* Fifth = Loot(5);
	TestTrue(TEXT("A tap: into slot 3"), Fifth && Inventory->TryPickup(Fifth) && Inventory->GetWeapons().Num() == 3);
	TestTrue(TEXT("...slot 4 still shut"), Inventory->GetSlotLock(3) == EGunSlotLock::Story && !Inventory->HasFreeSlot());

	// The story done: the fourth.
	Inventory->SetSlotProgressForTesting(SlotTestProgress(5, true));
	TestTrue(TEXT("Story done: four open, a fourth gun goes in"), Inventory->GetUnlockedSlots() == 4 && Inventory->GiveWeapon(Gun(6))
		&& Inventory->GetWeapons().Num() == 4);

	// Anyone but the player has every slot.
	Inventory->SetSlotProgressForTesting(TOptional<FGunSlotProgress>());
	TestEqual(TEXT("Not the player's: every slot open"), Inventory->GetUnlockedSlots(), Inventory->MaxWeapons);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunSlotSavesTest, "Looter.GunSlots.Saves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunSlotSavesTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = SlotTestRifle();
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();

	// An old three-slot save, the third gun in hand, loaded below level 3: every gun comes back, in its slot, the third in hand.
	FWeaponInventorySave OldSave;
	for (int32 Seed = 1; Seed <= 3; ++Seed)
	{
		FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1);
		Instance.Seed = Seed;
		OldSave.Equipped.Add(Instance);
	}
	OldSave.ActiveSlot = 2;
	UWeaponManagerComponent* Low = SlotTestHolder(World, SlotTestProgress(1, false));
	if (!TestNotNull(TEXT("A holder"), Low))
	{
		return false;
	}
	Low->RestoreInventory(OldSave);
	TestTrue(TEXT("Below level 3: all three guns kept in their slots (never lost, never swapped out)"),
		Low->GetWeapons().Num() == 3 && Low->GetBackpack().IsEmpty() && Low->GetActiveSlot() == 2);
	TestTrue(TEXT("...slot 3 in use (its gun keeps it), slot 4 shut by the story"),
		Low->GetSlotLock(2) == EGunSlotLock::Open && Low->GetSlotLock(3) == EGunSlotLock::Story);
	TestFalse(TEXT("...no room for another gun"), Low->HasFreeSlot());
	TestTrue(TEXT("Slot 3's gun dropped: the slot shuts until level 3"), Low->DropSlot(2) && Low->GetSlotLock(2) == EGunSlotLock::Level);

	// The same save at level 5: three open, the fourth waiting for the story.
	UWeaponManagerComponent* Mid = SlotTestHolder(World, SlotTestProgress(5, false));
	Mid->RestoreInventory(OldSave);
	TestTrue(TEXT("At level 5: three guns in three open slots"), Mid->GetWeapons().Num() == 3 && Mid->GetUnlockedSlots() == 3);

	// Four slots saved and loaded: all four.
	UWeaponManagerComponent* Full = SlotTestHolder(World, SlotTestProgress(6, true));
	Full->RestoreInventory(OldSave);
	FWeaponInstanceData Fourth = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 1);
	Fourth.Seed = 4;
	TestTrue(TEXT("A fourth gun into the fourth slot"), Full->GiveWeapon(Fourth) != nullptr && Full->GetWeapons().Num() == 4);
	FWeaponInventorySave FourSave;
	Full->SaveInventory(FourSave);
	TestEqual(TEXT("The save keeps four"), FourSave.Equipped.Num(), 4);
	UWeaponManagerComponent* Again = SlotTestHolder(World, SlotTestProgress(6, true));
	Again->RestoreInventory(FourSave);
	TestEqual(TEXT("...and loads four"), Again->GetWeapons().Num(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunSlotUnlockMomentTest, "Looter.GunSlots.Moment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunSlotUnlockMomentTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWeaponManagerComponent* Inventory = SlotTestHolder(WorldWrapper.GetTestWorld(), SlotTestProgress(1, false));
	if (!TestNotNull(TEXT("A holder"), Inventory))
	{
		return false;
	}
	TArray<TPair<int32, EGunSlotLock>> Heard;
	const FDelegateHandle Listening = Inventory->OnSlotUnlocked.AddLambda([&Heard](int32 SlotIndex, EGunSlotLock Was) { Heard.Emplace(SlotIndex, Was); });

	Inventory->AnnounceSlotsOpened(SlotTestProgress(2, false), SlotTestProgress(3, false));
	TestTrue(TEXT("Level 2 to 3: slot 3, opened by the level"), Heard.Num() == 1 && Heard[0].Key == 2 && Heard[0].Value == EGunSlotLock::Level);
	Heard.Reset();
	Inventory->AnnounceSlotsOpened(SlotTestProgress(4, false), SlotTestProgress(5, false));
	TestTrue(TEXT("Level 4 to 5: nothing new"), Heard.IsEmpty());
	Inventory->AnnounceSlotsOpened(SlotTestProgress(5, false), SlotTestProgress(5, true));
	TestTrue(TEXT("The story's last mission: slot 4, opened by the story"), Heard.Num() == 1 && Heard[0].Key == 3 && Heard[0].Value == EGunSlotLock::Story);
	Heard.Reset();
	Inventory->AnnounceSlotsOpened(SlotTestProgress(1, false), SlotTestProgress(9, true));
	TestTrue(TEXT("Both at once: slot 3 then slot 4"), Heard.Num() == 2 && Heard[0].Key == 2 && Heard[1].Key == 3);
	Inventory->OnSlotUnlocked.Remove(Listening);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunSlotKeysTest, "Looter.GunSlots.Keys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunSlotKeysTest::RunTest(const FString& Parameters)
{
	// A key per slot, four of them, the fourth on 4 (rebindable like the others: the settings menu lists every slot key).
	TestEqual(TEXT("A key for every slot"), UKeyBindingSubsystem::NumWeaponSlotKeys, GunSlotRules::NumSlots);
	TestEqual(TEXT("...every slot the manager has"), GetDefault<UWeaponManagerComponent>()->MaxWeapons, GunSlotRules::NumSlots);
	TestEqual(TEXT("...and the HUD shows"), UHudWeaponSlotsWidget::MaxSlots, GunSlotRules::NumSlots);
	TestTrue(TEXT("Slot 4's key is 4"), UKeyBindingSubsystem::DefaultWeaponSlotKey(3) == EKeys::Four);
	TestTrue(TEXT("...its binding WeaponSlot4"), UKeyBindingSubsystem::WeaponSlotBindingId(3) == FName(TEXT("WeaponSlot4")));
	return true;
}

#endif
