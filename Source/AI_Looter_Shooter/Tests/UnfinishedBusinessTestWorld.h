#pragma once

// What the Side 2 tests ("Unfinished Business": UnfinishedBusinessTests.cpp, AmosTests.cpp) build in their test levels:
// Side 2 as the mission script makes it, or with only one of its steps played for real; Amos at his fence, his hay bales
// and his old hired hands' fight as the build script sets them up (Tools/Unreal/build_area_whitlock.py); and the ids, tags
// and topics they're found by.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureRank.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/UnpaidCreature.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionLastingInteractObjective.h"
#include "Missions/MissionObjective.h"
#include "Story/AmosWhitlock.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLine.h"
#include "Story/StoryLineSet.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/HayBale.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

namespace UnfinishedBusinessTestWorld
{
	// The ids and tags Side 2 finds its pieces by (Tools/Unreal/create_side_mission_assets.py, build_area_whitlock.py; Amos's
	// and the bales' are their classes' own).
	inline const FName MainFour(TEXT("Main4"));
	inline const FName MainSix(TEXT("Main6"));
	inline const FName SideTwo(TEXT("Side2"));
	inline const FName HandsId(TEXT("WhitlockHands"));
	inline const FName HandsTag(TEXT("Unpaid_WhitlockHands"));
	inline const FName Valley(TEXT("TestValley"));

	/** Side 2's steps: talk to Amos, load the bales, drive off the hands; then it's turned in to Amos (that talk was the fourth). */
	inline constexpr int32 SideTwoSteps = 3;
	inline constexpr int32 BaleCount = 6;
	inline constexpr int32 HandsBasic = 5;

	/**
	 * The barn yard round the hands' spawner, as the build script places it (cm, from the spawner; build_area_whitlock.py's
	 * HANDS_AT and HANDS_SPOTS, and the layout's barnYardFence): the yard's fence, and where they rise.
	 */
	inline const FVector YardFence[] = { FVector(1900.0, -1100.0, 0.0), FVector(1900.0, 1500.0, 0.0), FVector(-900.0, 1500.0, 0.0),
		FVector(-1100.0, -1100.0, 0.0) };
	inline const FVector HandsSpots[] = { FVector(1600.0, -600.0, 0.0), FVector(1600.0, 300.0, 0.0), FVector(1600.0, 1100.0, 0.0),
		FVector(1000.0, 1150.0, 0.0), FVector(0.0, -600.0, 0.0), FVector(-700.0, -700.0, 0.0), FVector(-100.0, 100.0, 0.0),
		FVector(-700.0, 200.0, 0.0), FVector(-700.0, 1150.0, 0.0), FVector(250.0, 1150.0, 0.0) };

	/** One of Side 2's steps as the mission script makes it (create_side_mission_assets.py), added to Mission. */
	inline void AddStep(UMissionDefinition* Mission, int32 Step)
	{
		switch (Step)
		{
		case 1:
		{
			UMissionLastingInteractObjective* Load = MissionTestWorld::AddObjective<UMissionLastingInteractObjective>(Mission, 1);
			Load->Target.ActorClass = AHayBale::StaticClass();
			Load->Target.ActorTag = AHayBale::BaleTag;
			Load->Count = BaleCount;
			Load->bHold = true;
			break;
		}
		case 2:
		{
			UMissionClearObjective* Hands = MissionTestWorld::AddObjective<UMissionClearObjective>(Mission, 2);
			Hands->SpawnerId = HandsId;
			Hands->Count = HandsBasic + 1;
			break;
		}
		default:
			MissionTestWorld::AddObjective<UMissionTalkObjective>(Mission, Step)->SpeakerTag = AAmosWhitlock::SpeakerTag;
			break;
		}
	}

