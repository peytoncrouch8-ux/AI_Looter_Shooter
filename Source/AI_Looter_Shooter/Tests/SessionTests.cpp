#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Loot/AmmoPickup.h"
#include "Session/SessionSave.h"
#include "Session/SessionSaveGate.h"
#include "Session/SessionSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const TCHAR* TutorialMapPath = TEXT("/Game/Maps/Lvl_TutorialIsland");
	const TCHAR* OldSkyreachMapPath = TEXT("/Game/Maps/Lvl_Skyreach");
	const TCHAR* SpiderKind = TEXT("/Script/AI_Looter_Shooter.SpiderCreature");

	UWeaponDefinition* LoadRifle()
	{
		return LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	}

	FWeaponInstanceData MakeGun(UWeaponDefinition* Definition, int32 Seed)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = Definition;
		Gun.Rarity = EWeaponRarity::Rare;
		Gun.Level = 7;
		Gun.Seed = Seed;
		Gun.Parts = { TEXT("BodyA"), TEXT("BarrelB") };
		Gun.SavedMagazine = 13;
		return Gun;
	}

	/** The player as a session keeps them: level 12, two guns equipped and one in the backpack, ammo, hurt, on the island. */
	void FillPlayer(ULooterSessionSave& Save, UWeaponDefinition* Rifle)
	{
		Save.Created = FDateTime(2026, 9, 30, 20, 15, 0);
		Save.Saved = FDateTime(2026, 10, 1, 14, 32, 0);
		Save.PlayedSeconds = 3725.0;
		Save.Map = TutorialMapPath;
		Save.Progress.Level = 12;
		Save.Progress.XP = 345;
		Save.Progress.bTutorialDone = true;
		Save.Progress.Defeated.Add(SpiderKind, 9);
		Save.Progress.Encountered.Add(SpiderKind);
		Save.bHasPlayerSpot = true;
		Save.PlayerLocation = FVector(-5930.0, -4230.0, 120.0);
		Save.PlayerView = FRotator(-10.f, 35.f, 0.f);
		Save.Health = 64.f;
		Save.bHasInventory = true;
		Save.Inventory.Equipped = { MakeGun(Rifle, 1), MakeGun(Rifle, 2) };
		Save.Inventory.ActiveSlot = 1;
		Save.Inventory.Backpack = { MakeGun(Rifle, 3) };
		Save.Inventory.Ammo = { 120, 16, 0, 0, 0 };
	}

	/** Everything FillPlayer gave the player is still there, the guns by their data asset; with bWithSpot, where they stood. */
	void CheckPlayer(FAutomationTestBase& Test, const ULooterSessionSave& Save, UWeaponDefinition* Rifle, bool bWithSpot)
	{
		Test.TestEqual(TEXT("Level"), Save.Progress.Level, 12);
		Test.TestEqual(TEXT("Experience"), Save.Progress.XP, int64(345));
		Test.TestTrue(TEXT("Tutorial done"), Save.Progress.bTutorialDone);
		Test.TestEqual(TEXT("Defeats"), Save.Progress.Defeated.FindRef(SpiderKind), 9);
		Test.TestTrue(TEXT("Kinds met"), Save.Progress.Encountered.Contains(SpiderKind));
		Test.TestEqual(TEXT("Health"), Save.Health, 64.f);
		Test.TestEqual(TEXT("Time played"), Save.PlayedSeconds, 3725.0);
		Test.TestTrue(TEXT("When it was saved"), Save.Saved == FDateTime(2026, 10, 1, 14, 32, 0));
		if (bWithSpot)
		{
			Test.TestTrue(TEXT("Where the player stood"), Save.bHasPlayerSpot && Save.PlayerLocation.Equals(FVector(-5930.0, -4230.0, 120.0)));
			Test.TestTrue(TEXT("Where they looked"), Save.PlayerView.Equals(FRotator(-10.f, 35.f, 0.f)));
		}
		Test.TestTrue(TEXT("Inventory kept"), Save.bHasInventory);
		Test.TestEqual(TEXT("Guns carried"), Save.CountGuns(), 3);
		Test.TestEqual(TEXT("In hand"), Save.Inventory.ActiveSlot, 1);
		if (Test.TestEqual(TEXT("Equipped guns"), Save.Inventory.Equipped.Num(), 2))
		{
			const FWeaponInstanceData& Gun = Save.Inventory.Equipped[1];
			Test.TestTrue(TEXT("A gun's kind comes back as its data asset"), Gun.Definition == Rifle);
			Test.TestTrue(TEXT("Rarity"), Gun.Rarity == EWeaponRarity::Rare);
			Test.TestEqual(TEXT("Gun level"), Gun.Level, 7);
			Test.TestEqual(TEXT("Seed"), Gun.Seed, 2);
			Test.TestEqual(TEXT("Parts"), Gun.Parts.Num(), 2);
			Test.TestEqual(TEXT("Magazine"), Gun.SavedMagazine, 13);
		}
		Test.TestTrue(TEXT("Backpack"), Save.Inventory.Backpack.Num() == 1 && Save.Inventory.Backpack[0].Seed == 3 && Save.Inventory.Backpack[0].Definition == Rifle);
		Test.TestTrue(TEXT("Ammo"), Save.Inventory.Ammo.Num() == 5 && Save.Inventory.Ammo[0] == 120 && Save.Inventory.Ammo[1] == 16);
	}

	/** Through the save format and back, read as the game reads a session (brought up to date); null when it failed. */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionSaveTest, "Looter.Session.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSessionSaveTest::RunTest(const FString& Parameters)
{
	// A session round trip through the save format, in memory (the player's real save slots stay untouched): the player
	// and their guns (by their data asset), two maps' worlds, the story so far and where a trip arrives.
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestNotNull(TEXT("Rifle definition"), Rifle))
	{
		return false;
	}
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	FillPlayer(*Save, Rifle);
	Save->ArrivalTag = TEXT("Landing_Jetty");

	FSavedMapWorld& Island = Save->FindOrAddWorld(TutorialMapPath);
	FSavedLootWeapon& Loot = Island.LootWeapons.AddDefaulted_GetRef();
	Loot.Weapon = MakeGun(Rifle, 4242);
	Loot.Transform = FTransform(FRotator(0.f, 90.f, 0.f), FVector(100.0, 200.0, 300.0));
	FSavedAmmoPickup& Shells = Island.AmmoPickups.AddDefaulted_GetRef();
	Shells.Type = EAmmoType::Shotgun;
	Shells.Amount = 12;
	Shells.Location = FVector(1.0, 2.0, 3.0);
	FSavedWeaponRack& Rack = Island.Racks.AddDefaulted_GetRef();
	Rack.Rack = TEXT("WeaponRack_1");
	Rack.AmmoPickupsLeft = 1;
	Island.TutorialStep = 3;
	Island.PromotionsRolledAt = 1200.0;
	Island.LegendaryDefeatedAt.Add(FName(TEXT("Gravemother")), 2400.0);
	Save->FindOrAddWorld(OldSkyreachMapPath).AmmoPickups.AddDefaulted_GetRef().Amount = 30;

	FCampaignRecord& Story = Save->Campaign;
	Story.ActiveMission = TEXT("Main1");
	Story.Complete(TEXT("Main1"));
	Story.ActiveMission = TEXT("Main2");
	Story.ActiveMissionStep = 3;
	Story.OpenArea(TEXT("RansomsRest"));
	Story.RecordBossDefeat(TEXT("AbelRansom"));
	Story.bFirstCastOff = true;
	Story.bColdOpenSeen = true;

	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Saved"), UGameplayStatics::SaveGameToMemory(Save, Bytes)))
	{
		return false;
	}
	const ULooterSessionSave* Loaded = USessionSubsystem::ReadSave(Bytes);
	if (!TestNotNull(TEXT("Loaded"), Loaded))
	{
		return false;
	}
	TestEqual(TEXT("Version"), Loaded->Version, ULooterSessionSave::CurrentVersion);
	TestEqual(TEXT("Map"), Loaded->Map, FString(TutorialMapPath));
	CheckPlayer(*this, *Loaded, Rifle, true);
	TestTrue(TEXT("Arrival"), Loaded->ArrivalTag == FName(TEXT("Landing_Jetty")));
	TestEqual(TEXT("Two maps' worlds"), Loaded->Worlds.Num(), 2);
	const FSavedMapWorld* Back = Loaded->FindWorld(TutorialMapPath);
	if (TestNotNull(TEXT("The island's world"), Back))
	{
		if (TestEqual(TEXT("Loot on the ground"), Back->LootWeapons.Num(), 1))
		{
			TestTrue(TEXT("Loot where it lay"), Back->LootWeapons[0].Transform.GetLocation().Equals(FVector(100.0, 200.0, 300.0)));
			TestTrue(TEXT("Loot gun"), Back->LootWeapons[0].Weapon.Definition == Rifle && Back->LootWeapons[0].Weapon.Seed == 4242);
		}
		TestTrue(TEXT("Ammo pickup"), Back->AmmoPickups.Num() == 1 && Back->AmmoPickups[0].Type == EAmmoType::Shotgun && Back->AmmoPickups[0].Amount == 12);
		TestTrue(TEXT("Rack"), Back->Racks.Num() == 1 && Back->Racks[0].Rack == FName(TEXT("WeaponRack_1")) && !Back->Racks[0].bWeaponOffered
			&& Back->Racks[0].AmmoPickupsLeft == 1);
		TestEqual(TEXT("Tutorial step"), Back->TutorialStep, 3);
		TestEqual(TEXT("Promotions rolled"), Back->PromotionsRolledAt, 1200.0);
		TestEqual(TEXT("Legendary beaten"), Back->LegendaryDefeatedAt.FindRef(FName(TEXT("Gravemother"))), 2400.0);
	}
	const FSavedMapWorld* Other = Loaded->FindWorld(OldSkyreachMapPath);
	TestTrue(TEXT("The other map's world"), Other && Other->AmmoPickups.Num() == 1 && Other->AmmoPickups[0].Amount == 30 && Other->TutorialStep == INDEX_NONE);
	const FCampaignRecord& Read = Loaded->Campaign;
	TestTrue(TEXT("Missions done"), Read.HasCompleted(TEXT("Main1")) && Read.CompletedMissions.Num() == 1);
	TestTrue(TEXT("Mission being played"), Read.ActiveMission == FName(TEXT("Main2")) && Read.ActiveMissionStep == 3);
	TestTrue(TEXT("Areas open"), Read.IsAreaOpen(TEXT("RansomsRest")));
	TestTrue(TEXT("Bosses beaten"), Read.DefeatedBosses.Num() == 1 && Read.DefeatedBosses[0] == FName(TEXT("AbelRansom")));
	TestTrue(TEXT("First cast-off and cold open"), Read.bFirstCastOff && Read.bColdOpenSeen);
	AddInfo(FString::Printf(TEXT("Session save size: %d bytes"), Bytes.Num()));

	// The record's rules: a mission finishes once, an area opens once, and only a boss's first defeat is the first.
	FCampaignRecord Record;
	Record.ActiveMission = TEXT("Main3");
	Record.Complete(TEXT("Main3"));
	Record.Complete(TEXT("Main3"));
	TestTrue(TEXT("Finished once, and no longer played"), Record.CompletedMissions.Num() == 1 && Record.ActiveMission.IsNone());
	TestTrue(TEXT("An area opens once"), Record.OpenArea(TEXT("Lily")) && !Record.OpenArea(TEXT("Lily")));
	TestTrue(TEXT("Only the first defeat is the first"), Record.RecordBossDefeat(TEXT("Abel")) && !Record.RecordBossDefeat(TEXT("Abel")));

	// A new game has nothing captured: every level starts as built and the player as the character is given.
	const ULooterSessionSave* Fresh = GetDefault<ULooterSessionSave>();
	TestTrue(TEXT("New game: no worlds"), Fresh->Worlds.IsEmpty());
	TestFalse(TEXT("New game: no spot"), Fresh->bHasPlayerSpot);
	TestFalse(TEXT("New game: no inventory"), Fresh->bHasInventory);
	TestTrue(TEXT("New game: no arrival"), Fresh->ArrivalTag.IsNone());
	TestFalse(TEXT("New game: the story hasn't begun"), Fresh->Campaign.bFirstCastOff);
	TestEqual(TEXT("New game: level 1"), Fresh->Progress.Level, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionUpgradeTest, "Looter.Session.Upgrade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSessionUpgradeTest::RunTest(const FString& Parameters)
{
	// A version 1 session as the first sessions wrote it: one world (the level it was saved in), the player and their
	// guns. Only fields version 1 had are set, so its bytes are what version 1 wrote. Read as the game reads a session,
	// it comes up to version 2 with that world filed under its map, and nothing else moves.
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestNotNull(TEXT("Rifle definition"), Rifle))
	{
		return false;
	}
	ULooterSessionSave* Old = NewObject<ULooterSessionSave>();
	Old->Version = 1;
	FillPlayer(*Old, Rifle);
	Old->bHasWorld = true;
	FSavedLootWeapon& Loot = Old->LootWeapons.AddDefaulted_GetRef();
	Loot.Weapon = MakeGun(Rifle, 99);
	Loot.Transform = FTransform(FVector(100.0, 200.0, 300.0));
	FSavedAmmoPickup& Shells = Old->AmmoPickups.AddDefaulted_GetRef();
	Shells.Type = EAmmoType::Shotgun;
	Shells.Amount = 12;
	FSavedWeaponRack& Rack = Old->Racks.AddDefaulted_GetRef();
	Rack.Rack = TEXT("WeaponRack_0");
	Rack.AmmoPickupsLeft = 1;
	Old->TutorialStep = 3;

	ULooterSessionSave* Upgraded = WriteAndRead(Old);
	if (!TestNotNull(TEXT("Read"), Upgraded))
	{
		return false;
	}
	TestEqual(TEXT("Brought up to date"), Upgraded->Version, ULooterSessionSave::CurrentVersion);
	TestEqual(TEXT("Still on the tutorial island"), Upgraded->Map, FString(TutorialMapPath));
	CheckPlayer(*this, *Upgraded, Rifle, true);
	TestTrue(TEXT("The spot is still good there"), Upgraded->HasPlayerSpotOn(TutorialMapPath));
	TestEqual(TEXT("One world"), Upgraded->Worlds.Num(), 1);
	const FSavedMapWorld* Island = Upgraded->FindWorld(TutorialMapPath);
	if (TestNotNull(TEXT("Filed under the tutorial island"), Island))
	{
		TestTrue(TEXT("Its gun on the ground"), Island->LootWeapons.Num() == 1 && Island->LootWeapons[0].Weapon.Definition == Rifle
			&& Island->LootWeapons[0].Weapon.Seed == 99 && Island->LootWeapons[0].Transform.GetLocation().Equals(FVector(100.0, 200.0, 300.0)));
		TestTrue(TEXT("Its shells on the ground"), Island->AmmoPickups.Num() == 1 && Island->AmmoPickups[0].Type == EAmmoType::Shotgun
			&& Island->AmmoPickups[0].Amount == 12);
		TestTrue(TEXT("Its rack"), Island->Racks.Num() == 1 && Island->Racks[0].Rack == FName(TEXT("WeaponRack_0")) && !Island->Racks[0].bWeaponOffered
			&& Island->Racks[0].AmmoPickupsLeft == 1);
		TestEqual(TEXT("The tutorial's step"), Island->TutorialStep, 3);
		TestTrue(TEXT("No promotions rolled yet"), Island->PromotionsRolledAt < 0.0 && Island->LegendaryDefeatedAt.IsEmpty());
	}
	TestFalse(TEXT("Version 1's world fields emptied"), Upgraded->bHasWorld || Upgraded->LootWeapons.Num() > 0 || Upgraded->AmmoPickups.Num() > 0
		|| Upgraded->Racks.Num() > 0 || Upgraded->TutorialStep != INDEX_NONE);
	TestTrue(TEXT("No trip under way"), Upgraded->ArrivalTag.IsNone() && Upgraded->GetArrivalOn(TutorialMapPath).IsNone());
	TestTrue(TEXT("The story hasn't begun"), !Upgraded->Campaign.bFirstCastOff && Upgraded->Campaign.CompletedMissions.IsEmpty()
		&& Upgraded->Campaign.OpenedAreas.IsEmpty());

	// Saved again it's a version 2 save: read back the same, and not upgraded twice.
	const ULooterSessionSave* Again = WriteAndRead(Upgraded);
	if (TestNotNull(TEXT("Read again"), Again))
	{
		CheckPlayer(*this, *Again, Rifle, true);
		const FSavedMapWorld* Kept = Again->FindWorld(TutorialMapPath);
		TestTrue(TEXT("Read again: the same world"), Again->Worlds.Num() == 1 && Kept && Kept->LootWeapons.Num() == 1 && Kept->AmmoPickups.Num() == 1
			&& Kept->TutorialStep == 3);
	}

	// A version 1 world saved in another level is that level's, not the tutorial island's.
	ULooterSessionSave* Elsewhere = NewObject<ULooterSessionSave>();
	Elsewhere->Version = 1;
	Elsewhere->Map = OldSkyreachMapPath;
	Elsewhere->bHasWorld = true;
	Elsewhere->AmmoPickups.AddDefaulted_GetRef().Amount = 30;
	const ULooterSessionSave* ElsewhereRead = WriteAndRead(Elsewhere);
	const FSavedMapWorld* ElsewhereWorld = ElsewhereRead ? ElsewhereRead->FindWorld(OldSkyreachMapPath) : nullptr;
	TestTrue(TEXT("Saved in another level: filed under it"), ElsewhereWorld && ElsewhereWorld->AmmoPickups.Num() == 1
		&& !ElsewhereRead->FindWorld(TutorialMapPath));

	// With no level named, the world is the tutorial island's, where every version 1 session was played.
	ULooterSessionSave* NoMap = NewObject<ULooterSessionSave>();
	NoMap->Version = 1;
	NoMap->bHasWorld = true;
	NoMap->TutorialStep = 2;
	const ULooterSessionSave* NoMapRead = WriteAndRead(NoMap);
	const FSavedMapWorld* NoMapWorld = NoMapRead ? NoMapRead->FindWorld(ULooterSessionSave::Version1Map) : nullptr;
	TestTrue(TEXT("No level named: filed under the tutorial island"), NoMapWorld && NoMapWorld->TutorialStep == 2);

	// Progress only, nothing captured (the old progress save carried into session 1): kept, with no world to file.
	ULooterSessionSave* ProgressOnly = NewObject<ULooterSessionSave>();
	ProgressOnly->Version = 1;
	ProgressOnly->Map = TutorialMapPath;
	ProgressOnly->Progress.Level = 5;
	const ULooterSessionSave* ProgressRead = WriteAndRead(ProgressOnly);
	TestTrue(TEXT("Progress only: kept, with no world"), ProgressRead && ProgressRead->Worlds.IsEmpty() && ProgressRead->Progress.Level == 5
		&& ProgressRead->Version == ULooterSessionSave::CurrentVersion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionTravelTest, "Looter.Session.Travel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSessionTravelTest::RunTest(const FString& Parameters)
{
	// A trip's bookkeeping, with no level opened (USessionSubsystem::TravelToMap does this, writes the save, then opens
	// the destination): what the session holds as the destination opens, and the saves held back until it has begun.
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestNotNull(TEXT("Rifle definition"), Rifle))
	{
		return false;
	}
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	FillPlayer(*Save, Rifle);
	FSavedMapWorld& Island = Save->FindOrAddWorld(TutorialMapPath);
	Island.TutorialStep = 4;
	Island.LootWeapons.AddDefaulted_GetRef().Weapon = MakeGun(Rifle, 77);
	TestTrue(TEXT("Before: standing on the island"), Save->HasPlayerSpotOn(TutorialMapPath));

	Save->PrepareTrip(OldSkyreachMapPath, TEXT("Landing_Jetty"));
	TestEqual(TEXT("The session continues in the destination"), Save->Map, FString(OldSkyreachMapPath));
	TestFalse(TEXT("The spot stays behind"), Save->bHasPlayerSpot || Save->HasPlayerSpotOn(TutorialMapPath) || Save->HasPlayerSpotOn(OldSkyreachMapPath));
	TestTrue(TEXT("The arrival is written"), Save->ArrivalTag == FName(TEXT("Landing_Jetty")));
	TestTrue(TEXT("The destination arrives at the landing"), Save->GetArrivalOn(OldSkyreachMapPath) == FName(TEXT("Landing_Jetty")));
	TestTrue(TEXT("A level the trip didn't go to starts at its start"), Save->GetArrivalOn(TutorialMapPath).IsNone());
	CheckPlayer(*this, *Save, Rifle, false);
	const FSavedMapWorld* Left = Save->FindWorld(TutorialMapPath);
	TestTrue(TEXT("The world left behind is kept"), Left && Left->TutorialStep == 4 && Left->LootWeapons.Num() == 1 && Left->LootWeapons[0].Weapon.Seed == 77);

	// The destination reads the trip's own save, written before it opened.
	ULooterSessionSave* Arriving = WriteAndRead(Save);
	if (!TestNotNull(TEXT("The trip's save reads"), Arriving))
	{
		return false;
	}
	TestEqual(TEXT("Read: the destination"), Arriving->Map, FString(OldSkyreachMapPath));
	TestTrue(TEXT("Read: arriving at the landing, the island's world kept"), !Arriving->bHasPlayerSpot
		&& Arriving->GetArrivalOn(OldSkyreachMapPath) == FName(TEXT("Landing_Jetty")) && Arriving->FindWorld(TutorialMapPath));
	// Once there and saved, where the player stands wins over the landing.
	Arriving->bHasPlayerSpot = true;
	TestTrue(TEXT("A spot there wins over the landing"), Arriving->GetArrivalOn(OldSkyreachMapPath).IsNone());
	// A trip that names no landing arrives at the level's start.
	Arriving->PrepareTrip(TutorialMapPath, NAME_None);
	TestTrue(TEXT("No landing: the level's start"), Arriving->GetArrivalOn(TutorialMapPath).IsNone() && !Arriving->HasPlayerSpotOn(TutorialMapPath));

	// From the trip's own save until the destination begins nothing saves; holds (a ride, a fade) only make the
	// autosave and save-soons wait.
	FSessionSaveGate Gate;
	auto AllowsAll = [&Gate]() { return Gate.Allows(ESessionSaveReason::Autosave) && Gate.Allows(ESessionSaveReason::Soon)
		&& Gate.Allows(ESessionSaveReason::Asked) && Gate.Allows(ESessionSaveReason::LevelEnd); };
	TestTrue(TEXT("Played: every save goes"), AllowsAll());
	Gate.BeginTrip(OldSkyreachMapPath);
	TestTrue(TEXT("A trip is under way"), Gate.IsTravelling() && Gate.GetTripDestination() == OldSkyreachMapPath);
	TestFalse(TEXT("The level being left doesn't save as it tears down"), Gate.Allows(ESessionSaveReason::LevelEnd));
	TestFalse(TEXT("No autosave, save-soon or save asked for during a trip"), Gate.Allows(ESessionSaveReason::Autosave)
		|| Gate.Allows(ESessionSaveReason::Soon) || Gate.Allows(ESessionSaveReason::Asked));
	Gate.Reset();
	TestTrue(TEXT("The destination began: every save goes"), AllowsAll() && !Gate.IsTravelling());
	Gate.Hold(TEXT("SkiffRide"));
	Gate.Hold(TEXT("Fade"));
	Gate.Hold(TEXT("Fade"));
	TestFalse(TEXT("Held: no autosave or save-soon"), Gate.Allows(ESessionSaveReason::Autosave) || Gate.Allows(ESessionSaveReason::Soon));
	TestTrue(TEXT("Held: a save asked for and the level's end still save"), Gate.Allows(ESessionSaveReason::Asked) && Gate.Allows(ESessionSaveReason::LevelEnd));
	TestFalse(TEXT("Another hold remains"), Gate.Release(TEXT("SkiffRide")));
	TestTrue(TEXT("The last hold"), Gate.Release(TEXT("Fade")));
	TestTrue(TEXT("Released: every save goes"), AllowsAll());
	TestFalse(TEXT("Nothing left to release"), Gate.Release(TEXT("Fade")));
	Gate.Hold(TEXT("Scene"));
	Gate.Reset();
	TestFalse(TEXT("A level change ends every hold"), Gate.IsHeld());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionRoundTripTest, "Looter.Session.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSessionRoundTripTest::RunTest(const FString& Parameters)
{
	// A trip there and back keeps each map's world: what was left lying on one level is there again on the way back, and
	// the other level keeps its own. Two test levels stand in for the tutorial island and Skyreach; the session's real
	// capture and restore run on them, and the save is written and read at every trip.
	UWeaponDefinition* Rifle = LoadRifle();
	FTestWorldWrapper IslandLevel;
	FTestWorldWrapper SkyLevel;
	if (!TestNotNull(TEXT("Rifle definition"), Rifle)
		|| !TestTrue(TEXT("Test levels made"), IslandLevel.CreateTestWorld(EWorldType::EditorPreview) && SkyLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* Island = IslandLevel.GetTestWorld();
	UWorld* Sky = SkyLevel.GetTestWorld();
	const FString IslandMap = USessionSubsystem::MapOf(Island);
	const FString SkyMap = USessionSubsystem::MapOf(Sky);
	TestFalse(TEXT("Two levels"), IslandMap.Equals(SkyMap));

	// Left on the island: a gun and twelve shells. On Skyreach: thirty rifle rounds.
	AWeaponBase* Dropped = UWeaponRollLibrary::SpawnWeapon(Island, MakeGun(Rifle, 1234), FTransform(FVector(100.0, 200.0, 50.0)));
	if (!TestNotNull(TEXT("A gun on the island"), Dropped))
	{
		return false;
	}
	Dropped->Toss(FVector::ZeroVector);
	TestNotNull(TEXT("Shells on the island"), AAmmoPickup::SpawnAmmo(Island, EAmmoType::Shotgun, 12, FVector(300.0, 0.0, 50.0)));
	TestNotNull(TEXT("Rounds on Skyreach"), AAmmoPickup::SpawnAmmo(Sky, EAmmoType::AssaultRifle, 30, FVector(-500.0, 0.0, 50.0)));

	// Played on the island, off to Skyreach, played there, and back.
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(Island, *Save);
	TestEqual(TEXT("Saved on the island"), Save->Map, IslandMap);
	Save->PrepareTrip(SkyMap, TEXT("Landing_Jetty"));
	ULooterSessionSave* OnSky = WriteAndRead(Save);
	if (!TestNotNull(TEXT("Read on Skyreach"), OnSky))
	{
		return false;
	}
	USessionSubsystem::CaptureWorld(Sky, *OnSky);
	OnSky->PrepareTrip(IslandMap, NAME_None);
	const ULooterSessionSave* Back = WriteAndRead(OnSky);
	if (!TestNotNull(TEXT("Read back on the island"), Back))
	{
		return false;
	}
	TestEqual(TEXT("A world for each level"), Back->Worlds.Num(), 2);
	const FSavedMapWorld* IslandWorld = Back->FindWorld(IslandMap);
	const FSavedMapWorld* SkyWorld = Back->FindWorld(SkyMap);
	if (!TestNotNull(TEXT("The island's world"), IslandWorld) || !TestNotNull(TEXT("Skyreach's world"), SkyWorld))
	{
		return false;
	}
	TestTrue(TEXT("The island's gun"), IslandWorld->LootWeapons.Num() == 1 && IslandWorld->LootWeapons[0].Weapon.Seed == 1234
		&& IslandWorld->LootWeapons[0].Transform.GetLocation().Equals(FVector(100.0, 200.0, 50.0), 1.0));
	TestTrue(TEXT("The island's shells"), IslandWorld->AmmoPickups.Num() == 1 && IslandWorld->AmmoPickups[0].Type == EAmmoType::Shotgun
		&& IslandWorld->AmmoPickups[0].Amount == 12);
	TestTrue(TEXT("Skyreach's rounds, and no gun"), SkyWorld->LootWeapons.IsEmpty() && SkyWorld->AmmoPickups.Num() == 1 && SkyWorld->AmmoPickups[0].Amount == 30);

	// Back on the island the level isn't as it was left (the gun gone, shells that were never there): the session puts
	// back exactly what was left lying there, and Skyreach gets its own.
	Dropped->Destroy();
	AAmmoPickup::SpawnAmmo(Island, EAmmoType::Pistol, 5, FVector(0.0, 900.0, 50.0));
	USessionSubsystem::RestoreWorld(Island, *Back);
	USessionSubsystem::RestoreWorld(Sky, *Back);
	auto GunSeeds = [](UWorld* TestLevel)
	{
		TArray<int32> Seeds;
		for (TActorIterator<AWeaponBase> It(TestLevel); It; ++It)
		{
			if (It->IsPickup())
			{
				Seeds.Add(It->GetInstance().Seed);
			}
		}
		return Seeds;
	};
	auto AmmoAmounts = [](UWorld* TestLevel)
	{
		TArray<int32> Amounts;
		for (TActorIterator<AAmmoPickup> It(TestLevel); It; ++It)
		{
			Amounts.Add(It->GetAmount());
		}
		return Amounts;
	};
	TestTrue(TEXT("The island has its gun again"), GunSeeds(Island) == TArray<int32>{ 1234 });
	TestTrue(TEXT("The island has its shells, and only those"), AmmoAmounts(Island) == TArray<int32>{ 12 });
	TestTrue(TEXT("Skyreach has no gun"), GunSeeds(Sky).IsEmpty());
	TestTrue(TEXT("Skyreach has its rounds"), AmmoAmounts(Sky) == TArray<int32>{ 30 });
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

	// A level's file name for people, which the picker shows when no area is played there (AreaTests.cpp has the areas).
	TestEqual(TEXT("Place"), USessionSubsystem::PlaceName(TEXT("/Game/Maps/Lvl_TutorialIsland")), FString(TEXT("Tutorial Island")));
	TestEqual(TEXT("Place without a prefix"), USessionSubsystem::PlaceName(TEXT("/Game/Maps/Skyreach")), FString(TEXT("Skyreach")));
	return true;
}

#endif
