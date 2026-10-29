// UMapWidget: the finds block (FINDS.md section 3): "FINDS 4 / 15" with the notch tally, "LEDGER 2 / 6" and each region's count,
// floating text over the frame's top-left corner under "MAP · NORTH UP" (the user's call, 2026-10-28: the total and each region's
// count, no pins on the map). The side column has no room for five more rows beside the grave list. Slate & Salt colours.

#include "UI/Inventory/MapWidget.h"
#include "UI/Inventory/MapFinds.h"
#include "UI/Style/LooterUIStyle.h"
#include "World/FindRules.h"
#include "World/FindSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

using namespace LooterUI;

namespace
{
	/** The block's top-left, in from the frame's corner (under "MAP · NORTH UP" at 44). */
	const FVector2D BlockOffset(27.0, 66.0);

	/** The notch tally before "FINDS", the total's type, the ledger's and the regions'. */
	constexpr float TallyWidth = 20.f;
	constexpr float TallyHeight = 18.f;
	constexpr int32 TitleSize = 10;
	constexpr int32 TotalSize = 15;
	constexpr int32 LedgerSize = 9;
	constexpr int32 LedgerCountSize = 11;
	constexpr int32 RegionSize = 8;
	constexpr int32 RegionCountSize = 9;
	constexpr float RegionTickSize = 10.f;
	constexpr float RegionNameWidth = 124.f;
	constexpr float LedgerGap = 4.f;
	constexpr float RegionGap = 2.f;
	constexpr float RegionsTop = 8.f;

	/** The tally (the kit's four cuts and a slash) drawn inked, white to tint: the same as the HUD's notch mark. */
	const FPaintedIcon& InkedTally()
	{
		static const FPaintedIcon Picture = []
		{
			TArray<TArray<FVector2D>> Strokes;
			for (const float X : { 3.5f, 7.f, 10.5f, 14.f })
			{
				Strokes.Add({ FVector2D(X, 3.f), FVector2D(X, 13.f) });
			}
			Strokes.Add({ FVector2D(1.5f, 11.5f), FVector2D(16.5f, 4.5f) });
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(18.f, 16.f);
			FPaintLayer Ink;
			Ink.Strokes = Strokes;
			Ink.StrokeWidth = 3.4f;
			Ink.Color = Color::Ink();
			Result.Layers.Add(Ink);
			FPaintLayer Cuts;
			Cuts.Strokes = Strokes;
			Cuts.StrokeWidth = 1.6f;
			Cuts.Color = FLinearColor::White;
			Result.Layers.Add(Cuts);
			return Result;
		}();
		return Picture;
	}

	/** A tick, inked, white to tint: a region with every find found. */
	const FPaintedIcon& InkedTick()
	{
		static const FPaintedIcon Picture = []
		{
			TArray<TArray<FVector2D>> Stroke;
			Stroke.Add({ FVector2D(1.5f, 5.5f), FVector2D(4.f, 8.f), FVector2D(8.5f, 2.f) });
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(10.f, 10.f);
			FPaintLayer Ink;
			Ink.Strokes = Stroke;
			Ink.StrokeWidth = 3.4f;
			Ink.Color = Color::Ink();
			Result.Layers.Add(Ink);
			FPaintLayer Line;
			Line.Strokes = Stroke;
			Line.StrokeWidth = 1.8f;
			Line.Color = FLinearColor::White;
			Result.Layers.Add(Line);
			return Result;
		}();
		return Picture;
	}

	void SetTextIfChanged(UTextBlock* Text, const FString& Words)
	{
		if (Text && !Text->GetText().ToString().Equals(Words, ESearchCase::CaseSensitive))
		{
			Text->SetText(FText::FromString(Words));
		}
	}
}

// ---------------------------------------------------------------------------
// The words
// ---------------------------------------------------------------------------

FString MapFinds::FindsWords(const FCounts& Counts)
{
	return FString::Printf(TEXT("%d / %d"), Counts.Found, Counts.Total);
}

FString MapFinds::LedgerWords(const FCounts& Counts)
{
	return FString::Printf(TEXT("%d / %d"), Counts.LoreFound, Counts.LoreTotal);
}


