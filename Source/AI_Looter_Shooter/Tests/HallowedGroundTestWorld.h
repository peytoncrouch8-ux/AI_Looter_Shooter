#pragma once

// What the Main 4 tests ("Hallowed Ground": HallowedGroundTests.cpp, ChapelTests.cpp) build in their test levels: Main 4 as
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

	/** Main 4's steps: the way up, the yard, the bell, the Reliquary, Aldana. */
	inline constexpr int32 MainFourSteps = 5;

	/**
	 * The yard round the chapel, in the chapel's frame (cm; build_area_chapel.py): its creatures' spots (YARD_SPOTS) and the
	 * churchyard's iron fence round it (the layout's churchyardFence). A test level's spawner stands at the origin, unturned.
	 */
	inline const FVector YardSpots[] = { FVector(800.0, 900.0, 0.0), FVector(800.0, -700.0, 0.0), FVector(850.0, 1500.0, 0.0),
		FVector(850.0, -1450.0, 0.0), FVector(100.0, 560.0, 0.0), FVector(-450.0, 560.0, 0.0), FVector(-950.0, 0.0, 0.0),
		FVector(-1000.0, 500.0, 0.0), FVector(-1000.0, -500.0, 0.0), FVector(150.0, -600.0, 0.0), FVector(-750.0, -700.0, 0.0) };
	inline const FVector Fence[] = { FVector(1000.0, -1700.0, 0.0), FVector(1000.0, 1700.0, 0.0), FVector(-1200.0, 1700.0, 0.0),
		FVector(-1200.0, -1700.0, 0.0) };

	/** One of Main 4's steps as the mission script makes it (create_mission_assets.py), added to Mission. */
	inline void AddStep(UMissionDefinition* Mission, int32 Step)
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
			Yard->Count = 13;
			break;
		}
		case 2:
		{
			UMissionInteractObjective* Bell = MissionTestWorld::AddObjective<UMissionInteractObjective>(Mission, 2);
			Bell->Target.ActorTag = AChapelBell::BellTag;
			Bell->bHold = true;
			break;
		}
		case 3:
		{
			UMissionEventObjective* Sight = MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 3);
			Sight->Event = AChapelReliquary::SightEvent;
			Sight->Waypoint = EMissionWaypoint::Actor;
			Sight->WaypointActor.ActorTag = AChapelReliquary::ReliquaryTag;
			break;
		}
		default:
			MissionTestWorld::AddObjective<UMissionTalkObjective>(Mission, 4)->SpeakerTag = AldanaTag;
			break;
		}
	}

	/** A stand-in for Main 3: one event finishes it. */
	inline UMissionDefinition* MakeMainThree(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main3"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.MainThreeDone");
		return Mission;
	}

	/** Main 4 as the mission script makes it, in code: after Main 3, its five steps, 30% of a level. */
	inline UMissionDefinition* MakeMainFour(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main4"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		Mission->Prerequisites = { MainThree };
		for (int32 Step = 0; Step < MainFourSteps; ++Step)
		{
			AddStep(Mission, Step);
		}
		Mission->Rewards.ExperienceShare = 0.3f;
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
				AddStep(Mission, Index);
				continue;
			}
			MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, Index)->Event = FName(*FString::Printf(TEXT("Test.Step%d"), Index));
		}
		return Mission;
	}

	/** The yard's fight as the build script sets it up: two waves of six, the Restless one with the second, on Main 4's second step. */
	inline void SetUpYard(AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = YardId;
		FEncounterGroup Restless = EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 1, ECreatureRank::Rare);
		Restless.FirstWave = 2;
		Restless.LastWave = 2;
		Setup.Groups = { EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 6), Restless };
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

	/** The Reliquary as the build script places it (its foot at Foot): looked at from Main 4's fourth step on (from 0: 3). */
	inline AChapelReliquary* SpawnReliquary(UWorld* World, const FVector& Foot)
	{
		AChapelReliquary* Reliquary = World->SpawnActor<AChapelReliquary>(Foot, FRotator::ZeroRotator);
		if (Reliquary)
		{
			Reliquary->LookWhen.DuringMission = MainFour;
			Reliquary->LookWhen.FromStep = 3;
			Reliquary->DispatchBeginPlay();
		}
		return Reliquary;
	}

	// --- Father Aldana ---

	/** His words at the vestry door (Docs/Areas/RansomsRest.md, Main 4), in order. */
	inline const TCHAR* const AldanaWords[] = {
		TEXT("A keeper's lantern can find an ember."),
		TEXT("In every town they robbed, the gang shot the keeper first and smashed his lantern."),
		TEXT("Your father's fell in the dark, whole."),
		TEXT("Spiders hoard anything a saint has touched."),
		TEXT("Look in the Sink."),
	};
	inline constexpr int32 AldanaWordCount = static_cast<int32>(UE_ARRAY_COUNT(AldanaWords));

	/** His other words, the story script's first drafts (create_story_lines.py): barred, waiting, after Main 4. */
	inline const TCHAR* const AldanaBarred = TEXT("Stay back from that door. I've no comfort left for the dead, and the yard is full of them.");
	inline const TCHAR* const AldanaWaiting = TEXT("They're quiet. Ring her bell, then look at what was done to her. Then we'll talk.");
	inline const TCHAR* const AldanaAfter = TEXT("The Sink, Ellis. Your father's lantern. I'll pray it's still whole.");

	inline FStoryLine Said(const TCHAR* Speaker, const TCHAR* Words, float Seconds = 3.f)
	{
		return FStoryLine::Make(FText::FromString(Speaker), FText::FromString(Words), Seconds);
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

	/**
	 * His vestry door as the build script sets it up (not begun): barred, his words at Main 4's last step (topic 0), the bell
	 * and the Reliquary once the yard is quiet (topic 1), the Sink after (topic 2); from the line sets when they're made,
	 * else their words in code.
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
		const UStoryLineSet* MainSet = LoadLines(TEXT("DA_Lines_AldanaMain4"));
		const UStoryLineSet* WaitingSet = LoadLines(TEXT("DA_Lines_AldanaWaiting"));
		const UStoryLineSet* AfterSet = LoadLines(TEXT("DA_Lines_AldanaAfterMain4"));
		Talk->Lines = BarredSet ? BarredSet->Lines : TArray<FStoryLine>({ Said(TEXT(""), AldanaBarred) });
		FSpeakerTopic Doc;
		Doc.When.DuringMission = MainFour;
		Doc.When.FromStep = 4;
		if (MainSet)
		{
			Doc.Lines = MainSet->Lines;
		}
		else
		{
			Doc.Lines = { Said(TEXT(""), TEXT("You rang her bell, and she didn't answer. Now you've seen why.")),
				Said(TEXT("Ellis"), TEXT("How do I find them?")) };
			for (const TCHAR* Words : AldanaWords)
			{
				Doc.Lines.Add(Said(TEXT(""), Words));
			}
		}
		FSpeakerTopic Quiet;
		Quiet.When.DuringMission = MainFour;
		Quiet.When.FromStep = 2;
		Quiet.Lines = WaitingSet ? WaitingSet->Lines : TArray<FStoryLine>({ Said(TEXT(""), AldanaWaiting) });
		FSpeakerTopic Later;
		Later.When.AfterMissions = { MainFour };
		Later.Lines = AfterSet ? AfterSet->Lines : TArray<FStoryLine>({ Said(TEXT(""), AldanaAfter) });
		Talk->Topics = { Doc, Quiet, Later };
		Door->Tags.Add(AldanaTag);
		return Door;
	}
}

#endif
