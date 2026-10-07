#pragma once

// What the Main 5 tests ("The Keeper's Lantern": EggSacTests.cpp, KeepersLanternTests.cpp) build in their test levels: Main 5
// as the mission script makes it (Tools/Unreal/create_mission_assets.py), the egg sacs, the lantern in the webbing and the
// floor's spiders as the build script sets them up (Tools/Unreal/build_area_sink.py), round a Sink whose floor is the test
// level's z = 0 and whose middle is its origin; and the ids, tags and words they're found by.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureRank.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/HuntingGround.h"
#include "Creatures/SpiderCreature.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Story/StoryCondition.h"
#include "Tests/MissionTestWorld.h"
#include "World/EggSac.h"
#include "World/KeepersLantern.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "UObject/Script.h"

namespace KeepersLanternTestWorld
{
	// The ids and tags Main 5 finds its pieces by (create_mission_assets.py, build_area_sink.py; the sacs' and the
	// lantern's are their classes' own).
	inline const FName MainFour(TEXT("Main4"));
	inline const FName MainFive(TEXT("Main5"));
	inline const FName FloorPlace(TEXT("Place_SinkFloor"));
	inline const FName RimPlace(TEXT("Place_SinkRim"));
	inline const FName FloorId(TEXT("SinkFloor"));
	inline const FName FloorTag(TEXT("Spider_SinkFloor"));
	inline const FName SacSpiderTag(TEXT("Spider_EggSac"));
	inline const FName Valley(TEXT("TestValley"));

	/** Main 5's steps: down into the Sink, the egg sacs, the lantern, out. */
	inline constexpr int32 MainFiveSteps = 4;

	/** The floor's spiders: how many, and how far above their spawner a player is still on their ground (cm). */
	inline constexpr int32 FloorSpiders = 4;
	inline constexpr float FloorRise = 250.f;

	/** How close counts as on the floor, and as out at the rim (cm, height counted: the mission script's). */
	inline constexpr float FloorRadius = 900.f;
	inline constexpr float RimRadius = 600.f;

	/** The Sink's floor as the layout's zone draws it, round its middle (cm): twelve corners 18 m out. */
	inline TArray<FVector> FloorCorners()
	{
		TArray<FVector> Corners;
		for (int32 Corner = 0; Corner < 12; ++Corner)
		{
			const double Angle = FMath::DegreesToRadians(30.0 * Corner);
			Corners.Add(FVector(1800.0 * FMath::Cos(Angle), 1800.0 * FMath::Sin(Angle), 0.0));
		}
		return Corners;
	}

	/** The floor's spiders' spots among the blocks, round the Sink's middle (cm; build_area_sink.py's FLOOR_SPOTS). */
	inline const FVector FloorSpots[] = { FVector(350.0, -450.0, 0.0), FVector(1150.0, -450.0, 0.0), FVector(550.0, 100.0, 0.0),
		FVector(1300.0, 300.0, 0.0) };

	/** Their hunting ground and their spiders' as the build script sets it: the floor, and 2.5 m up the ramp's foot. */
	inline FHuntingGround MakeFloorGround()
	{
		FHuntingGround Ground;
		Ground.Corners = FloorCorners();
		Ground.Margin = 200.f;
		Ground.MaxRise = FloorRise;
		Ground.bAroundHome = true;
		return Ground;
	}

