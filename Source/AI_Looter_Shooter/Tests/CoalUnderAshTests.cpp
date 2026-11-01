#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/DredgerHoseMan.h"
#include "Creatures/DredgerRifleman.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/RappelSpawner.h"
#include "Creatures/UnpaidCreature.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/GraveSightSubsystem.h"
#include "Story/HobBird.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "Story/WardenCrewMember.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/HallowedGroundTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/ChapelBell.h"
#include "World/ChapelReliquary.h"
#include "World/ChapelWindowHose.h"
#include "World/CompanyHoseCart.h"
#include "World/CompanyLoudspeaker.h"
#include "World/CompanyTenderDrop.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Main 4, "A Coal Under Ash" (the Wardens story since 2026-11-01, was "Hallowed Ground"; Docs/Work/2026-10-31-Wardens/
// MAINS_2_7.md section 6, lanes/MAIN45.md): up the north road to the Chapel of Saint Ada; her yard cleared of the dead the
// Company's crew spilled and its hose pulled out of her side window; the knock at the vestry door; the bell; her ember
// flaring under the ash while the barge's answer comes down on ropes; Father Aldana's words at the vestry door; the Unpaid
// on boot hill and the north road from then on. The bell's and the Reliquary's own rules, and Aldana's topics, are
// ChapelTests.cpp's; the hose's, the tender's and the ropes' are Main45WorldTests.cpp's. (This file was HallowedGroundTests.cpp.)

using namespace HallowedGroundTestWorld;

namespace
{
	const FName BootHillId(TEXT("BootHill"));
	const FName NorthRoadId(TEXT("NorthRoad"));
	const FName CartTag(TEXT("HoseCart_ChapelYard"));
	const FName TenderTag(TEXT("Tender_Chapel"));
	const FName LychgateTag(TEXT("Loudspeaker_Lychgate"));

	FString OnScreen(const UCaptionSubsystem& Captions)
	{
		const FCaptionEntry* Current = Captions.GetCurrent();
		return Current ? Current->Line.Speaker.ToString() + TEXT("|") + Current->Line.Text.ToString() : FString(TEXT("none"));
	}

	/** A roaming stretch as the build script sets it up: Unpaid of the area's ranks after Main 4, standing about. */
	void SetUpRoaming(AEncounterSpawner& Setup, FName Id)
	{
		Setup.SpawnerId = Id;
		FEncounterGroup Roamers;
		Roamers.CreatureClass = AUnpaidCreature::StaticClass();
		Roamers.Count = 3;
		Roamers.RankRoll = EEncounterRankRoll::Area;
		Setup.Groups = { Roamers };
		Setup.CreatureTags = { FName(*(TEXT("Unpaid_") + Id.ToString())) };
		Setup.ActiveWhen.AfterMissions = { MainFour };
		Setup.bHuntOnSpawn = false;
		Setup.SpawnRadius = 600.f;
		Setup.GiveUpRadius = 2500.f;
	}

