#include "UI/HUD/HudXPBarWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"

// UHudXPBarWidget's widget tree and shapes: the bar's two halves, the level circle between them and the numbers over
// the bar's right end. How it follows the player's progress is in HudXPBarWidget.cpp.

using namespace LooterUI;

namespace
{
	/** The level circle in the bar's middle (reference pixels); the whole cluster is as tall as it. */
	constexpr float BadgeDiameter = 52.f;
	constexpr float BadgeRingWidth = 2.f;
	/** The ring that spreads out of the circle on a level-up: a little bolder than the ring, so it still reads as it fades. */
	constexpr float BadgePulseWidth = 3.f;
	/** Points round the circle: at the drawn size (twice that in the texture) the facets stay well under a pixel. */
	constexpr int32 BadgeSegments = 64;
	/** The circle's textures are drawn at twice the size they show at, so they stay crisp. */
	constexpr float BadgePixelsPerUnit = 2.f;

	/**
	 * The bar: two halves, one each side of the circle, each running in under the circle's ring by XPBarTuck so the bar
	 * reads as passing behind it, level with the circle's middle. Each half is a core XPCoreHeight tall inside a thin dark
	 * edge, leaning like the HUD's other bars (PlayerHUDWidget's BarSlant, the same way as the health bar).
	 */
	constexpr float XPCoreHeight = 8.f;
	constexpr float XPEdge = 2.f;
	constexpr float XPBarHeight = XPCoreHeight + XPEdge * 2.f;
	constexpr float XPBarSlant = -16.f;
	constexpr float XPBarTuck = 4.f;
	constexpr float XPBarTop = (BadgeDiameter - XPBarHeight) * 0.5f;
	constexpr float XPHalfWidth = UHudXPBarWidget::BarWidth * 0.5f;
	/** The circle's and the right half's left edges, and the whole cluster's width. */
	constexpr float BadgeLeft = XPHalfWidth - XPBarTuck;
	constexpr float XPRightHalfLeft = BadgeLeft + BadgeDiameter - XPBarTuck;
	constexpr float XPClusterWidth = XPRightHalfLeft + XPHalfWidth;

	/**
	 * Ten sections, each a tenth of the way to the next level: five in each half, marked by thin notches (the circle
	 * marks the middle).
	 */
	constexpr int32 XPSectionCount = 10;
	constexpr int32 XPSectionsPerHalf = XPSectionCount / 2;
	constexpr float XPTickWidth = 1.5f;
	/** The light line at the earned part's leading edge. */
	constexpr float LeadEdgeWidth = 3.f;
	/** The numbers sit this far over the bar. */
	constexpr float NumbersGap = 3.f;

	/** The badge's circle, Inset in from its edge, clockwise on screen from the right, as a convex polygon. */
	TArray<FVector2D> BadgeCircle(float Inset)
	{
		const float Radius = BadgeDiameter * 0.5f - Inset;
		const FVector2D Center(BadgeDiameter * 0.5f, BadgeDiameter * 0.5f);
		TArray<FVector2D> Points;
		Points.Reserve(BadgeSegments + 1);
		for (int32 Index = 0; Index < BadgeSegments; ++Index)
		{
			const float Angle = 2.f * UE_PI * static_cast<float>(Index) / static_cast<float>(BadgeSegments);
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Points;
	}

	/** The disc (a fill) or a ring StrokeWidth wide that stays inside the circle's box (a closed stroke). */
	FVectorIcon BadgeIcon(bool bFill, float StrokeWidth)
	{
		FVectorIcon Icon;
		Icon.ViewBox = FVector2D(BadgeDiameter, BadgeDiameter);
		Icon.StrokeWidth = StrokeWidth;
		if (bFill)
		{
			Icon.Fills.Add(BadgeCircle(1.f));
		}
		else
		{
			TArray<FVector2D> Ring = BadgeCircle(StrokeWidth * 0.5f + 0.5f);
			// A copy first: adding an element of the array to itself could read it after the array has moved.
			const FVector2D First = Ring[0];
			Ring.Add(First);
			Icon.Strokes.Add(Ring);
		}
		return Icon;
	}

	const FVectorIcon& BadgeDiscIcon() { static const FVectorIcon Icon = BadgeIcon(true, 0.f); return Icon; }
	const FVectorIcon& BadgeRingIcon() { static const FVectorIcon Icon = BadgeIcon(false, BadgeRingWidth); return Icon; }
	const FVectorIcon& BadgePulseIcon() { static const FVectorIcon Icon = BadgeIcon(false, BadgePulseWidth); return Icon; }

	/**
	 * A notch's color: a dark cut through the lit bar (the earned part or the just-earned stretch), a light line on the
	 * faint track still to earn. One color for both would vanish on one of them.
	 */
	FLinearColor LitTickColor() { return Color::Outline() * FLinearColor(1.f, 1.f, 1.f, 0.7f); }
	FLinearColor TrackTickColor() { return Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.45f); }

