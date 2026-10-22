#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/World/LootLabelLayout.h"

// The loot labels' layout on screen (UI/World/LootLabelLayout; ULootLabelSubsystem plays it): at an opened strongbox a
// ward's label drew over a gun's focused card (play-test 2026-10-22).

namespace
{
	FLootLabelBox LabelBox(double X, double Y, double W, double H, float Distance, bool bFocused = false, float Previous = 0.f)
	{
		FLootLabelBox Box;
		Box.Anchor = FVector2D(X, Y);
		Box.Size = FVector2D(W, H);
		Box.Distance = Distance;
		Box.bFocused = bFocused;
		Box.Previous = Previous;
		return Box;
	}

	FBox2D Placed(const FLootLabelBox& Box, float Shift)
	{
		return FBox2D(FVector2D(Box.Anchor.X - Box.Size.X * 0.5, Box.Anchor.Y - Box.Size.Y + Shift),
			FVector2D(Box.Anchor.X + Box.Size.X * 0.5, Box.Anchor.Y + Shift));
	}

	bool AnyCrowd(const TArray<FLootLabelBox>& Boxes, const TArray<float>& Shifts, float Gap)
	{
		for (int32 A = 0; A < Boxes.Num(); ++A)
		{
			for (int32 B = A + 1; B < Boxes.Num(); ++B)
			{
				if (LootLabelLayout::Crowds(Placed(Boxes[A], Shifts[A]), Placed(Boxes[B], Shifts[B]), Gap))
				{
					return true;
				}
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootLabelLayoutTest, "Looter.Loot.Labels.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootLabelLayoutTest::RunTest(const FString& Parameters)
{
	constexpr float Gap = 4.f;
	constexpr float Keep = 0.5f;

	// Apart already: nobody moves.
	{
		const TArray<FLootLabelBox> Boxes = { LabelBox(200.0, 300.0, 120.0, 30.0, 300.f), LabelBox(600.0, 300.0, 120.0, 30.0, 320.f) };
		const TArray<float> Shifts = LootLabelLayout::Arrange(Boxes, Gap, Keep);
		TestTrue(TEXT("Labels apart stay put"), Shifts.Num() == 2 && Shifts[0] == 0.f && Shifts[1] == 0.f);
	}

	// The strongbox: a gun's focused card (big) and a ward's small label hanging just under it. The card keeps its place;
	// the ward's steps clear of it, the smaller way (below: 8 down beats 144 up).
	{
		const TArray<FLootLabelBox> Boxes = { LabelBox(480.0, 270.0, 220.0, 120.0, 260.f, /*bFocused*/ true),
			LabelBox(470.0, 290.0, 90.0, 24.0, 240.f) };
		const TArray<float> Shifts = LootLabelLayout::Arrange(Boxes, Gap, Keep);
		TestEqual(TEXT("The focused card never moves"), Shifts[0], 0.f);
		TestTrue(TEXT("The ward's label steps clear"), Shifts[1] != 0.f && !AnyCrowd(Boxes, Shifts, Gap));
		TestTrue(TEXT("...the smaller way: just below the card"), FMath::IsNearlyEqual(Shifts[1], 270.f + Gap - (290.f - 24.f), 0.05f));
	}

	// Three on one spot (a chest's guns and ward landing together): stacked, Gap apart, the focused one where it is.
	{
		const TArray<FLootLabelBox> Boxes = { LabelBox(400.0, 300.0, 100.0, 30.0, 300.f), LabelBox(410.0, 302.0, 100.0, 30.0, 250.f),
			LabelBox(395.0, 298.0, 100.0, 30.0, 280.f, /*bFocused*/ true) };
		const TArray<float> Shifts = LootLabelLayout::Arrange(Boxes, Gap, Keep);
		TestEqual(TEXT("The focused one stays"), Shifts[2], 0.f);
		TestFalse(TEXT("None crowds another"), AnyCrowd(Boxes, Shifts, Gap));
	}

	// It keeps to its side as the view moves: below is now the shorter move (30 to 38), but it was above.
	{
		const TArray<FLootLabelBox> Boxes = { LabelBox(400.0, 300.0, 100.0, 30.0, 200.f, /*bFocused*/ true),
			LabelBox(400.0, 304.0, 100.0, 30.0, 220.f, false, /*Previous*/ -48.f) };
		const TArray<float> Shifts = LootLabelLayout::Arrange(Boxes, Gap, Keep);
		TestTrue(TEXT("Stays above"), Shifts[1] < 0.f && !AnyCrowd(Boxes, Shifts, Gap));
		// No longer in the way: it comes home even though it was moved.
		const TArray<FLootLabelBox> Apart = { Boxes[0], LabelBox(700.0, 304.0, 100.0, 30.0, 220.f, false, -48.f) };
		TestEqual(TEXT("Home once clear"), LootLabelLayout::Arrange(Apart, Gap, Keep)[1], 0.f);
	}
	return true;
}

#endif