	/** The barge's answer as a plain stand-in for the flow (the ropes are Main45WorldTests.cpp's): six, during Main 4's step 4. */
	void SetUpAnswer(AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = AnswerId;
		Setup.Groups = { EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 6) };
		Setup.ActiveWhen.DuringMission = MainFour;
		Setup.ActiveWhen.FromStep = ReliquaryStep;
		Setup.ActiveWhen.BeforeStep = ReliquaryStep + 1;
		Setup.bHuntOnSpawn = true;
		Setup.ActivationRadius = 6000.f;
		Setup.SpawnRadius = 500.f;
	}

	/** How many creatures of Class at Rank a spawner's waves bring in all: each group's count, for every wave it joins. */
	int32 CountBrought(const AEncounterSpawner& Spawner, const UClass* Class, ECreatureRank Rank)
	{
		const int32 Waves = FMath::Max(Spawner.NumWaves, 1);
		int32 Brought = 0;
		for (const FEncounterGroup& Group : Spawner.Groups)
		{
			if (!Group.CreatureClass || !Group.CreatureClass->IsChildOf(Class) || Group.RankRoll != EEncounterRankRoll::Fixed
				|| Group.Rank != Rank)
			{
				continue;
			}
			for (int32 Wave = 1; Wave <= Waves; ++Wave)
			{
				Brought += EncounterRules::JoinsWave(Group, Wave) ? Group.Count : 0;
			}
		}
		return Brought;
	}

	const ULevel* LoadRansomsRest(FAutomationTestBase& Test)
	{
		if (!FPackageName::DoesPackageExist(TEXT("/Game/Maps/Lvl_RansomsRest")))
		{
			Test.AddInfo(TEXT("Lvl_RansomsRest isn't built: skipped."));
			return nullptr;
		}
		const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"));
		return Test.TestNotNull(TEXT("Lvl_RansomsRest loads"), Map) ? Map->PersistentLevel.Get() : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoalUnderAshMissionTest, "Looter.Story.CoalUnderAsh.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoalUnderAshMissionTest::RunTest(const FString& Parameters)
{
	// Main 4 as its asset has it: id exactly Main4 (Side 2 opens after it), offered by Tilly after Main 3; up to the chapel,
	// the yard (16) and the window's hose (held), the knock, the bell (held), the Reliquary's flash and the barge's answer (6),
	// the keepers' roll optional; turned in to Aldana with the doc's words; 1,000 experience (1,300 until 2026-11-01: trimmed
	// for the Pacing bands, Docs/Progression.md) and a Saint Maud gun.
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main4")))
	{
		const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main4.DA_Mission_Main4"));
		if (TestNotNull(TEXT("DA_Mission_Main4 loads"), Asset))
		{
			TestTrue(TEXT("Main4 \"A Coal Under Ash\", a main mission Tilly offers on Ransom's Rest, after Main3"), Asset->GetMissionId() == MainFour
				&& Asset->Title.ToString() == TEXT("A Coal Under Ash") && Asset->Kind == EMissionKind::Main
				&& Asset->Start == EMissionStart::Offered && Asset->Offer.GiverTag == FName(TEXT("Speaker_Tilly"))
				&& Asset->Area == FName(TEXT("RansomsRest")) && Asset->Prerequisites == TArray<FName>({ MainThree }));
			TestTrue(TEXT("...its card has words"), !Asset->Offer.Briefing.IsEmpty());
			TestEqual(TEXT("Five steps"), Asset->Steps.Num(), MainFourSteps);
			const UMissionReachObjective* Up = Cast<UMissionReachObjective>(Asset->GetObjective(0, 0));
			const UMissionClearObjective* Yard = Cast<UMissionClearObjective>(Asset->GetObjective(1, 0));
			const UMissionInteractObjective* Hose = Cast<UMissionInteractObjective>(Asset->GetObjective(1, 1));
			const UMissionTalkObjective* Knock = Cast<UMissionTalkObjective>(Asset->GetObjective(2, 0));
			const UMissionInteractObjective* Bell = Cast<UMissionInteractObjective>(Asset->GetObjective(BellStep, 0));
			const UMissionEventObjective* Sight = Cast<UMissionEventObjective>(Asset->GetObjective(ReliquaryStep, 0));
			const UMissionClearObjective* Answer = Cast<UMissionClearObjective>(Asset->GetObjective(ReliquaryStep, 1));
			const UMissionObjective* Roll = Asset->GetObjective(ReliquaryStep, 2);
			TestTrue(TEXT("1: up to the Chapel of Saint Ada"), Up && Up->Place.Actor.ActorTag == ChapelPlace && Up->Place.Radius >= 1000.f);
			TestTrue(TEXT("2: the chapel yard cleared, all sixteen"), Yard && Yard->SpawnerId == YardId && Yard->Count == 16);
			TestTrue(TEXT("...and the hose out of her window, held"), Hose && Hose->Target.ActorTag == HoseTag && Hose->bHold && !Hose->bOptional);
			TestTrue(TEXT("3: the knock at the vestry door"), Knock && Knock->SpeakerTag == AldanaTag);
			TestTrue(TEXT("4: the chapel bell rung, held"), Bell && Bell->Target.ActorTag == AChapelBell::BellTag && Bell->bHold);
			TestTrue(TEXT("5: the Reliquary seen: done at its flash's end, the arrow on it"), Sight && Sight->Event == AChapelReliquary::SightEvent
				&& Sight->Waypoint == EMissionWaypoint::Actor && Sight->WaypointActor.ActorTag == AChapelReliquary::ReliquaryTag);
			TestTrue(TEXT("...the barge's answer held off: six on ropes"), Answer && Answer->SpawnerId == AnswerId && Answer->Count == 6);
			TestTrue(TEXT("...and, optional, the keepers' roll"), Roll && Roll->bOptional);
			TestTrue(TEXT("Turned in to Father Aldana at the vestry door, with the doc's words"), Asset->NeedsTurnIn()
				&& Asset->TurnIn.SpeakerTag == AldanaTag && HoldsAldanasWords(Asset->TurnIn.Lines, /*bNamed*/ false));
			TestEqual(TEXT("Its reward: 1,000 experience"), Asset->Rewards.Experience, 1000);
			TestTrue(TEXT("...and a gun"), Asset->Rewards.bGun);
		}
	}
	else
	{
		AddWarning(TEXT("DA_Mission_Main4 isn't made yet: run Tools/Unreal/create_mission_assets.py. The flow below runs on a copy."));
	}

	// Played through in a test level: Main 4 after Main 3; the yard's fight at the chapel; the hose freed by its cart's stop;
	// the knock; the bell; the flash and the answer; Aldana's words.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	UGraveSightSubsystem* Sight = UGraveSightSubsystem::Get(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-4000.0, 0.0, 0.0));
	AActor* Chapel = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector, ChapelPlace);
	AEncounterSpawner* Yard = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, SetUpYard);
	AEncounterSpawner* Answer = EncounterTestWorld::SpawnSpawner(World, FVector(0.0, 1500.0, 0.0), SetUpAnswer);
	AChapelBell* Bell = SpawnBell(World, FVector(0.0, 3000.0, 64.0));
	AChapelReliquary* Reliquary = SpawnReliquary(World, FVector(0.0, 6000.0, 0.0));
	ASpeakerPoint* Aldana = PlaceAldana(World, FVector(0.0, 9000.0, 170.0));
	ACompanyHoseCart* Cart = World->SpawnActor<ACompanyHoseCart>(FVector(-400.0, 1000.0, 0.0), FRotator::ZeroRotator);
	AChapelWindowHose* Hose = World->SpawnActor<AChapelWindowHose>(FVector(-440.0, 320.0, 175.0), FRotator(0.0, 90.0, 0.0));
	if (!Runner || !Captions || !Sight || !Player || !Chapel || !Yard || !Answer || !Bell || !Reliquary || !Aldana || !Cart || !Hose)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Cart->Tags.Add(CartTag);
	Hose->CartTag = CartTag;
	Hose->ShownWhen.DuringMission = MainFour;
	Hose->PullableWhen.DuringMission = MainFour;
	Hose->PullableWhen.FromStep = 1;
	Hose->PullableWhen.BeforeStep = 2;
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* CoalUnderAsh = MakeMainFour(Scratch);
	Runner->BeginForTesting({ MakeMainThree(Scratch), CoalUnderAsh }, Campaign, Player, Valley);
	Aldana->DispatchBeginPlay();
	Cart->DispatchBeginPlay();
	Hose->DispatchBeginPlay();

	Runner->Update(0.f);
	TestTrue(TEXT("Main 3 first; Main 4 waits for it"), Runner->IsRunning(MainThree) && Runner->GetStatus(*CoalUnderAsh) == EMissionStatus::Locked);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainThreeDone")));
	TestTrue(TEXT("Main 3 done: Main 4 starts by itself, on the way up"), Campaign.HasCompleted(MainThree) && Runner->GetStep(MainFour) == 0);
	Hose->RefreshStory();
	TestTrue(TEXT("The hose is in her window, drinking her (her ember lower)"), Hose->IsInWindow() && Reliquary->IsDrinking());
	TestFalse(TEXT("...not to be pulled on the way up"), Hose->CanPull());

	// At the chapel: the yard's dead rise; the hose can be pulled.
	Player->SetActorLocation(FVector(1100.0, 0.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("At the chapel: clear the yard and pull the hose"), Runner->GetStep(MainFour), 1);
	Hose->RefreshStory();
	TestTrue(TEXT("...now the hose can be pulled"), Hose->CanPull());
	Yard->UpdateEncounter(0.5f);
	TestEqual(TEXT("The first push: eight"), Yard->NumAlive(), 8);

	// The cart stopped (its valve): the hose bucks out of the window by itself, and the missions hear it out.
	Cart->RefreshStory();
	TestTrue(TEXT("The cart's valve shut"), Cart->StopPumping(nullptr, /*bBurst*/ false));
	Hose->Advance(0.1f);
	TestTrue(TEXT("...the hose whips out of her window, and she isn't drunk now"), !Hose->IsInWindow() && !Reliquary->IsDrinking());
	TestEqual(TEXT("...but the yard isn't clear: still the yard's step"), Runner->GetStep(MainFour), 1);
	for (int32 Push = 0; Push < 2; ++Push)
	{
		EncounterTestWorld::KillAll(*Yard);
		Yard->UpdateEncounter(0.5f);
		Yard->UpdateEncounter(5.f);
	}
	Runner->Update(0.2f);
	TestTrue(TEXT("Sixteen down and the hose out: knock at the vestry door"), Yard->GetKilled() == 16 && Runner->GetStep(MainFour) == 2);

	// The knock; the bell (a held Interact on its rope, as the player's interaction component tells the missions).
	Captions->Update(30.f);
	TestTrue(TEXT("Aldana answers the knock"), Aldana->SpeakerPoint->Talk(Player) && Runner->GetStep(MainFour) == BellStep);
	TestTrue(TEXT("The bell rung"), Bell->Ring(Player));
	Runner->NotifyEvent(FMissionEvent::Interaction(Bell, /*bHeld*/ true));
	TestEqual(TEXT("...and something answers: the Reliquary and the barge"), Runner->GetStep(MainFour), ReliquaryStep);

	// Her ember flares; the barge's answer comes and is fought off.
	TestTrue(TEXT("The Reliquary looked at"), Reliquary->Look(Player));
	Reliquary->Advance(2.1f);
	Sight->Advance(2.1f);
	TestEqual(TEXT("...the flash over, but the answer still to fight"), Runner->GetStep(MainFour), ReliquaryStep);
	TestTrue(TEXT("The barge's answer is on"), Answer->IsStoryActive());
	Answer->UpdateEncounter(0.5f);
	TestEqual(TEXT("...six of them"), Answer->NumAlive(), 6);
	EncounterTestWorld::KillAll(*Answer);
	Answer->UpdateEncounter(0.5f);
	Runner->Update(0.2f);
	TestTrue(TEXT("Held: ready to turn in to Aldana"), Runner->IsReadyToTurnIn(MainFour) && !Campaign.HasCompleted(MainFour));

	// Aldana at the vestry door: card first (the turn-in card's own test), then his words: Main 4 is turned in.
	Captions->Update(30.f);
	TestTrue(TEXT("Aldana talked to"), Aldana->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("Main 4 is turned in: finished"), Campaign.HasCompleted(MainFour) && !Runner->IsRunning(MainFour));
	bool bSink = false;
	for (int32 Line = 0; Line < 16 && !bSink && Captions->GetCurrent(); ++Line)
	{
		bSink = OnScreen(*Captions) == FString(TEXT("Father Aldana|")) + AldanaWords[AldanaWordCount - 1];
		Captions->Update(Captions->GetCurrent()->Seconds + 0.01f);
	}
	TestTrue(TEXT("...his words end on the Sink"), bSink);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoalUnderAshRoamingTest, "Looter.Story.CoalUnderAsh.Roaming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoalUnderAshRoamingTest::RunTest(const FString& Parameters)
{
	// "From now on the Unpaid walk the north road and boot hill": their spawners are off until Main 4 is finished and on for
	// good after it, of the area's ranks, standing about rather than coming at the player as they appear.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1200.0, 0.0, 0.0));
	AEncounterSpawner* Hill = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector,
		[](AEncounterSpawner& Setup) { SetUpRoaming(Setup, BootHillId); });
	AEncounterSpawner* Road = EncounterTestWorld::SpawnSpawner(World, FVector(0.0, 12000.0, 0.0),
		[](AEncounterSpawner& Setup) { SetUpRoaming(Setup, NorthRoadId); });
	if (!Runner || !Player || !Hill || !Road)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* MainFourStandIn = MissionTestWorld::NewMission(Scratch, TEXT("Main4"), EMissionKind::Main, EMissionStart::Automatic, Valley);
	MissionTestWorld::AddObjective<UMissionEventObjective>(MainFourStandIn, 0)->Event = TEXT("Test.MainFourDone");
	Runner->BeginForTesting({ MainFourStandIn }, Campaign, Player, Valley);
	Runner->Update(0.f);
	Hill->UpdateEncounter(0.5f);
	TestTrue(TEXT("During Main 4: boot hill is quiet"), Runner->IsRunning(MainFour) && Hill->GetState() == EEncounterState::Off
		&& Hill->NumAlive() == 0);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainFourDone")));
	TestTrue(TEXT("Main 4 done: both stretches are on"), Campaign.HasCompleted(MainFour) && Hill->IsStoryActive() && Road->IsStoryActive());
	Hill->UpdateEncounter(0.5f);
	Road->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Out = Hill->GetAliveCreatures();
	TestEqual(TEXT("Boot hill, the player near: three Unpaid"), EncounterTestWorld::CountOf(Out, AUnpaidCreature::StaticClass(),
		ECreatureRank::Basic), 3);
	TestFalse(TEXT("...walking about, not coming at the player as they appear"), Out.ContainsByPredicate([](const ACreatureBase* Unpaid)
	{
		return Unpaid->GetTarget() != nullptr;
	}));
	TestTrue(TEXT("The north road, the player far: waiting for them"), Road->GetState() == EEncounterState::Waiting && Road->NumAlive() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoalUnderAshPlacedTest, "Looter.Story.CoalUnderAsh.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoalUnderAshPlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (build_area_chapel.py, build_area_main4.py): the chapel's place, the yard's set piece (16 with
	// the crew's two hose-men), the window's hose and its cart, the bell, the whole Reliquary, Aldana's door, the lychgate's
	// loudspeaker, the tender and its six on ropes, Finch at the Reliquary after Main 4, Hob's perches.
	const ULevel* Level = LoadRansomsRest(*this);
	if (!Level)
	{
		return true;
	}
	const AEncounterSpawner* Yard = nullptr;
	const ARappelSpawner* Drop = nullptr;
	const AChapelWindowHose* Hose = nullptr;
	const ACompanyHoseCart* Cart = nullptr;
	const ACompanyTenderDrop* Tender = nullptr;
	const ACompanyLoudspeaker* Lychgate = nullptr;
	const AChapelReliquary* Reliquary = nullptr;
	const AWardenCrewMember* Finch = nullptr;
	const AHobBird* Hob = nullptr;
	bool bPlace = false;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (const AEncounterSpawner* Spawner = Cast<AEncounterSpawner>(Actor))
		{
			Yard = Spawner->SpawnerId == YardId ? Spawner : Yard;
		}
		Drop = Drop ? Drop : Cast<ARappelSpawner>(Actor);
		if (Drop && Drop->SpawnerId != AnswerId)
		{
			Drop = nullptr;
		}
		Hose = Hose ? Hose : Cast<AChapelWindowHose>(Actor);
		Cart = Cart ? Cart : (Actor->ActorHasTag(CartTag) ? Cast<ACompanyHoseCart>(Actor) : nullptr);
		Tender = Tender ? Tender : (Actor->ActorHasTag(TenderTag) ? Cast<ACompanyTenderDrop>(Actor) : nullptr);
		Lychgate = Lychgate ? Lychgate : (Actor->ActorHasTag(LychgateTag) ? Cast<ACompanyLoudspeaker>(Actor) : nullptr);
		Reliquary = Reliquary ? Reliquary : Cast<AChapelReliquary>(Actor);
		Finch = Finch ? Finch : (Actor->ActorHasTag(FinchTag) ? Cast<AWardenCrewMember>(Actor) : nullptr);
		Hob = Hob ? Hob : Cast<AHobBird>(Actor);
		bPlace |= Actor->ActorHasTag(ChapelPlace);
	}
	if (!Yard && !Hose && !Reliquary && !bPlace)
	{
		AddWarning(TEXT("Main 4's pieces aren't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	TestTrue(TEXT("The chapel's middle is marked for the way up"), bPlace);
	if (TestNotNull(TEXT("The chapel yard's set piece"), Yard))
	{
		TestEqual(TEXT("...thirteen Basic Unpaid over its pushes"), CountBrought(*Yard, AUnpaidCreature::StaticClass(), ECreatureRank::Basic), 13);
		TestEqual(TEXT("...the Leftover, a Gravebound leader"), CountBrought(*Yard, AUnpaidCreature::StaticClass(), ECreatureRank::Epic), 1);
		TestEqual(TEXT("...and the crew's two hose-men"), CountBrought(*Yard, ADredgerHoseMan::StaticClass(), ECreatureRank::Basic), 2);
		TestTrue(TEXT("...on Main 4's second step only"), Yard->ActiveWhen.DuringMission == MainFour && Yard->ActiveWhen.FromStep == 1
			&& Yard->ActiveWhen.BeforeStep == 2);
	}
	if (TestNotNull(TEXT("The hose in her side window"), Hose))
	{
		TestTrue(TEXT("...tagged for its objective, pulled in the yard's step"), Hose->ActorHasTag(HoseTag)
			&& Hose->PullableWhen.DuringMission == MainFour && Hose->PullableWhen.FromStep == 1 && Hose->PullableWhen.BeforeStep == 2);
		TestTrue(TEXT("...its cart named, and the cart there"), Hose->CartTag == CartTag && Cart != nullptr);
	}
	TestNotNull(TEXT("Vane's loudspeaker by the lychgate"), Lychgate);
	if (TestNotNull(TEXT("The barge's tender"), Tender))
	{
		TestTrue(TEXT("...it comes as the bell is answered, with ropes, for the drop"), Tender->ComeWhen.DuringMission == MainFour
			&& Tender->ComeWhen.FromStep == ReliquaryStep && Tender->NumRopeAnchors() >= 2 && Tender->DropSpawnerId == AnswerId);
	}
	if (TestNotNull(TEXT("The barge's answer (BellAnswered, on ropes)"), Drop))
	{
		TestEqual(TEXT("...four riflemen"), CountBrought(*Drop, ADredgerRifleman::StaticClass(), ECreatureRank::Basic), 4);
		TestEqual(TEXT("...two hose-men"), CountBrought(*Drop, ADredgerHoseMan::StaticClass(), ECreatureRank::Basic), 2);
		TestTrue(TEXT("...off the tender, during the bell's answer"), Drop->AnchorTag == TenderTag && Drop->ActiveWhen.DuringMission == MainFour
			&& Drop->ActiveWhen.FromStep == ReliquaryStep);
	}
	if (TestNotNull(TEXT("The Reliquary"), Reliquary))
	{
		TestTrue(TEXT("...looked at from Main 4's fifth step"), Reliquary->LookWhen.DuringMission == MainFour && Reliquary->LookWhen.FromStep == ReliquaryStep);
		if (!Reliquary->HasModel())
		{
			AddInfo(TEXT("The whole Reliquary's model isn't placed: its stand-in shows until the Tripo one is in (TRIPO_MODELS.md M45-1)."));
		}
	}
	if (TestNotNull(TEXT("Brother Finch at the Reliquary"), Finch))
	{
		TestTrue(TEXT("...the keeper, shown after Main 4"), Finch->Crew == EWardenCrew::Keeper && Finch->ShownWhen.AfterMissions.Contains(MainFour));
	}
	if (TestNotNull(TEXT("Hob"), Hob))
	{
		TestTrue(TEXT("Hob has perches in Main 4"), Hob->Perches.ContainsByPredicate([](const FHobPerch& Perch)
		{
			return Perch.When.DuringMission == MainFour;
		}));
	}
	return true;
}

#endif