	/**
	 * Notches at every tenth in one half of the bar: five equal cells, each with one at its right end but the last. The
	 * notches are white, tinted per notch by PaintTicks; they start out in the track's color.
	 */
	UWidget* MakeTenths(UWidgetTree* Tree, TArray<TObjectPtr<UImage>>& OutTicks)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 Index = 0; Index < XPSectionsPerHalf; ++Index)
		{
			UOverlay* Cell = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			if (Index + 1 < XPSectionsPerHalf)
			{
				UImage* Tick = MakeImage(Tree, RectBrush(FLinearColor::White));
				Tick->SetColorAndOpacity(TrackTickColor());
				OutTicks.Add(Tick);
				UOverlaySlot* TickSlot = Cell->AddChildToOverlay(MakeSized(Tree, Tick, XPTickWidth));
				TickSlot->SetHorizontalAlignment(HAlign_Right);
				TickSlot->SetVerticalAlignment(VAlign_Fill);
			}
			Row->AddChildToHorizontalBox(Cell)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		return Row;
	}

	UImage* AddCentered(UWidgetTree* Tree, UOverlay* Overlay, const FSlateBrush& Brush)
	{
		UImage* Image = MakeImage(Tree, Brush);
		UOverlaySlot* LayerSlot = Overlay->AddChildToOverlay(Image);
		LayerSlot->SetHorizontalAlignment(HAlign_Center);
		LayerSlot->SetVerticalAlignment(VAlign_Center);
		return Image;
	}
}

