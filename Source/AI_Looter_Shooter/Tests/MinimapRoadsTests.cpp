#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/MinimapSubsystem.h"

namespace
{
	// A 64 x 64 picture over 64 m (a meter a texel), all land, and a road running north-south down its middle
	// (world Y 0: the column between U 31 and 32).
	constexpr int32 TestN = 64;
	const FBox2D TestBounds(FVector2D(-3200.0), FVector2D(3200.0));
	const FColor Land(32, 82, 96, 225);

	FMinimapRoadLine NorthSouth(float Width, EMapRoadKind Kind)
	{
		FMinimapRoadLine Line;
		Line.Points = { FVector2D(-3000.0, 0.0), FVector2D(3000.0, 0.0) };
		Line.Width = Width;
		Line.Kind = Kind;
		return Line;
	}

	TArray<FColor> LandPicture()
	{
		TArray<FColor> Colors;
		Colors.Init(Land, TestN * TestN);
		return Colors;
	}

	int32 At(int32 U, int32 V) { return V * TestN + U; }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinimapRoadsTest, "Looter.UI.Minimap.Roads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMinimapRoadsTest::RunTest(const FString& Parameters)
{
	// A 6 m road: its middle the road's fill, a texel past its side the dark edge, well off it the land untouched.
	{
		TArray<FColor> Colors = LandPicture();
		const FMinimapRoadLine Road = NorthSouth(600.f, EMapRoadKind::Road);
		UMinimapSubsystem::PaintRoadLines(Colors, TestN, TestBounds, MakeArrayView(&Road, 1), {});
		TestEqual(TEXT("A road's middle is its fill"), Colors[At(31, 32)], UMinimapSubsystem::RoadFillColor(EMapRoadKind::Road));
		TestEqual(TEXT("Inside its width, still fill"), Colors[At(29, 32)], UMinimapSubsystem::RoadFillColor(EMapRoadKind::Road));
		// Its side is 3 texels from the middle (U 29 and 34 are the last inside): U 35's center lies half a texel past it.
		TestEqual(TEXT("Just past its side, the edge"), Colors[At(35, 32)], UMinimapSubsystem::RoadEdgeColor());
		TestEqual(TEXT("Well off the road, the land"), Colors[At(40, 32)], Land);
	}

	// A path draws no edge, and a 1 m path still shows at least MinPathTexels (2) across: U 31 and 32 are full fill.
	{
		TArray<FColor> Colors = LandPicture();
		const FMinimapRoadLine Path = NorthSouth(100.f, EMapRoadKind::Path);
		UMinimapSubsystem::PaintRoadLines(Colors, TestN, TestBounds, MakeArrayView(&Path, 1), {});
		TestEqual(TEXT("A path's middle is the path's fill"), Colors[At(31, 20)], UMinimapSubsystem::RoadFillColor(EMapRoadKind::Path));
		TestEqual(TEXT("A path has no dark edge"), Colors[At(33, 20)], Land);
		TestTrue(TEXT("The path's fill is paler than a road's edge"),
			UMinimapSubsystem::RoadFillColor(EMapRoadKind::Path) != UMinimapSubsystem::RoadEdgeColor());
	}

	// Only ground takes paint: an obstacle (a building, a fence) and the void keep theirs.
	{
		TArray<FColor> Colors = LandPicture();
		TArray<EMinimapTexel> Kinds;
		Kinds.Init(EMinimapTexel::Ground, TestN * TestN);
		const FColor House(176, 218, 232, 235);
		Colors[At(31, 10)] = House;
		Kinds[At(31, 10)] = EMinimapTexel::Obstacle;
		Colors[At(31, 50)] = FColor(0, 0, 0, 0);
		Kinds[At(31, 50)] = EMinimapTexel::Void;
		const FMinimapRoadLine Road = NorthSouth(600.f, EMapRoadKind::Road);
		UMinimapSubsystem::PaintRoadLines(Colors, TestN, TestBounds, MakeArrayView(&Road, 1), Kinds);
		TestEqual(TEXT("A building on the road keeps its colour"), Colors[At(31, 10)], House);
		TestEqual(TEXT("The void stays clear"), Colors[At(31, 50)], FColor(0, 0, 0, 0));
		TestEqual(TEXT("The ground between them is road"), Colors[At(31, 30)], UMinimapSubsystem::RoadFillColor(EMapRoadKind::Road));
	}

	// Two roads meeting are one shape: no dark edge runs across the junction, inside either road.
	{
		TArray<FColor> Colors = LandPicture();
		FMinimapRoadLine Lines[2] = { NorthSouth(600.f, EMapRoadKind::Road), NorthSouth(600.f, EMapRoadKind::Road) };
		Lines[1].Points = { FVector2D(0.0, 0.0), FVector2D(0.0, 3000.0) };   // east from the middle
		UMinimapSubsystem::PaintRoadLines(Colors, TestN, TestBounds, MakeArrayView(Lines, 2), {});
		TestEqual(TEXT("Where the east road leaves, the north-south road's side is fill, not edge"), Colors[At(35, 32)],
			UMinimapSubsystem::RoadFillColor(EMapRoadKind::Road));
		TestEqual(TEXT("The east road's own side has its edge"), Colors[At(45, 35)], UMinimapSubsystem::RoadEdgeColor());
	}

	// Nothing to paint, or a picture of the wrong size: left as it is.
	{
		TArray<FColor> Colors = LandPicture();
		UMinimapSubsystem::PaintRoadLines(Colors, TestN, TestBounds, {}, {});
		TestEqual(TEXT("No roads, no paint"), Colors[At(31, 32)], Land);
	}
	return true;
}

#endif
