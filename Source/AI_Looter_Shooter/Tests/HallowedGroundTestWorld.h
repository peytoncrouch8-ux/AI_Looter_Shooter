#pragma once

// What the Main 4 tests ("A Coal Under Ash", was "Hallowed Ground": CoalUnderAshTests.cpp, ChapelTests.cpp) build in their test levels: Main 4 as
// the mission script makes it, or with only one of its steps played for real; the chapel yard's fight, the bell, the
// Reliquary and Father Aldana's vestry door as the build script sets them up (Tools/Unreal/build_area_chapel.py); and the
// ids, tags and words they're found by.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureRank.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/UnpaidCreature.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLine.h"
#include "Story/StoryLineSet.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/ChapelBell.h"
#include "World/ChapelReliquary.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

namespace HallowedGroundTestWorld
{
	// The ids and tags Main 4 finds its pieces by (Tools/Unreal/create_mission_assets.py, build_area_chapel.py; the bell's and
	// the Reliquary's are their classes' own).
	inline const FName MainThree(TEXT("Main3"));
	inline const FName MainFour(TEXT("Main4"));
	inline const FName ChapelPlace(TEXT("Place_Chapel"));
	inline const FName YardId(TEXT("ChapelYard"));
	inline const FName YardTag(TEXT("Unpaid_ChapelYard"));
	inline const FName AldanaTag(TEXT("Speaker_Aldana"));
	inline const FName Valley(TEXT("TestValley"));
	// A Coal Under Ash's (2026-11-01, lane MAIN45: create_mission_assets.py, build_area_main4.py): the window's hose, the
	// barge's answer, Finch at the Reliquary.
	inline const FName HoseTag(TEXT("Hose_ChapelWindow"));
	inline const FName AnswerId(TEXT("BellAnswered"));
	inline const FName FinchTag(TEXT("Speaker_FinchChapel"));

	/**
	 * Main 4's steps as the asset has them (A Coal Under Ash): 0 the way up, 1 the yard and the window's hose, 2 the knock at
	 * the vestry door, 3 the bell, 4 the Reliquary and the barge's answer; then it's turned in to Aldana.
	 */
	inline constexpr int32 MainFourSteps = 5;
	inline constexpr int32 BellStep = 3;
	inline constexpr int32 ReliquaryStep = 4;

	/**
	 * The yard round the chapel, in the chapel's frame (cm; build_area_chapel.py): its creatures' spots (YARD_SPOTS) and the
	 * churchyard's iron fence round it (the layout's churchyardFence). A test level's spawner stands at the origin, unturned.
	 */
	inline const FVector YardSpots[] = { FVector(800.0, 900.0, 0.0), FVector(800.0, -700.0, 0.0), FVector(850.0, 1500.0, 0.0),
		FVector(850.0, -1450.0, 0.0), FVector(100.0, 560.0, 0.0), FVector(-450.0, 560.0, 0.0), FVector(-950.0, 0.0, 0.0),
		FVector(-1000.0, 500.0, 0.0), FVector(-1000.0, -500.0, 0.0), FVector(150.0, -600.0, 0.0), FVector(-750.0, -700.0, 0.0) };
	inline const FVector Fence[] = { FVector(1000.0, -1700.0, 0.0), FVector(1000.0, 1700.0, 0.0), FVector(-1200.0, 1700.0, 0.0),
		FVector(-1200.0, -1700.0, 0.0) };

