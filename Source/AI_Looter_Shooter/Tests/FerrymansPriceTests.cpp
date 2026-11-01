#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bestiary/Ledger.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/DredgerHoseMan.h"
#include "Creatures/DredgerRifleman.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/HobBird.h"
#include "Story/MisterSexton.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "Story/StoryLineSet.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/CompanyHoseCart.h"
#include "World/CompanyLoudspeaker.h"
#include "World/FerryLadder.h"
#include "World/SextonsFerry.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Main 2, "The Ferryman's Price" (the Wardens story since 2026-11-01; was "Shall We Talk Business?", whose tests this file
// held: Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 4): the keeper's path out to Gravewind Point, the Company's
// hose-cart on the burial boards and its crew, the spill's dead, Mister Sexton's price at his ferry, the Warden's Log. The
// cart's, the loudspeaker's and the banner's own rules are CompanyPropsTests.cpp's; the ferry's and its ladder's
// SextonsFerryTests.cpp's; the Log's own rules LedgerTests.cpp's.

namespace
{
	// The ids and tags Main 2 finds its pieces by (Tools/Unreal/create_mission_assets.py, build_area_main2.py).
	const FName FerryMainOne(TEXT("Main1"));
	const FName FerryMainTwo(TEXT("Main2"));
	const FName FerryPointPlace(TEXT("Place_GravewindPoint"));
	const FName FerryBridgePlace(TEXT("Place_KeepersBridge"));
	const FName FerrySextonTag(TEXT("Speaker_Sexton"));
	const FName FerryHobTag(TEXT("Speaker_Hob"));
	const FName FerryCartTag(TEXT("HoseCart_Boards"));
	const FName FerryCrewId(TEXT("BoardsCrew"));
	const FName FerryNestId(TEXT("BluffNest"));
	const FName FerryGateSpeaker(TEXT("Loudspeaker_KeepersGate"));
	const FName FerryValley(TEXT("TestValley"));

	/** The deal's words (MAINS_2_7.md, Main 2 step 2), in order, the last handing the Log up. */
	const TCHAR* const FerryDealWords[] = {
		TEXT("Warden. You've come about the barge. Everyone does, eventually."),
		TEXT("She's out of reach. But a barge must eat, and Company men are greedy."),
		TEXT("Starve her, and they'll bring her down to where the food is."),
		TEXT("Mind: a keeper's soul goes in his own lantern, or it goes out like a match."),
		TEXT("Write down every soul you meet. I bill by the head."),
	};
	/** His price, as Main 2 is turned in (create_mission_assets.py's). */
	const TCHAR* const FerryPriceWords[] = {
		TEXT("My price: the barge's manifest. Every soul Vane burned, with my fare beside it."),
		TEXT("And if I don't bring it?"),
		TEXT("Then you'll owe me, friend. Ask the crow how that goes."),
		TEXT("Come back when you've silver. I know where things lie on the Rest."),
	};
	const TCHAR* const FerryBlueWords = TEXT("See the blue on that one? Fed longer on the smog. Hits harder, and its iron's better.");

	const UStoryLineSet* FerryLoadLines(const TCHAR* Name)
	{
		const FString Package = FString(TEXT("/Game/Data/Story/")) + Name;
		return FPackageName::DoesPackageExist(Package) ? LoadObject<UStoryLineSet>(nullptr, *(Package + TEXT(".") + Name)) : nullptr;
	}

	FString FerryOnScreen(const UCaptionSubsystem& Captions)
	{
		const FCaptionEntry* Current = Captions.GetCurrent();
		return Current ? Current->Line.Speaker.ToString() + TEXT("|") + Current->Line.Text.ToString() : FString(TEXT("none"));
	}

