#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/World/LootLabelLayout.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Weapons/WeaponTypes.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The loot labels' layout on screen (UI/World/LootLabelLayout; ULootLabelSubsystem plays it): at an opened strongbox a
// ward's label drew over a gun's focused card (play-test 2026-10-22); a pile of guns stacked a tower of names (2026-10-27).

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

	// The pickup feed's name flash for a rare drop is skipped only for the gun whose plate is up (looked at, label showing).
	TestTrue(TEXT("Looked at, label up: the plate names it"), LootLabelLayout::PlateNamesGun(true, true));
	TestFalse(TEXT("Not looked at: its name is not on a plate"), LootLabelLayout::PlateNamesGun(false, true));
	TestFalse(TEXT("Its label down (out of range): no plate"), LootLabelLayout::PlateNamesGun(true, false));
	// The looked-at card stays on screen: a gun looked down at close by would put its card past the top.
	{
		const FVector2D View(1920.0, 1080.0);
		const FLootLabelBox High = LabelBox(960.0, 120.0, 420.0, 300.0, 150.f);
		TestEqual(TEXT("Past the top: pushed down to the margin"), LootLabelLayout::KeepOnScreen(High, 0.f, View, 16.f).Y, 196.0);
		const FLootLabelBox Fine = LabelBox(960.0, 700.0, 420.0, 300.0, 150.f);
		TestTrue(TEXT("On screen: left alone"), LootLabelLayout::KeepOnScreen(Fine, 0.f, View, 16.f).IsNearlyZero());
		const FLootLabelBox Edge = LabelBox(100.0, 700.0, 420.0, 300.0, 150.f);
		TestEqual(TEXT("Past the left edge: pushed right"), LootLabelLayout::KeepOnScreen(Edge, 0.f, View, 16.f).X, 126.0);
	}
	return true;
}

namespace
{
	FLootPileLabel PileLabel(const FVector& Where, float Distance, EWeaponRarity Rarity, bool bFocused = false, bool bWasShown = false)
	{
		FLootPileLabel Label;
		Label.Location = Where;
		Label.Distance = Distance;
		Label.Rank = static_cast<int32>(Rarity);
		Label.bFocused = bFocused;
		Label.bWasShown = bWasShown;
		return Label;
	}

