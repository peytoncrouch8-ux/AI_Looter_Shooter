#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/FallRecoveryTracker.h"
#include "World/MinimapPaint.h"
#include "World/MinimapSubsystem.h"
#include "World/PlayableArea.h"
#include "World/PlayableBoundary.h"
#include "World/WorldQueries.h"
#include "CollisionShape.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

namespace
{
	/**
	 * A 40 m square of ground. X runs north and Y east, as in the world, and the corners go counterclockwise from the
	 * south-west: edge 0 is the west side (Y = 0), edge 1 the north (X = 4000), edge 2 the east (Y = 4000) and edge 3
	 * the south (X = 0). The north edge is open, a drop that is part of play. The north corners' ground is 2 m higher.
	 */
	TArray<FVector> SquareCorners()
	{
		return { FVector(0.0, 0.0, 0.0), FVector(4000.0, 0.0, 0.0), FVector(4000.0, 4000.0, 200.0), FVector(0.0, 4000.0, 200.0) };
	}

	TArray<bool> SquareOpenEdges()
	{
		return { false, true, false, false };
	}

	FPlayableBoundary SquareBoundary()
	{
		TArray<FVector2D> Flat;
		for (const FVector& Corner : SquareCorners())
		{
			Flat.Emplace(Corner.X, Corner.Y);
		}
		return FPlayableBoundary(MoveTemp(Flat), SquareOpenEdges());
	}

	/** A valley with a notch cut 20 m into its east side, as Stage Gap cuts into the east ridge. Every edge closed. */
	TArray<FVector2D> NotchedValley()
	{
		return { FVector2D(0.0, 0.0), FVector2D(8000.0, 0.0), FVector2D(8000.0, 10000.0), FVector2D(5000.0, 10000.0),
			FVector2D(5000.0, 8000.0), FVector2D(3000.0, 8000.0), FVector2D(3000.0, 10000.0), FVector2D(0.0, 10000.0) };
	}

	/** Whether a point lies in a wall's footprint, more than Margin (cm) in from its sides. */
	bool InWallFootprint(const FPlayableWall& Wall, const FVector2D& Point, double Margin)
	{
		const FVector2D Offset = Point - Wall.Center;
		const FVector2D Across(-Wall.Direction.Y, Wall.Direction.X);
		return FMath::Abs(FVector2D::DotProduct(Offset, Wall.Direction)) < Wall.HalfLength - Margin
			&& FMath::Abs(FVector2D::DotProduct(Offset, Across)) < Wall.HalfThickness - Margin;
	}

	/** One frame of the fall recovery tests (s), and gravity as the characters fall (cm/s/s). */
	constexpr float RecoveryFrame = 1.f / 60.f;
	constexpr double RecoveryGravity = 980.0;

