#pragma once

// What the Main 6 tests ("The Gravewind": GravewindTests.cpp) build in their test levels: Main 6 as the mission script makes
// it (Tools/Unreal/create_mission_assets.py), Delia's door with its Main 6 topic, the keeper's post and the lighting as the
// build script sets them up (Tools/Unreal/build_area_deck.py, build_area_farm.py), a level lit in Day with a Dusk to
// switch to; and the ids, tags and words they're found by.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelKeeper.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Scenes/SitWithPa.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
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
	// The ids, tags and events Main 6 finds its pieces by (create_mission_assets.py, build_area_deck.py, build_area_farm.py;
	// the boss's and the post's are their classes' own).
	inline const FName MainFive(TEXT("Main5"));
	inline const FName MainSix(TEXT("Main6"));
	inline const FName PointPlace(TEXT("Place_GravewindPoint"));
	inline const FName DeliaTag(TEXT("Speaker_Delia"));
	inline const FName DeliaEvent(TEXT("Delia.Main6"));
	inline const FName KeepersGrave(TEXT("KeepersGrave"));
	inline const FName Dusk(TEXT("Dusk"));
	inline const FName Valley(TEXT("TestValley"));

	/** Main 6's steps: to Gravewind Point, the lantern hung, Abel defeated, sitting with Pa. */
	inline constexpr int32 MainSixSteps = 4;

	/** How close to the Keeper's Gate counts as at Gravewind Point (cm, on the map). */
	inline constexpr float PointRadius = 1000.f;

	/** Delia's line through the door that starts it (the doc's words). */
	inline const TCHAR* DeliaLine = TEXT("Take him the lantern. Show him the way, even if he can't go.");

	/** A stand-in for Main 5: one event finishes it. */
	inline UMissionDefinition* MakeMainFive(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main5"), EMissionKind::Main, EMissionStart::Automatic, Valley);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.MainFiveDone");
		return Mission;
	}

	/** Main 6 as the mission script makes it, in code: after Main 5, started at Delia's door, its four steps, 30% of a level. */
	inline UMissionDefinition* MakeMainSix(UObject* Outer)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("Main6"), EMissionKind::Main, EMissionStart::OnEvent, Valley);
		Mission->Prerequisites = { MainFive };
		Mission->StartEvent = DeliaEvent;
		UMissionReachObjective* Point = MissionTestWorld::AddObjective<UMissionReachObjective>(Mission, 0);
		Point->Place.Actor.ActorTag = PointPlace;
		Point->Place.Radius = PointRadius;
		MissionTestWorld::AddObjective<UMissionInteractObjective>(Mission, 1)->Target.ActorTag = AKeeperLanternPost::KeepersPostTag;
		MissionTestWorld::AddObjective<UMissionKillNamedObjective>(Mission, 2)->ActorTag = AAbelKeeper::BossTag;
		MissionTestWorld::AddObjective<UMissionSceneObjective>(Mission, 3)->Scene = SitWithPa::SceneName();
		Mission->Rewards.ExperienceShare = 0.3f;
		return Mission;
	}

	/** The keeper's post's story as the build script sets it: hung in Main 6's second step, there after it, lit after Main 6. */
	inline void SetUpKeepersPost(AKeeperLanternPost& Post)
	{
		Post.HangWhen = FStoryCondition();
		Post.HangWhen.DuringMission = MainSix;
		Post.HangWhen.FromStep = AAbelKeeper::HangStep;
		Post.HangWhen.BeforeStep = AAbelKeeper::FightStep;
		FStoryCondition During;
		During.DuringMission = MainSix;
		During.FromStep = AAbelKeeper::FightStep;
		FStoryCondition After;
		After.AfterMissions = { MainSix };
		Post.HungWhen = { During, After };
		Post.LitWhen = After;
	}

	/** Delia's screen door with her Main 6 topic (after Main 5, until Main 6 is done), whose event starts it. */
	inline ASpeakerPoint* PlaceDeliasDoor(UWorld* World, const FVector& Where)
	{
		ASpeakerPoint* Door = World->SpawnActor<ASpeakerPoint>(Where, FRotator::ZeroRotator);
		if (!Door)
		{
			return nullptr;
		}
		Door->SpeakerPoint->SetRelativeLocation(FVector::ZeroVector);
		Door->SpeakerPoint->SpeakerName = FText::FromString(TEXT("Grandma Delia"));
		Door->SpeakerPoint->Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("There's a plate on the porch."))) };
		FSpeakerTopic Topic;
		Topic.When.AfterMissions = { MainFive };
		Topic.When.BeforeMissions = { MainSix };
		Topic.Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(DeliaLine)) };
		Topic.Event = DeliaEvent;
		Door->SpeakerPoint->Topics = { Topic };
		Door->Tags.Add(DeliaTag);
		Door->DispatchBeginPlay();
		return Door;
	}

	/** A level lit in Day (its lights placed and found) with a Dusk to switch to, as build_area_environment.py places them. */
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
		Sun->GetComponent()->SetMobility(EComponentMobility::Movable);
		Sun->GetComponent()->SetAtmosphereSunLight(true);
		Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		Post->bUnbound = true;
		FLightingState Day;
		FLightingState Evening;
		Evening.Name = Dusk;
		Evening.SunBearing = 252.f;
		Evening.SunElevation = 4.f;
		States->States = { Day, Evening };
		States->Sun = Sun;
		States->SkyLight = Sky;
		States->Atmosphere = Air;
		States->HeightFog = Fog;
		States->PostVolume = Post;
		FLightingTargets::Find(nullptr, States).Write(Day);
		return States;
	}

	/** The story's light as the build script sets it: Main 6 (and Main 7, which goes on at dusk) at Dusk. */
	inline AStoryLighting* SpawnStoryLighting(UWorld* World)
	{
		AStoryLighting* Light = World->SpawnActor<AStoryLighting>();
		if (Light)
		{
			FStoryLightingRule DuringSix;
			DuringSix.When.DuringMission = MainSix;
			DuringSix.State = Dusk;
			Light->Rules = { DuringSix };
		}
		return Light;
	}
}

#endif
