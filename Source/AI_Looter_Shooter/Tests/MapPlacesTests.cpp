#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/Inventory/MapPlaces.h"

// The map page's place names kept from drawing over each other (UI/Inventory/MapPlaces, Declutter): Widow's Pines and the
// Chapel of Saint Ada overlapped at the first zoom (playtest 2026-10-27).

namespace
{
	MapPlaces::FNameBox Name(const TCHAR* Text, double X, double Y, bool bMajor)
	{
		MapPlaces::FNameBox Box;
		Box.Center = FVector2D(X, Y);
		Box.Size = MapPlaces::NameSize(Text, bMajor);
		Box.bMajor = bMajor;
		return Box;
	}

	FBox2D NameRect(const MapPlaces::FNameBox& Box)
	{
		return FBox2D(Box.Center - Box.Size * 0.5, Box.Center + Box.Size * 0.5);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapPlacesDeclutterTest, "Looter.UI.Map.Places.Declutter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMapPlacesDeclutterTest::RunTest(const FString& Parameters)
{
	// A name's size follows its letters, and a major one is bigger.
	const FVector2D Short = MapPlaces::NameSize(TEXT("DEPOT"), true);
	const FVector2D Long = MapPlaces::NameSize(TEXT("CHAPEL OF SAINT ADA"), true);
	TestTrue(TEXT("A longer name is wider"), Long.X > Short.X * 3.0);
	TestTrue(TEXT("A minor name is smaller"), MapPlaces::NameSize(TEXT("DEPOT"), false).X < Short.X);

	// The page's two that overlapped: 5 units apart in height, 60 apart across, both wider than that.
	TArray<MapPlaces::FNameBox> Boxes = { Name(TEXT("CHAPEL OF SAINT ADA"), 640.0, 530.0, true), Name(TEXT("WIDOW'S PINES"), 580.0, 535.0, true) };
	MapPlaces::Declutter(Boxes, 4.0);
	TestTrue(TEXT("Both still named"), Boxes[0].bShown && Boxes[1].bShown);
	TestFalse(TEXT("...and no longer over each other"), NameRect(Boxes[0]).Intersect(NameRect(Boxes[1])));
	TestEqual(TEXT("The first stays where it was"), Boxes[0].Center.Y, 530.0);
	TestTrue(TEXT("...the second slid"), !FMath::IsNearlyEqual(Boxes[1].Center.Y, 535.0));

	// Names already apart don't move.
	TArray<MapPlaces::FNameBox> Apart = { Name(TEXT("DEPOT"), 100.0, 100.0, true), Name(TEXT("THE SINK"), 400.0, 300.0, true) };
	MapPlaces::Declutter(Apart, 4.0);
	TestTrue(TEXT("Apart: both shown, in place"), Apart[0].bShown && Apart[1].bShown && Apart[0].Center == FVector2D(100.0, 100.0) && Apart[1].Center == FVector2D(400.0, 300.0));

	// A major wins a spot from a minor listed before it.
	TArray<MapPlaces::FNameBox> Ranked = { Name(TEXT("DRY WASH"), 200.0, 200.0, false), Name(TEXT("MAIN STREET"), 200.0, 200.0, true) };
	MapPlaces::Declutter(Ranked, 4.0);
	TestTrue(TEXT("The major keeps its place"), Ranked[1].Center == FVector2D(200.0, 200.0));
	TestFalse(TEXT("...the minor moved off it"), Ranked[0].bShown && NameRect(Ranked[0]).Intersect(NameRect(Ranked[1])));

	// No free spot within reach: the lesser is left off, never drawn over the other.
	TArray<MapPlaces::FNameBox> Packed;
	for (int32 Index = 0; Index < 9; ++Index)
	{
		Packed.Add(Name(TEXT("SAME PLACE"), 300.0, 300.0, true));
	}
	MapPlaces::Declutter(Packed, 4.0);
	int32 Shown = 0;
	for (int32 A = 0; A < Packed.Num(); ++A)
	{
		Shown += Packed[A].bShown ? 1 : 0;
		for (int32 B = A + 1; B < Packed.Num(); ++B)
		{
			TestFalse(TEXT("Nothing shown overlaps"), Packed[A].bShown && Packed[B].bShown && NameRect(Packed[A]).Intersect(NameRect(Packed[B])));
		}
	}
	TestTrue(TEXT("Some are left off, the first always shown"), Shown >= 1 && Shown < Packed.Num() && Packed[0].bShown);

	// An icon on a name's point: the name slides past it (up, down or beside its point) and never covers it.
	const FBox2D Icon(FVector2D(190.0, 90.0), FVector2D(210.0, 110.0));
	TArray<MapPlaces::FNameBox> Covered = { Name(TEXT("THE SINK"), 200.0, 100.0, true) };
	MapPlaces::Declutter(Covered, 4.0, MakeArrayView(&Icon, 1));
	TestTrue(TEXT("Under an icon: still named"), Covered[0].bShown);
	TestFalse(TEXT("...and clear of the icon"), NameRect(Covered[0]).Intersect(Icon));
	TestTrue(TEXT("...a line away at most"), FMath::Abs(Covered[0].Center.Y - 100.0) < 40.0);
	// Two icons side by side on it (the store and a giver on MAIN STREET): the name still finds a clear spot.
	const FBox2D Icons[] = { FBox2D(FVector2D(170.0, 90.0), FVector2D(190.0, 110.0)), FBox2D(FVector2D(210.0, 90.0), FVector2D(230.0, 110.0)) };
	TArray<MapPlaces::FNameBox> Two = { Name(TEXT("MAIN STREET"), 200.0, 100.0, true) };
	MapPlaces::Declutter(Two, 4.0, MakeArrayView(Icons));
	TestTrue(TEXT("Two icons: named, clear of both"), Two[0].bShown && !NameRect(Two[0]).Intersect(Icons[0]) && !NameRect(Two[0]).Intersect(Icons[1]));

	// Names not wanted (out of the frame, a minor before the zoom) are never shown nor in the way.
	TArray<MapPlaces::FNameBox> Hidden = { Name(TEXT("OLD QUARRY"), 100.0, 100.0, false), Name(TEXT("DEPOT"), 100.0, 100.0, true) };
	Hidden[0].bWanted = false;
	MapPlaces::Declutter(Hidden, 4.0);
	TestFalse(TEXT("Not wanted: not shown"), Hidden[0].bShown);
	TestTrue(TEXT("...and the other keeps its place"), Hidden[1].Center == FVector2D(100.0, 100.0));
	return true;
}

#endif
