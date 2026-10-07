#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Gravemother's return and Side 3 (Docs/Areas/RansomsRest.md, Side 3: "She comes back on an arrival after at least 20
// minutes of play since her last death"): the rule, the session keeping her death with the map's world, her lair (a
// Legendary encounter) away and back, and the mission. Her body and fight are in GravemotherTests.cpp.

#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/GravemotherCreature.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* RestMap = TEXT("/Game/Maps/Lvl_RansomsRest");

	/** A lair like the den's, set up by Setup before its play begins: one Gravemother, Legendary, by the given ids. */
	AEncounterSpawner* SpawnLair(UWorld* World, const FVector& Where, FName SpawnerId, FName LegendaryId, bool bOnApproach)
	{
		return EncounterTestWorld::SpawnSpawner(World, Where, [SpawnerId, LegendaryId, bOnApproach](AEncounterSpawner& Setup)
		{
			Setup.SpawnerId = SpawnerId;
			Setup.LegendaryId = LegendaryId;
			Setup.Groups = { EncounterTestWorld::MakeGroup(AGravemotherCreature::StaticClass(), 1, ECreatureRank::Legendary) };
			Setup.bSpawnOnApproach = bOnApproach;
			Setup.ActivationRadius = 6000.f;
			Setup.GiveUpRadius = 3600.f;
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherReturnTest, "Looter.Creatures.Gravemother.Return",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherReturnTest::RunTest(const FString& Parameters)
{
	// She comes back on an arrival (loading the session, back from Skyreach, a train's arrival) at least 20 minutes of play
	// after her last death, and not before; the session keeps that death with the map's world (FSavedMapWorld::
	// LegendaryDefeatedAt) through the save. Her lair, while she's away, starts the visit cleared and brings nothing as the
	// player comes; the console brings her back; killed, she clears it.
	TestEqual(TEXT("Twenty minutes of play"), USessionSubsystem::LegendaryReturnTime, 1200.0);
	TestTrue(TEXT("Never beaten: there"), USessionSubsystem::IsLegendaryReturnDue(-1.0, 0.0));
	TestFalse(TEXT("19 minutes 59 on: away"), USessionSubsystem::IsLegendaryReturnDue(600.0, 600.0 + 1199.0));
	TestTrue(TEXT("20 minutes on: back"), USessionSubsystem::IsLegendaryReturnDue(600.0, 1800.0));
	TestTrue(TEXT("Beaten \"later\" than now (an odd save): back rather than waiting it out"),
		USessionSubsystem::IsLegendaryReturnDue(500.0, 100.0));

	// A session's arrivals: each one asks whether she's back; a death in play starts her time again.
	struct FMoment
	{
		double PlayedSeconds;
		const TCHAR* What;
		bool bArrival;
		bool bBack;
	};
	const FMoment Moments[] = {
		{ 0.0, TEXT("a new session arrives: there"), true, true },
		{ 300.0, TEXT("killed in play"), false, false },
		{ 600.0, TEXT("Save & Quit, then Continue, 5 minutes on: away"), true, false },
		{ 1499.0, TEXT("back from Skyreach a second short of 20 minutes: away"), true, false },
		{ 1500.0, TEXT("and at 20 minutes: back"), true, true },
		{ 1600.0, TEXT("killed again"), false, false },
		{ 1700.0, TEXT("an arrival at once: away"), true, false },
		{ 2800.0, TEXT("a train's arrival 20 minutes on: back"), true, true } };
	double DefeatedAt = -1.0;
	for (const FMoment& Moment : Moments)
	{
		if (!Moment.bArrival)
		{
			DefeatedAt = Moment.PlayedSeconds;
			continue;
		}
		TestTrue(FString::Printf(TEXT("At %.0f s of play, %s"), Moment.PlayedSeconds, Moment.What),
			USessionSubsystem::IsLegendaryReturnDue(DefeatedAt, Moment.PlayedSeconds) == Moment.bBack);
	}

	// Kept per map with the session, through the save format and back.
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->PlayedSeconds = 1300.0;
	Save->FindOrAddWorld(RestMap).LegendaryDefeatedAt.Add(Gravemother::LairId, 1250.0);
	TArray<uint8> Bytes;
	const ULooterSessionSave* ReadBack = UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	const FSavedMapWorld* Rest = ReadBack ? ReadBack->FindWorld(RestMap) : nullptr;
	if (TestNotNull(TEXT("The session read back, with Ransom's Rest's world"), Rest))
	{
		const double* Kept = Rest->LegendaryDefeatedAt.Find(Gravemother::LairId);
		TestTrue(TEXT("Her death is kept"), Kept && FMath::IsNearlyEqual(*Kept, 1250.0));
		TestFalse(TEXT("...so Continue 50 s of play later finds her away"),
			Kept && USessionSubsystem::IsLegendaryReturnDue(*Kept, ReadBack->PlayedSeconds));
	}

	// Her lair: away this visit, it's cleared and nothing comes as the player comes; the console brings her back.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(2000.0, 0.0, 0.0));
	AEncounterSpawner* Lair = SpawnLair(World, FVector::ZeroVector, TEXT("TestLair"), TEXT("TestMother"), true);
	AEncounterSpawner* Fresh = SpawnLair(World, FVector(0.0, 20000.0, 0.0), TEXT("TestFreshLair"), TEXT("TestOtherMother"), true);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Her lair"), Lair) || !TestNotNull(TEXT("Another lair"), Fresh))
	{
		return false;
	}
	TestFalse(TEXT("No session in a test level: she's there"), Lair->IsLegendaryAway());
	Lair->SendLegendaryAway();
	TestTrue(TEXT("Away: the lair starts the visit cleared"), Lair->IsLegendaryAway() && Lair->GetState() == EEncounterState::Cleared);
	TestTrue(TEXT("...and says so"), Lair->Describe().Contains(TEXT("away")));
	Lair->UpdateEncounter(0.5f);
	TestEqual(TEXT("...and brings nothing as the player comes"), Lair->NumAlive(), 0);
	TestTrue(TEXT("The console brings her back"), Lair->TriggerWave(/*bForce*/ true));
	const TArray<ACreatureBase*> Back = Lair->GetAliveCreatures();
	TestTrue(TEXT("...the Gravemother, Legendary"), Back.Num() == 1 && Back[0]->IsA<AGravemotherCreature>()
		&& Back[0]->GetRank() == ECreatureRank::Legendary && !Lair->IsLegendaryAway());
	EncounterTestWorld::KillAll(*Lair);
	Lair->UpdateEncounter(0.5f);
	TestTrue(TEXT("Killed: her lair is cleared"), Lair->GetState() == EEncounterState::Cleared && Lair->GetKilled() == 1);

	// A lair she's not away from brings her as the player comes, on its ground.
	EncounterTestWorld::SpawnPlayer(World, FVector(2000.0, 20000.0, 0.0));
	Fresh->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Lurking = Fresh->GetAliveCreatures();
	TestTrue(TEXT("Back on an arrival: she's there as the player comes"), Lurking.Num() == 1 && Lurking[0]->IsA<AGravemotherCreature>());
	TestTrue(TEXT("...hunting her lair's ground"), Lurking.Num() == 1 && Lurking[0]->HuntingGround.IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherMissionTest, "Looter.Story.Gravemother.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherMissionTest::RunTest(const FString& Parameters)
{
	// Side 3 as create_side_mission_assets.py makes it: shut until Main 5 is finished, then enter the den (its place,
	// measured with its height: the Sink's rim over it doesn't count), then kill the Gravemother (her lair's encounter
	// cleared: a kill before the step began counts). Experience, and no reward gun: her loot is her own.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	if (!TestNotNull(TEXT("The level has a mission runner"), Runner))
	{
		return false;
	}
	const FName Rest(TEXT("TestRest"));
	const FName MainId(TEXT("TestMain5"));
	const FName SideId(TEXT("TestSide3"));
	const FName DenTag(TEXT("TestDen"));
	const FName LairId(TEXT("TestDenLair"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Main = MissionTestWorld::NewMission(Scratch, TEXT("TestMain5"), EMissionKind::Main, EMissionStart::Manual, Rest);
	MissionTestWorld::AddObjective<UMissionEventObjective>(Main, 0)->Event = TEXT("Test.Never");
	UMissionDefinition* Side = MissionTestWorld::NewMission(Scratch, TEXT("TestSide3"), EMissionKind::Side, EMissionStart::Automatic, Rest);
	Side->Prerequisites = { MainId };
	UMissionReachObjective* Enter = MissionTestWorld::AddObjective<UMissionReachObjective>(Side, 0);
	Enter->Place.Actor.ActorTag = DenTag;
	Enter->Place.Radius = 450.f;
	Enter->Place.bIgnoreHeight = false;
	UMissionClearObjective* Kill = MissionTestWorld::AddObjective<UMissionClearObjective>(Side, 1);
	Kill->SpawnerId = LairId;
	Side->Rewards.ExperienceShare = 0.2f;

	// The den's place 30 m off, her lair at its mouth (she comes only when called, here), the player at the Sink's middle.
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	AActor* Den = MissionTestWorld::SpawnMarker(World, FVector(3000.0, 0.0, 90.0), DenTag);
	AEncounterSpawner* Lair = SpawnLair(World, FVector(2700.0, 0.0, 0.0), LairId, TEXT("TestDenMother"), false);
	if (!TestTrue(TEXT("Player, den and lair placed"), Player && Den && Lair))
	{
		return false;
	}
	Runner->BeginForTesting({ Main, Side }, Campaign, Player, Rest);
	Runner->Update(0.f);
	TestFalse(TEXT("Shut until Main 5 is finished"), Runner->IsRunning(SideId));
	TestTrue(TEXT("Main 5 finished"), Runner->CompleteMission(MainId));
	TestTrue(TEXT("It opens after Main 5, at entering the den"), Runner->IsRunning(SideId) && Runner->GetStep(SideId) == 0);

	// She comes out and is killed on the Sink's floor before the player ever enters the den.
	TestTrue(TEXT("She comes out"), Lair->TriggerWave());
	EncounterTestWorld::KillAll(*Lair);
	Lair->UpdateEncounter(0.5f);
	TestTrue(TEXT("Killed on the floor: her lair is cleared"), Lair->GetState() == EEncounterState::Cleared);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestEqual(TEXT("Still at entering the den"), Runner->GetStep(SideId), 0);

	// Over the den on the rim, 12 m up: not in it.
	Player->SetActorLocation(FVector(3000.0, 0.0, 1290.0));
	Runner->Update(UMissionRunner::UpdateInterval);
	TestEqual(TEXT("On the rim over the den: not entered"), Runner->GetStep(SideId), 0);
	Player->SetActorLocation(FVector(2900.0, 100.0, 90.0));
	Runner->Update(UMissionRunner::UpdateInterval);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestFalse(TEXT("In the den: and the kill before counts, so it's done"), Runner->IsRunning(SideId));
	TestTrue(TEXT("Recorded in the campaign"), Campaign.HasCompleted(SideId));

	// The mission asset, once create_side_mission_assets.py has made it (it waits for Main 5), asks for the same.
	const TCHAR* AssetPath = TEXT("/Game/Data/Missions/DA_Mission_Side3.DA_Mission_Side3");
	const UMissionDefinition* Asset = FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(AssetPath)))
		? LoadObject<UMissionDefinition>(nullptr, AssetPath) : nullptr;
	if (!Asset)
	{
		AddInfo(TEXT("DA_Mission_Side3 isn't made yet (Tools/Unreal/create_side_mission_assets.py, once Main 5 exists): only the ")
			TEXT("rules were checked."));
		return true;
	}
	const UMissionReachObjective* AssetEnter = Cast<UMissionReachObjective>(Asset->GetObjective(0, 0));
	const UMissionClearObjective* AssetKill = Cast<UMissionClearObjective>(Asset->GetObjective(1, 0));
	TestTrue(TEXT("Side 3 is a side mission after Main 5 on Ransom's Rest"), Asset->Kind == EMissionKind::Side
		&& Asset->Prerequisites.Contains(FName(TEXT("Main5"))) && Asset->Area == FName(TEXT("RansomsRest")) && Asset->Steps.Num() == 2);
	TestTrue(TEXT("Its first step: enter the den, measured with its height"), AssetEnter
		&& AssetEnter->Place.Actor.ActorTag == Gravemother::DenPlaceTag && !AssetEnter->Place.bIgnoreHeight);
	TestTrue(TEXT("Its second: her lair cleared"), AssetKill && AssetKill->SpawnerId == Gravemother::LairId);
	TestTrue(TEXT("A side mission's experience, no reward gun"), FMath::IsNearlyEqual(Asset->Rewards.ExperienceShare, 0.2f)
		&& !Asset->Rewards.bGun);
	return true;
}

#endif