	/** One of Main 5's steps as the mission script makes it, added to Mission. */
	inline void AddStep(UMissionDefinition* Mission, int32 Step)
	{
		switch (Step)
		{
		case 0:
		{
			UMissionReachObjective* Down = MissionTestWorld::AddObjective<UMissionReachObjective>(Mission, 0);
			Down->Place.Actor.ActorTag = FloorPlace;
			Down->Place.Radius = FloorRadius;
			Down->Place.bIgnoreHeight = false;
			break;
		}
		case 1:
		{
			UMissionEventObjective* Sacs = MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 1);
			Sacs->Event = AEggSac::BurstEvent;
			Sacs->Count = 3;
			Sacs->Waypoint = EMissionWaypoint::Actor;
			Sacs->WaypointActor.ActorTag = AEggSac::EggSacTag;
			break;
		}
		case 2:
			MissionTestWorld::AddObjective<UMissionInteractObjective>(Mission, 2)->Target.ActorTag = AKeepersLantern::LanternTag;
			break;
		default:
		{
			UMissionReachObjective* Out = MissionTestWorld::AddObjective<UMissionReachObjective>(Mission, 3);
			Out->Place.Actor.ActorTag = RimPlace;
			Out->Place.Radius = RimRadius;
			Out->Place.bIgnoreHeight = false;
			break;
		}
		}
	}

	/** A stand-in for Main 4: one event finishes it. */
	inline UMissionDefinition* MakeMainFour(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main4"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.MainFourDone");
		return Mission;
	}

	/** Main 5 as the mission script makes it, in code: after Main 4, its four steps, 30% of a level. */
	inline UMissionDefinition* MakeMainFive(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main5"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		Mission->Prerequisites = { MainFour };
		for (int32 Step = 0; Step < MainFiveSteps; ++Step)
		{
			AddStep(Mission, Step);
		}
		Mission->Rewards.ExperienceShare = 0.3f;
		return Mission;
	}

	/** Shot at only in Main 5's second step (from 0: 1); down from the start from its third on, and after it. */
	inline void SetUpSacStory(AEggSac& Sac)
	{
		Sac.ShootableWhen = FStoryCondition();
		Sac.ShootableWhen.DuringMission = MainFive;
		Sac.ShootableWhen.FromStep = 1;
		Sac.ShootableWhen.BeforeStep = 2;
		FStoryCondition Past;
		Past.DuringMission = MainFive;
		Past.FromStep = 2;
		FStoryCondition After;
		After.AfterMissions = { MainFive };
		Sac.DownWhen = { Past, After };
	}

	/**
	 * An egg sac hanging with its pivot (its bottom) at Where, facing Yaw, as the build script sets it up: its story, its
	 * spiders' ground and tag, its drop to the floor (z = 0; a test level's traces find no ground), begun.
	 */
	inline AEggSac* SpawnSac(UWorld* World, const FVector& Where, double Yaw, float BurstOut = 0.f)
	{
		AEggSac* Sac = World->SpawnActor<AEggSac>(Where, FRotator(0.0, Yaw, 0.0));
		if (!Sac)
		{
			return nullptr;
		}
		SetUpSacStory(*Sac);
		Sac->SpiderGround = MakeFloorGround();
		Sac->SpiderTags = { SacSpiderTag };
		Sac->DropHeight = static_cast<float>(Where.Z);
		Sac->BurstOut = BurstOut;
		Sac->DispatchBeginPlay();
		return Sac;
	}

	/** A shot as the game's damage deals it (AActor::TakeDamage, its listeners let run as play lets them). */
	inline float Shoot(AEggSac* Sac, float Damage, AController* By = nullptr)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		return Sac ? Sac->TakeDamage(Damage, FDamageEvent(UDamageType::StaticClass()), By, nullptr) : 0.f;
	}

	/** Lets a falling sac drop until it has landed (at most four seconds). */
	inline void FallAll(AEggSac* Sac)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		for (int32 Frame = 0; Sac && Frame < 80 && Sac->GetState() == EEggSacState::Falling; ++Frame)
		{
			Sac->Advance(0.05f);
		}
	}

	/** The lantern in the webbing as the build script sets it up (taken in Main 5's third step, from 0: 2), not begun. */
	inline AKeepersLantern* PlaceLantern(UWorld* World, const FVector& Where, double Yaw)
	{
		AKeepersLantern* Lantern = World->SpawnActor<AKeepersLantern>(Where, FRotator(0.0, Yaw, 0.0));
		if (!Lantern)
		{
			return nullptr;
		}
		Lantern->TakeWhen = FStoryCondition();
		Lantern->TakeWhen.DuringMission = MainFive;
		Lantern->TakeWhen.FromStep = 2;
		Lantern->TakeWhen.BeforeStep = 3;
		return Lantern;
	}

	/** Moves Lantern so the point it's taken at (its middle) lands on Point. */
	inline void MoveLanternTo(AKeepersLantern* Lantern, const FVector& Point)
	{
		const TOptional<FVector> At = Lantern ? Lantern->GetInteractionLocation() : TOptional<FVector>();
		if (At.IsSet())
		{
			Lantern->SetActorLocation(Lantern->GetActorLocation() + (Point - At.GetValue()));
		}
	}

	/** The floor's spiders as the build script sets them up: four of the area's ranks among the blocks, during Main 5 up to the lantern. */
	inline void SetUpFloor(AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = FloorId;
		FEncounterGroup Spiders;
		Spiders.CreatureClass = ASpiderCreature::StaticClass();
		Spiders.Count = FloorSpiders;
		Spiders.RankRoll = EEncounterRankRoll::Area;
		Setup.Groups = { Spiders };
		Setup.CreatureTags = { FloorTag };
		Setup.ActiveWhen.DuringMission = MainFive;
		Setup.ActiveWhen.BeforeStep = 3;
		Setup.bHuntOnSpawn = false;
		Setup.SpawnPoints = TArray<FVector>(FloorSpots, UE_ARRAY_COUNT(FloorSpots));
		Setup.GroundCorners = FloorCorners();
		Setup.GroundMaxRise = FloorRise;
	}
}

#endif
