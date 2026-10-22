#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionLastingInteractObjective.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Session/CampaignRecord.h"
#include "Session/CampaignStepMigration.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Templates/Function.h"
#include "UObject/Package.h"

// Old saves mid Main 4 landed a step early after 2026-10-13 (the knock at the vestry door went in before the bell), and the
// side missions that used to start by themselves came back as offers: a version 2 save from before is moved onto today's
// missions as it's read (CampaignStepMigration, save version 3).

namespace
{
	const FName Main3(TEXT("Main3"));
	const FName Main4(TEXT("Main4"));
	const FName Side1(TEXT("Side1"));
	const FName Side2(TEXT("Side2"));
	const FName Side3(TEXT("Side3"));

	/** The evening before the expansion; its morning, after the givers and before the step lists; the day after both. */
	const FDateTime BeforeAll(2026, 10, 12, 20, 0, 0);
	const FDateTime BetweenWaves(2026, 10, 13, 12, 0, 0);
	const FDateTime AfterAll(2026, 10, 14, 1, 0, 0);

	/** A session as the game wrote it on a day: Main 1 to 3 finished, Main 4 on Step; Version as written then. */
	ULooterSessionSave* MakeOld(int32 Version, const FDateTime& Saved, int32 Step)
	{
		ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
		Save->Version = Version;
		Save->Created = Saved - FTimespan::FromHours(3.0);
		Save->Saved = Saved;
		Save->Campaign.bFirstCastOff = true;
		Save->Campaign.CompletedMissions = { TEXT("Main1"), TEXT("Main2"), Main3 };
		Save->Campaign.ActiveMission = Main4;
		Save->Campaign.ActiveMissionStep = Step;
		return Save;
	}

