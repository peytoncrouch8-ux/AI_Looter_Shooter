#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/SilverPickup.h"
#include "Loot/Chest.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "World/FindRules.h"
#include "World/FindSpot.h"
#include "World/FindSubsystem.h"
#include "World/ReadableProp.h"
#include "World/RoostCrows.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Tests/AutomationCommon.h"

// The finds in a test level (Docs/Work/2026-10-28-Core/FINDS.md §3, §5 lane A): a chest found after its loot is out, not on
// the tap; a readable read; a tagged kill; the XP after the banner; the named gun once; the silver; an old save's finds counted
// silently; the tells (crows over unfound finds only, two glints at most). The rules alone: FindRulesTests.cpp.

namespace
{
	/** A spot as the build script sets it up (before play begins), begun: it registers with the level's finds. */
	AFindSpot* SpawnSpot(UWorld* World, const TCHAR* Id, EFindKind Kind, const FVector& Where, AActor* Source = nullptr,
		TFunctionRef<void(AFindSpot&)> Setup = [](AFindSpot&) {})
	{
		AFindSpot* Spot = World->SpawnActor<AFindSpot>(Where, FRotator::ZeroRotator);
		if (!Spot)
		{
			return nullptr;
		}
		Spot->FindId = FName(Id);
		Spot->DisplayName = FText::FromString(Id);
		Spot->Region = FText::FromString(TEXT("Farm & Point"));
		Spot->Kind = Kind;
		Spot->Source = Source;
		Spot->ZoneLevel = 1;
		Setup(*Spot);
		Spot->DispatchBeginPlay();
		return Spot;
	}

	AChest* SpawnChest(UWorld* World, const FVector& Where, FName Id)
	{
		const FTransform At(FRotator(0.0, 180.0, 0.0), Where);
		AChest* Chest = World->SpawnActorDeferred<AChest>(AChest::StaticClass(), At);
		if (!Chest)
		{
			return nullptr;
		}
		Chest->Kind = EChestKind::Footlocker;
		Chest->ChestId = Id;
		Chest->FinishSpawning(At);
		Chest->DispatchBeginPlay();
		return Chest;
	}

	AReadableProp* SpawnReadable(UWorld* World, const FVector& Where, FName Id, FName SetsFlag = NAME_None)
	{
		AReadableProp* Readable = World->SpawnActor<AReadableProp>(Where, FRotator::ZeroRotator);
		if (!Readable)
		{
			return nullptr;
		}
		Readable->ReadableId = Id;
		Readable->SetsFlag = SetsFlag;
		Readable->DispatchBeginPlay();
		return Readable;
	}

	/** The finds' clock moved on Seconds, a frame at a time (a test level never ticks). */
	void Run(UFindSubsystem* Finds, float Seconds)
	{
		constexpr float Frame = 1.f / 30.f;
		for (float Done = 0.f; Done < Seconds; Done += Frame)
		{
			Finds->Advance(Frame);
		}
	}

	void RunChest(AChest* Chest, float Seconds)
	{
		constexpr float Frame = 1.f / 60.f;
		for (float Done = 0.f; Done < Seconds; Done += Frame)
		{
			Chest->Advance(Frame);
		}
	}

	int32 SilverLying(UWorld* World)
	{
		int32 Silver = 0;
		for (TActorIterator<ASilverPickup> It(World); It; ++It)
		{
			Silver += It->IsActorBeingDestroyed() ? 0 : It->GetAmount();
		}
		return Silver;
	}

