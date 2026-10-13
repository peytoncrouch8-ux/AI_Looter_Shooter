#pragma once

// What the ore lift's tests build in their test levels (OreLiftTests.cpp, OreLiftRideTests.cpp): a lift on the design's
// line, Dead Lift as a test mission, a rider, and the scenes forced on.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "World/OreLift.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace OreLiftTestWorld
{
	inline constexpr float Frame = 1.f / 30.f;

	/** Dead Lift's id, and its step that rides the lift up (from 0), where the lift opens. */
	inline const FName DeadLiftId(TEXT("DeadLift"));
	inline constexpr int32 RideStep = 3;

	inline const FName QuarryArea(TEXT("TestQuarry"));

	/** Looter.Scenes set for a test (2: on, as in a game), and put back after it. */
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

	/** A test level with an ore lift at its origin (the design's line, its stand-in cage), Dead Lift as a test mission and a rider. */
	struct FLiftLevel
	{
		/** The story the runner plays against: it outlives the level. */
		FCampaignRecord Campaign;
		FTestWorldWrapper Level;
		UWorld* World = nullptr;
		AOreLift* Lift = nullptr;
		USceneSubsystem* Scenes = nullptr;
		UMissionRunner* Runner = nullptr;
		AActor* Rider = nullptr;
		TArray<FName> Heard;
		FDelegateHandle Listening;
		float Clock = 0.f;

		~FLiftLevel()
		{
			if (Runner)
			{
				Runner->OnEvent.Remove(Listening);
			}
		}

		/** Dead Lift at Step (INDEX_NONE: not taken); the rider a training dummy (it can die) or a plain stand-in. */
		bool Build(int32 Step, bool bDummyRider = false)
		{
			if (!Level.CreateTestWorld(EWorldType::EditorPreview))
			{
				return false;
			}
			World = Level.GetTestWorld();
			Scenes = World->GetSubsystem<USceneSubsystem>();
			Runner = World->GetSubsystem<UMissionRunner>();
			Lift = World->SpawnActor<AOreLift>(FVector::ZeroVector, FRotator::ZeroRotator);
			const FVector Beside(-250.0, 0.0, 100.0);
			Rider = bDummyRider ? static_cast<AActor*>(MissionTestWorld::SpawnDummy(World, Beside)) : MissionTestWorld::SpawnMarker(World, Beside);
			if (!Scenes || !Runner || !Lift || !Rider)
			{
				return false;
			}
			Lift->DispatchBeginPlay();
			// Dead Lift: three steps that never finish by themselves, then the ride (Board.Lift).
			UPackage* Scratch = CreatePackage(nullptr);
			UMissionDefinition* DeadLift = MissionTestWorld::NewMission(Scratch, TEXT("DeadLift"), EMissionKind::Side, EMissionStart::Manual, QuarryArea);
			for (int32 Each = 0; Each < RideStep; ++Each)
			{
				MissionTestWorld::AddObjective<UMissionEventObjective>(DeadLift, Each)->Event = TEXT("Test.NeverHappens");
			}
			MissionTestWorld::AddObjective<UMissionBoardObjective>(DeadLift, RideStep)->Vehicle = TEXT("Lift");
			Runner->BeginForTesting({ DeadLift }, Campaign, Rider, QuarryArea);
			if (Step != INDEX_NONE)
			{
				Runner->StartMission(DeadLiftId, Step, /*bForce*/ true);
			}
			Listening = Runner->OnEvent.AddLambda([this](const FMissionEvent& Event) { Heard.Add(Event.Name); });
			return true;
		}

		/** The level's scenes and the lift for Seconds, in frames. */
		void Tick(float Seconds)
		{
			for (float Done = 0.f; Done < Seconds - KINDA_SMALL_NUMBER; Done += Frame)
			{
				const float Delta = FMath::Min(Frame, Seconds - Done);
				Clock += Delta;
				Scenes->Tick(Delta);
				Lift->Advance(Delta);
			}
		}

		/** Ticks until no scene plays and the cage is still (at most Limit s); how long that took. */
		float Settle(float Limit)
		{
			const float Start = Clock;
			while ((Scenes->IsPlaying() || Lift->IsMoving()) && Clock - Start < Limit)
			{
				Tick(Frame);
			}
			return Clock - Start;
		}

		bool HasHeard(const TCHAR* Name) const
		{
			return Heard.Contains(FName(Name));
		}

		/** The rider stands on End's landing, in the cage's place there (where a save would find them). */
		bool IsRiderOn(EOreLiftEnd End) const
		{
			const FVector Spot = Lift->GetRiderSpot(End);
			const FVector At = Rider->GetActorLocation();
			return FVector::Dist2D(Spot, At) < 5.0 && At.Z >= Spot.Z && At.Z < Spot.Z + 250.0;
		}

		/** The cage rests at End's landing. */
		bool IsCageAt(EOreLiftEnd End) const
		{
			return Lift->GetCageEnd() == End && !Lift->IsMoving() && Lift->GetCageTransform().Equals(Lift->GetCageRest(End), 0.5);
		}
	};
}

#endif
