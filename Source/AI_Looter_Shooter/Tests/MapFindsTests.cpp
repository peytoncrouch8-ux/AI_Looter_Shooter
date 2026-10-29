#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/Inventory/MapFinds.h"
#include "UI/Inventory/MapWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The map page's finds block (FINDS.md section 3; the user's call, 2026-10-28: the total and each region's count): the words from
// a made-up tally, the five regions in their order with a done one ticked, the ledger pages on their own line, and nothing at all
// in a level without finds.

namespace
{
	MapFinds::FRegionCount Region(const TCHAR* Name, int32 Found, int32 Total)
	{
		MapFinds::FRegionCount Row;
		Row.Name = FText::FromString(Name);
		Row.Found = Found;
		Row.Total = Total;
		return Row;
	}

	/** Four of fifteen: the farm and point 2 of 4, the west road 1 of 3, the town 1 of 2 (done after one more), the rest none. */
	MapFinds::FCounts MadeUpTally()
	{
		MapFinds::FCounts Counts;
		Counts.Found = 4;
		Counts.Total = 15;
		Counts.LoreFound = 2;
		Counts.LoreTotal = 6;
		Counts.Regions = { Region(TEXT("Farm & Point"), 2, 4), Region(TEXT("West Road"), 1, 3), Region(TEXT("Town & Boot Hill"), 1, 2),
			Region(TEXT("South Fields"), 0, 3), Region(TEXT("The Mountain"), 0, 3) };
		return Counts;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapFindsWordsTest, "Looter.UI.Map.Finds.Words",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMapFindsWordsTest::RunTest(const FString& Parameters)
{
	MapFinds::FCounts Counts = MadeUpTally();
	TestEqual(TEXT("The total"), MapFinds::FindsWords(Counts), FString(TEXT("4 / 15")));
	TestEqual(TEXT("The ledger pages, on their own"), MapFinds::LedgerWords(Counts), FString(TEXT("2 / 6")));
	TestTrue(TEXT("A level with finds"), Counts.HasFinds());
	TestFalse(TEXT("A level without any"), MapFinds::FCounts().HasFinds());
	TestFalse(TEXT("A region part found is not done"), Counts.Regions[0].IsDone());
	TestFalse(TEXT("A region with none in it is not done"), Region(TEXT("Empty"), 0, 0).IsDone());
	TestTrue(TEXT("A region with all found is done"), Region(TEXT("Done"), 2, 2).IsDone());
	TestEqual(TEXT("Five regions"), Counts.Regions.Num(), 5);
	int32 RegionTotal = 0;
	for (const MapFinds::FRegionCount& Row : Counts.Regions)
	{
		RegionTotal += Row.Total;
	}
	TestEqual(TEXT("The regions add up to the total"), RegionTotal, Counts.Total);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapFindsPageTest, "Looter.UI.Map.Finds.Page",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMapFindsPageTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMapWidget* Page = CreateWidget<UMapWidget>(World, UMapWidget::StaticClass());
	if (!TestNotNull(TEXT("The page"), Page))
	{
		return false;
	}
	Page->Open(nullptr);
	const TSharedRef<SWidget> PageSlate = Page->TakeWidget();

	// A test level has no find subsystem: the block stays out of sight.
	TestFalse(TEXT("A level without finds shows no block"), Page->GetFindsView().bShown);

	// The made-up tally: the total, the ledger line and the five regions in order, the done ones ticked.
	Page->ApplyFinds(MadeUpTally());
	UMapWidget::FFindsView View = Page->GetFindsView();
	TestTrue(TEXT("The block shows"), View.bShown);
	TestEqual(TEXT("The total"), View.Finds, FString(TEXT("4 / 15")));
	TestEqual(TEXT("The ledger"), View.Ledger, FString(TEXT("2 / 6")));
	if (TestEqual(TEXT("Five region rows"), View.Regions.Num(), 5))
	{
		TestEqual(TEXT("The first, in capitals"), View.Regions[0], FString(TEXT("FARM & POINT  2 / 4")));
		TestEqual(TEXT("The last"), View.Regions[4], FString(TEXT("THE MOUNTAIN  0 / 3")));
		bool bNoneDone = true;
		for (const bool bDone : View.Done)
		{
			bNoneDone = bNoneDone && !bDone;
		}
		TestTrue(TEXT("None done yet: no tick"), bNoneDone);
	}

	// A region finished: its row ticked, the others not; the total follows.
	MapFinds::FCounts Later = MadeUpTally();
	Later.Found = 5;
	Later.Regions[2].Found = 2;
	Page->ApplyFinds(Later);
	View = Page->GetFindsView();
	TestEqual(TEXT("The total moves"), View.Finds, FString(TEXT("5 / 15")));
	if (TestEqual(TEXT("Still five rows"), View.Regions.Num(), 5))
	{
		TestTrue(TEXT("The finished town is ticked"), View.Done[2]);
		TestFalse(TEXT("The others are not"), View.Done[0] || View.Done[1] || View.Done[3] || View.Done[4]);
	}

	// Fewer regions: the extra rows go. No ledger pages: the line goes.
	MapFinds::FCounts Few = MadeUpTally();
	Few.Regions.SetNum(2);
	Few.LoreTotal = 0;
	Few.LoreFound = 0;
	Page->ApplyFinds(Few);
	View = Page->GetFindsView();
	TestEqual(TEXT("Two rows"), View.Regions.Num(), 2);
	TestTrue(TEXT("No ledger line"), View.Ledger.IsEmpty());

	// Back to a level without finds: gone.
	Page->ApplyFinds(MapFinds::FCounts());
	TestFalse(TEXT("No finds: no block"), Page->GetFindsView().bShown);
	return true;
}

#endif
