#pragma once

// What the barge's tests (BargeBoardingTests.cpp: her kit on the hull; BargeBoardingBeatsTests.cpp: the grate ring, the
// valves, the cage, her dying and her fall) build in their test levels: a barge, begun and played as if Main 6 were on a
// step, with no story; and the traces they stand things on with.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/BargeBoarding.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace BargeTestWorld
{
	/** A barge in a fresh test level at Where, begun and played as if Main 6 were on Step. */
	inline ABargeBoarding* SpawnBarge(FTestWorldWrapper& TestLevel, FAutomationTestBase& Test, int32 Step, const FVector& Where = FVector(0.0, 0.0, 10000.0))
	{
		if (!Test.TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
		{
			return nullptr;
		}
		UWorld* World = TestLevel.GetTestWorld();
		ABargeBoarding* Barge = World->SpawnActor<ABargeBoarding>(Where, FRotator::ZeroRotator);
		if (!Test.TestNotNull(TEXT("The barge"), Barge))
		{
			return nullptr;
		}
		Barge->DispatchBeginPlay();
		Barge->ForceStep(Step);
		return Barge;
	}

	/** Where a straight drop from From hits world-static ground, or false. */
	inline bool GroundUnder(const UWorld* World, const FVector& From, FVector& OutHit)
	{
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, From, From - FVector(0.0, 0.0, 3000.0), FCollisionObjectQueryParams(ECC_WorldStatic)))
		{
			OutHit = Hit.ImpactPoint;
			return true;
		}
		return false;
	}
}

#endif
