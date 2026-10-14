#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Progression/LevelUpBurstSubsystem.h"
#include "Tests/LocomotionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"

// The level-up in the world (concept F; Looter.Progression.LevelUpBurst): a ring of light runs out along the ground round
// the player's feet and embers rise, once per level (the same level-up told twice bursts once, a new level does again),
// and it's done in its time, after which nothing ticks.

namespace
{
	constexpr float BurstTestFrame = 1.f / 60.f;

	/** Moves the burst on Seconds, frame by frame. */
	void BurstTestRun(ULevelUpBurstSubsystem& Burst, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds; Time += BurstTestFrame)
		{
			Burst.Advance(BurstTestFrame);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelUpBurstTest, "Looter.Progression.LevelUpBurst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLevelUpBurstTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ULevelUpBurstSubsystem* Burst = ULevelUpBurstSubsystem::Get(World);
	APawn* Player = World->SpawnActor<APawn>(FVector(0.0, 0.0, 90.0), FRotator::ZeroRotator);
	if (!TestTrue(TEXT("The burst, a player and the ground"), Burst && Player
		&& LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(1500.0, 1500.0, 100.0))))
	{
		return false;
	}
	TestFalse(TEXT("Idle: nothing ticks"), Burst->IsPlaying() || Burst->IsTickable());
	TestFalse(TEXT("No player: no burst"), Burst->Play(nullptr, 2));

	TestTrue(TEXT("A level-up bursts"), Burst->Play(Player, 2));
	TestTrue(TEXT("...and plays (ticking)"), Burst->IsPlaying() && Burst->IsTickable());
	TestFalse(TEXT("The same level-up told again bursts once"), Burst->Play(Player, 2));
	TestEqual(TEXT("...one burst"), Burst->GetBurstCount(), 1);

	BurstTestRun(*Burst, 0.1f);
	const float EarlyRadius = Burst->GetRingRadius();
	TestEqual(TEXT("The ring and every ember drawn as streaks"), Burst->GetStreaksDrawn(),
		ULevelUpBurstSubsystem::Segments * 2 + ULevelUpBurstSubsystem::Embers);
	TestTrue(FString::Printf(TEXT("Embers glow at the feet (%d)"), Burst->NumEmbersGlowing()), Burst->NumEmbersGlowing() > 0);
	BurstTestRun(*Burst, 0.4f);
	const float LaterRadius = Burst->GetRingRadius();
	TestTrue(FString::Printf(TEXT("The ring runs out over the ground (%.0f cm, then %.0f cm)"), EarlyRadius, LaterRadius),
		EarlyRadius > 0.f && LaterRadius > EarlyRadius && LaterRadius <= ULevelUpBurstSubsystem::RingRadius);
	BurstTestRun(*Burst, ULevelUpBurstSubsystem::RingSeconds - 0.4f);
	TestEqual(TEXT("The ring has faded by its time"), Burst->GetRingRadius(), 0.f);
	TestTrue(TEXT("...the embers it left still rising"), Burst->NumEmbersGlowing() > 0);
	BurstTestRun(*Burst, ULevelUpBurstSubsystem::Seconds);
	TestFalse(TEXT("Done in its time: nothing drawn, nothing ticks"), Burst->IsPlaying() || Burst->IsTickable());
	TestEqual(TEXT("...no streaks"), Burst->GetStreaksDrawn(), 0);

	TestTrue(TEXT("The next level bursts again"), Burst->Play(Player, 3));
	TestTrue(TEXT("...two bursts, the last for level 3"), Burst->GetBurstCount() == 2 && Burst->GetLastLevel() == 3);
	TestTrue(TEXT("A level's burst while one plays restarts it"), Burst->Play(Player, 4) && Burst->IsPlaying() && Burst->GetBurstCount() == 3);
	return true;
}

#endif