MapFinds::FCounts MapFinds::Gather(const UWorld* World)
{
	FCounts Counts;
	const UFindSubsystem* Finds = World ? World->GetSubsystem<UFindSubsystem>() : nullptr;
	if (!Finds)
	{
		return Counts;
	}
	// Assumes FindRules.h's FFindTally { Found, Total, ByRegion } with ByRegion a list of { Region (FText), Found, Total } in the
	// map's order, and the ledger pages' tally from GetLoreTally (FINDS.md section 3). The one place that reads their shapes.
	const FFindTally Tally = Finds->GetTally();
	Counts.Found = Tally.Found;
	Counts.Total = Tally.Total;
	for (const auto& Region : Tally.ByRegion)
	{
		FRegionCount& Row = Counts.Regions.AddDefaulted_GetRef();
		Row.Name = Region.Region;
		Row.Found = Region.Found;
		Row.Total = Region.Total;
	}
	const FFindTally Lore = Finds->GetLoreTally();
	Counts.LoreFound = Lore.Found;
	Counts.LoreTotal = Lore.Total;
	return Counts;
}

// ---------------------------------------------------------------------------
// Building it
// ---------------------------------------------------------------------------

void UMapWidget::BuildFinds(UCanvasPanel& Page)
{
	using namespace MapLayout;
	UVerticalBox* Block = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// "FINDS 4 / 15" with the tally before it.
	UHorizontalBox* Total = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Total->AddChildToHorizontalBox(MakeImage(WidgetTree,
		PaintedIconBrush(TEXT("MapFindsTally"), InkedTally(), 3.f, FVector2D(TallyWidth, TallyHeight), Color::SoulGold())))
		->SetVerticalAlignment(VAlign_Center);
	UTextBlock* Title = MakeFloatingText(WidgetTree, TitleSize, Color::SlateLabel(), 220, ETextJustify::Left);
	Title->SetText(FText::FromString(TEXT("FINDS")));
	UHorizontalBoxSlot* TitleSlot = Total->AddChildToHorizontalBox(Title);
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	TitleSlot->SetPadding(FMargin(8.f, 0.f, 8.f, 0.f));
	FindsCount = MakeFloatingText(WidgetTree, TotalSize, Color::SoulGold(), 100, ETextJustify::Left);
	Total->AddChildToHorizontalBox(FindsCount)->SetVerticalAlignment(VAlign_Center);
	Block->AddChildToVerticalBox(Total);

	// "LEDGER 2 / 6" under it, quieter.
	UHorizontalBox* Ledger = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* LedgerTitle = MakeFloatingText(WidgetTree, LedgerSize, Color::SlateLabel(), 220, ETextJustify::Left);
	LedgerTitle->SetText(FText::FromString(TEXT("LEDGER")));
	Ledger->AddChildToHorizontalBox(MakeSized(WidgetTree, LedgerTitle, TallyWidth + 8.f + 52.f))->SetVerticalAlignment(VAlign_Center);
	LedgerCount = MakeFloatingText(WidgetTree, LedgerCountSize, Color::SlateText(), 100, ETextJustify::Left);
	Ledger->AddChildToHorizontalBox(LedgerCount)->SetVerticalAlignment(VAlign_Center);
	Block->AddChildToVerticalBox(Ledger)->SetPadding(FMargin(0.f, LedgerGap, 0.f, 0.f));

	// The regions, one row each: a tick once every find in it is found, its name and its count.
	FindRegionRows.Reset();
	for (int32 Index = 0; Index < MapFinds::MaxRegions; ++Index)
	{
		FFindRegionRow& Region = FindRegionRows.AddDefaulted_GetRef();
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Region.Tick = MakeImage(WidgetTree,
			PaintedIconBrush(TEXT("MapFindsTick"), InkedTick(), 4.f, FVector2D(RegionTickSize, RegionTickSize), Color::SaltLine()));
		Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Region.Tick, RegionTickSize + 6.f, RegionTickSize))->SetVerticalAlignment(VAlign_Center);
		Region.Name = MakeFloatingText(WidgetTree, RegionSize, Color::SlateLabel(), 160, ETextJustify::Left);
		Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Region.Name, RegionNameWidth))->SetVerticalAlignment(VAlign_Center);
		Region.Count = MakeFloatingText(WidgetTree, RegionCountSize, Color::SlateText(), 100, ETextJustify::Left);
		Row->AddChildToHorizontalBox(Region.Count)->SetVerticalAlignment(VAlign_Center);
		Region.Row = Row;
		Row->SetVisibility(ESlateVisibility::Collapsed);
		Block->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, Index == 0 ? RegionsTop : RegionGap, 0.f, 0.f));
	}

	// Hidden until a level with finds gives it numbers; it never takes the mouse.
	Block->SetVisibility(ESlateVisibility::Collapsed);
	FindsBlock = Block;
	UCanvasPanelSlot* BlockSlot = Page.AddChildToCanvas(Block);
	BlockSlot->SetAutoSize(true);
	BlockSlot->SetPosition(FramePosition + BlockOffset);
}