	/** Written and read back as the game reads a session (USessionSubsystem::ReadSave upgrades it). */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}

	/** The mission data asset, when it's made. */
	const UMissionDefinition* LoadMission(const TCHAR* Asset)
	{
		const FString Package = FString(TEXT("/Game/Data/Missions/")) + Asset;
		return FPackageName::DoesPackageExist(Package)
			? LoadObject<UMissionDefinition>(nullptr, *(Package + TEXT(".") + Asset)) : nullptr;
	}

	/** Step Step of Mission has an objective of class T that Rule accepts. */
	template <typename T>
	bool StepHas(const UMissionDefinition* Mission, int32 Step, TFunctionRef<bool(const T&)> Rule)
	{
		if (!Mission || !Mission->Steps.IsValidIndex(Step))
		{
			return false;
		}
		for (const UMissionObjective* Objective : Mission->Steps[Step].Objectives)
		{
			const T* Typed = Cast<T>(Objective);
			if (Typed && Rule(*Typed))
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionStepMigrationRulesTest, "Looter.Session.StepMigration.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSessionStepMigrationRulesTest::RunTest(const FString& Parameters)
{
	// Main 4: the chapel and the yard stay; the bell (old 2) and the Reliquary (old 3) moved up one past the new knock; old
	// 4, waiting for Aldana, is one past today's last (5).
	const int32 Main4Old[] = { 0, 1, 2, 3, 4 };
	const int32 Main4New[] = { 0, 1, 3, 4, 5 };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Main4Old); ++Index)
	{
		TestEqual(FString::Printf(TEXT("Main 4's old step %d"), Main4Old[Index] + 1), CampaignStepMigration::MapOldStep(Main4, Main4Old[Index]),
			Main4New[Index]);
	}
	// Side 2: the talk (its offer now) and the bales land on the bales; the hands stay; waiting stays waiting.
	TestEqual(TEXT("Side 2's old talk: the bales"), CampaignStepMigration::MapOldStep(Side2, 0), 0);
	TestEqual(TEXT("Side 2's old bales: the bales"), CampaignStepMigration::MapOldStep(Side2, 1), 0);
	TestEqual(TEXT("Side 2's old hands: the hands"), CampaignStepMigration::MapOldStep(Side2, 2), 2);
	TestEqual(TEXT("Side 2 waiting: waiting"), CampaignStepMigration::MapOldStep(Side2, 3), 3);
	// Main 3's three steps didn't move (the same since 2026-10-09), nor did anything else.
	for (int32 Step = 0; Step <= 3; ++Step)
	{
		TestEqual(FString::Printf(TEXT("Main 3's step %d stays"), Step + 1), CampaignStepMigration::MapOldStep(Main3, Step), Step);
	}
	TestEqual(TEXT("A mission that didn't change"), CampaignStepMigration::MapOldStep(TEXT("Main5"), 2), 2);
	TestEqual(TEXT("Past the table: moved as its last was"), CampaignStepMigration::MapOldStep(Main4, 7), 8);
	TestTrue(TEXT("The givers came in before the step lists, both on 2026-10-13"),
		CampaignStepMigration::GiversSince() < CampaignStepMigration::StepListsSince()
		&& CampaignStepMigration::GiversSince().GetDay() == 13 && CampaignStepMigration::StepListsSince().GetDay() == 13);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionStepMigrationOldSavesTest, "Looter.Session.StepMigration.OldSaves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSessionStepMigrationOldSavesTest::RunTest(const FString& Parameters)
{
	// A version 2 save from the evening before, on Main 4's bell: read, it's on today's bell (step 4), version 3, and Side 1
	// (running by itself then, after Main 3) is taken; Side 2 waits for Main 4.
	const ULooterSessionSave* OnBell = WriteAndRead(MakeOld(2, BeforeAll, 2));
	if (!TestNotNull(TEXT("Read"), OnBell))
	{
		return false;
	}
	TestEqual(TEXT("Brought up to date"), OnBell->Version, ULooterSessionSave::CurrentVersion);
	TestEqual(TEXT("On the bell still: today's step 4"), OnBell->Campaign.ActiveMissionStep, 3);
	TestTrue(TEXT("Side 1 ran by itself after Main 3: taken"), OnBell->Campaign.IsAccepted(Side1));
	TestFalse(TEXT("Side 2 waits for Main 4"), OnBell->Campaign.IsAccepted(Side2));
	TestFalse(TEXT("...and Side 3 for Main 5"), OnBell->Campaign.IsAccepted(Side3));
	TestEqual(TEXT("Nothing else moves"), OnBell->Campaign.CompletedMissions.Num(), 3);

	// On the Reliquary: today's step 5. Waiting for Aldana: one past today's last, still waiting.
	const ULooterSessionSave* OnReliquary = WriteAndRead(MakeOld(2, BeforeAll, 3));
	TestTrue(TEXT("On the Reliquary still: today's step 5"), OnReliquary && OnReliquary->Campaign.ActiveMissionStep == 4);
	ULooterSessionSave* Waiting = MakeOld(2, BeforeAll, 4);
	Waiting->Campaign.ReadyMissions = { Main4 };
	Waiting->Campaign.OptionalDone = { TEXT("Main4.3.1"), TEXT("Main2.2.1") };
	const ULooterSessionSave* WaitingRead = WriteAndRead(Waiting);
	if (TestNotNull(TEXT("Read the one waiting"), WaitingRead))
	{
		TestTrue(TEXT("Waiting for Aldana: one past today's last, still waiting"), WaitingRead->Campaign.ActiveMissionStep == 5
			&& WaitingRead->Campaign.IsReadyToTurnIn(Main4));
		TestTrue(TEXT("An optional kept by its step moves with it; another mission's stays"),
			WaitingRead->Campaign.OptionalDone.Contains(FName(TEXT("Main4.4.1"))) && WaitingRead->Campaign.OptionalDone.Contains(FName(TEXT("Main2.2.1")))
			&& WaitingRead->Campaign.OptionalDone.Num() == 2);
	}
	// The chapel's yard (step 2) is where it was.
	const ULooterSessionSave* InYard = WriteAndRead(MakeOld(2, BeforeAll, 1));
	TestTrue(TEXT("In the chapel yard: the same step"), InYard && InYard->Campaign.ActiveMissionStep == 1);

	// Main 3's steps didn't move; Main 4 finished, Side 2 (running by itself then) is taken too.
	ULooterSessionSave* OnMain3 = MakeOld(2, BeforeAll, 2);
	OnMain3->Campaign.CompletedMissions = { TEXT("Main1"), TEXT("Main2") };
	OnMain3->Campaign.ActiveMission = Main3;
	const ULooterSessionSave* OnMain3Read = WriteAndRead(OnMain3);
	TestTrue(TEXT("Main 3 on its step 3: the same"), OnMain3Read && OnMain3Read->Campaign.ActiveMission == Main3
		&& OnMain3Read->Campaign.ActiveMissionStep == 2 && !OnMain3Read->Campaign.IsAccepted(Side1));
	ULooterSessionSave* PastMain4 = MakeOld(2, BeforeAll, 0);
	PastMain4->Campaign.CompletedMissions.Add(Main4);
	PastMain4->Campaign.CompletedMissions.Add(Side1);
	PastMain4->Campaign.ActiveMission = TEXT("Main5");
	PastMain4->Campaign.ActiveMissionStep = 1;
	const ULooterSessionSave* PastMain4Read = WriteAndRead(PastMain4);
	if (TestNotNull(TEXT("Read the one past Main 4"), PastMain4Read))
	{
		TestTrue(TEXT("Side 2 ran by itself after Main 4: taken"), PastMain4Read->Campaign.IsAccepted(Side2));
		TestFalse(TEXT("Side 1, finished, isn't taken again"), PastMain4Read->Campaign.IsAccepted(Side1));
		TestEqual(TEXT("Main 5's step stays"), PastMain4Read->Campaign.ActiveMissionStep, 1);
	}

	// Written between the waves (givers in, step lists not yet): the steps move, the side missions were offers already.
	const ULooterSessionSave* Between = WriteAndRead(MakeOld(2, BetweenWaves, 2));
	TestTrue(TEXT("Between the waves: the bell moves to step 4"), Between && Between->Campaign.ActiveMissionStep == 3);
	TestTrue(TEXT("...and Side 1 isn't taken for the player"), Between && !Between->Campaign.IsAccepted(Side1));

	// Written after both: today's lists already; nothing moves (step 4 is the bell there).
	const ULooterSessionSave* Fresh = WriteAndRead(MakeOld(2, AfterAll, 3));
	TestTrue(TEXT("A version 2 save from after: left on its step"), Fresh && Fresh->Campaign.ActiveMissionStep == 3
		&& !Fresh->Campaign.IsAccepted(Side1) && Fresh->Version == ULooterSessionSave::CurrentVersion);

	// A current save is never moved, however early its clock says it was written; read again, nothing moves twice.
	const ULooterSessionSave* Current = WriteAndRead(MakeOld(ULooterSessionSave::CurrentVersion, BeforeAll, 2));
	TestTrue(TEXT("A current save: as written"), Current && Current->Campaign.ActiveMissionStep == 2 && !Current->Campaign.IsAccepted(Side1));
	ULooterSessionSave* Again = OnBell ? DuplicateObject<ULooterSessionSave>(OnBell, GetTransientPackage()) : nullptr;
	const ULooterSessionSave* AgainRead = Again ? WriteAndRead(Again) : nullptr;
	TestTrue(TEXT("Read again: not moved twice"), AgainRead && AgainRead->Campaign.ActiveMissionStep == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionStepMigrationDataTest, "Looter.Session.StepMigration.Data",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSessionStepMigrationDataTest::RunTest(const FString& Parameters)
{
	// The map against the missions' data (Tools/Unreal/create_mission_assets.py, create_side_mission_assets.py): where an
	// old step lands asks what it asked then.
	const UMissionDefinition* MainFour = LoadMission(TEXT("DA_Mission_Main4"));
	if (MainFour)
	{
		TestEqual(TEXT("Main 4: five steps"), MainFour->Steps.Num(), 5);
		TestTrue(TEXT("Today's step 3 is the knock at the vestry door (new)"), StepHas<UMissionTalkObjective>(MainFour, 2,
			[](const UMissionTalkObjective& Talk) { return Talk.SpeakerTag == FName(TEXT("Speaker_Aldana")); }));
		TestTrue(TEXT("Old step 3, the bell, lands on the bell"), StepHas<UMissionInteractObjective>(MainFour,
			CampaignStepMigration::MapOldStep(Main4, 2),
			[](const UMissionInteractObjective& Use) { return Use.Target.ActorTag == FName(TEXT("Bell_Chapel")); }));
		TestTrue(TEXT("Old step 4, the Reliquary, lands on the Reliquary"), StepHas<UMissionEventObjective>(MainFour,
			CampaignStepMigration::MapOldStep(Main4, 3),
			[](const UMissionEventObjective& Seen) { return Seen.Event == FName(TEXT("GraveSight.Reliquary")); }));
		TestEqual(TEXT("Old waiting lands one past the last"), CampaignStepMigration::MapOldStep(Main4, 4), MainFour->Steps.Num());
	}
	else
	{
		AddWarning(TEXT("DA_Mission_Main4 isn't made yet: run Tools/Unreal/create_mission_assets.py."));
	}
	const UMissionDefinition* MainThree = LoadMission(TEXT("DA_Mission_Main3"));
	if (MainThree)
	{
		TestTrue(TEXT("Main 3: its three steps as before (the gate, its fight, Tilly's)"), MainThree->Steps.Num() == 3
			&& StepHas<UMissionClearObjective>(MainThree, 1, [](const UMissionClearObjective& Fight) { return Fight.SpawnerId == FName(TEXT("TownGate")); })
			&& StepHas<UMissionReachObjective>(MainThree, 2,
				[](const UMissionReachObjective& Reach) { return Reach.Place.Actor.ActorTag == FName(TEXT("Speaker_Tilly")); }));
	}
	// Side 2 through both moves: 2026-10-13's (MapOldStep), then 2026-10-21's (MapStepBeforeTalkCut).
	auto SideTwoNow = [](int32 Old) { return CampaignStepMigration::MapStepBeforeTalkCut(Side2, CampaignStepMigration::MapOldStep(Side2, Old)); };
	const UMissionDefinition* SideTwo = LoadMission(TEXT("DA_Mission_Side2"));
	if (SideTwo)
	{
		TestTrue(TEXT("Side 2's old bales land on the bales"), StepHas<UMissionLastingInteractObjective>(SideTwo, SideTwoNow(1),
			[](const UMissionLastingInteractObjective& Load) { return Load.Target.ActorTag == FName(TEXT("HayBale")); }));
		TestTrue(TEXT("...its old hands on the hands"), StepHas<UMissionClearObjective>(SideTwo, SideTwoNow(2),
			[](const UMissionClearObjective& Fight) { return Fight.SpawnerId == FName(TEXT("WhitlockHands")); }));
		TestEqual(TEXT("...and its old waiting one past today's last"), SideTwoNow(3), SideTwo->Steps.Num());
	}
	const UMissionDefinition* Knocking = LoadMission(TEXT("DA_Mission_KnockingBelow"));
	if (Knocking)
	{
		const FName KnockingId(TEXT("KnockingBelow"));
		TestTrue(TEXT("Knocking Below's old slate step lands on the way into the cellar (the slate's optional with it)"),
			StepHas<UMissionReachObjective>(Knocking, CampaignStepMigration::MapStepBeforeTalkCut(KnockingId, 2),
				[](const UMissionReachObjective& Down) { return Down.Place.Actor.ActorTag == FName(TEXT("Place_CellarFloor")); }));
		TestTrue(TEXT("...its old fight on the fight"), StepHas<UMissionClearObjective>(Knocking,
			CampaignStepMigration::MapStepBeforeTalkCut(KnockingId, 3),
			[](const UMissionClearObjective& Fight) { return Fight.SpawnerId == FName(TEXT("TillyCellar")); }));
		TestEqual(TEXT("...and its old waiting one past today's last"), CampaignStepMigration::MapStepBeforeTalkCut(KnockingId, 7),
			Knocking->Steps.Num());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionStepMigrationTalkCutTest, "Looter.Session.StepMigration.TalkCut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSessionStepMigrationTalkCutTest::RunTest(const FString& Parameters)
{
	// A version 3 session read after 2026-10-21's cut of the fetches: the moved missions' optionals move with their steps,
	// Dead Lift's first-step optional goes, the retired crank and coal leave the carried, and it's brought up to date.
	ULooterSessionSave* Old = MakeOld(3, FDateTime(2026, 10, 21, 12, 0, 0), 2);
	Old->Campaign.OptionalDone = { TEXT("Side2.2.1"), TEXT("KnockingBelow.2.1"), TEXT("DeadLift.0.1"), TEXT("Main4.4.1") };
	Old->Campaign.CarriedItems = { TEXT("WinchCrank"), TEXT("CoalSack"), TEXT("ClampPin") };
	const ULooterSessionSave* Read = WriteAndRead(Old);
	if (!TestNotNull(TEXT("Read back"), Read))
	{
		return false;
	}
	const TArray<FName>& Done = Read->Campaign.OptionalDone;
	TestTrue(TEXT("Side 2's wages chalk: with the hands' step now"), Done.Contains(FName(TEXT("Side2.1.1"))) && !Done.Contains(FName(TEXT("Side2.2.1"))));
	TestTrue(TEXT("Knocking Below's slate: with the way into the cellar"), Done.Contains(FName(TEXT("KnockingBelow.1.1"))));
	TestFalse(TEXT("Dead Lift's old report optional: gone"), Done.Contains(FName(TEXT("DeadLift.0.1"))));
	TestTrue(TEXT("...a main mission's: as it was"), Done.Contains(FName(TEXT("Main4.4.1"))) && Done.Num() == 3);
	TestTrue(TEXT("The crank and the coal no longer carried, the pin still"), !Read->Campaign.IsCarrying(TEXT("WinchCrank"))
		&& !Read->Campaign.IsCarrying(TEXT("CoalSack")) && Read->Campaign.IsCarrying(TEXT("ClampPin")));
	TestEqual(TEXT("Main 4's step: as it was (its list didn't move)"), Read->Campaign.ActiveMissionStep, 2);
	TestEqual(TEXT("Brought up to date"), Read->Version, ULooterSessionSave::CurrentVersion);
	TestEqual(TEXT("Side 2's old road talk: the hands"), CampaignStepMigration::MapStepBeforeTalkCut(Side2, 1), 1);
	TestEqual(TEXT("Knocking Below's old workshop door: the key"), CampaignStepMigration::MapStepBeforeTalkCut(TEXT("KnockingBelow"), 0), 0);
	TestEqual(TEXT("...its old tape: Thaddeus's step"), CampaignStepMigration::MapStepBeforeTalkCut(TEXT("KnockingBelow"), 6), 4);
	TestEqual(TEXT("A mission that didn't move"), CampaignStepMigration::MapStepBeforeTalkCut(TEXT("DeadLift"), 3), 3);
	return true;
}

#endif