	/** Texels per side of the minimap test's map. */
	constexpr int32 MapSide = 32;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayableAreaTest, "Looter.World.PlayableArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayableAreaTest::RunTest(const FString& Parameters)
{
	// Contains(): a valley with a notch in it, its corners given either way round.
	TArray<FVector2D> Reversed = NotchedValley();
	Algo::Reverse(Reversed);
	for (const FPlayableBoundary& Valley : { FPlayableBoundary(NotchedValley(), {}), FPlayableBoundary(Reversed, {}) })
	{
		TestTrue(TEXT("The valley is a usable boundary"), Valley.IsValid());
		TestEqual(TEXT("It encloses 80 x 100 m less the 20 x 20 m notch"), Valley.SurfaceArea(), 8000.0 * 10000.0 - 2000.0 * 2000.0, 1.0);
		TestTrue(TEXT("Inside, in its south-west"), Valley.Contains(FVector2D(1000.0, 1000.0)));
		TestTrue(TEXT("Inside, beside the notch"), Valley.Contains(FVector2D(2000.0, 9000.0)) && Valley.Contains(FVector2D(6000.0, 9000.0)));
		TestTrue(TEXT("Inside, at the notch's mouth"), Valley.Contains(FVector2D(4000.0, 7000.0)));
		TestFalse(TEXT("Outside, in the notch"), Valley.Contains(FVector2D(4000.0, 9000.0)));
		TestFalse(TEXT("Outside, past the north edge"), Valley.Contains(FVector2D(9000.0, 5000.0)));
		TestFalse(TEXT("Outside, past the west edge"), Valley.Contains(FVector2D(4000.0, -100.0)));
	}
	const FPlayableBoundary Valley(NotchedValley(), {});
	TestEqual(TEXT("Walking from the mouth into the notch crosses its end"), Valley.FindCrossedEdge(FVector2D(4000.0, 7000.0), FVector2D(4000.0, 9000.0)), 4);
	TestTrue(TEXT("A walk inside crosses nothing"), Valley.FindCrossedEdge(FVector2D(1000.0, 1000.0), FVector2D(2000.0, 2000.0)) == INDEX_NONE);
	double Distance = 0.0;
	TestEqual(TEXT("The notch's end is nearest just inside it"), Valley.FindNearestEdge(FVector2D(4000.0, 8100.0), &Distance), 4);
	TestEqual(TEXT("...a meter away"), Distance, 100.0, 0.01);

	// The walls' footprints never stand inside the area, even at the notch's inward corners, and at outward corners
	// they run on until they meet, leaving no gap; with the walls on the boundary and set back from it.
	for (const double Setback : { 0.0, 300.0 })
	{
		const TArray<FPlayableWall> Footprints = Valley.MakeWalls(Setback, 200.0);
		TestEqual(FString::Printf(TEXT("One wall per closed edge (setback %.0f)"), Setback), Footprints.Num(), 8);
		int32 Intrusions = 0;
		for (double X = 125.0; X < 8000.0; X += 250.0)
		{
			for (double Y = 125.0; Y < 10000.0; Y += 250.0)
			{
				const FVector2D Point(X, Y);
				for (const FPlayableWall& Wall : Footprints)
				{
					Intrusions += Valley.Contains(Point) && InWallFootprint(Wall, Point, 1.0) ? 1 : 0;
				}
			}
		}
		TestEqual(FString::Printf(TEXT("No wall stands inside the area (setback %.0f)"), Setback), Intrusions, 0);
		// The south-west corner (0, 0) turns outward: both walls cover the square just past it, beyond their faces.
		const FVector2D PastCorner(-(Setback + 100.0), -(Setback + 100.0));
		const bool bWest = InWallFootprint(Footprints[0], PastCorner, 1.0);
		const bool bSouth = InWallFootprint(Footprints[7], PastCorner, 1.0);
		TestTrue(FString::Printf(TEXT("The walls meet past an outward corner (setback %.0f)"), Setback), bWest && bSouth);
	}

	// The walls in a test world: one per closed edge, using the profile that blocks only walking pawns. (A preview
	// world's scene doesn't pick up new bodies for world traces, so the sweeps below sweep each wall's own body.)
	FCollisionResponseTemplate Profile;
	if (TestTrue(TEXT("DefaultEngine.ini has the PlayableBounds collision profile"),
		UCollisionProfile::Get()->GetProfileTemplate(APlayableArea::WallProfile, Profile)))
	{
		TestTrue(TEXT("...world static"), Profile.ObjectType == ECC_WorldStatic);
		TestTrue(TEXT("...blocking walking pawns"), Profile.ResponseToChannels.GetResponse(ECC_Pawn) == ECR_Block);
	}
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	TestNull(TEXT("No playable area in an empty world"), APlayableArea::Find(World));
	APlayableArea* Area = World->SpawnActor<APlayableArea>();
	if (!TestNotNull(TEXT("Playable area spawned"), Area))
	{
		return false;
	}
	Area->Corners = SquareCorners();
	Area->OpenEdges = SquareOpenEdges();
	Area->Rebuild();
	TestTrue(TEXT("Found as the world's playable area"), APlayableArea::Find(World) == Area);
	TestTrue(TEXT("Contains a point inside, whatever its height"), Area->Contains(FVector(2000.0, 2000.0, 99999.0)));
	TestFalse(TEXT("...and not one outside"), Area->Contains(FVector(2000.0, -50.0, 0.0)));
	const FCollisionQueryParams GroundQuery = LooterWorld::StaticGeometryParams(World, TEXT("PlayableAreaTest"));
	TestTrue(TEXT("Ground traces skip it"), GroundQuery.GetIgnoredSourceObjects().Contains(Area->GetUniqueID()));

	const TArray<TObjectPtr<UBoxComponent>>& Walls = Area->GetWalls();
	if (!TestEqual(TEXT("Three walls: none on the open edge"), Walls.Num(), 3))
	{
		return false;
	}
	const ECollisionChannel Passing[] = { ECC_WorldStatic, ECC_WorldDynamic, ECC_Visibility, ECC_Camera, ECC_PhysicsBody, ECC_Vehicle,
		ECC_Destructible, ECC_GameTraceChannel1 /*Projectile*/, ECC_GameTraceChannel2 /*Weapon: bullets*/ };
	for (const UBoxComponent* Wall : Walls)
	{
		if (!TestNotNull(TEXT("Wall"), Wall))
		{
			return false;
		}
		const FString Name = Wall->GetName();
		TestTrue(Name + TEXT(" uses the PlayableBounds profile"), Wall->GetCollisionProfileName() == APlayableArea::WallProfile);
		TestTrue(Name + TEXT(" collides"), Wall->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
		TestTrue(Name + TEXT(" blocks walking pawns"), Wall->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
		for (const ECollisionChannel Channel : Passing)
		{
			TestTrue(FString::Printf(TEXT("%s lets channel %d through"), *Name, static_cast<int32>(Channel)),
				Wall->GetCollisionResponseToChannel(Channel) == ECR_Ignore);
		}
		TestTrue(Name + TEXT(" is invisible"), Wall->bHiddenInGame != 0);
	}
	// Heights: from 10 m under the lower corner to 50 m over the higher one (the west edge's corners are at 0, the east's at 2 m).
	const FBox West = Walls[0]->Bounds.GetBox();
	const FBox East = Walls[1]->Bounds.GetBox();
	TestTrue(FString::Printf(TEXT("The west wall stands from -10 m to 50 m (%.0f to %.0f)"), West.Min.Z, West.Max.Z),
		FMath::IsNearlyEqual(West.Min.Z, -1000.0, 1.0) && FMath::IsNearlyEqual(West.Max.Z, 5000.0, 1.0));
	TestTrue(FString::Printf(TEXT("The east wall stands from -8 m to 52 m (%.0f to %.0f)"), East.Min.Z, East.Max.Z),
		FMath::IsNearlyEqual(East.Min.Z, -800.0, 1.0) && FMath::IsNearlyEqual(East.Max.Z, 5200.0, 1.0));

	// A player-sized capsule walked out of the area: stopped inside by a closed edge's wall, never by the open edge.
	auto SweepWalls = [&Walls](const FVector& From, const FVector& To, FHitResult& OutHit)
	{
		bool bStopped = false;
		OutHit = FHitResult();
		OutHit.Time = 1.f;
		for (UBoxComponent* Wall : Walls)
		{
			FHitResult Hit;
			if (Wall && Wall->SweepComponent(Hit, From, To, FQuat::Identity, FCollisionShape::MakeCapsule(34.f, 88.f)) && Hit.Time <= OutHit.Time)
			{
				OutHit = Hit;
				bStopped = true;
			}
		}
		return bStopped;
	};
	struct FWalk
	{
		const TCHAR* What;
		FVector From;
		FVector To;
		bool bStopped;
	};
	const FWalk Walks[] = {
		{ TEXT("West, across a closed edge"), FVector(2000.0, 300.0, 100.0), FVector(2000.0, -600.0, 100.0), true },
		{ TEXT("East, across a closed edge"), FVector(2000.0, 3700.0, 300.0), FVector(2000.0, 4600.0, 300.0), true },
		{ TEXT("Out of the south-west corner"), FVector(300.0, 300.0, 100.0), FVector(-600.0, -600.0, 100.0), true },
		{ TEXT("Out of the south-east corner"), FVector(300.0, 3700.0, 300.0), FVector(-600.0, 4600.0, 300.0), true },
		{ TEXT("North, across the open edge"), FVector(3700.0, 2000.0, 100.0), FVector(4600.0, 2000.0, 100.0), false },
		{ TEXT("North over the open edge, beside the west wall's end"), FVector(3700.0, 300.0, 100.0), FVector(4600.0, 300.0, 100.0), false },
	};
	for (const FWalk& Walk : Walks)
	{
		FHitResult Hit;
		const bool bStopped = SweepWalls(Walk.From, Walk.To, Hit);
		TestTrue(FString::Printf(TEXT("%s: %s"), Walk.What, Walk.bStopped ? TEXT("stopped") : TEXT("not stopped")), bStopped == Walk.bStopped);
		if (bStopped && Walk.bStopped)
		{
			TestTrue(FString::Printf(TEXT("%s: still inside where it stops (%s)"), Walk.What, *Hit.Location.ToCompactString()),
				Area->Contains(Hit.Location));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFallRecoveryTest, "Looter.World.FallRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFallRecoveryTest::RunTest(const FString& Parameters)
{
	const FPlayableBoundary Square = SquareBoundary();
	FFallRecoveryTracker::FRules Rules;
	const FRotator Facing = FRotator::ZeroRotator;

	// Walking north at 6 m/s off the lip, half a meter short of the open edge (the boundary sits a step past the
	// walkable edge), and falling: back on the last safe spot within a second of leaving the area.
	{
		FFallRecoveryTracker Tracker;
		FVector Location(2000.0, 2000.0, 0.0);
		double FallSpeed = 0.0;
		int32 LeftOn = INDEX_NONE;
		int32 RecoveredOn = INDEX_NONE;
		TOptional<EFallRecoveryReason> Reason;
		for (int32 Frame = 1; Frame <= 600 && RecoveredOn == INDEX_NONE; ++Frame)
		{
			Location.X += 600.0 * RecoveryFrame;
			const bool bOnGround = Location.X <= 3950.0;
			if (!bOnGround)
			{
				FallSpeed += RecoveryGravity * RecoveryFrame;
				Location.Z -= FallSpeed * RecoveryFrame;
			}
			if (LeftOn == INDEX_NONE && !Square.Contains(FVector2D(Location.X, Location.Y)))
			{
				LeftOn = Frame;
			}
			Reason = Tracker.Update(RecoveryFrame, Location, Facing, bOnGround, &Square, Rules);
			RecoveredOn = Reason.IsSet() ? Frame : INDEX_NONE;
		}
		TestTrue(TEXT("Brought back off the open edge"), Reason.IsSet() && *Reason == EFallRecoveryReason::OffOpenEdge);
		const float Seconds = (RecoveredOn - LeftOn) * RecoveryFrame;
		TestTrue(FString::Printf(TEXT("Within a second of leaving the area (%.2f s)"), Seconds), LeftOn != INDEX_NONE && RecoveredOn != INDEX_NONE && Seconds <= 1.f);
		TestTrue(TEXT("To a safe spot inside, on the ground"), Tracker.HasSafeSpot()
			&& Square.Contains(FVector2D(Tracker.GetSafeLocation().X, Tracker.GetSafeLocation().Y)) && FMath::IsNearlyZero(Tracker.GetSafeLocation().Z));
		TestEqual(TEXT("It left over the north (open) edge"), Tracker.GetExitEdge(), 1);
		// Put back on the spot, it's inside again and stays put.
		Tracker.MarkRecovered();
		TestFalse(TEXT("Back on the spot, nothing more happens"), Tracker.Update(RecoveryFrame, Tracker.GetSafeLocation(), Facing, true, &Square, Rules).IsSet());
		TestFalse(TEXT("...and the player is inside"), Tracker.IsOutside());
	}

	// No safe spot is ever taken outside, even standing on walkable ground there (past a closed edge).
	{
		FFallRecoveryTracker Tracker;
		for (int32 Frame = 0; Frame < 120; ++Frame)
		{
			Tracker.Update(RecoveryFrame, FVector(2000.0, -300.0, 0.0), Facing, true, &Square, Rules);
		}
		TestFalse(TEXT("Standing outside takes no safe spot"), Tracker.HasSafeSpot());
		bool bAllInside = true;
		for (int32 Frame = 0; Frame < 600; ++Frame)
		{
			// Back and forth across the west edge, between 3 m out and 3 m in.
			const double Y = -300.0 + 600.0 * FMath::Abs(FMath::Fmod(Frame / 120.0, 2.0) - 1.0);
			Tracker.Update(RecoveryFrame, FVector(2000.0, Y, 0.0), Facing, true, &Square, Rules);
			bAllInside &= !Tracker.HasSafeSpot() || Square.Contains(FVector2D(Tracker.GetSafeLocation().X, Tracker.GetSafeLocation().Y));
		}
		TestTrue(TEXT("Walking in takes one"), Tracker.HasSafeSpot());
		TestTrue(TEXT("Never one outside"), bAllInside);
	}

	// Standing at the open edge, or stepping down onto a ledge just past it, never fires.
	{
		FFallRecoveryTracker Tracker;
		bool bFired = false;
		for (int32 Frame = 0; Frame < 180; ++Frame)
		{
			bFired |= Tracker.Update(RecoveryFrame, FVector(3990.0, 2000.0, 0.0), Facing, true, &Square, Rules).IsSet();
		}
		for (int32 Frame = 0; Frame < 60; ++Frame)
		{
			bFired |= Tracker.Update(RecoveryFrame, FVector(4050.0, 2000.0, -200.0), Facing, true, &Square, Rules).IsSet();
		}
		TestFalse(TEXT("Standing at the edge, or 2 m down past it, isn't a fall"), bFired);
	}

	// Past a closed edge (through a gap in its wall) or down a pit inside, only the 30 m backstop applies.
	{
		FFallRecoveryTracker Tracker;
		for (int32 Frame = 0; Frame < 60; ++Frame)
		{
			Tracker.Update(RecoveryFrame, FVector(2000.0, 100.0, 0.0), Facing, true, &Square, Rules);
		}
		TestFalse(TEXT("Past a closed edge, 10 m down isn't enough"),
			Tracker.Update(RecoveryFrame, FVector(2000.0, -100.0, -1000.0), Facing, false, &Square, Rules).IsSet());
		TestEqual(TEXT("...it left over the west (closed) edge"), Tracker.GetExitEdge(), 0);
		const TOptional<EFallRecoveryReason> Deep = Tracker.Update(RecoveryFrame, FVector(2000.0, -100.0, -3100.0), Facing, false, &Square, Rules);
		TestTrue(TEXT("...but 31 m down is"), Deep.IsSet() && *Deep == EFallRecoveryReason::LongFall);

		FFallRecoveryTracker InPit;
		for (int32 Frame = 0; Frame < 60; ++Frame)
		{
			InPit.Update(RecoveryFrame, FVector(2000.0, 2000.0, 0.0), Facing, true, &Square, Rules);
		}
		TestFalse(TEXT("Down a pit inside, 10 m isn't enough"), InPit.Update(RecoveryFrame, FVector(2000.0, 2000.0, -1000.0), Facing, false, &Square, Rules).IsSet());
		const TOptional<EFallRecoveryReason> PitBottom = InPit.Update(RecoveryFrame, FVector(2000.0, 2000.0, -3100.0), Facing, false, &Square, Rules);
		TestTrue(TEXT("...but 31 m is"), PitBottom.IsSet() && *PitBottom == EFallRecoveryReason::LongFall);
	}

	// Without a playable area (the tutorial island), today's rule: only a long fall.
	{
		FFallRecoveryTracker Tracker;
		for (int32 Frame = 0; Frame < 60; ++Frame)
		{
			Tracker.Update(RecoveryFrame, FVector(0.0, 0.0, 0.0), Facing, true, nullptr, Rules);
		}
		TestTrue(TEXT("Without an area, a safe spot anywhere on the ground"), Tracker.HasSafeSpot());
		TestFalse(TEXT("...10 m down isn't enough"), Tracker.Update(RecoveryFrame, FVector(500.0, 0.0, -1000.0), Facing, false, nullptr, Rules).IsSet());
		const TOptional<EFallRecoveryReason> Deep = Tracker.Update(RecoveryFrame, FVector(500.0, 0.0, -3100.0), Facing, false, nullptr, Rules);
		TestTrue(TEXT("...31 m down is"), Deep.IsSet() && *Deep == EFallRecoveryReason::LongFall);
		TestFalse(TEXT("...and nobody is ever outside"), Tracker.IsOutside());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinimapBoundsTest, "Looter.World.Minimap.Bounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMinimapBoundsTest::RunTest(const FString& Parameters)
{
	// About a meter per texel: the tutorial island keeps its 256, a 384 m valley gets 384, and a huge map stops at 1024.
	TestEqual(TEXT("The tutorial island's 209 m map"), UMinimapSubsystem::ResolutionFor(20900.0), 256);
	TestEqual(TEXT("A 384 m map"), UMinimapSubsystem::ResolutionFor(38400.0), 384);
	TestEqual(TEXT("A 2 km map"), UMinimapSubsystem::ResolutionFor(200000.0), 1024);

	// One texel's rules: inside stays, outside dims, the line covers, and nothing is ever drawn over the void.
	const FColor Land(50, 100, 110, 228);
	const FColor Clear(0, 0, 0, 0);
	const FColor Dimmed = MinimapPaint::BoundaryTexel(Land, false, false);
	TestTrue(TEXT("Inside, a texel stays as painted"), MinimapPaint::BoundaryTexel(Land, true, false) == Land);
	TestTrue(TEXT("Outside, it's darker and lets more of the glass through"), Dimmed.A < Land.A && Dimmed.R < Land.R && Dimmed.G < Land.G && Dimmed.B < Land.B && Dimmed.A > 0);
	TestTrue(TEXT("On the line, it's the line"), MinimapPaint::BoundaryTexel(Land, false, true) == MinimapPaint::BoundaryColor());
	TestTrue(TEXT("Void stays clear, line or not"), MinimapPaint::BoundaryTexel(Clear, false, true) == Clear && MinimapPaint::BoundaryTexel(Clear, true, false) == Clear);

	// A 32 m map at a meter per texel. The valley's floor rises gently east (1-3 m), ridges (40 m) stand past its
	// north, east and south edges, and west of it the Rim drops into a canyon with no collision, which the bake finds
	// as void. Its boundary: a closed edge along each ridge's foot, and an open one a step past the Rim's lip.
	const FBox2D MapBounds(FVector2D(-1600.0, -1600.0), FVector2D(1600.0, 1600.0));
	auto TexelX = [&MapBounds](int32 V) { return MapBounds.Max.X - (V + 0.5) / MapSide * MapBounds.GetSize().X; };
	auto TexelY = [&MapBounds](int32 U) { return MapBounds.Min.Y + (U + 0.5) / MapSide * MapBounds.GetSize().Y; };
	auto At = [](int32 U, int32 V) { return V * MapSide + U; };
	TArray<EMinimapTexel> Kinds;
	TArray<float> Heights;
	Kinds.Init(EMinimapTexel::Ground, MapSide * MapSide);
	Heights.Init(TNumericLimits<float>::Lowest(), MapSide * MapSide);
	for (int32 V = 0; V < MapSide; ++V)
	{
		for (int32 U = 0; U < MapSide; ++U)
		{
			const double X = TexelX(V);
			const double Y = TexelY(U);
			if (Y < -1000.0)
			{
				Kinds[At(U, V)] = EMinimapTexel::Void;
				continue;
			}
			const bool bRidge = X > 1000.0 || X < -1000.0 || Y > 1000.0;
			Heights[At(U, V)] = bRidge ? 4000.f : static_cast<float>(100.0 + (Y + 1000.0) * 0.1);
		}
	}
	const FPlayableBoundary Valley({ FVector2D(-1000.0, -1020.0), FVector2D(1000.0, -1020.0), FVector2D(1000.0, 1000.0), FVector2D(-1000.0, 1000.0) },
		{ true, false, false, false });

	TArray<uint8> Inside;
	TArray<uint8> Line;
	MinimapPaint::RasterizeBoundary(Valley, MapBounds, MapSide, Inside, Line);
	if (!TestEqual(TEXT("A flag per texel"), Inside.Num(), MapSide * MapSide) || !TestEqual(TEXT("...for the line too"), Line.Num(), MapSide * MapSide))
	{
		return false;
	}
	int32 Disagreements = 0;
	for (int32 V = 0; V < MapSide; ++V)
	{
		for (int32 U = 0; U < MapSide; ++U)
		{
			Disagreements += (Inside[At(U, V)] != 0) != Valley.Contains(FVector2D(TexelX(V), TexelY(U))) ? 1 : 0;
		}
	}
	TestEqual(TEXT("Inside texels are exactly those whose centers Contains() says are inside"), Disagreements, 0);

	// The tint spans only the valley's heights: the ridges outside would squash it.
	float MinZ = 0.f;
	float MaxZ = 0.f;
	TestTrue(TEXT("Heights inside"), MinimapPaint::HeightRange(Kinds, Heights, Inside, MinZ, MaxZ));
	TestTrue(FString::Printf(TEXT("...only the valley's (%.0f to %.0f cm)"), MinZ, MaxZ), MinZ >= 100.f && MaxZ <= 300.f);
	MinimapPaint::HeightRange(Kinds, Heights, TConstArrayView<uint8>(), MinZ, MaxZ);
	TestEqual(TEXT("Without the area, the ridges count"), MaxZ, 4000.f);

	MinimapPaint::HeightRange(Kinds, Heights, Inside, MinZ, MaxZ);
	TArray<FColor> Painted;
	MinimapPaint::Terrain(MapSide, Kinds, Heights, MinZ, MaxZ, Painted);
	TArray<FColor> Final = Painted;
	MinimapPaint::ApplyBoundary(Inside, Line, Final);
	const FColor Boundary = MinimapPaint::BoundaryColor();

	TestTrue(TEXT("The valley's middle is as painted"), Final[At(16, 16)] == Painted[At(16, 16)] && Painted[At(16, 16)].A > 0);
	for (const FIntPoint Ridge : { FIntPoint(29, 16), FIntPoint(16, 29), FIntPoint(16, 2) })
	{
		const FColor& Before = Painted[At(Ridge.X, Ridge.Y)];
		const FColor& After = Final[At(Ridge.X, Ridge.Y)];
		TestTrue(FString::Printf(TEXT("The ridge at texel %d, %d is dimmed"), Ridge.X, Ridge.Y), After.A > 0 && After.A < Before.A && After.G < Before.G);
	}
	bool bNorthLine = true;
	bool bOpenEdgeClear = true;
	for (int32 Step = 7; Step <= 24; ++Step)
	{
		// The north edge (X = 10 m) runs between texel rows 5 and 6; the open west edge just west of column 6.
		bNorthLine &= Final[At(Step, 5)] == Boundary && Final[At(Step, 6)] == Boundary;
		bOpenEdgeClear &= Final[At(6, Step)] != Boundary && Final[At(6, Step)] == Painted[At(6, Step)] && Painted[At(6, Step)].A > 0;
	}
	TestTrue(TEXT("The closed north edge is drawn as the boundary line"), bNorthLine);
	TestTrue(TEXT("The open edge gets no line: the Rim's lip shows as the land drew it"), bOpenEdgeClear);

	bool bVoidClear = true;
	for (int32 V = 0; V < MapSide; ++V)
	{
		for (int32 U = 0; U <= 5; ++U)
		{
			bVoidClear &= Final[At(U, V)].A == 0;
		}
	}
	TestTrue(TEXT("The canyon stays void"), bVoidClear);
	TestTrue(TEXT("...even where the boundary line reaches over it"), Line[At(5, 5)] != 0 && Final[At(5, 5)].A == 0);
	return true;
}

#endif