	int32 CountShown(const TArray<bool>& Shown)
	{
		int32 Count = 0;
		for (const bool bShown : Shown)
		{
			Count += bShown ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootLabelPileTest, "Looter.Loot.Labels.Pile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootLabelPileTest::RunTest(const FString& Parameters)
{
	// A boss's drop of twelve guns once stacked twelve names into a tower (playtest 2026-10-27): a pile shows a few names in
	// full, the looked-at one, every legendary, then the rarer and the nearer; the rest fold to their rarity's dot.
	constexpr float Radius = 300.f;
	constexpr int32 MaxShown = 4;
	constexpr int32 Always = static_cast<int32>(EWeaponRarity::Legendary);

	// Twelve commons lying within a metre and a half: the four nearest show.
	TArray<FLootPileLabel> Commons;
	for (int32 Index = 0; Index < 12; ++Index)
	{
		Commons.Add(PileLabel(FVector(Index * 12.0, 0.0, 0.0), 500.f + Index * 10.f, EWeaponRarity::Common));
	}
	const TArray<bool> CommonShown = LootLabelLayout::PickShown(Commons, Radius, MaxShown, Always);
	TestEqual(TEXT("Twelve together: four names"), CountShown(CommonShown), MaxShown);
	TestTrue(TEXT("...the four nearest"), CommonShown[0] && CommonShown[1] && CommonShown[2] && CommonShown[3] && !CommonShown[4]);

	// Looking at a far one in the pile shows it (and a near one folds to make room).
	TArray<FLootPileLabel> LookedAt = Commons;
	LookedAt[11].bFocused = true;
	const TArray<bool> LookedShown = LootLabelLayout::PickShown(LookedAt, Radius, MaxShown, Always);
	TestTrue(TEXT("The looked-at one always shows"), LookedShown[11]);
	TestEqual(TEXT("...still four in all"), CountShown(LookedShown), MaxShown);
	TestFalse(TEXT("...the fourth nearest folds for it"), LookedShown[3]);

	// The rarer win the places; every legendary shows, however many.
	TArray<FLootPileLabel> Mixed = Commons;
	Mixed[9].Rank = static_cast<int32>(EWeaponRarity::Epic);
	Mixed[10].Rank = static_cast<int32>(EWeaponRarity::Legendary);
	Mixed[11].Rank = static_cast<int32>(EWeaponRarity::Legendary);
	Mixed[8].Rank = static_cast<int32>(EWeaponRarity::Legendary);
	Mixed[7].Rank = static_cast<int32>(EWeaponRarity::Legendary);
	Mixed[6].Rank = static_cast<int32>(EWeaponRarity::Legendary);
	const TArray<bool> MixedShown = LootLabelLayout::PickShown(Mixed, Radius, MaxShown, Always);
	TestTrue(TEXT("Five legendaries in one pile: all five show"), MixedShown[6] && MixedShown[7] && MixedShown[8] && MixedShown[10] && MixedShown[11]);
	TestFalse(TEXT("...and fill the pile: the epic folds"), MixedShown[9]);
	TestFalse(TEXT("...the commons too"), MixedShown[0]);
	TArray<FLootPileLabel> OneEpic = Commons;
	OneEpic[11].Rank = static_cast<int32>(EWeaponRarity::Epic);
	const TArray<bool> EpicShown = LootLabelLayout::PickShown(OneEpic, Radius, MaxShown, Always);
	TestTrue(TEXT("An epic at the back shows before the nearer commons"), EpicShown[11] && !EpicShown[3]);

	// Two piles apart (a strongbox here, a boss's drop 20 m on): each shows its own four.
	TArray<FLootPileLabel> TwoPiles = Commons;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		TwoPiles.Add(PileLabel(FVector(2000.0 + Index * 12.0, 0.0, 0.0), 1500.f + Index * 10.f, EWeaponRarity::Common));
	}
	const TArray<bool> TwoShown = LootLabelLayout::PickShown(TwoPiles, Radius, MaxShown, Always);
	TestEqual(TEXT("Two piles: four names each"), CountShown(TwoShown), MaxShown * 2);
	TestTrue(TEXT("...the far pile's nearest among them"), TwoShown[12]);

	// Of two alike, the one shown before keeps its name as the player walks (the nearer by a hair doesn't take it).
	const TArray<FLootPileLabel> Pair = { PileLabel(FVector::ZeroVector, 500.f, EWeaponRarity::Rare, false, /*bWasShown*/ false),
		PileLabel(FVector(20.0, 0.0, 0.0), 505.f, EWeaponRarity::Rare, false, /*bWasShown*/ true) };
	const TArray<bool> PairShown = LootLabelLayout::PickShown(Pair, Radius, 1, Always);
	TestTrue(TEXT("The one shown before stays"), !PairShown[0] && PairShown[1]);

	// A few guns: every name, as before; the rule off (0): every name.
	TestEqual(TEXT("Three guns: three names"), CountShown(LootLabelLayout::PickShown(MakeArrayView(Commons.GetData(), 3), Radius, MaxShown, Always)), 3);
	TestEqual(TEXT("Off: all twelve"), CountShown(LootLabelLayout::PickShown(Commons, Radius, 0, Always)), 12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootLabelFoldTest, "Looter.Loot.Labels.Fold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootLabelFoldTest::RunTest(const FString& Parameters)
{
	// A gun's label folded to its dot, opened when looked at, and whole again once unfolded.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWeaponLabelWidget* Label = CreateWidget<UWeaponLabelWidget>(WorldWrapper.GetTestWorld(), UWeaponLabelWidget::StaticClass());
	if (!TestNotNull(TEXT("A loot label"), Label))
	{
		return false;
	}
	Label->TakeWidget();
	TestFalse(TEXT("A label starts whole"), Label->IsFolded() || Label->IsDotShown());
	Label->SetFolded(true);
	TestTrue(TEXT("Folded: its dot"), Label->IsFolded() && Label->IsDotShown());
	Label->TakeWidget()->SlatePrepass(1.f);
	// The dot's halo is 26 units (WeaponLabelWidget.cpp: big enough to read in a legendary beam's glow).
	TestTrue(TEXT("...taking no more room than the dot"), Label->GetDesiredSize().X <= 28.0 && Label->GetDesiredSize().Y <= 28.0);
	// The size the dot is drawn at (its halo's), not the Slate measure, which read under 20 here for a reason not found.
	TestTrue(TEXT("...a dot big enough to read"), Label->GetDotSize().X >= 20.0 && Label->GetDotSize().Y >= 20.0);
	Label->SetFocused(true);
	TestFalse(TEXT("Looked at: the card, not the dot"), Label->IsDotShown());
	Label->SetFocused(false);
	TestTrue(TEXT("Looked away: the dot again"), Label->IsDotShown());
	Label->SetFolded(false);
	TestFalse(TEXT("Unfolded: whole again"), Label->IsDotShown());
	return true;
}

#endif
