#pragma once

// What Abel's tests (AbelTests.cpp, AbelFightTests.cpp, AbelCorruptionTests.cpp, GravewindTests.cpp) build in their test
// levels: Abel on a deck whose middle is the origin and whose open end is +X (the sunset), the deck's three lantern posts,
// the six dead on the boards round the aisle, a player stand-in toward the gate (-X), and time moved on as the game moves it
// (his boss's fight, then him). (Pa's board went with Main 7's rework, 2026-11-01: he's carried home in his lantern now.)

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelCorruption.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Tests/BossTestWorld.h"
#include "World/KeeperLanternPost.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "UObject/Script.h"

namespace AbelTestWorld
{
	/** One frame of the tests (s). */
	inline constexpr float Frame = 0.05f;

	/** Where the deck's pieces stand round its middle (cm): the two front posts, the keeper's post by the entrance. */
	inline const FVector FrontLeftPost(860.0, -760.0, 0.0);
	inline const FVector FrontRightPost(860.0, 760.0, 0.0);
	inline const FVector KeepersPost(-940.0, -460.0, 0.0);

	/** The six dead on their biers either side of the aisle (a body's middle, cm), as build_area_deck.py sets them on him. */
	inline TArray<FVector> BoardsDead()
	{
		return { FVector(500.0, -380.0, 100.0), FVector(500.0, 380.0, 100.0), FVector(0.0, -380.0, 100.0), FVector(0.0, 380.0, 100.0),
			FVector(-500.0, -380.0, 100.0), FVector(-500.0, 380.0, 100.0) };
	}

	/** Looter.Scenes set for a test (2: on, 0: off), and put back after it. */
	class FScenesSetting
	{
	public:
		explicit FScenesSetting(int32 Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Scenes")))
		{
			if (Variable)
			{
				Previous = Variable->GetInt();
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}

		~FScenesSetting()
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

	/**
	 * His smog tendrils off for a test about something else (Looter.Abel.Tendrils 0), and back after: a tendril drains the
	 * player stand-in (100 health) and its death would start the fight over mid-test.
	 */
	class FTendrilsOff
	{
	public:
		FTendrilsOff()
			: Variable(IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Abel.Tendrils")))
		{
			if (Variable)
			{
				Previous = Variable->GetInt();
				Variable->Set(0, ECVF_SetByConsole);
			}
		}

		~FTendrilsOff()
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

	/** A lantern post as the build script sets it up, begun. */
	inline AKeeperLanternPost* SpawnPost(UWorld* World, const FVector& Where, bool bKeepers)
	{
		AKeeperLanternPost* Post = World->SpawnActor<AKeeperLanternPost>(Where, FRotator(0.0, 180.0, 0.0));
		if (Post)
		{
			Post->bKeepersPost = bKeepers;
			if (bKeepers)
			{
				Post->Tags.AddUnique(AKeeperLanternPost::KeepersPostTag);
			}
			Post->DispatchBeginPlay();
		}
		return Post;
	}

	/**
	 * Abel on the deck's middle facing the sunset (+X), with the deck's three posts and the six dead's spots, begun. With
	 * bStory his story is the class's (Main 7); without it he's always there and his fight can start any time (the fight's
	 * tests). bDead off: no dead on the boards (the optional's "none to save").
	 */
	inline AAbelKeeper* SpawnAbel(UWorld* World, bool bStory = false, TArray<AKeeperLanternPost*>* OutPosts = nullptr, bool bDead = true)
	{
		TArray<AKeeperLanternPost*> Posts = { SpawnPost(World, FrontLeftPost, false), SpawnPost(World, FrontRightPost, false),
			SpawnPost(World, KeepersPost, true) };
		AAbelKeeper* Abel = World->SpawnActor<AAbelKeeper>(FVector(0.0, 0.0, 120.0), FRotator::ZeroRotator);
		if (!Abel || Posts.Contains(nullptr))
		{
			return nullptr;
		}
		if (!bStory)
		{
			Abel->PresentWhen = FStoryCondition();
			Abel->FightWhen = FStoryCondition();
			Abel->EndingWhen = FStoryCondition();
		}
		for (AKeeperLanternPost* Post : Posts)
		{
			Abel->LanternPosts.Add(Post);
		}
		if (bDead)
		{
			Abel->GetCorruption()->BoardsDead = BoardsDead();
		}
		// The deck's open end, 10 m on from his spot.
		Abel->OpenEndDistance = 1000.f;
		Abel->DispatchBeginPlay();
		BossTestWorld::NoLoot(Abel);
		if (OutPosts)
		{
			*OutPosts = Posts;
		}
		return Abel;
	}

	/**
	 * Answers the call-back he's knelt for (the keeper's post's hold done), as the player would. False when no call is open.
	 * The fight goes on into the phase's own (the fog, or the surge and the wind).
	 */
	inline bool CallBack(AAbelKeeper* Abel, AActor* By = nullptr)
	{
		AKeeperLanternPost* Post = Abel ? Abel->GetKeepersPost() : nullptr;
		return Post && Post->CallBack(By);
	}

	/** Hurts him to just under a phase's line and answers its call-back at once (the fight's tests that are about the phase). */
	inline void HurtIntoPhase(AAbelKeeper* Abel, float Share)
	{
		if (Abel)
		{
			BossTestWorld::HurtTo(*Abel->GetBoss(), Share);
			CallBack(Abel);
		}
	}

	/** Moves his fight and him on by Seconds, a frame at a time, as the game would (his boss's fight, then his frame). */
	inline void Run(AAbelKeeper* Abel, float Seconds)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		for (float Done = 0.f; Abel && Done < Seconds - KINDA_SMALL_NUMBER; Done += Frame)
		{
			Abel->GetBoss()->TickFight(Frame);
			Abel->Tick(Frame);
		}
	}

	/** Runs until Done holds (at most Limit seconds); the seconds it took, or a negative number if it never held. */
	inline float RunUntil(AAbelKeeper* Abel, float Limit, TFunctionRef<bool()> Done)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		for (float Time = 0.f; Abel && Time <= Limit; Time += Frame)
		{
			if (Done())
			{
				return Time;
			}
			Abel->GetBoss()->TickFight(Frame);
			Abel->Tick(Frame);
		}
		return -1.f;
	}

	/** His fight against a player stand-in 7 m toward the gate, started. */
	inline UBossComponent* StartFight(FAutomationTestBase& Test, UWorld* World, AAbelKeeper* Abel, ACharacter*& OutPlayer)
	{
		OutPlayer = BossTestWorld::SpawnPlayer(World, FVector(-700.0, 0.0, 120.0));
		UBossComponent* Boss = Abel ? Abel->GetBoss() : nullptr;
		if (!Test.TestNotNull(TEXT("Player stand-in"), OutPlayer) || !Test.TestNotNull(TEXT("Abel's boss"), Boss))
		{
			return nullptr;
		}
		FEditorScriptExecutionGuard RunActorEvents;
		Boss->StartFight(OutPlayer);
		Test.TestTrue(TEXT("His fight is on"), Boss->IsFighting());
		return Boss;
	}

	/** Hurts him to just under a share of his health (as the game's damage arrives). */
	inline void HurtTo(AAbelKeeper* Abel, float Share)
	{
		if (Abel)
		{
			BossTestWorld::HurtTo(*Abel->GetBoss(), Share);
		}
	}
}

#endif
