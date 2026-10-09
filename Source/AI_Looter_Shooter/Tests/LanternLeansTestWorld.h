#pragma once

// What the Main 7 tests ("The Lantern Leans": LanternLeansTests.cpp, TrainTests.cpp) build in their test levels: Main 7 as
// the mission script makes it (Tools/Unreal/create_mission_assets.py), a stand-in for Main 6, the area the Lily is, and
// Delia's door, its screen door and the hand-off as the build script sets them up (Tools/Unreal/build_area_depot.py); and
// the ids, tags and words they're found by.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/StationBoard.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Story/DoorHandoff.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLine.h"
#include "Tests/MissionTestWorld.h"
#include "World/TrainStation.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"

namespace LanternLeansTestWorld
{
	// The ids and tags Main 7 finds its pieces by (create_mission_assets.py, build_area_depot.py, create_area_assets.py).
	inline const FName MainSix(TEXT("Main6"));
	inline const FName MainSeven(TEXT("Main7"));
	inline const FName LilyId(TEXT("GildedLily"));
	inline const FName DeliasDoor(TEXT("Speaker_Delia"));
	inline const FName DepotPlace(TEXT("Place_Depot"));
	inline const FName TillysWindow(TEXT("Speaker_Tilly"));
	inline const FName HeirloomId(TEXT("Heirloom"));
	inline const FName Valley(TEXT("TestValley"));

	/**
	 * Main 7's steps: home to Delia; then it's turned in to Tilly at her window, on the way to the depot (the depot and the
	 * station board come after it, once the Lily is open on the board).
	 */
	inline constexpr int32 MainSevenSteps = 1;

	/** How close to the hearse car's door counts as at the depot (cm, the mission script's). */
	inline constexpr float DepotRadius = 800.f;

	/** Delia's line as the doc gives it, as her door says it in four captions. */
	inline const TCHAR* const DeliasWords[] = {
		TEXT("A keeper's buried with his lantern, not his iron."),
		TEXT("His lantern wasn't on him, so I kept this back."),
		TEXT("He'd want you to have it."),
		TEXT("Hold the door."),
	};

	/** Hob's line as the lantern leans (the doc's). */
	inline const TCHAR* HobsLean = TEXT("That's Purcell. The Lily's moored out that way.");

	/** A stand-in for Main 6: one event finishes it. */
	inline UMissionDefinition* MakeMainSix(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main6"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.MainSixDone");
		return Mission;
	}

	/**
	 * Main 7 as the mission script makes it, in code: after Main 6; Delia's door (Talk); turned in to Tilly at her window;
	 * 45 experience, the Lily opened, Heirloom handed over in the story (not dropped).
	 */
	inline UMissionDefinition* MakeMainSeven(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main7"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		Mission->Title = FText::FromString(TEXT("The Lantern Leans"));
		Mission->Prerequisites = { MainSix };
		MissionTestWorld::AddObjective<UMissionTalkObjective>(Mission, 0)->SpeakerTag = DeliasDoor;
		Mission->TurnIn.SpeakerTag = TillysWindow;
		Mission->TurnIn.GiverName = FText::FromString(TEXT("Tilly"));
		Mission->Rewards.Experience = 45;
		Mission->Rewards.UnlockAreas = { LilyId };
		Mission->Rewards.NamedGun = HeirloomId;
		Mission->Rewards.bNamedGunByHand = true;
		return Mission;
	}

	/** An area made in code, in a scratch package so it never meets the project's assets; its id is its asset name's end. */
	inline UAreaDefinition* NewArea(UObject* Outer, const TCHAR* AssetName, const TCHAR* Name, const TCHAR* Map, const TCHAR* Landing, bool bPractice)
	{
		UAreaDefinition* Area = NewObject<UAreaDefinition>(Outer, AssetName, RF_Transient);
		Area->DisplayName = FText::FromString(Name);
		Area->Map = FSoftObjectPath(FString::Printf(TEXT("%s.%s"), Map, *FPackageName::GetShortName(Map)));
		Area->Landings = { FName(Landing) };
		Area->bPractice = bPractice;
		return Area;
	}

	/** The Lily as the area script makes it: no level yet, "the Lily" in a sentence. */
	inline UAreaDefinition* NewLily(UObject* Outer)
	{
		UAreaDefinition* Lily = NewArea(Outer, TEXT("DA_Area_GildedLily"), TEXT("The Gilded Lily"), TEXT("/Game/Maps/Lvl_GildedLily"),
			TEXT("Landing_Platform"), false);
		Lily->SpokenName = FText::FromString(TEXT("the Lily"));
		return Lily;
	}

	/** Delia's door placed as the build script places it: its point at the actor's spot, facing Facing; her line on it. */
	inline ASpeakerPoint* PlaceDeliasDoor(UWorld* World, const FVector& Where, double Facing)
	{
		ASpeakerPoint* Door = World->SpawnActor<ASpeakerPoint>(Where, FRotator(0.0, Facing, 0.0));
		if (!Door)
		{
			return nullptr;
		}
		Door->SpeakerPoint->SetRelativeLocation(FVector::ZeroVector);
		Door->SpeakerPoint->SpeakerName = FText::FromString(TEXT("Grandma Delia"));
		for (const TCHAR* Words : DeliasWords)
		{
			Door->SpeakerPoint->Lines.Add(FStoryLine::Make(FText::GetEmpty(), FText::FromString(Words)));
		}
		Door->Tags.Add(DeliasDoor);
		Door->DispatchBeginPlay();
		return Door;
	}

	/**
	 * The hand-off at Where (the farmhouse's SOCKET_Handoff), facing out along Facing, with ScreenDoor to swing, Main 7's
	 * first step, not begun.
	 */
	inline ADoorHandoff* PlaceHandoff(UWorld* World, const FVector& Where, double Facing, AActor* ScreenDoor)
	{
		ADoorHandoff* Handoff = World->SpawnActor<ADoorHandoff>(Where, FRotator(0.0, Facing, 0.0));
		if (Handoff)
		{
			Handoff->Door = ScreenDoor;
			Handoff->Mission = MainSeven;
			Handoff->Step = 0;
		}
		return Handoff;
	}

	/** Looter.Scenes set for a test (2: scenes play even in a test run), and put back after it. */
	class FScenesOn
	{
	public:
		FScenesOn()
			: Variable(IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Scenes")))
		{
			if (Variable)
			{
				Previous = Variable->GetInt();
				Variable->Set(2, ECVF_SetByConsole);
			}
		}

		~FScenesOn()
		{
			if (Variable)
			{
				Variable->Set(Previous, ECVF_SetByConsole);
			}
		}

	private:
		IConsoleVariable* Variable = nullptr;
		int32 Previous = 1;
	};
}

#endif