	/**
	 * One of Main 4's steps as the mission script makes it (create_mission_assets.py), added to Mission. bWhole off: only the
	 * piece the step is named for (the yard without the hose, the Reliquary without the barge's answer), for a piece's own test.
	 */
	inline void AddStep(UMissionDefinition* Mission, int32 Step, bool bWhole = true)
	{
		switch (Step)
		{
		case 0:
		{
			UMissionReachObjective* Up = MissionTestWorld::AddObjective<UMissionReachObjective>(Mission, 0);
			Up->Place.Actor.ActorTag = ChapelPlace;
			Up->Place.Radius = 1500.f;
			break;
		}
		case 1:
		{
			UMissionClearObjective* Yard = MissionTestWorld::AddObjective<UMissionClearObjective>(Mission, 1);
			Yard->SpawnerId = YardId;
			Yard->Count = 16;
			if (bWhole)
			{
				UMissionInteractObjective* Hose = MissionTestWorld::AddObjective<UMissionInteractObjective>(Mission, 1);
				Hose->Target.ActorTag = HoseTag;
				Hose->bHold = true;
			}
			break;
		}
		case 2:
			MissionTestWorld::AddObjective<UMissionTalkObjective>(Mission, 2)->SpeakerTag = AldanaTag;
			break;
		case 3:
		{
			UMissionInteractObjective* Bell = MissionTestWorld::AddObjective<UMissionInteractObjective>(Mission, 3);
			Bell->Target.ActorTag = AChapelBell::BellTag;
			Bell->bHold = true;
			break;
		}
		default:
		{
			UMissionEventObjective* Sight = MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 4);
			Sight->Event = AChapelReliquary::SightEvent;
			Sight->Waypoint = EMissionWaypoint::Actor;
			Sight->WaypointActor.ActorTag = AChapelReliquary::ReliquaryTag;
			if (bWhole)
			{
				UMissionClearObjective* Answer = MissionTestWorld::AddObjective<UMissionClearObjective>(Mission, 4);
				Answer->SpawnerId = AnswerId;
				Answer->Count = 6;
			}
			break;
		}
		}
	}

	/** Main 4's turn-in, as the mission script gives it: to Father Aldana at the vestry door, with his own words. */
	inline void SetTurnIn(UMissionDefinition* Mission);

	/** A stand-in for Main 3: one event finishes it. */
	inline UMissionDefinition* MakeMainThree(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main3"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.MainThreeDone");
		return Mission;
	}

	/** Main 4 as the mission script makes it, in code: after Main 3, its five steps, turned in to Aldana, 40 experience. */
	inline UMissionDefinition* MakeMainFour(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main4"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		Mission->Prerequisites = { MainThree };
		for (int32 Step = 0; Step < MainFourSteps; ++Step)
		{
			AddStep(Mission, Step);
		}
		SetTurnIn(Mission);
		Mission->Rewards.Experience = 40;
		return Mission;
	}

	/** Main 4 with every step a named event ("Test.Step<n>") but the one a test plays for real (Step), Main 4's own. */
	inline UMissionDefinition* MakeMainFourAt(UObject* Outer, int32 Step)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main4"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		for (int32 Index = 0; Index < MainFourSteps; ++Index)
		{
			if (Index == Step)
			{
				AddStep(Mission, Index, /*bWhole*/ false);
				continue;
			}
			MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, Index)->Event = FName(*FString::Printf(TEXT("Test.Step%d"), Index));
		}
		SetTurnIn(Mission);
		return Mission;
	}

	/** The yard's fight as the build script sets it up: two waves, eight Unpaid and then seven with the Restless one (fights enlarged 2026-10-21; six and six before), on Main 4's second step. */
	inline void SetUpYard(AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = YardId;
		FEncounterGroup Restless = EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 1, ECreatureRank::Rare);
		Restless.FirstWave = 2;
		Restless.LastWave = 2;
		FEncounterGroup First = EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 8);
		First.FirstWave = 1;
		First.LastWave = 1;
		FEncounterGroup Second = EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 7);
		Second.FirstWave = 2;
		Second.LastWave = 2;
		Setup.Groups = { First, Second, Restless };
		Setup.NumWaves = 2;
		Setup.bWaitForClear = true;
		Setup.WaveInterval = 4.f;
		Setup.CreatureTags = { YardTag };
		Setup.ActiveWhen.DuringMission = MainFour;
		Setup.ActiveWhen.FromStep = 1;
		Setup.ActiveWhen.BeforeStep = 2;
		Setup.SpawnPoints = TArray<FVector>(YardSpots, UE_ARRAY_COUNT(YardSpots));
		Setup.bHuntOnSpawn = true;
		Setup.GroundCorners = TArray<FVector>(Fence, UE_ARRAY_COUNT(Fence));
		Setup.MaxGroundStep = 900.f;
		Setup.GroundMaxRise = 1000.f;
	}

	/** The chapel bell with its rope's grip at Grip, begun. */
	inline AChapelBell* SpawnBell(UWorld* World, const FVector& Grip)
	{
		AChapelBell* Bell = World->SpawnActor<AChapelBell>(Grip, FRotator::ZeroRotator);
		if (Bell)
		{
			Bell->DispatchBeginPlay();
		}
		return Bell;
	}

	/** The Reliquary as the build script places it (its foot at Foot): looked at from Main 4's fifth step on (from 0: 4). */
	inline AChapelReliquary* SpawnReliquary(UWorld* World, const FVector& Foot)
	{
		AChapelReliquary* Reliquary = World->SpawnActor<AChapelReliquary>(Foot, FRotator::ZeroRotator);
		if (Reliquary)
		{
			Reliquary->LookWhen.DuringMission = MainFour;
			Reliquary->LookWhen.FromStep = ReliquaryStep;
			Reliquary->DispatchBeginPlay();
		}
		return Reliquary;
	}

	// --- Father Aldana ---

	/** His words at Main 4's turn-in (MAINS_2_7.md section 6, A Coal Under Ash), in order: the mission's turn-in lines. */
	inline const TCHAR* const AldanaWords[] = {
		TEXT("She's a coal under ash, Warden. Without her keeper she's going out."),
		TEXT("When she does, every grave on the Rest is open to that hose."),
		TEXT("I watched from the vestry. Ned Purcell wouldn't break Abel's lantern. Bad luck, he said."),
		TEXT("He threw it down the Sink. Spiders hoard what a saint has touched."),
	};
	inline constexpr int32 AldanaWordCount = static_cast<int32>(UE_ARRAY_COUNT(AldanaWords));

	/** His other words (create_story_lines.py): barred, the knock, the bell, the barge's answer, after Main 4. */
	inline const TCHAR* const AldanaBarred = TEXT("Go away. I've nothing left to give anybody.");
	inline const TCHAR* const AldanaWaiting = TEXT("Then ring her bell. If she can still answer, she will.");
	inline const TCHAR* const AldanaBell = TEXT("The rope's inside the door, Warden. Ring her.");
	inline const TCHAR* const AldanaAnswer = TEXT("She answered! Look at her, Warden. And mind the yard: so did they.");
	inline const TCHAR* const AldanaAfter = TEXT("The Sink, Warden. Bring his lantern up whole.");

	inline FStoryLine Said(const TCHAR* Speaker, const TCHAR* Words, float Seconds = 3.f)
	{
		return FStoryLine::Make(FText::FromString(Speaker), FText::FromString(Words), Seconds);
	}

	inline void SetTurnIn(UMissionDefinition* Mission)
	{
		Mission->TurnIn.SpeakerTag = AldanaTag;
		Mission->TurnIn.GiverName = FText::FromString(TEXT("Father Aldana"));
		for (const TCHAR* Words : AldanaWords)
		{
			Mission->TurnIn.Lines.Add(Said(TEXT(""), Words));
		}
	}

	/** A line set the story script makes, when it's in this checkout. */
	inline const UStoryLineSet* LoadLines(const TCHAR* Name)
	{
		const FString Package = FString(TEXT("/Game/Data/Story/")) + Name;
		return FPackageName::DoesPackageExist(Package) ? LoadObject<UStoryLineSet>(nullptr, *(Package + TEXT(".") + Name)) : nullptr;
	}

	/** The doc's words are among Lines in order, each in his name (bNamed: as his door says them) or in no one's (his set's). */
	inline bool HoldsAldanasWords(const TArray<FStoryLine>& Lines, bool bNamed)
	{
		int32 Next = 0;
		for (const FStoryLine& Line : Lines)
		{
			const bool bHis = bNamed ? Line.Speaker.ToString() == TEXT("Father Aldana") : Line.Speaker.IsEmpty();
			if (Next < AldanaWordCount && bHis && Line.Text.ToString() == AldanaWords[Next])
			{
				++Next;
			}
		}
		return Next == AldanaWordCount;
	}

	/** A topic during Main 4 from FromStep (and before BeforeStep: 0 none), its words from Set when made, else Words. */
	inline FSpeakerTopic MainFourTopic(int32 FromStep, int32 BeforeStep, const TCHAR* Set, const TCHAR* Words)
	{
		FSpeakerTopic Topic;
		Topic.When.DuringMission = MainFour;
		Topic.When.FromStep = FromStep;
		Topic.When.BeforeStep = BeforeStep;
		const UStoryLineSet* Made = LoadLines(Set);
		Topic.Lines = Made ? Made->Lines : TArray<FStoryLine>({ Said(TEXT(""), Words) });
		return Topic;
	}

	/**
	 * His vestry door as the build script sets it up (story_texts.TOPICS['Speaker_Aldana'], not begun): barred; the barge's
	 * answer (step 4 only: topic 0), the bell (from step 3: 1), the knock (from step 2: 2), the Sink after Main 4 (3); his
	 * Main 4 words are the mission's turn-in lines. From the line sets when they're made, else their words in code.
	 */
	inline ASpeakerPoint* PlaceAldana(UWorld* World, const FVector& Where)
	{
		ASpeakerPoint* Door = World->SpawnActor<ASpeakerPoint>(Where, FRotator(0.0, 180.0, 0.0));
		if (!Door)
		{
			return nullptr;
		}
		USpeakerPointComponent* Talk = Door->SpeakerPoint;
		Talk->SetRelativeLocation(FVector::ZeroVector);
		Talk->SpeakerName = FText::FromString(TEXT("Father Aldana"));
		const UStoryLineSet* BarredSet = LoadLines(TEXT("DA_Lines_AldanaBarred"));
		const UStoryLineSet* AfterSet = LoadLines(TEXT("DA_Lines_AldanaAfterMain4"));
		Talk->Lines = BarredSet ? BarredSet->Lines : TArray<FStoryLine>({ Said(TEXT(""), AldanaBarred) });
		FSpeakerTopic Later;
		Later.When.AfterMissions = { MainFour };
		Later.Lines = AfterSet ? AfterSet->Lines : TArray<FStoryLine>({ Said(TEXT(""), AldanaAfter) });
		Talk->Topics = { MainFourTopic(ReliquaryStep, ReliquaryStep + 1, TEXT("DA_Lines_AldanaAnswer"), AldanaAnswer),
			MainFourTopic(BellStep, 0, TEXT("DA_Lines_AldanaBell"), AldanaBell),
			MainFourTopic(2, 0, TEXT("DA_Lines_AldanaWaiting"), AldanaWaiting), Later };
		Door->Tags.Add(AldanaTag);
		return Door;
	}
}

#endif