// ---------------------------------------------------------------------------
// Showing it
// ---------------------------------------------------------------------------

void UMapWidget::RefreshFinds()
{
	ApplyFinds(MapFinds::Gather(GetWorld()));
}

void UMapWidget::ApplyFinds(const MapFinds::FCounts& Counts)
{
	if (!FindsBlock)
	{
		return;
	}
	const ESlateVisibility Wanted = Counts.HasFinds() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (FindsBlock->GetVisibility() != Wanted)
	{
		FindsBlock->SetVisibility(Wanted);
	}
	if (!Counts.HasFinds())
	{
		return;
	}
	SetTextIfChanged(FindsCount, MapFinds::FindsWords(Counts));
	// Every find found: the total turns to the salt colour, as a finished region does.
	if (FindsCount)
	{
		FindsCount->SetColorAndOpacity(Counts.Found >= Counts.Total ? Color::SaltLine() : Color::SoulGold());
	}
	SetTextIfChanged(LedgerCount, MapFinds::LedgerWords(Counts));
	// No ledger pages in the level: the line goes.
	if (LedgerCount && LedgerCount->GetParent())
	{
		LedgerCount->GetParent()->SetVisibility(Counts.LoreTotal > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	for (int32 Index = 0; Index < FindRegionRows.Num(); ++Index)
	{
		const FFindRegionRow& Row = FindRegionRows[Index];
		if (!Counts.Regions.IsValidIndex(Index))
		{
			Row.Row->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		const MapFinds::FRegionCount& Region = Counts.Regions[Index];
		const bool bDone = Region.IsDone();
		Row.Row->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetTextIfChanged(Row.Name, Region.Name.ToString().ToUpper());
		SetTextIfChanged(Row.Count, FString::Printf(TEXT("%d / %d"), Region.Found, Region.Total));
		// A region done turns to the salt colour with a tick before it.
		Row.Name->SetColorAndOpacity(bDone ? Color::SaltLine() : Color::SlateLabel());
		Row.Count->SetColorAndOpacity(bDone ? Color::SaltLine() : Color::SlateText());
		Row.Tick->SetVisibility(bDone ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

UMapWidget::FFindsView UMapWidget::GetFindsView() const
{
	FFindsView Result;
	Result.bShown = FindsBlock && FindsBlock->GetVisibility() != ESlateVisibility::Collapsed;
	if (!Result.bShown)
	{
		return Result;
	}
	Result.Finds = FindsCount ? FindsCount->GetText().ToString() : FString();
	const bool bLedgerShown = LedgerCount && LedgerCount->GetParent() && LedgerCount->GetParent()->GetVisibility() != ESlateVisibility::Collapsed;
	Result.Ledger = bLedgerShown ? LedgerCount->GetText().ToString() : FString();
	for (const FFindRegionRow& Row : FindRegionRows)
	{
		if (Row.Row && Row.Row->GetVisibility() != ESlateVisibility::Collapsed)
		{
			Result.Regions.Add(Row.Name->GetText().ToString() + TEXT("  ") + Row.Count->GetText().ToString());
			Result.Done.Add(Row.Tick->GetVisibility() != ESlateVisibility::Hidden);
		}
	}
	return Result;
}