TSharedRef<SWidget> UHudXPBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		// Everything is placed in one overlay by its top-left corner, so the circle, the bar and the numbers line up to the
		// pixel: the bar's halves first, then the numbers over its right end, then the circle over the halves' inner ends.
		UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		// (Not "Padding": that would hide UUserWidget's own.)
		auto Place = [Layers](UWidget* Child, EHorizontalAlignment H, EVerticalAlignment V, const FMargin& Offset)
		{
			UOverlaySlot* ChildSlot = Layers->AddChildToOverlay(Child);
			ChildSlot->SetHorizontalAlignment(H);
			ChildSlot->SetVerticalAlignment(V);
			ChildSlot->SetPadding(Offset);
		};

		FilledSlots.Reset();
		GainedSlots.Reset();
		RestSlots.Reset();
		FillEdges.Reset();
		TickMarks.Reset();
		for (const float HalfLeft : { 0.f, XPRightHalfLeft })
		{
			// A half's core, back to front: the faint track, the earned part (with its leading edge), the just-earned
			// stretch and the rest sharing the width, then the notches at its tenths over it all, each dark on the lit part
			// and light on the track (PaintTicks), so the tenths read on both.
			UOverlay* Core = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			FillOverlaySlot(Core->AddChildToOverlay(MakeImage(WidgetTree, RectBrush(Hex(90, 200, 255, 46)))));

			UOverlay* Earned = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			FillOverlaySlot(Earned->AddChildToOverlay(MakeImage(WidgetTree, RectBrush(Color::Accent()))));
			UImage* Edge = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
			Edge->SetVisibility(ESlateVisibility::Hidden);
			FillEdges.Add(Edge);
			// An overlay never gives a child more room than it has, so a sliver of earned bar shows as a sliver of edge.
			UOverlaySlot* EdgeSlot = Earned->AddChildToOverlay(MakeSized(WidgetTree, Edge, LeadEdgeWidth));
			EdgeSlot->SetHorizontalAlignment(HAlign_Right);
			EdgeSlot->SetVerticalAlignment(VAlign_Fill);

			UHorizontalBox* Split = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			FSlateChildSize NoWidth(ESlateSizeRule::Fill);
			NoWidth.Value = 0.f;
			UHorizontalBoxSlot* Filled = Split->AddChildToHorizontalBox(Earned);
			Filled->SetSize(NoWidth);
			FilledSlots.Add(Filled);
			UHorizontalBoxSlot* Gained = Split->AddChildToHorizontalBox(MakeImage(WidgetTree, RectBrush(Hex(255, 232, 190))));
			Gained->SetSize(NoWidth);
			GainedSlots.Add(Gained);
			UHorizontalBoxSlot* Rest = Split->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()));
			Rest->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			RestSlots.Add(Rest);
			FillOverlaySlot(Core->AddChildToOverlay(Split));
			FillOverlaySlot(Core->AddChildToOverlay(MakeTenths(WidgetTree, TickMarks)));

			// The thin dark edge round the core keeps the bar readable on bright sky and snow; it is the bar's only backing,
			// so it fades with the UI transparency setting. Both halves lean like the HUD's other bars.
			UBorder* Backing = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Backing->SetBrush(RectBrush(Color::Outline() * FLinearColor(1.f, 1.f, 1.f, 0.55f)));
			MarkBackground(Backing);
			Backing->SetPadding(FMargin(XPEdge));
			Backing->SetContent(Core);
			USizeBox* Half = MakeSized(WidgetTree, Backing, XPHalfWidth, XPBarHeight);
			Half->SetRenderShear(FVector2D(XPBarSlant, 0.f));
			Place(Half, HAlign_Left, VAlign_Top, FMargin(HalfLeft, XPBarTop, 0.f, 0.f));
		}

		// Over the bar's right end: "+10 XP" (only after a gain) and "40 / 100 XP".
		UHorizontalBox* Numbers = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		GainText = MakeFloatingText(WidgetTree, 14, Color::Accent(), 60, ETextJustify::Right);
		GainText->SetVisibility(ESlateVisibility::Hidden);
		UHorizontalBoxSlot* PlusSlot = Numbers->AddChildToHorizontalBox(GainText);
		PlusSlot->SetVerticalAlignment(VAlign_Bottom);
		PlusSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		XPText = MakeFloatingText(WidgetTree, 13, Color::TextDim(), 60, ETextJustify::Right);
		Numbers->AddChildToHorizontalBox(XPText)->SetVerticalAlignment(VAlign_Bottom);
		Place(Numbers, HAlign_Right, VAlign_Bottom, FMargin(0.f, 0.f, 0.f, BadgeDiameter - XPBarTop + NumbersGap));

		// The level circle, between the halves: the pulse ring (behind, spreading out on a level-up), the dark inside (a
		// background, so it fades with the UI transparency setting), the accent ring and the number.
		UOverlay* Badge = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		const FVector2D BadgeSize(BadgeDiameter, BadgeDiameter);
		BadgePulse = AddCentered(WidgetTree, Badge, IconBrush(TEXT("HudXPBadgePulse"), BadgePulseIcon(), BadgePixelsPerUnit, BadgeSize, FLinearColor::White));
		BadgePulse->SetColorAndOpacity(Color::Accent());
		BadgePulse->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		BadgePulse->SetVisibility(ESlateVisibility::Hidden);
		UImage* Inside = AddCentered(WidgetTree, Badge, IconBrush(TEXT("HudXPBadgeDisc"), BadgeDiscIcon(), BadgePixelsPerUnit, BadgeSize, FLinearColor::White));
		Inside->SetColorAndOpacity(Hex(7, 26, 40, 150));
		MarkBackground(Inside);
		BadgeRing = AddCentered(WidgetTree, Badge, IconBrush(TEXT("HudXPBadgeRing"), BadgeRingIcon(), BadgePixelsPerUnit, BadgeSize, FLinearColor::White));
		// Chakra Petch's digits sit in the middle of their line, so centering the line centers them in the circle.
		LevelValue = MakeFloatingText(WidgetTree, 21, Color::Text(), 0, ETextJustify::Center);
		UOverlaySlot* LevelSlot = Badge->AddChildToOverlay(LevelValue);
		LevelSlot->SetHorizontalAlignment(HAlign_Center);
		LevelSlot->SetVerticalAlignment(VAlign_Center);
		Place(MakeSized(WidgetTree, Badge, BadgeDiameter, BadgeDiameter), HAlign_Left, VAlign_Top, FMargin(BadgeLeft, 0.f, 0.f, 0.f));

		USizeBox* Root = MakeSized(WidgetTree, Layers, XPClusterWidth, BadgeDiameter);
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		Cluster = Root;
		WidgetTree->RootWidget = Root;

		ShownFill = -1.f;
		ShownGainEnd = -1.f;
		ShownLitTicks = INDEX_NONE;
		PaintBadge(0.f);
		PaintEdge(0.f);
	}
	return Super::RebuildWidget();
}

void UHudXPBarWidget::PaintTicks(float LitEnd)
{
	// The notches sit at the tenths' ends, all but the middle one (the circle): a notch is on the lit part once LitEnd
	// reaches its tenth's end. LitEnd comes in whole bar steps (BarWidth of them), and a tenth's end is a whole number of
	// steps too, so the comparison is exact.
	auto NotchAt = [](int32 Index)
	{
		const int32 Tenth = Index + 1 + (Index >= XPSectionsPerHalf - 1 ? 1 : 0);
		return static_cast<float>(Tenth) / static_cast<float>(XPSectionCount);
	};
	int32 LitTicks = 0;
	while (LitTicks < TickMarks.Num() && NotchAt(LitTicks) <= LitEnd)
	{
		++LitTicks;
	}
	if (LitTicks == ShownLitTicks)
	{
		return;
	}
	ShownLitTicks = LitTicks;
	for (int32 Index = 0; Index < TickMarks.Num(); ++Index)
	{
		if (UImage* Tick = TickMarks[Index])
		{
			Tick->SetColorAndOpacity(Index < LitTicks ? LitTickColor() : TrackTickColor());
		}
	}
}
