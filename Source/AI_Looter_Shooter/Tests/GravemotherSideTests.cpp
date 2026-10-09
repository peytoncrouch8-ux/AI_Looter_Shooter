#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Gravemother's return and Side 3 (Docs/Areas/RansomsRest.md, Side 3: "She comes back on an arrival after at least 20
// minutes of play since her last death"): the rule, the session keeping her death with the map's world, her lair (a
// Legendary encounter) away and back and where it puts her, and the mission. Her body and fight are in
// GravemotherTests.cpp.

#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterGroundProbe.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SpiderCreature.h"
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
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
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

	/** What the level's rocks and buildings carry for the minimap: nothing spawns on top of one. */
	const FName ObstacleTag(TEXT("Obstacle"));

	/** Her lair's step (build_area_den.py's GROUND_STEP): her spots stay on the den's level. */
	constexpr float LairStep = 250.f;

	/**
	 * Rock or ground for a test level, which otherwise has none: blocking world-static boxes (each from its Min to its Max,
	 * world cm) as one actor, tagged Tag.
	 */
	AActor* SpawnRock(UWorld* World, const TArray<FBox>& Boxes, FName Tag)
	{
		AActor* Rock = World->SpawnActor<AActor>();
		if (!Rock)
		{
			return nullptr;
		}
		USceneComponent* Root = NewObject<USceneComponent>(Rock, TEXT("Root"));
		Rock->SetRootComponent(Root);
		Root->RegisterComponent();
		for (const FBox& Each : Boxes)
		{
			UBoxComponent* Box = NewObject<UBoxComponent>(Rock);
			Box->SetBoxExtent(Each.GetExtent());
			Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Box->SetCollisionObjectType(ECC_WorldStatic);
			Box->SetCollisionResponseToAllChannels(ECR_Block);
			Box->SetupAttachment(Root);
			Box->SetRelativeLocation(Each.GetCenter());
			Box->RegisterComponent();
		}
		if (!Tag.IsNone())
		{
			Rock->Tags.Add(Tag);
		}
		return Rock;
	}

	/** A lair like the den's at Where with one point of its own (relative to it, as build_area_den.py sets it): one Gravemother. */
	AEncounterSpawner* SpawnPointLair(UWorld* World, const FVector& Where, FName SpawnerId, const FVector& Point)
	{
		return EncounterTestWorld::SpawnSpawner(World, Where, [SpawnerId, &Point](AEncounterSpawner& Setup)
		{
			Setup.SpawnerId = SpawnerId;
			Setup.Groups = { EncounterTestWorld::MakeGroup(AGravemotherCreature::StaticClass(), 1, ECreatureRank::Legendary) };
			Setup.SpawnPoints = { Point };
			Setup.MaxGroundStep = LairStep;
			Setup.bSpawnOnApproach = false;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherLairRoomTest, "Looter.Creatures.Gravemother.LairRoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherLairRoomTest::RunTest(const FString& Parameters)
{
	// Where her lair puts her (the play-test of 2026-10-08: she came out at the den's mouth with her legs in its jamb, not
	// 3 m in): a spot with room for her own body, not a spider's; her den's floor, though the den is carved in a rock
	// tagged Obstacle; and on that floor once she's begun, under a ceiling lower than her look for the ground starts. A
	// test level with ground: a field at height 0 with a wall along it, and east of it a den rock (tagged Obstacle) with
	// its floor 10 cm up, walls 8 m apart, a back, a roof 4.1 m over the floor (the den's at her spot), and a boulder of
	// the same rock out in the open.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const double WallFaceY = 150.0;
	const FVector DenMouth(10000.0, 30.0, 10.0);
	const FVector DenSpot(10000.0, 330.0, 10.0);
	const AActor* Field = SpawnRock(World, { FBox(FVector(-3000.0, -3000.0, -100.0), FVector(14000.0, 3000.0, 0.0)) }, NAME_None);
	const AActor* Wall = SpawnRock(World, { FBox(FVector(-2000.0, WallFaceY, 0.0), FVector(2000.0, WallFaceY + 250.0, 800.0)) },
		ObstacleTag);
	const AActor* DenRock = SpawnRock(World, {
		FBox(FVector(9600.0, 0.0, -50.0), FVector(10400.0, 900.0, 10.0)),		// its floor
		FBox(FVector(9100.0, 0.0, -50.0), FVector(9600.0, 1300.0, 700.0)),		// its walls
		FBox(FVector(10400.0, 0.0, -50.0), FVector(10900.0, 1300.0, 700.0)),
		FBox(FVector(9600.0, 900.0, -50.0), FVector(10400.0, 1300.0, 700.0)),	// its back
		FBox(FVector(9600.0, -100.0, 420.0), FVector(10400.0, 900.0, 700.0)),	// its roof, a brow a metre past its mouth
		FBox(FVector(8400.0, -900.0, 0.0), FVector(8800.0, -500.0, 120.0)) },	// a boulder of it out in the open
		ObstacleTag);
	if (!TestTrue(TEXT("The field, the wall and the den rock"), Field && Wall && DenRock))
	{
		return false;
	}

	// The ground under her spots, for her body (her lair's largest), before anything stands on it.
	const FEncounterBody HerBody = EncounterRules::LargestBody(
		{ EncounterTestWorld::MakeGroup(AGravemotherCreature::StaticClass(), 1, ECreatureRank::Legendary) }, nullptr);
	FEncounterGroundProbe FromField(World, nullptr, TEXT("TestLairRoom"), HerBody, LairStep);
	const bool bFieldHome = FromField.FindHome(FVector(0.0, 0.0, 60.0), 300.0, 2000.0);
	TestTrue(TEXT("A lair on the field: its ground at 0"), bFieldHome && FMath::IsNearlyZero(FromField.GetHomeZ(), 1.0));
	const FEncounterGroundHit ByWall = FromField.Look(FVector(0.0, 90.0, 0.0));
	TestTrue(*FString::Printf(TEXT("60 cm from the wall: room for a smaller body only (%.2f of hers)"), ByWall.Room),
		ByWall.bFound && ByWall.bStandable && ByWall.Room > 0.f && ByWall.Room < 1.f);
	TestTrue(TEXT("Out in the field: room for the whole of her"), FromField.Look(FVector(0.0, -800.0, 0.0)).Room >= 1.f);
	TestFalse(TEXT("The den's floor is a rock's, not the field's ground"), FromField.Look(DenSpot).bStandable);
	FEncounterGroundProbe FromDen(World, nullptr, TEXT("TestLairRoom"), HerBody, LairStep);
	const bool bDenHome = FromDen.FindHome(DenMouth + FVector(0.0, 0.0, 50.0), 300.0, 2000.0);
	TestTrue(TEXT("A lair at the den's mouth: its ground the den's floor"), bDenHome
		&& FMath::IsNearlyEqual(FromDen.GetHomeZ(), DenMouth.Z, 1.0));
	const FEncounterGroundHit InDen = FromDen.Look(DenSpot);
	TestTrue(TEXT("...3 m in, under its roof: ground, with room for the whole of her"),
		InDen.bFound && InDen.bStandable && InDen.Room >= 1.f);
	const FEncounterGroundHit OnBoulder = FromDen.Look(FVector(8600.0, -700.0, 0.0));
	TestTrue(TEXT("...but not the top of a boulder of the same rock out in the open"), OnBoulder.bFound && !OnBoulder.bStandable);
	// At the real den's lip the Sink's floor and the den's lie within a few centimetres: a lair that finds the field under
	// it, under the den's brow, has the den's floor for ground all the same.
	FEncounterGroundProbe FromLip(World, nullptr, TEXT("TestLairRoom"), HerBody, LairStep);
	const bool bLipHome = FromLip.FindHome(FVector(10000.0, -50.0, 60.0), 300.0, 2000.0);
	TestTrue(TEXT("A lair on the field under the den's brow: the den's floor is its ground too"),
		bLipHome && FMath::IsNearlyZero(FromLip.GetHomeZ(), 1.0) && FromLip.Look(DenSpot).bStandable);

	// By the wall: her lair's own point 60 cm from it, where a spider fits and she doesn't. She comes out where she fits.
	AEncounterSpawner* WallLair = SpawnPointLair(World, FVector(0.0, 0.0, 60.0), TEXT("TestWallLair"), FVector(0.0, 90.0, 0.0));
	if (!TestNotNull(TEXT("Her lair by the wall"), WallLair) || !TestTrue(TEXT("She comes out"), WallLair->TriggerWave()))
	{
		return false;
	}
	const TArray<ACreatureBase*> ByWallOut = WallLair->GetAliveCreatures();
	if (!TestEqual(TEXT("...one Gravemother"), ByWallOut.Num(), 1))
	{
		return false;
	}
	const float HerRadius = ByWallOut[0]->GetCapsuleComponent()->GetScaledCapsuleRadius();
	TestNearlyEqual(TEXT("Her lair measures room for her body as it is in play"), HerBody.Radius, HerRadius, 0.5f);
	const FVector Out = ByWallOut[0]->GetActorLocation();
	TestTrue(*FString::Printf(TEXT("...where it fits: clear of the wall (%.0f cm from it, %.0f wide)"), WallFaceY - Out.Y, HerRadius),
		WallFaceY - Out.Y >= HerRadius - 1.0);

	// In the den: her point 3 m in is on the den rock's floor, and she comes out on it.
	AEncounterSpawner* DenLair = SpawnPointLair(World, DenMouth + FVector(0.0, 0.0, 50.0), TEXT("TestDenLair"), DenSpot - DenMouth);
	if (!TestNotNull(TEXT("Her lair in the den"), DenLair) || !TestTrue(TEXT("She comes out"), DenLair->TriggerWave()))
	{
		return false;
	}
	const TArray<ACreatureBase*> InDenOut = DenLair->GetAliveCreatures();
	if (!TestEqual(TEXT("...one Gravemother"), InDenOut.Num(), 1))
	{
		return false;
	}
	const FVector Lurk = InDenOut[0]->GetActorLocation();
	TestTrue(*FString::Printf(TEXT("At her point 3 m into the den (%.0f cm off it)"), FVector::Dist2D(Lurk, DenSpot)),
		FVector::Dist2D(Lurk, DenSpot) < 5.0);
	TestNearlyEqual(TEXT("...on its floor"), Lurk.Z - InDenOut[0]->GetSimpleCollisionHalfHeight(), DenSpot.Z, 10.0);

	// Her brood comes up round her there too: the den's floor is ground for them as it is for her.
	AGravemotherCreature* Mother = Cast<AGravemotherCreature>(InDenOut[0]);
	if (TestNotNull(TEXT("She's the Gravemother"), Mother) && TestTrue(TEXT("She calls her brood"), Mother->CallBrood() > 0))
	{
		int32 InTheDen = 0;
		for (const ASpiderCreature* Spiderling : Mother->GetBrood())
		{
			const FVector At = Spiderling->GetActorLocation();
			InTheDen += At.X > 9600.0 && At.X < 10400.0 && At.Y > 0.0 && At.Y < 900.0 ? 1 : 0;
		}
		TestTrue(*FString::Printf(TEXT("...some of them in the den (%d)"), InTheDen), InTheDen > 0);
	}
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
	// A hunt nobody asked for: done as she dies, no one to turn it in to.
	TestTrue(TEXT("Its experience (40), no reward gun, finished by itself"), Asset->Rewards.Experience == 40 && !Asset->Rewards.bGun
		&& Asset->TurnIn.bAutomatic);
	return true;
}

#endif
