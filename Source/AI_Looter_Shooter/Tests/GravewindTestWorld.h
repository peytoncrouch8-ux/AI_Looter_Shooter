#pragma once

// What the Gravewind tests (GravewindTests.cpp: Abel's fight on the burial boards, Main 7 "Given to the Wind" since
// 2026-11-01; it was Main 6 "The Gravewind") build in their test levels: Main 7's fight steps as the mission script makes them
// (Tools/Unreal/create_mission_assets.py; its carry and Reliquary steps are stand-ins here: lane W5 tests them), the keeper's
// post and the lighting as the build script sets them up (Tools/Unreal/build_area_deck.py), a level lit in Day with a
// DuskSmog and a Dusk to switch to; and the ids, tags and words they're found by.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelKeeper.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Scenes/FreeAbel.h"
#include "Story/StoryCondition.h"
#include "Tests/MissionTestWorld.h"
#include "World/KeeperLanternPost.h"
#include "World/LightingState.h"
#include "World/LightingStates.h"
#include "World/LightingTargets.h"
#include "World/StoryLighting.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "Components/SkyAtmosphereComponent.h"

namespace GravewindTestWorld
{
	// The ids, tags and events Main 7 finds its pieces by (create_mission_assets.py, build_area_deck.py; the boss's and the
	// post's are their classes' own).
	inline const FName MainSix(TEXT("Main6"));
	inline const FName MainSeven(TEXT("Main7"));
	inline const FName PointPlace(TEXT("Place_GravewindPoint"));
	inline const FName ChapelPlace(TEXT("Place_Chapel"));
	inline const FName StartEvent(TEXT("Test.MainSevenOffered"));
	inline const FName ReliquaryEvent(TEXT("Test.ReliquaryDone"));
	inline const FName KeepersGrave(TEXT("KeepersGrave"));
	inline const FName Dusk(TEXT("Dusk"));
	inline const FName DuskSmog(TEXT("DuskSmog"));
	inline const FName Valley(TEXT("TestValley"));

	/** Main 7's steps: the point, the lantern hung, Abel freed, the scene, the carry home, the Reliquary. */
	inline constexpr int32 MainSevenSteps = 6;
	inline constexpr int32 CarryStep = 4;

	/** How close to the Keeper's Gate counts as at Gravewind Point (cm, on the map). */
	inline constexpr float PointRadius = 1000.f;

	/** A stand-in for Main 6: one event finishes it. */
	inline UMissionDefinition* MakeMainSix(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main6"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.MainSixDone");
		return Mission;
	}

	/**
	 * Main 7 as the mission script makes it, in code: after Main 6, started by an event here (Holloway's word on the move in
	 * the game), the point, the lantern hung, Abel freed (any kill), the scene, then stand-ins for the carry and the
	 * Reliquary (lane W5's: Place_Chapel reached, an event).
	 */
	inline UMissionDefinition* MakeMainSeven(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main7"), EMissionKind::Main, EMissionStart::OnEvent, Valley);
		Mission->Prerequisites = { MainSix };
		Mission->StartEvent = StartEvent;
		UMissionReachObjective* Point = MissionTestWorld::AddObjective<UMissionReachObjective>(Mission, 0);
		Point->Place.Actor.ActorTag = PointPlace;
		Point->Place.Radius = PointRadius;
		MissionTestWorld::AddObjective<UMissionInteractObjective>(Mission, 1)->Target.ActorTag = AKeeperLanternPost::KeepersPostTag;
		UMissionKillNamedObjective* Free = MissionTestWorld::AddObjective<UMissionKillNamedObjective>(Mission, 2);
		Free->ActorTag = AAbelKeeper::BossTag;
		Free->bPlayerKillsOnly = false;
		MissionTestWorld::AddObjective<UMissionSceneObjective>(Mission, 3)->Scene = FreeAbel::SceneName();
		UMissionReachObjective* Chapel = MissionTestWorld::AddObjective<UMissionReachObjective>(Mission, 4);
		Chapel->Place.Actor.ActorTag = ChapelPlace;
		Chapel->Place.Radius = 1500.f;
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 5)->Event = ReliquaryEvent;
		Mission->Rewards.ExperienceShare = 0.3f;
		return Mission;
	}

	/**
	 * The keeper's post's story as build_area_deck.py sets it: hung in Main 7's second step, there through the fight and the
	 * scene, gone from the hook from the carry step (and after Main 7: at the Reliquary).
	 */
	inline void SetUpKeepersPost(AKeeperLanternPost& Post)
	{
		Post.HangWhen = FStoryCondition();
		Post.HangWhen.DuringMission = MainSeven;
		Post.HangWhen.FromStep = AAbelKeeper::HangStep;
		Post.HangWhen.BeforeStep = AAbelKeeper::FightStep;
		FStoryCondition During;
		During.DuringMission = MainSeven;
		During.FromStep = AAbelKeeper::FightStep;
		During.BeforeStep = CarryStep;
		Post.HungWhen = { During };
		Post.LitWhen = FStoryCondition();
		FStoryCondition Carried;
		Carried.DuringMission = MainSeven;
		Carried.FromStep = CarryStep;
		FStoryCondition After;
		After.AfterMissions = { MainSeven };
		Post.GoneWhen = { Carried, After };
	}

	/** A level lit in Day (its lights placed and found) with a DuskSmog and a Dusk to switch to, as build_area_environment.py places them. */
	inline ALightingStates* SpawnDuskLevel(UWorld* World)
	{
		ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>();
		ASkyLight* Sky = World->SpawnActor<ASkyLight>();
		ASkyAtmosphere* Air = World->SpawnActor<ASkyAtmosphere>();
		AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>();
		APostProcessVolume* Post = World->SpawnActor<APostProcessVolume>();
		ALightingStates* States = World->SpawnActor<ALightingStates>();
		if (!Sun || !Sky || !Air || !Fog || !Post || !States)
		{
			return nullptr;
		}
		Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetMobility(EComponentMobility::Movable);
		Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetAtmosphereSunLight(true);
		Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		Post->bUnbound = true;
		FLightingState Day;
		FLightingState Evening;
		Evening.Name = Dusk;
		Evening.SunBearing = 252.f;
		Evening.SunElevation = 4.f;
		FLightingState Smoggy = Evening;
		Smoggy.Name = DuskSmog;
		States->States = { Day, Smoggy, Evening };
		States->Sun = Sun;
		States->SkyLight = Sky;
		States->Atmosphere = Air;
		States->HeightFog = Fog;
		States->PostVolume = Post;
		FLightingTargets::Find(nullptr, States).Write(Day);
		return States;
	}

	/** The story's light as build_area_deck.py sets it: Main 7 at DuskSmog until Saint Ada wakes (its step 6 on), then Dusk. */
	inline AStoryLighting* SpawnStoryLighting(UWorld* World)
	{
		AStoryLighting* Light = World->SpawnActor<AStoryLighting>();
		if (Light)
		{
			FStoryLightingRule Woken;
			Woken.When.DuringMission = MainSeven;
			Woken.When.FromStep = MainSevenSteps;
			Woken.State = Dusk;
			FStoryLightingRule DuringSeven;
			DuringSeven.When.DuringMission = MainSeven;
			DuringSeven.State = DuskSmog;
			Light->Rules = { Woken, DuringSeven };
		}
		return Light;
	}
}

#endif