	int32 CountNamed(UWorld* World, const UNamedWeaponDefinition* Named)
	{
		int32 Count = 0;
		for (TActorIterator<AWeaponBase> It(World); It; ++It)
		{
			Count += !It->IsActorBeingDestroyed() && It->GetInstance().Named == Named ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFindWorldTest, "Looter.Finds.World",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFindWorldTest::RunTest(const FString& Parameters)
{
	// Outlives the test level, whose runner keeps its flags in it.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UFindSubsystem* Finds = UFindSubsystem::Get(World);
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	if (!Runner || !Finds || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player);

	UNamedWeaponDefinition* Named = UNamedWeaponDefinition::FindByName(TEXT("Heirloom"));
	AChest* Chest = SpawnChest(World, FVector(300.0, 0.0, 0.0), TEXT("Find_TestBox"));
	AReadableProp* Tag = SpawnReadable(World, FVector(0.0, 300.0, 50.0), TEXT("Find_TestTag"));
	AFindSpot* BoxSpot = SpawnSpot(World, TEXT("TestBox"), EFindKind::Cache, FVector(300.0, 0.0, 0.0), Chest,
		[Named](AFindSpot& Spot) { Spot.NamedGun = Named ? FName(TEXT("Heirloom")) : NAME_None; });
	AFindSpot* TagSpot = SpawnSpot(World, TEXT("TestTag"), EFindKind::Hidden, FVector(0.0, 300.0, 0.0), Tag);
	AFindSpot* EliteSpot = SpawnSpot(World, TEXT("TestElite"), EFindKind::Elite, FVector(-300.0, 0.0, 0.0), nullptr,
		[](AFindSpot& Spot) { Spot.SourceTag = TEXT("Find_TestElite"); Spot.Silver = 45; });
	AFindSpot* PageSpot = SpawnSpot(World, TEXT("TestPage"), EFindKind::Lore, FVector(0.0, -300.0, 0.0));
	if (!Chest || !Tag || !BoxSpot || !TagSpot || !EliteSpot || !PageSpot)
	{
		AddError(TEXT("The finds weren't placed."));
		return false;
	}
	FName LastFound;
	int32 LastCount = 0;
	Finds->OnFound.AddLambda([&LastFound, &LastCount](FName Id, int32 Found, int32 Total) { LastFound = Id; LastCount = Found; });
	Finds->BeginForTesting(Player);
	TestTrue(TEXT("Watching after the start"), Finds->IsWatching());
	TestEqual(TEXT("Three finds, the page apart"), Finds->GetTally().Total, 3);
	TestEqual(TEXT("One page"), Finds->GetLoreTally().Total, 1);

	// The chest: the tap opens it, but it counts only once its loot is out (the banner follows the lid).
	TestTrue(TEXT("The chest opens"), Chest->Open(Player, /*bForce*/ true));
	Run(Finds, 0.3f);
	TestFalse(TEXT("Not found on the tap"), Finds->IsFound(TEXT("TestBox")));
	RunChest(Chest, Chest->GetShowSeconds() + 0.5f);
	TestTrue(TEXT("Its loot is out"), Chest->HasGivenLoot());
	Run(Finds, 0.3f);
	TestTrue(TEXT("Found after its loot"), Finds->IsFound(TEXT("TestBox")));
	TestTrue(TEXT("Saved as its flag"), Campaign.HasFlag(TEXT("Find.TestBox")));
	TestEqual(TEXT("Listeners told: 1 of 3"), LastCount, 1);
	TestEqual(TEXT("No XP before the banner is done"), Finds->GetXpPaid(), int64(0));
	TestEqual(TEXT("Its banner waits"), Finds->GetPendingShows(), 1);
	Run(Finds, FindRules::BannerSeconds + 0.2f);
	TestEqual(TEXT("Its XP after the banner"), Finds->GetXpPaid(), FindRules::XpFor(EFindKind::Cache, 1));
	TestEqual(TEXT("Nothing waits"), Finds->GetPendingShows(), 0);
	if (Named)
	{
		TestEqual(TEXT("The named gun, once"), CountNamed(World, Named), 1);
		Finds->Forget(TEXT("TestBox"));
		TestFalse(TEXT("Forgotten"), Finds->IsFound(TEXT("TestBox")));
		Finds->MarkFound(TEXT("TestBox"), /*bAnnounce*/ true);
		TestEqual(TEXT("Never a second"), CountNamed(World, Named), 1);
		Run(Finds, FindRules::BannerSeconds + 0.2f);
	}
	else
	{
		AddInfo(TEXT("DA_Named_Heirloom isn't in the project: the named gun's check skipped."));
	}
	const int64 PaidBefore = Finds->GetXpPaid();

	// The readable: read, it's found; its event is the runner's, heard through it.
	TestTrue(TEXT("The tag is read"), Tag->Read(Player));
	TestTrue(TEXT("Found by reading"), Finds->IsFound(TEXT("TestTag")));
	TestEqual(TEXT("Told"), LastFound, FName(TEXT("TestTag")));

	// The kill: a creature carrying the elite's tag finds it; another doesn't. Its silver lies by the body.
	AActor* Stranger = MissionTestWorld::SpawnMarker(World, FVector(-300.0, 100.0, 0.0), TEXT("SomeoneElse"));
	Finds->NotifyKill(Stranger);
	TestFalse(TEXT("An untagged kill finds nothing"), Finds->IsFound(TEXT("TestElite")));
	const int32 SilverBefore = SilverLying(World);
	AActor* Elite = MissionTestWorld::SpawnMarker(World, FVector(-300.0, 0.0, 0.0), TEXT("Find_TestElite"));
	Finds->NotifyKill(Elite);
	TestTrue(TEXT("The tagged kill finds it"), Finds->IsFound(TEXT("TestElite")));
	TestEqual(TEXT("Its silver"), SilverLying(World) - SilverBefore, FindRules::SilverAt(45, 1));
	TestEqual(TEXT("Its count is every find"), LastCount, 3);

	// The last find brings the completion: its banner after the find's, a thousand on top. The page is no part of it.
	TestEqual(TEXT("Two finds' banners and the completion's wait"), Finds->GetPendingShows(), 3);
	Run(Finds, (FindRules::BannerSeconds + 0.1f) * 3.f);
	const int64 Expected = FindRules::XpFor(EFindKind::Hidden, 1) + FindRules::XpFor(EFindKind::Elite, 1) + FindRules::CompletionXP;
	TestEqual(TEXT("Their XP and the completion's"), Finds->GetXpPaid() - PaidBefore, Expected);
	TestFalse(TEXT("The page is still to find"), Finds->IsFound(TEXT("TestPage")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFindLegacyTest, "Looter.Finds.Legacy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFindLegacyTest::RunTest(const FString& Parameters)
{
	// An old save: a Secret_ chest opened and a secret's readable read (its Secret.* flag set) before finds existed. Both count
	// as play begins, silently: no banner, no XP, no silver. A closed one doesn't.
	FCampaignRecord Campaign;
	Campaign.SetFlag(TEXT("Secret.TestBottle"));
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UFindSubsystem* Finds = UFindSubsystem::Get(World);
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	if (!Runner || !Finds || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player);
	AChest* Opened = SpawnChest(World, FVector(300.0, 0.0, 0.0), TEXT("Secret_TestBox"));
	AChest* Closed = SpawnChest(World, FVector(-300.0, 0.0, 0.0), TEXT("Secret_TestShut"));
	AReadableProp* Bottle = SpawnReadable(World, FVector(0.0, 300.0, 50.0), TEXT("Secret_TestBottle"), TEXT("Secret.TestBottle"));
	if (!Opened || !Closed || !Bottle)
	{
		AddError(TEXT("The secrets weren't placed."));
		return false;
	}
	Opened->RestoreOpened();
	SpawnSpot(World, TEXT("TestBox"), EFindKind::Cache, Opened->GetActorLocation(), Opened, [](AFindSpot& Spot) { Spot.Silver = 45; });
	SpawnSpot(World, TEXT("TestShut"), EFindKind::Cache, Closed->GetActorLocation(), Closed);
	SpawnSpot(World, TEXT("TestBottle"), EFindKind::Cache, Bottle->GetActorLocation(), Bottle);
	int32 Told = 0;
	Finds->OnFound.AddLambda([&Told](FName, int32, int32) { ++Told; });
	const int32 SilverBefore = SilverLying(World);
	Finds->BeginForTesting(Player);
	TestTrue(TEXT("The opened chest counts"), Finds->IsFound(TEXT("TestBox")));
	TestTrue(TEXT("The read bottle counts"), Finds->IsFound(TEXT("TestBottle")));
	TestFalse(TEXT("The closed one doesn't"), Finds->IsFound(TEXT("TestShut")));
	TestEqual(TEXT("Listeners told of both"), Told, 2);
	TestEqual(TEXT("No banner"), Finds->GetPendingShows(), 0);
	Run(Finds, 5.f);
	TestEqual(TEXT("No XP"), Finds->GetXpPaid(), int64(0));
	TestEqual(TEXT("No silver"), SilverLying(World), SilverBefore);
	TestEqual(TEXT("2 / 3"), Finds->GetTally().Found, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFindTellsTest, "Looter.Finds.Tells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFindTellsTest::RunTest(const FString& Parameters)
{
	// Crows circle over unfound finds only (and only where wanted), leave with the find; glints never more than two at once;
	// the tells switched off take everything away.
	FCampaignRecord Campaign;
	Campaign.SetFlag(FindRules::FlagFor(TEXT("Done")));
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UFindSubsystem* Finds = UFindSubsystem::Get(World);
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	if (!Runner || !Finds || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player);
	SpawnSpot(World, TEXT("Open"), EFindKind::Cache, FVector(2000.0, 0.0, 0.0));
	SpawnSpot(World, TEXT("Done"), EFindKind::Cache, FVector(-2000.0, 0.0, 0.0));
	SpawnSpot(World, TEXT("Quiet"), EFindKind::Cache, FVector(0.0, 2000.0, 0.0), nullptr, [](AFindSpot& Spot) { Spot.Crows = 0; });
	// Six more round the player at 20 m: their glints would overlap without the cap.
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const FVector Where = FVector(2000.0, 0.0, 0.0).RotateAngleAxis(30.0 + 50.0 * Index, FVector::UpVector);
		SpawnSpot(World, *FString::Printf(TEXT("Ring%d"), Index), EFindKind::Cache, Where, nullptr, [](AFindSpot& Spot) { Spot.Crows = 0; });
	}
	TestNull(TEXT("No crows before play is under way"), Finds->GetCrows(TEXT("Open")));
	Finds->BeginForTesting(Player);
	ARoostCrows* Crows = Finds->GetCrows(TEXT("Open"));
	TestNotNull(TEXT("Crows over the unfound find"), Crows);
	TestNull(TEXT("None over the found one"), Finds->GetCrows(TEXT("Done")));
	TestNull(TEXT("None where none are wanted"), Finds->GetCrows(TEXT("Quiet")));
	if (Crows)
	{
		TestEqual(TEXT("Its circle hangs CrowHeight over the find"), Crows->GetActorLocation().Z, 700.0, 1.0);
	}

	int32 Most = 0;
	int32 Ever = 0;
	constexpr float Frame = 1.f / 30.f;
	for (float Done = 0.f; Done < 15.f; Done += Frame)
	{
		Finds->Advance(Frame);
		const int32 Now = Finds->GetGlintsShowing();
		Most = FMath::Max(Most, Now);
		Ever += Now > 0 ? 1 : 0;
	}
	TestTrue(TEXT("Glints show"), Ever > 0);
	TestTrue(TEXT("Never more than two at once"), Most <= FindRules::MaxGlints);

	TestTrue(TEXT("The find made"), Finds->MarkFound(TEXT("Open"), /*bAnnounce*/ true));
	TestNull(TEXT("Its crows gone from it"), Finds->GetCrows(TEXT("Open")));
	Finds->SetTellsEnabled(false);
	TestEqual(TEXT("Tells off: no glints"), Finds->GetGlintsShowing(), 0);
	Finds->Forget(TEXT("Open"));
	TestNull(TEXT("Tells off: no crows for a find forgotten"), Finds->GetCrows(TEXT("Open")));
	Finds->SetTellsEnabled(true);
	TestNotNull(TEXT("Tells on: its crows back"), Finds->GetCrows(TEXT("Open")));
	return true;
}

#endif
