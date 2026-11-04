#pragma once

// What the barge's tests (BargeBoardingTests.cpp: her kit and decks; BargeBoardingHallTests.cpp: below the deck and the
// hull; BargeBoardingBeatsTests.cpp: the grate ring, the valves, the cage, her dying and her fall) build in their test
// levels: a barge, begun and played as if Main 6 were on a step, with no story; and the traces they stand things on with.
// Points "on her" are in her frame (BargeRules'): the helpers turn them into the world.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/BargeBoarding.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "Tests/AutomationCommon.h"

namespace BargeTestWorld
{
	/** A barge in a fresh test level at Where, begun and played as if Main 6 were on Step; the level's physics flushed. */
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
		// The test level never ticks: its physics scene's commands and query structure are brought up to date here, as a
		// playing level's are every frame (2026-11-04: the hull's own traces found her decks where the level's drops didn't).
		if (FPhysScene* Scene = World->GetPhysicsScene())
		{
			Scene->Flush();
		}
		return Barge;
	}

	/** Where a straight drop from From (world) hits world-static ground, or false. */
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

	/** The height (her frame) of the world-static floor under a point on her, or false. */
	inline bool FloorUnder(const UWorld* World, const ABargeBoarding& Barge, const FVector& OnHer, double& OutZ)
	{
		FVector Hit;
		if (!GroundUnder(World, Barge.GetActorTransform().TransformPosition(OnHer), Hit))
		{
			return false;
		}
		OutZ = Barge.GetActorTransform().InverseTransformPosition(Hit).Z;
		return true;
	}

	/** Where a straight climb from From (world) meets world-static iron within Reach, or false. */
	inline bool IronOver(const UWorld* World, const FVector& From, double Reach, FVector& OutHit)
	{
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, From, From + FVector(0.0, 0.0, Reach), FCollisionObjectQueryParams(ECC_WorldStatic)))
		{
			OutHit = Hit.ImpactPoint;
			return true;
		}
		return false;
	}

	/** True when a pawn's way between two points on her (at a walker's chest) meets nothing a pawn bumps into. */
	inline bool WalkClear(const UWorld* World, const ABargeBoarding& Barge, const FVector& From, const FVector& To)
	{
		const FTransform& Her = Barge.GetActorTransform();
		return !World->LineTraceTestByChannel(Her.TransformPosition(From), Her.TransformPosition(To), ECC_Pawn);
	}

	/** True when an eye at From sees To (both on her): nothing in the way of a look. */
	inline bool SeesClear(const UWorld* World, const ABargeBoarding& Barge, const FVector& From, const FVector& To)
	{
		const FTransform& Her = Barge.GetActorTransform();
		return !World->LineTraceTestByChannel(Her.TransformPosition(From), Her.TransformPosition(To), ECC_Visibility);
	}

	/** The player's capsule (34 x 0.85) to the nearest hand: clear of the step under him and of a 33 degree slope from this
	 *  far over his feet. */
	inline constexpr float Shoulders = 30.f;
	inline constexpr double OverFeet = 30.0;

	/** True when a walker with his feet at Feet (on her) has Headroom over that: a capsule of his shoulders from OverFeet up
	 *  Headroom meets nothing a pawn bumps into. */
	inline bool HeadroomClear(const UWorld* World, const ABargeBoarding& Barge, const FVector& Feet, double Headroom)
	{
		const float Half = static_cast<float>(Headroom * 0.5);
		const FVector Middle = Barge.GetActorTransform().TransformPosition(Feet + FVector(0.0, 0.0, OverFeet + Half));
		return !World->OverlapAnyTestByChannel(Middle, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Shoulders, Half));
	}

	/** What a straight drop from a point on her meets first (world-static, as FloorUnder): the part, its actor and the
	 *  height on her, or that nothing is under it; for the tests' messages. */
	inline FString FloorWhat(const UWorld* World, const ABargeBoarding& Barge, const FVector& OnHer)
	{
		const FVector From = Barge.GetActorTransform().TransformPosition(OnHer);
		FHitResult Hit;
		if (!World->LineTraceSingleByObjectType(Hit, From, From - FVector(0.0, 0.0, 3000.0), FCollisionObjectQueryParams(ECC_WorldStatic)))
		{
			return TEXT("nothing under it");
		}
		const UPrimitiveComponent* Part = Hit.GetComponent();
		const AActor* Owner = Part ? Part->GetOwner() : nullptr;
		return FString::Printf(TEXT("%s of %s at %.1f%s"), Part ? *Part->GetName() : TEXT("?"), Owner ? *Owner->GetName() : TEXT("?"),
			Barge.GetActorTransform().InverseTransformPosition(Hit.ImpactPoint).Z, Hit.bStartPenetrating ? TEXT(", the drop starting inside it") : TEXT(""));
	}

	/** The first part a pawn bumps into in HeadroomClear's capsule (its name), or empty. */
	inline FString HeadroomWhat(const UWorld* World, const ABargeBoarding& Barge, const FVector& Feet, double Headroom)
	{
		const float Half = static_cast<float>(Headroom * 0.5);
		const FVector Middle = Barge.GetActorTransform().TransformPosition(Feet + FVector(0.0, 0.0, OverFeet + Half));
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByChannel(Overlaps, Middle, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Shoulders, Half));
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (Overlap.bBlockingHit && Overlap.GetComponent())
			{
				return Overlap.GetComponent()->GetName();
			}
		}
		return FString();
	}

	/** A component's box now (computed, not the cached bounds: instanced looks laid this frame count). */
	inline FBox BoxOf(const UPrimitiveComponent& Part)
	{
		return Part.CalcBounds(Part.GetComponentTransform()).GetBox();
	}

	/** The same in her frame (the barge stands unturned in the tests: her axes are the world's). */
	inline FBox BoxOnHer(const ABargeBoarding& Barge, const UPrimitiveComponent& Part)
	{
		const FBox Box = BoxOf(Part);
		const FVector Offset = Barge.GetActorLocation();
		return FBox(Box.Min - Offset, Box.Max - Offset);
	}

	inline FBox2D Flat(const FBox& Box)
	{
		return FBox2D(FVector2D(Box.Min.X, Box.Min.Y), FVector2D(Box.Max.X, Box.Max.Y));
	}

	/** A flat box round a deck piece (centre, half size). */
	inline FBox2D Around(const FVector& Spot, double HalfX, double HalfY)
	{
		return FBox2D(FVector2D(Spot.X - HalfX, Spot.Y - HalfY), FVector2D(Spot.X + HalfX, Spot.Y + HalfY));
	}
}

#endif
