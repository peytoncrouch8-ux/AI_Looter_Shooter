#pragma once

// What the barge's tests (BargeBoardingTests.cpp: her kit and decks; BargeBoardingHallTests.cpp: below the deck and the
// hull; BargeBoardingBeatsTests.cpp: the grate ring, the valves, the cage, her dying and her fall) build in their test
// levels: a barge, begun and played as if Main 6 were on a step, with no story; and the traces they stand things on with.
// Points "on her" are in her frame (BargeRules'): the helpers turn them into the world.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/BargeBoarding.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
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

	/** True when a body of Radius stands clear with its feet at Feet (on her): a capsule from OverFeet up to Tall over them
	 *  meets nothing a pawn bumps into. Blocker (if given) gets the first part it meets. */
	inline bool CapsuleClear(const UWorld* World, const ABargeBoarding& Barge, const FVector& Feet, double Radius, double Tall, FString* Blocker = nullptr)
	{
		const float Half = FMath::Max(static_cast<float>((Tall - OverFeet) * 0.5), static_cast<float>(Radius));
		const FVector Middle = Barge.GetActorTransform().TransformPosition(Feet + FVector(0.0, 0.0, OverFeet + Half));
		const FCollisionShape Shape = FCollisionShape::MakeCapsule(static_cast<float>(Radius), Half);
		if (!Blocker)
		{
			return !World->OverlapAnyTestByChannel(Middle, FQuat::Identity, ECC_Pawn, Shape);
		}
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByChannel(Overlaps, Middle, FQuat::Identity, ECC_Pawn, Shape);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (Overlap.bBlockingHit && Overlap.GetComponent())
			{
				*Blocker = Overlap.GetComponent()->GetName();
				return false;
			}
		}
		return true;
	}

	/**
	 * A body of Radius standing at Feet (on her) as a walking pawn's movement finds it: CapsuleClear's capsule swept a
	 * centimetre in place against simple collision, any blocking hit (one it starts in too) in its way. Movement sweeps; it
	 * never meets her per-poly mesh. The overlap CapsuleClear makes did, 2026-11-05: a render face across the starboard
	 * house walk at (1607, 753, 2376) stopped the stern push's route, though no simple shape of hers is there and the
	 * Dredgers walk through. Blocker gets the first blocking component's name.
	 */
	inline bool BodyFits(const UWorld* World, const ABargeBoarding& Barge, const FVector& Feet, double Radius, double Tall, FString* Blocker = nullptr)
	{
		const float Half = FMath::Max(static_cast<float>((Tall - OverFeet) * 0.5), static_cast<float>(Radius));
		const FVector Middle = Barge.GetActorTransform().TransformPosition(Feet + FVector(0.0, 0.0, OverFeet + Half));
		const FCollisionQueryParams Params(SCENE_QUERY_STAT(BargeBodyFits), false);
		TArray<FHitResult> Hits;
		World->SweepMultiByChannel(Hits, Middle + FVector(0.0, 0.0, 1.0), Middle, FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeCapsule(static_cast<float>(Radius), Half), Params);
		for (const FHitResult& Hit : Hits)
		{
			if (Hit.bBlockingHit && Hit.GetComponent())
			{
				if (Blocker)
				{
					*Blocker = Hit.GetComponent()->GetName();
				}
				return false;
			}
		}
		return true;
	}

	/**
	 * A walk along Points (on her, flat) a step at a time: at every step a floor within Rise of Z under it and a body of
	 * Radius standing clear there (CapsuleClear, to 1.8 m). Why gets the first step it fails and on what.
	 */
	inline bool WalkLegs(const UWorld* World, const ABargeBoarding& Barge, const TArray<FVector2D>& Points, double Z, double Radius, FString* Why = nullptr)
	{
		constexpr double Step = 30.0;
		constexpr double Rise = 10.0;
		constexpr double Tall = 180.0;
		for (int32 Leg = 0; Leg + 1 < Points.Num(); ++Leg)
		{
			const FVector2D From = Points[Leg];
			const FVector2D Run = Points[Leg + 1] - From;
			const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Run.Size() / Step));
			for (int32 Index = 0; Index <= Steps; ++Index)
			{
				const FVector2D At = From + Run * (static_cast<double>(Index) / Steps);
				double Floor = 0.0;
				const bool bFloor = FloorUnder(World, Barge, FVector(At, Z + 120.0), Floor) && FMath::Abs(Floor - Z) <= Rise;
				FString Blocker;
				if (!bFloor || !CapsuleClear(World, Barge, FVector(At, bFloor ? Floor : Z), Radius, Tall, &Blocker))
				{
					if (Why)
					{
						*Why = FString::Printf(TEXT("at (%.0f, %.0f): %s"), At.X, At.Y, bFloor ? *(TEXT("in ") + Blocker) : *(TEXT("floor ") + FloorWhat(World, Barge, FVector(At, Z + 120.0))));
					}
					return false;
				}
			}
		}
		return true;
	}

	/**
	 * A walker's way across her where the walls wander (the walks' mouths, the stern push's walks): a grid of cells 30 cm
	 * along X by 15 cm across over YMin..YMax, from Start's cell toward GoalX; a cell stands where a drop from over it finds
	 * a floor within 30 cm of Z and a body of Radius stands on it as a walker's movement finds it (BodyFits, to 1.8 m). Open when a cell at GoalX
	 * is reached through cells side by side. Why gets the farthest it got and what stopped it past there; OutReached and
	 * OutPast those two cells (for BargeBlockProbe.h's DescribeBlock).
	 */
	inline constexpr double RouteTall = 180.0;
	inline bool RouteOpen(const UWorld* World, const ABargeBoarding& Barge, const FVector2D& Start, double GoalX, double YMin, double YMax, double Z,
		double Radius, FString* Why = nullptr, FVector2D* OutReached = nullptr, FVector2D* OutPast = nullptr)
	{
		constexpr double StepX = 30.0;
		constexpr double StepY = 15.0;
		constexpr double Rise = 30.0;
		constexpr double Tall = RouteTall;
		const double Way = GoalX >= Start.X ? 1.0 : -1.0;
		const int32 Columns = FMath::CeilToInt32(FMath::Abs(GoalX - Start.X) / StepX) + 1;
		const int32 Rows = FMath::Max(1, FMath::CeilToInt32((YMax - YMin) / StepY) + 1);
		const auto At = [&Start, Way, YMin](int32 Column, int32 Row)
		{
			return FVector2D(Start.X + Way * Column * StepX, YMin + Row * StepY);
		};
		// 0 not looked at, 1 no stand, 2 stands, 3 reached.
		TArray<uint8> Cells;
		Cells.Init(0, Columns * Rows);
		const auto Stands = [&](int32 Column, int32 Row)
		{
			uint8& Cell = Cells[Column * Rows + Row];
			if (Cell == 0)
			{
				const FVector2D Spot = At(Column, Row);
				double Floor = 0.0;
				const bool bFloor = FloorUnder(World, Barge, FVector(Spot, Z + 120.0), Floor) && FMath::Abs(Floor - Z) <= Rise;
				Cell = bFloor && BodyFits(World, Barge, FVector(Spot, Floor), Radius, Tall) ? 2 : 1;
			}
			return Cell >= 2;
		};
		TArray<FIntPoint> Queue;
		const int32 StartRow = FMath::Clamp(FMath::RoundToInt32((Start.Y - YMin) / StepY), 0, Rows - 1);
		for (const int32 Offset : { 0, 1, -1, 2, -2 })
		{
			const int32 Row = StartRow + Offset;
			if (Row >= 0 && Row < Rows && Stands(0, Row))
			{
				Cells[Row] = 3;
				Queue.Add(FIntPoint(0, Row));
				break;
			}
		}
		FIntPoint Farthest(0, StartRow);
		for (int32 Head = 0; Head < Queue.Num(); ++Head)
		{
			const FIntPoint Cell = Queue[Head];
			if (Cell.X >= Columns - 1)
			{
				return true;
			}
			if (Cell.X > Farthest.X)
			{
				Farthest = Cell;
			}
			for (const FIntPoint Step : { FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(0, -1), FIntPoint(-1, 0) })
			{
				const FIntPoint Next = Cell + Step;
				if (Next.X < 0 || Next.X >= Columns || Next.Y < 0 || Next.Y >= Rows || Cells[Next.X * Rows + Next.Y] == 3)
				{
					continue;
				}
				if (Stands(Next.X, Next.Y))
				{
					Cells[Next.X * Rows + Next.Y] = 3;
					Queue.Add(Next);
				}
			}
		}
		if (OutReached)
		{
			*OutReached = At(Farthest.X, Farthest.Y);
		}
		if (OutPast)
		{
			*OutPast = At(FMath::Min(Farthest.X + 1, Columns - 1), Farthest.Y);
		}
		if (Why)
		{
			const FVector2D Past = At(FMath::Min(Farthest.X + 1, Columns - 1), Farthest.Y);
			double Floor = 0.0;
			FString Blocker;
			const bool bFloor = FloorUnder(World, Barge, FVector(Past, Z + 120.0), Floor) && FMath::Abs(Floor - Z) <= Rise;
			BodyFits(World, Barge, FVector(Past, bFloor ? Floor : Z), Radius, Tall, &Blocker);
			*Why = Queue.IsEmpty() ? FString::Printf(TEXT("no room at the start (%.0f, %.0f)"), Start.X, Start.Y)
				: FString::Printf(TEXT("got to x %.0f; past it at (%.0f, %.0f): %s"), At(Farthest.X, Farthest.Y).X, Past.X, Past.Y,
					bFloor ? *(TEXT("in ") + Blocker) : *(TEXT("floor ") + FloorWhat(World, Barge, FVector(Past, Z + 120.0))));
		}
		return false;
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