	/** A stand-in for Main 4: one event finishes it. */
	inline UMissionDefinition* MakeMainFour(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main4"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.MainFourDone");
		return Mission;
	}

	/** A stand-in for Main 6, started and finished by hand. */
	inline UMissionDefinition* MakeMainSix(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main6"), EMissionKind::Main, EMissionStart::Manual, Valley);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.Never");
		return Mission;
	}

	/** Side 2's turn-in, as the mission script gives it: to Amos at his fence, with his own words (his thanks). */
	inline void SetTurnIn(UMissionDefinition* Mission)
	{
		Mission->TurnIn.SpeakerTag = AAmosWhitlock::SpeakerTag;
		Mission->TurnIn.GiverName = FText::FromString(TEXT("Amos"));
	}

	/** Side 2 as the mission script makes it, in code: after Main 4, its three steps, turned in to Amos, 40 experience. */
	inline UMissionDefinition* MakeSideTwo(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Side2"), EMissionKind::Side, EMissionStart::Automatic, Valley);
		Mission->Prerequisites = { MainFour };
		for (int32 Step = 0; Step < SideTwoSteps; ++Step)
		{
			AddStep(Mission, Step);
		}
		SetTurnIn(Mission);
		Mission->Rewards.Experience = 40;
		return Mission;
	}

	/** Side 2 after Main 4 with every step a named event ("Test.Step<n>") but the one a test plays for real (Step), Side 2's own. */
	inline UMissionDefinition* MakeSideTwoAt(UObject* Outer, int32 Step)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Side2"), EMissionKind::Side, EMissionStart::Automatic, Valley);
		Mission->Prerequisites = { MainFour };
		for (int32 Index = 0; Index < SideTwoSteps; ++Index)
		{
			if (Index == Step)
			{
				AddStep(Mission, Index);
				continue;
			}
			MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, Index)->Event = FName(*FString::Printf(TEXT("Test.Step%d"), Index));
		}
		SetTurnIn(Mission);
		return Mission;
	}

	/**
	 * The hired hands' fight as the build script sets it up: five Unpaid and a Restless one in the yard, Side 2's third step.
	 * Its spots are the spawner's own (relative), its ground the yard's fence in the world, round wherever it stands.
	 */
	inline void SetUpHands(AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = HandsId;
		Setup.Groups = { EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), HandsBasic),
			EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 1, ECreatureRank::Rare) };
		Setup.CreatureTags = { HandsTag };
		Setup.ActiveWhen.DuringMission = SideTwo;
		Setup.ActiveWhen.FromStep = 2;
		Setup.ActiveWhen.BeforeStep = 3;
		Setup.SpawnPoints = TArray<FVector>(HandsSpots, UE_ARRAY_COUNT(HandsSpots));
		Setup.ActivationRadius = 2200.f;
		Setup.bHuntOnSpawn = true;
		Setup.GroundCorners.Reset();
		for (const FVector& Corner : YardFence)
		{
			Setup.GroundCorners.Add(Setup.GetActorLocation() + Corner);
		}
	}

	/** A bale lying at Field with its place in the stack at Stack, as the build script sets it (its story the class's own), begun. */
	inline AHayBale* SpawnBale(UWorld* World, const FVector& Field, const FVector& Stack, double Yaw = 0.0)
	{
		AHayBale* Bale = World->SpawnActor<AHayBale>(Field, FRotator(0.0, Yaw, 0.0));
		if (!Bale)
		{
			return nullptr;
		}
		Bale->Stacked->SetWorldLocationAndRotation(Stack, FRotator::ZeroRotator);
		Bale->DispatchBeginPlay();
		return Bale;
	}

	/** A line set the story script makes, when it's in this checkout. */
	inline const UStoryLineSet* LoadLines(const TCHAR* Name)
	{
		const FString Package = FString(TEXT("/Game/Data/Story/")) + Name;
		return FPackageName::DoesPackageExist(Package) ? LoadObject<UStoryLineSet>(nullptr, *(Package + TEXT(".") + Name)) : nullptr;
	}

	/** Amos's topics in the build script's order, each its line set when made, else one line naming it. */
	inline const TCHAR* const TopicSets[] = { TEXT("DA_Lines_AmosThanks"), TEXT("DA_Lines_AmosHands"), TEXT("DA_Lines_AmosBales"),
		TEXT("DA_Lines_AmosMeet"), TEXT("DA_Lines_AmosAfterMain6"), TEXT("DA_Lines_AmosFence") };
	enum class EAmosTopic : int32 { Thanks, Hands, Bales, Meet, AfterMainSix, Fence };

	inline FSpeakerTopic MakeTopic(const TCHAR* SetName, const FStoryCondition& When)
	{
		FSpeakerTopic Topic;
		Topic.When = When;
		if (const UStoryLineSet* Set = LoadLines(SetName))
		{
			Topic.Lines = Set->Lines;
		}
		else
		{
			Topic.Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(SetName), 3.f) };
		}
		return Topic;
	}

	/** Amos at his fence as the build script places him (not begun): shown after Main 4, on the rail after Side 2, his topics. */
	inline AAmosWhitlock* PlaceAmos(UWorld* World, const FVector& Where, double Yaw = 0.0)
	{
		AAmosWhitlock* Amos = World->SpawnActor<AAmosWhitlock>(Where, FRotator(0.0, Yaw, 0.0));
		if (!Amos)
		{
			return nullptr;
		}
		auto During = [](int32 FromStep)
		{
			FStoryCondition When;
			When.DuringMission = SideTwo;
			When.FromStep = FromStep;
			return When;
		};
		FStoryCondition AfterSide;
		AfterSide.AfterMissions = { SideTwo };
		FStoryCondition AfterSideAndSix;
		AfterSideAndSix.AfterMissions = { SideTwo, MainSix };
		Amos->SpeakerPoint->Topics = {
			MakeTopic(TopicSets[0], During(3)), MakeTopic(TopicSets[1], During(2)), MakeTopic(TopicSets[2], During(1)),
			MakeTopic(TopicSets[3], During(0)), MakeTopic(TopicSets[4], AfterSideAndSix), MakeTopic(TopicSets[5], AfterSide) };
		Amos->SpeakerPoint->Lines = Amos->SpeakerPoint->Topics[static_cast<int32>(EAmosTopic::Meet)].Lines;
		return Amos;
	}
}

#endif
