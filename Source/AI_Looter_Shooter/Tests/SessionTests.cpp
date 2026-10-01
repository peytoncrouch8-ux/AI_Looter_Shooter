#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Weapons/WeaponDefinition.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionSaveTest, "Looter.Session.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSessionSaveTest::RunTest(const FString& Parameters)
{
	// A session round trip through the save format, in memory (the player's real save slots stay untouched): the player,
	// their guns (by their data asset) and the world.
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestNotNull(TEXT("Rifle definition"), Rifle))
	{
		return false;
	}
	FWeaponInstanceData Gun;
	Gun.Definition = Rifle;
	Gun.Rarity = EWeaponRarity::Rare;
	Gun.Level = 7;
	Gun.Seed = 4242;
	Gun.Parts = { TEXT("BodyA"), TEXT("BarrelB") };
	Gun.SavedMagazine = 13;

	const FString Spider = TEXT("/Script/AI_Looter_Shooter.SpiderCreature");
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->Created = FDateTime(2026, 9, 30, 20, 15, 0);
	Save->Saved = FDateTime(2026, 10, 1, 14, 32, 0);
	Save->PlayedSeconds = 3725.0;
	Save->Map = TEXT("/Game/Maps/Lvl_TutorialIsland");
	Save->Progress.Level = 12;
	Save->Progress.XP = 345;
	Save->Progress.bTutorialDone = true;
	Save->Progress.Defeated.Add(Spider, 9);
	Save->Progress.Encountered.Add(Spider);
	Save->bHasPlayerSpot = true;
	Save->PlayerLocation = FVector(-5930.0, -4230.0, 120.0);
	Save->PlayerView = FRotator(-10.f, 35.f, 0.f);
	Save->Health = 64.f;
	Save->bHasInventory = true;
	Save->Inventory.Equipped = { Gun, Gun };
	Save->Inventory.ActiveSlot = 1;
	Save->Inventory.Backpack = { Gun };
	Save->Inventory.Ammo = { 120, 16, 0, 0, 0 };
	Save->bHasWorld = true;
	FSavedLootWeapon& Loot = Save->LootWeapons.AddDefaulted_GetRef();
	Loot.Weapon = Gun;
	Loot.Transform = FTransform(FRotator(0.f, 90.f, 0.f), FVector(100.0, 200.0, 300.0));
	FSavedAmmoPickup& Ammo = Save->AmmoPickups.AddDefaulted_GetRef();
	Ammo.Type = EAmmoType::Shotgun;
	Ammo.Amount = 12;
	Ammo.Location = FVector(1.0, 2.0, 3.0);
	FSavedWeaponRack& Rack = Save->Racks.AddDefaulted_GetRef();
	Rack.Rack = TEXT("WeaponRack_1");
	Rack.bWeaponOffered = false;
	Rack.AmmoPickupsLeft = 1;
	Save->TutorialStep = 3;

	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Saved"), UGameplayStatics::SaveGameToMemory(Save, Bytes)))
	{
		return false;
	}
	const ULooterSessionSave* Loaded = Cast<ULooterSessionSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Loaded"), Loaded))
	{
		return false;
	}
	TestEqual(TEXT("Version"), Loaded->Version, ULooterSessionSave::CurrentVersion);
	TestTrue(TEXT("Saved time"), Loaded->Saved == Save->Saved);
	TestEqual(TEXT("Play time"), Loaded->PlayedSeconds, 3725.0);
	TestEqual(TEXT("Map"), Loaded->Map, Save->Map);
	TestEqual(TEXT("Level"), Loaded->Progress.Level, 12);
	TestEqual(TEXT("XP"), Loaded->Progress.XP, int64(345));
	TestTrue(TEXT("Tutorial done"), Loaded->Progress.bTutorialDone);
	TestEqual(TEXT("Defeats"), Loaded->Progress.Defeated.FindRef(Spider), 9);
	TestTrue(TEXT("Player spot"), Loaded->bHasPlayerSpot && Loaded->PlayerLocation.Equals(Save->PlayerLocation));
	TestTrue(TEXT("Player view"), Loaded->PlayerView.Equals(Save->PlayerView));
	TestEqual(TEXT("Health"), Loaded->Health, 64.f);
	if (TestEqual(TEXT("Equipped guns"), Loaded->Inventory.Equipped.Num(), 2))
	{
		const FWeaponInstanceData& Back = Loaded->Inventory.Equipped[0];
		TestTrue(TEXT("The gun's kind comes back as its data asset"), Back.Definition == Rifle);
		TestTrue(TEXT("Rarity"), Back.Rarity == EWeaponRarity::Rare);
		TestEqual(TEXT("Gun level"), Back.Level, 7);
		TestEqual(TEXT("Seed"), Back.Seed, 4242);
		TestEqual(TEXT("Parts"), Back.Parts.Num(), 2);
		TestEqual(TEXT("Magazine"), Back.SavedMagazine, 13);
	}
	TestEqual(TEXT("In hand"), Loaded->Inventory.ActiveSlot, 1);
	TestEqual(TEXT("Backpack"), Loaded->Inventory.Backpack.Num(), 1);
	TestTrue(TEXT("Ammo"), Loaded->Inventory.Ammo.Num() == 5 && Loaded->Inventory.Ammo[0] == 120 && Loaded->Inventory.Ammo[1] == 16);
	if (TestEqual(TEXT("Loot on the ground"), Loaded->LootWeapons.Num(), 1))
	{
		TestTrue(TEXT("Loot where it lay"), Loaded->LootWeapons[0].Transform.GetLocation().Equals(FVector(100.0, 200.0, 300.0)));
		TestTrue(TEXT("Loot gun"), Loaded->LootWeapons[0].Weapon.Definition == Rifle);
	}
	if (TestEqual(TEXT("Ammo pickups"), Loaded->AmmoPickups.Num(), 1))
	{
		TestTrue(TEXT("Ammo type"), Loaded->AmmoPickups[0].Type == EAmmoType::Shotgun);
		TestEqual(TEXT("Ammo amount"), Loaded->AmmoPickups[0].Amount, 12);
	}
	if (TestEqual(TEXT("Racks"), Loaded->Racks.Num(), 1))
	{
		TestTrue(TEXT("Rack name"), Loaded->Racks[0].Rack == FName(TEXT("WeaponRack_1")));
		TestFalse(TEXT("Rack weapon taken"), Loaded->Racks[0].bWeaponOffered);
		TestEqual(TEXT("Rack ammo left"), Loaded->Racks[0].AmmoPickupsLeft, 1);
	}
	TestEqual(TEXT("Tutorial step"), Loaded->TutorialStep, 3);
	AddInfo(FString::Printf(TEXT("Session save size: %d bytes"), Bytes.Num()));

	// A new game has nothing captured: the level starts as built and the player as the character is given.
	const ULooterSessionSave* Fresh = GetDefault<ULooterSessionSave>();
	TestFalse(TEXT("New game: no world"), Fresh->bHasWorld);
	TestFalse(TEXT("New game: no spot"), Fresh->bHasPlayerSpot);
	TestFalse(TEXT("New game: no inventory"), Fresh->bHasInventory);
	TestEqual(TEXT("New game: level 1"), Fresh->Progress.Level, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionWordsTest, "Looter.Session.Words",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSessionWordsTest::RunTest(const FString& Parameters)
{
	// What the session picker shows: slots, play time, when saved and where.
	TestEqual(TEXT("Slots are numbered from 1"), USessionSubsystem::SlotName(0), FString(TEXT("Session1")));
	TestEqual(TEXT("Third slot"), USessionSubsystem::SlotName(2), FString(TEXT("Session3")));
	TestEqual(TEXT("Three sessions"), USessionSubsystem::MaxSessions, 3);

	TestEqual(TEXT("Seconds"), USessionSubsystem::FormatPlayTime(45.7), FString(TEXT("45 s")));
	TestEqual(TEXT("Minutes"), USessionSubsystem::FormatPlayTime(12 * 60 + 30), FString(TEXT("12 min")));
	TestEqual(TEXT("An hour"), USessionSubsystem::FormatPlayTime(3600), FString(TEXT("1 h 00 min")));
	TestEqual(TEXT("Hours"), USessionSubsystem::FormatPlayTime(3 * 3600 + 5 * 60 + 59), FString(TEXT("3 h 05 min")));
	TestEqual(TEXT("Nothing played"), USessionSubsystem::FormatPlayTime(-5), FString(TEXT("0 s")));

	const FDateTime Now(2026, 10, 1, 18, 0, 0);
	TestEqual(TEXT("Today"), USessionSubsystem::FormatSavedTime(FDateTime(2026, 10, 1, 14, 32, 0), Now), FString(TEXT("Today 14:32")));
	TestEqual(TEXT("Yesterday"), USessionSubsystem::FormatSavedTime(FDateTime(2026, 9, 30, 9, 5, 0), Now), FString(TEXT("Yesterday 09:05")));
	TestEqual(TEXT("Earlier"), USessionSubsystem::FormatSavedTime(FDateTime(2026, 9, 28, 22, 0, 0), Now), FString(TEXT("Sep 28, 2026")));

	TestEqual(TEXT("Place"), USessionSubsystem::PlaceName(TEXT("/Game/Maps/Lvl_TutorialIsland")), FString(TEXT("Tutorial Island")));
	TestEqual(TEXT("Place without a prefix"), USessionSubsystem::PlaceName(TEXT("/Game/Maps/Skyreach")), FString(TEXT("Skyreach")));
	return true;
}

#endif