	/** A stand-in for Main 1: one event finishes it. */
	UMissionDefinition* MakeFerryMainOne(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main1"), EMissionKind::Main, EMissionStart::Automatic, FerryValley);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.MainOneDone");
		return Mission;
	}

	/**
	 * Main 2 as the mission script makes it, in code (started by itself here: Hob's words start it on the move in the game):
	 * the way out, the cart and its crew (four here: two of the crew, two of the spill), Sexton, the Log; turned in to him.
	 */
	UMissionDefinition* MakeFerrymansPrice(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main2"), EMissionKind::Main, EMissionStart::Automatic, FerryValley);
		Mission->Prerequisites = { FerryMainOne };
		UMissionReachObjective* Way = MissionTestWorld::AddObjective<UMissionReachObjective>(Mission, 0);
		Way->Place.Actor.ActorTag = FerryPointPlace;
		Way->Place.Radius = 2500.f;
		UMissionEventObjective* Stop = MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 1);
		Stop->Event = ACompanyHoseCart::StoppedEvent;
		UMissionClearObjective* Crew = MissionTestWorld::AddObjective<UMissionClearObjective>(Mission, 1);
		Crew->SpawnerId = FerryCrewId;
		Crew->Count = 4;
		MissionTestWorld::AddObjective<UMissionTalkObjective>(Mission, 2)->SpeakerTag = FerrySextonTag;
		MissionTestWorld::AddObjective<UMissionOpenPageObjective>(Mission, 3)->Page = EMissionPage::Bestiary;
		Mission->TurnIn.SpeakerTag = FerrySextonTag;
		Mission->TurnIn.GiverName = FText::FromString(TEXT("Mister Sexton"));
		for (const TCHAR* Words : FerryPriceWords)
		{
			Mission->TurnIn.Lines.Add(FStoryLine::Make(FText::GetEmpty(), FText::FromString(Words)));
		}
		Mission->Rewards.Experience = 550;
		return Mission;
	}

	/** The boards' crew as the build sets it up, Unpaid standing in for the Dredgers: two at work, then two from the spill. */
	void SetUpFerryCrew(AEncounterSpawner& Setup)
	{
		FEncounterGroup Crew = EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 2);
		Crew.FirstWave = 1;
		Crew.LastWave = 1;
		FEncounterGroup Spill = EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 2);
		Spill.FirstWave = 2;
		Spill.LastWave = 2;
		Setup.SpawnerId = FerryCrewId;
		Setup.Groups = { Crew, Spill };
		Setup.NumWaves = 2;
		Setup.WaveInterval = 0.f;
		Setup.bWaitForClear = false;
		Setup.ActiveWhen.DuringMission = FerryMainTwo;
		Setup.ActiveWhen.BeforeStep = 2;
		Setup.ActivationRadius = 100000.f;
		Setup.SpawnRadius = 600.f;
	}

	/** Sexton as the build sets him up (story_texts.TOPICS['Speaker_Sexton']), his topics in code: there from Main 2 on. */
	AMisterSexton* PlaceFerrySexton(UWorld* World, const FVector& Where)
	{
		AMisterSexton* Sexton = World->SpawnActor<AMisterSexton>(Where, FRotator::ZeroRotator);
		if (!Sexton)
		{
			return nullptr;
		}
		Sexton->Tags.Add(FerrySextonTag);
		Sexton->ShownWhen.AfterMissions = { FerryMainOne };
		FSpeakerTopic Log;
		Log.When.DuringMission = FerryMainTwo;
		Log.When.FromStep = 3;
		Log.Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("Open it, friend. My pages are in your Log now."))) };
		FSpeakerTopic Deal;
		Deal.When.DuringMission = FerryMainTwo;
		Deal.When.FromStep = 2;
		for (const TCHAR* Words : FerryDealWords)
		{
			Deal.Lines.Add(FStoryLine::Make(FText::GetEmpty(), FText::FromString(Words)));
		}
		FSpeakerTopic Waiting;
		Waiting.When.DuringMission = FerryMainTwo;
		Waiting.Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("The boards first, friend. Vane's men are drinking my fares."))) };
		Sexton->SpeakerPoint->Topics = { Log, Deal, Waiting };
		return Sexton;
	}

	const ULevel* LoadFerryRest(FAutomationTestBase& Test)
	{
		if (!FPackageName::DoesPackageExist(TEXT("/Game/Maps/Lvl_RansomsRest")))
		{
			Test.AddInfo(TEXT("Lvl_RansomsRest isn't built: skipped."));
			return nullptr;
		}
		const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"));
		return Test.TestNotNull(TEXT("Lvl_RansomsRest loads"), Map) ? Map->PersistentLevel.Get() : nullptr;
	}

	int32 FerryCountBrought(const AEncounterSpawner& Spawner, const UClass* Class, ECreatureRank Rank)
	{
		int32 Brought = 0;
		for (const FEncounterGroup& Group : Spawner.Groups)
		{
			const bool bOfClass = Group.CreatureClass && Group.CreatureClass->IsChildOf(Class);
			Brought += bOfClass && Group.RankRoll == EEncounterRankRoll::Fixed && Group.Rank == Rank ? Group.Count : 0;
		}
		return Brought;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFerrymansPriceMissionTest, "Looter.Story.FerrymansPrice.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFerrymansPriceMissionTest::RunTest(const FString& Parameters)
{
	// Main 2 as its asset has it, once the mission script has made it: offered by Hob after Main 1 on Ransom's Rest; the way
	// out (+ Keeper's Bridge), the cart stopped and its crew fought off (+ the gate's loudspeaker), Sexton, the Log (the
	// step the Log's rule names); turned in to Sexton with his price, for 400 experience (550 until 2026-11-01: Main trimmed
	// it to keep the Pacing band, Docs/Progression.md).
	if (!FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main2")))
	{
		AddWarning(TEXT("DA_Mission_Main2 isn't made yet: run Tools/Unreal/create_mission_assets.py."));
		return true;
	}
	const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main2.DA_Mission_Main2"));
	if (!TestNotNull(TEXT("DA_Mission_Main2 loads"), Asset))
	{
		return false;
	}
	TestEqual(TEXT("The Ferryman's Price"), Asset->Title.ToString(), FString(TEXT("The Ferryman's Price")));
	TestTrue(TEXT("Main2, a main mission Hob offers on Ransom's Rest, after Main1"), Asset->GetMissionId() == FerryMainTwo
		&& Asset->Kind == EMissionKind::Main && Asset->Start == EMissionStart::Offered && Asset->Offer.GiverTag == FerryHobTag
		&& !Asset->Offer.Lines.IsEmpty() && !Asset->Offer.Briefing.IsEmpty() && Asset->Area == FName(TEXT("RansomsRest"))
		&& Asset->Prerequisites == TArray<FName>({ FerryMainOne }));
	TestEqual(TEXT("Four steps"), Asset->Steps.Num(), 4);
	const UMissionReachObjective* Way = Cast<UMissionReachObjective>(Asset->GetObjective(0, 0));
	const UMissionReachObjective* Bridge = Cast<UMissionReachObjective>(Asset->GetObjective(0, 1));
	const UMissionEventObjective* Stop = Cast<UMissionEventObjective>(Asset->GetObjective(1, 0));
	const UMissionClearObjective* Crew = Cast<UMissionClearObjective>(Asset->GetObjective(1, 1));
	const UMissionKillNamedObjective* Horn = Cast<UMissionKillNamedObjective>(Asset->GetObjective(1, 2));
	const UMissionTalkObjective* Talk = Cast<UMissionTalkObjective>(Asset->GetObjective(2, 0));
	const UMissionOpenPageObjective* Open = Cast<UMissionOpenPageObjective>(Asset->GetObjective(Ledger::HandedOverStep, 0));
	TestTrue(TEXT("1: the keeper's path to Gravewind Point, 25 m round the Keeper's Gate"), Way && Way->Place.Actor.ActorTag == FerryPointPlace
		&& Way->Place.Radius >= 2000.f);
	TestTrue(TEXT("...and, optional, Keeper's Bridge"), Bridge && Bridge->bOptional && Bridge->Place.Actor.ActorTag == FerryBridgePlace);
	TestTrue(TEXT("2: the hose-cart stopped, either way (its event)"), Stop && Stop->Event == ACompanyHoseCart::StoppedEvent && !Stop->bOptional);
	TestTrue(TEXT("...its crew and its spill fought off, all eight, the count hidden (the spill is the twist)"), Crew
		&& Crew->SpawnerId == FerryCrewId && Crew->Count == 8 && !Crew->bShowCount);
	TestTrue(TEXT("...and, optional, Vane's loudspeaker at the Keeper's Gate shot out"), Horn && Horn->bOptional
		&& Horn->ActorTag == FerryGateSpeaker && Horn->bPlayerKillsOnly);
	TestTrue(TEXT("3: talk to Mister Sexton at his ferry"), Talk && Talk->SpeakerTag == FerrySextonTag);
	TestTrue(TEXT("4: open the Warden's Log, the step the Log is handed over on"), Open && Open->Page == EMissionPage::Bestiary
		&& Ledger::HandedOverStep == 3);
	TestTrue(TEXT("Turned in to Sexton, his price in his own words"), Asset->NeedsTurnIn() && Asset->TurnIn.SpeakerTag == FerrySextonTag
		&& Asset->TurnIn.Lines.Num() == static_cast<int32>(UE_ARRAY_COUNT(FerryPriceWords))
		&& Asset->TurnIn.Lines[0].Text.ToString() == FerryPriceWords[0]);
	TestEqual(TEXT("Its reward: 400 experience"), Asset->Rewards.Experience, 400);
	TestFalse(TEXT("...and no gun"), Asset->Rewards.bGun);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFerrymansPriceFlowTest, "Looter.Story.FerrymansPrice.Flow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFerrymansPriceFlowTest::RunTest(const FString& Parameters)
{
	// Played through in a test level: Main 2 after Main 1; the cart stands pumping on the way out but can't be stopped till
	// its step; the crew at work; the valve shut brings the spill's dead and tells the step; both down, Sexton; his deal
	// opens the Log; turned in to him.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-8000.0, 0.0, 0.0));
	AActor* Gate = MissionTestWorld::SpawnMarker(World, FVector(-1500.0, 0.0, 0.0), FerryPointPlace);
	AEncounterSpawner* Crew = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, SetUpFerryCrew);
	ACompanyHoseCart* Cart = World->SpawnActor<ACompanyHoseCart>(FVector(300.0, 0.0, 0.0), FRotator::ZeroRotator);
	AMisterSexton* Sexton = PlaceFerrySexton(World, FVector(2500.0, 0.0, -380.0));
	if (!Runner || !Captions || !Encounters || !Player || !Gate || !Crew || !Cart || !Sexton)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	// As build_area_main2.py sets the cart up.
	Cart->Tags.Add(FerryCartTag);
	Cart->ShownWhen.DuringMission = FerryMainTwo;
	Cart->PumpingWhen.DuringMission = FerryMainTwo;
	Cart->PumpingWhen.FromStep = 1;
	Cart->PumpingWhen.BeforeStep = 2;
	FStoryCondition Past;
	Past.DuringMission = FerryMainTwo;
	Past.FromStep = 2;
	FStoryCondition After;
	After.AfterMissions = { FerryMainTwo };
	Cart->StoppedWhen = { Past, After };
	Cart->CrewSpawnerId = FerryCrewId;
	Cart->SpillSpots = { FVector(260.0, 60.0, 0.0), FVector(-260.0, -60.0, 0.0) };

	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Price = MakeFerrymansPrice(Scratch);
	Runner->BeginForTesting({ MakeFerryMainOne(Scratch), Price }, Campaign, Player, FerryValley);
	Sexton->DispatchBeginPlay();
	Cart->DispatchBeginPlay();
	Runner->Update(0.f);
	TestTrue(TEXT("Main 1 first: no cart on the boards, no Ferryman"), Cart->GetCartState() == EHoseCartState::Away && !Sexton->IsShown());

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainOneDone")));
	Cart->RefreshStory();
	Sexton->RefreshShown();
	Encounters->RefreshStory();
	TestEqual(TEXT("Main 1 done: Main 2 on its way out"), Runner->GetStep(FerryMainTwo), 0);
	TestTrue(TEXT("The cart pumps on the boards as the player comes"), Cart->GetCartState() == EHoseCartState::Pumping);
	TestFalse(TEXT("...but can't be stopped before its step (nobody listening yet)"), Cart->IsLive());
	TestTrue(TEXT("The Ferryman is at his ferry"), Sexton->IsShown() && Sexton->SpeakerPoint->CanTalk());
	TestTrue(TEXT("Talked to now: the boards first"), Sexton->SpeakerPoint->Talk(Player)
		&& FerryOnScreen(*Captions).EndsWith(TEXT("The boards first, friend. Vane's men are drinking my fares.")));
	Captions->Update(30.f);

	// At the Keeper's Gate: the cart's step.
	Player->SetActorLocation(FVector(-1500.0, 400.0, 0.0));
	Runner->Update(0.2f);
	Cart->RefreshStory();
	TestEqual(TEXT("At the Keeper's Gate: stop the cart"), Runner->GetStep(FerryMainTwo), 1);
	TestTrue(TEXT("...and now the cart can be stopped"), Cart->IsLive());
	Crew->UpdateEncounter(0.5f);
	TestEqual(TEXT("The crew at work: two"), Crew->NumAlive(), 2);
	EncounterTestWorld::KillAll(*Crew);
	Crew->UpdateEncounter(0.5f);
	Runner->Update(0.2f);
	TestEqual(TEXT("The crew down, the cart still pumping: the step waits"), Runner->GetStep(FerryMainTwo), 1);
	TestTrue(TEXT("...the crew's fight isn't over: the spill is still to come"), Crew->GetState() != EEncounterState::Cleared);

	// The valve shut: the spill's dead rise at its spots, and the step hears it.
	TestTrue(TEXT("The valve shut"), Cart->StopPumping(nullptr, /*bBurst*/ false));
	TestTrue(TEXT("...the cart stopped, its spill out, not burst"), Cart->GetCartState() == EHoseCartState::Stopped && !Cart->WasBurst());
	TestFalse(TEXT("...nothing more to stop"), Cart->StopPumping(nullptr, true));
	TestEqual(TEXT("...the crew's second wave came on the cart's word"), Crew->GetWavesStarted(), 2);
	TestEqual(TEXT("...at the spill's spots"), Crew->SpawnPoints.Num(), 2);
	Crew->UpdateEncounter(0.5f);
	TestEqual(TEXT("The spill's dead: two"), Crew->NumAlive(), 2);
	EncounterTestWorld::KillAll(*Crew);
	Crew->UpdateEncounter(0.5f);
	Runner->Update(0.2f);
	TestEqual(TEXT("The cart stopped and the boards cleared: on to Sexton"), Runner->GetStep(FerryMainTwo), 2);
	Cart->RefreshStory();
	TestTrue(TEXT("...the cart stays stopped"), Cart->GetCartState() == EHoseCartState::Stopped);
	TestFalse(TEXT("The Log isn't his yet"), Ledger::IsOpen(Campaign, Runner));

	// The deal, ending on the Log handed up.
	TestTrue(TEXT("Sexton talked to"), Sexton->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...the deal's first words"), FerryOnScreen(*Captions), FString(TEXT("Mister Sexton|")) + FerryDealWords[0]);
	TestEqual(TEXT("The deal struck: open the Log"), Runner->GetStep(FerryMainTwo), Ledger::HandedOverStep);
	TestTrue(TEXT("Sexton's pages are in the Log"), Ledger::IsOpen(Campaign, Runner));
	Runner->CompleteStep(FerryMainTwo);
	TestTrue(TEXT("The Log read: ready to turn in to Sexton"), Runner->IsReadyToTurnIn(FerryMainTwo));
	Captions->Update(600.f);
	TestTrue(TEXT("Sexton talked to"), Sexton->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...his price"), FerryOnScreen(*Captions), FString(TEXT("Mister Sexton|")) + FerryPriceWords[0]);
	TestTrue(TEXT("Main 2 is turned in"), Campaign.HasCompleted(FerryMainTwo));
	Captions->Update(600.f);
	Sexton->RefreshShown();
	TestTrue(TEXT("He stays at his ferry after (he sells secrets there)"), Sexton->IsShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFerrymansPriceLinesTest, "Looter.Story.FerrymansPrice.Lines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFerrymansPriceLinesTest::RunTest(const FString& Parameters)
{
	// The words as the story script writes them (when it has): the deal holds the plan's lines in their order, in his
	// voice; Vane's complaint is his; Hob's lines are Hob's; every caption short enough to read.
	const UStoryLineSet* Deal = FerryLoadLines(TEXT("DA_Lines_SextonDeal"));
	if (!Deal)
	{
		AddWarning(TEXT("The story's line sets aren't made yet: run Tools/Unreal/create_story_lines.py."));
		return true;
	}
	TestEqual(TEXT("The deal: the plan's lines, in order"), Deal->Lines.Num(), static_cast<int32>(UE_ARRAY_COUNT(FerryDealWords)));
	for (int32 Index = 0; Index < Deal->Lines.Num() && Index < UE_ARRAY_COUNT(FerryDealWords); ++Index)
	{
		TestEqual(FString::Printf(TEXT("...line %d"), Index + 1), Deal->Lines[Index].Text.ToString(), FString(FerryDealWords[Index]));
		TestTrue(FString::Printf(TEXT("...line %d in his voice (his point's name)"), Index + 1), Deal->Lines[Index].Speaker.IsEmpty());
	}
	if (const UStoryLineSet* Blue = FerryLoadLines(TEXT("DA_Lines_HobMain2Blue")))
	{
		TestTrue(TEXT("Hob on the blue one, the smog's"), !Blue->Lines.IsEmpty() && Blue->Lines[0].Text.ToString() == FerryBlueWords);
	}
	if (const UStoryLineSet* Complaint = FerryLoadLines(TEXT("DA_Lines_Main2VaneCart")))
	{
		TestTrue(TEXT("Vane's complaint, all his, ending on the bill (the line a shot horn cuts)"), Complaint->Lines.Num() == 2
			&& !Complaint->Lines.ContainsByPredicate([](const FStoryLine& Line) { return Line.Speaker.ToString() != TEXT("Cornelius Vane"); })
			&& Complaint->Lines.Last().Text.ToString() == TEXT("I'm adding it to your bill."));
	}
	const TCHAR* const Sets[] = { TEXT("DA_Lines_HobMain2Blue"), TEXT("DA_Lines_HobMain2Bridge"), TEXT("DA_Lines_HobMain2Cart"),
		TEXT("DA_Lines_HobMain2Spill"), TEXT("DA_Lines_Main2VaneCart"), TEXT("DA_Lines_HobMain2Ladder"), TEXT("DA_Lines_SextonWaiting"),
		TEXT("DA_Lines_SextonDeal"), TEXT("DA_Lines_SextonLedger"), TEXT("DA_Lines_SextonAfter") };
	for (const TCHAR* Name : Sets)
	{
		const UStoryLineSet* Set = FerryLoadLines(Name);
		if (!TestNotNull(FString::Printf(TEXT("%s is made"), Name), Set))
		{
			continue;
		}
		for (const FStoryLine& Line : Set->Lines)
		{
			TestTrue(FString::Printf(TEXT("%s: \"%s\" fits a caption (90 characters)"), Name, *Line.Text.ToString()), Line.Text.ToString().Len() <= 90);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFerrymansPricePlacedTest, "Looter.Story.FerrymansPrice.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFerrymansPricePlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (build_area_main2.py): one Sexton, in his ferry's stern, there from Main 2 on; the ferry and its
	// ladder; the Widows' nest; the boards' cart and its crew; the Keeper's Gate's loudspeaker; Hob's Main 2 perches.
	const ULevel* Level = LoadFerryRest(*this);
	if (!Level)
	{
		return true;
	}
	int32 Sextons = 0;
	const AMisterSexton* Sexton = nullptr;
	const ASextonsFerry* Ferry = nullptr;
	const AFerryLadder* Ladder = nullptr;
	const ACompanyHoseCart* Cart = nullptr;
	const ACompanyLoudspeaker* Horn = nullptr;
	const AEncounterSpawner* Nest = nullptr;
	const AEncounterSpawner* Crew = nullptr;
	const AHobBird* Hob = nullptr;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (const AMisterSexton* Found = Cast<AMisterSexton>(Actor))
		{
			++Sextons;
			Sexton = Found;
		}
		Ferry = Ferry ? Ferry : Cast<ASextonsFerry>(Actor);
		Ladder = Ladder ? Ladder : Cast<AFerryLadder>(Actor);
		Hob = Hob ? Hob : Cast<AHobBird>(Actor);
		if (const ACompanyHoseCart* Found = Cast<ACompanyHoseCart>(Actor); Found && Found->ActorHasTag(FerryCartTag))
		{
			Cart = Found;
		}
		if (const ACompanyLoudspeaker* Found = Cast<ACompanyLoudspeaker>(Actor); Found && Found->ActorHasTag(FerryGateSpeaker))
		{
			Horn = Found;
		}
		if (const AEncounterSpawner* Spawner = Cast<AEncounterSpawner>(Actor))
		{
			Nest = Spawner->SpawnerId == FerryNestId ? Spawner : Nest;
			Crew = Spawner->SpawnerId == FerryCrewId ? Spawner : Crew;
		}
	}
	if (!Ferry && !Cart && !Crew)
	{
		AddWarning(TEXT("Main 2's pieces aren't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	TestEqual(TEXT("One Sexton (the old rail Sexton gone)"), Sextons, 1);
	if (TestNotNull(TEXT("Sexton"), Sexton))
	{
		TestTrue(TEXT("...tagged for Main 2's talk"), Sexton->ActorHasTag(FerrySextonTag));
		TestTrue(TEXT("...there from Main 2 on (he stays at his ferry)"), Sexton->ShownWhen.AfterMissions.Contains(FerryMainOne)
			&& Sexton->ShownWhen.DuringMission.IsNone());
		TestTrue(TEXT("...the deal among his topics"), Sexton->SpeakerPoint->Topics.ContainsByPredicate([](const FSpeakerTopic& Topic)
		{
			return Topic.When.DuringMission == FerryMainTwo && Topic.When.FromStep == 2 && Topic.LineSet != nullptr;
		}));
	}
	if (TestNotNull(TEXT("The Ferryman's landing and ferry"), Ferry))
	{
		TestTrue(TEXT("...Sexton its passenger"), Ferry->Passenger == Sexton);
	}
	if (TestNotNull(TEXT("The ladder down to it"), Ladder))
	{
		TestTrue(TEXT("...hanging the landing's drop"), FMath::IsNearlyEqual(Ladder->Drop, ASextonsFerry::ClimbDrop));
	}
	if (TestNotNull(TEXT("The boards' hose-cart"), Cart))
	{
		TestTrue(TEXT("...its spill brings the crew's next wave"), Cart->CrewSpawnerId == FerryCrewId && !Cart->SpillSpots.IsEmpty());
		TestTrue(TEXT("...stoppable in the cart's step only"), Cart->PumpingWhen.DuringMission == FerryMainTwo && Cart->PumpingWhen.FromStep == 1
			&& Cart->PumpingWhen.BeforeStep == 2);
		TestTrue(TEXT("...Vane complains from the gate's loudspeaker"), Cart->LoudspeakerTag == FerryGateSpeaker && Cart->StopBroadcast != nullptr);
	}
	TestNotNull(TEXT("The Keeper's Gate's loudspeaker"), Horn);
	if (TestNotNull(TEXT("The boards' crew"), Crew))
	{
		TestEqual(TEXT("...three riflemen"), FerryCountBrought(*Crew, ADredgerRifleman::StaticClass(), ECreatureRank::Basic), 3);
		TestEqual(TEXT("...a hose-man"), FerryCountBrought(*Crew, ADredgerHoseMan::StaticClass(), ECreatureRank::Basic), 1);
		TestEqual(TEXT("...the spill's four dead"), FerryCountBrought(*Crew, AUnpaidCreature::StaticClass(), ECreatureRank::Basic), 4);
		TestTrue(TEXT("...the dead in the second wave, which only the cart brings"), Crew->NumWaves == 2 && Crew->WaveInterval == 0.f
			&& !Crew->bWaitForClear && Crew->Groups.ContainsByPredicate([](const FEncounterGroup& Group) { return Group.FirstWave == 2; }));
		TestFalse(TEXT("...all at the area's level 2, not the deck's"), Crew->Groups.ContainsByPredicate([](const FEncounterGroup& Group)
		{
			return Group.Level != 2;
		}));
	}
	if (TestNotNull(TEXT("The Three Widows' nest"), Nest))
	{
		TestEqual(TEXT("...four Basic spiders"), FerryCountBrought(*Nest, ASpiderCreature::StaticClass(), ECreatureRank::Basic), 4);
		TestEqual(TEXT("...and a Restless one"), FerryCountBrought(*Nest, ASpiderCreature::StaticClass(), ECreatureRank::Rare), 1);
		TestTrue(TEXT("...two out at most, during Main 2"), Nest->MaxAlive == 2 && Nest->ActiveWhen.DuringMission == FerryMainTwo);
		TestTrue(TEXT("...on its own ground"), Nest->GroundCorners.Num() >= 3);
		TestNotNull(TEXT("...Hob's word on the blue one as it's cleared"), Nest->ClearedLines.Get());
	}
	if (TestNotNull(TEXT("Hob"), Hob))
	{
		TestTrue(TEXT("Hob has perches in Main 2 (lane MAIN23's place_hob edit)"), Hob->Perches.ContainsByPredicate([](const FHobPerch& Perch)
		{
			return Perch.When.DuringMission == FerryMainTwo;
		}));
	}
	return true;
}

#endif
