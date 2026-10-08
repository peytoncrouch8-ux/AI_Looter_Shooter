// UHudMissionTrackerWidget's widgets and pictures: the medal, the title, the step bar, the route line, the objective row
// and the key hint, placed as the mockup (Docs/HudMockup/NewHud.dc.html) has them. How the tracker follows the tracked
// mission and paints what it shows is in HudMissionTrackerWidget.cpp.

#include "UI/HUD/HudMissionTrackerWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"

using namespace LooterUI;

namespace
{
	// --- Where things sit in the block: 1080p pixels from its top-left, which is (Left, Top) on screen ---
	// Text is placed by the middle of its line in the mockup (its CSS line box), so it sits right whatever height Slate
	// gives a line of the font.

	/** The block's own box: room for a long objective wrapped over two lines (nothing is clipped to it). */
	constexpr float BlockWidth = 720.f;
	constexpr float BlockHeight = 160.f;

	/** The medal's box; the medal is 32 px across, centred in it, at (53, 303) on screen. */
	constexpr float MedalBox = 34.f;
	/** The route line's box, from the block's corner: the line runs from under the medal to the objective row. */
	constexpr float RouteWidth = 48.f;
	constexpr float RouteHeight = 70.f;

	/** The title, from x 80 on screen, its line's middle at y 295 (a 24 px line from 283). */
	constexpr float TitleX = 44.f;
	constexpr float TitleMiddle = 9.f;

	/** The step bar: 208 x 10 from (83, 310) on screen, its ink edge outside that, the sections 2 px inside it. */
	constexpr float BarX = 47.f;
	constexpr float BarY = 24.f;
	constexpr float BarWidth = 208.f;
	constexpr float BarHeight = 10.f;
	constexpr float BarEdge = 1.5f;
	constexpr float BarInset = 2.f;
	constexpr float SectionGap = 3.f;
	/** Leaning like every HUD bar: the top leans right (the mockup's skewX(-16deg)). */
	constexpr float BarSlant = -16.f;

	/** "4 / 6" after the bar, from x 298 on screen, its line's middle at y 313 (a 16 px line from 305). */
	constexpr float StepX = 262.f;
	constexpr float StepMiddle = 27.f;

	/**
	 * The objective row from x 82 on screen, its first line's middle at y 344, where the route line turns in to it: the
	 * chevron, then the line and the count, 9 px apart. A long line wraps downward.
	 */
	constexpr float ObjectiveX = 46.f;
	constexpr float ObjectiveMiddle = 58.f;
	constexpr float MarkSize = 16.f;
	/** The tick's thick dark stroke reaches a little past the chevron's box, so it's drawn in a larger box round it. */
	constexpr float TickBox = 20.f;
	constexpr float RowGap = 9.f;
	/** A long line (a mission without a short one) wraps rather than run across the screen. */
	constexpr float LineWrap = 560.f;

	/** The key hint, from x 107 on screen, 25 px in from the row, its 26 px line's middle at y 375 under a one-line row. */
	constexpr float HintIndent = 25.f;
	constexpr float HintMiddle = 89.f;
	constexpr float HintLine = 26.f;
	/** The keycap: 22 px high and at least as wide, the key 4 px from its sides, the words 10 px after it. */
	constexpr float KeycapSize = 22.f;
	constexpr float KeycapPadding = 4.f;
	constexpr float KeycapGap = 10.f;
	/** Its top-right and bottom-left corners are cut this far, inside a cyan edge this wide. */
	constexpr float KeycapCut = 6.f;
	constexpr float KeycapEdge = 1.4f;
	/** Its ends, this share of its picture each, stay as drawn while its middle stretches to the key's width. */
	constexpr float KeycapEndShare = 0.34f;

	// --- Type: the mockup's sizes, as every HUD widget sets them; letter spacing in thousandths of an em ---

	constexpr int32 TitleSize = 17;
	constexpr int32 TitleSpacing = 94;   // 1.6 px at 17
	constexpr int32 StepSize = 12;
	constexpr int32 StepSpacing = 125;   // 1.5 px at 12
	constexpr int32 LineSize = 18;
	constexpr int32 KeySize = 13;
	constexpr int32 HintSize = 14;

	/** The pictures are drawn at twice the size they show at, so they stay crisp. */
	constexpr float TrackerPixelsPerUnit = 2.f;

	/** How tall Slate sets one line of Font (its tallest characters and its outline), to line things up on its middle. */
	float TrackerLineHeight(const FSlateFontInfo& Font)
	{
		if (FSlateApplication::IsInitialized())
		{
			if (FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer())
			{
				return static_cast<float>(Renderer->GetFontMeasureService()->GetMaxCharacterHeight(Font))
					+ static_cast<float>(Font.OutlineSettings.OutlineSize) * 2.f;
			}
		}
		// Without Slate's renderer (never in play): about what the kit's font measures.
		return Font.Size * 1.75f;
	}

	// --- The pictures (view-box units are the pixels they show at) ---

	/** A circle as a polygon, clockwise on screen from the right. */
	TArray<FVector2D> TrackerCircle(const FVector2D& Center, float Radius)
	{
		constexpr int32 Points = 72;
		TArray<FVector2D> Circle;
		Circle.Reserve(Points + 1);
		for (int32 Index = 0; Index < Points; ++Index)
		{
			const float Angle = 2.f * UE_PI * static_cast<float>(Index) / static_cast<float>(Points);
			Circle.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Circle;
	}

	/** The polyline closed on itself, for a stroke round a shape. */
	TArray<FVector2D> TrackerClosed(TArray<FVector2D> Points)
	{
		// A copy first: adding an element of the array to itself could read it after the array has moved.
		const FVector2D First = Points[0];
		Points.Add(First);
		return Points;
	}

	FPaintLayer TrackerFill(TArray<TArray<FVector2D>> Fills, const FLinearColor& LayerColor, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Fills = MoveTemp(Fills);
		Layer.Color = LayerColor;
		Layer.Opacity = Opacity;
		return Layer;
	}

	FPaintLayer TrackerStroke(TArray<FVector2D> Line, float Width, const FLinearColor& LayerColor, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Strokes.Add(MoveTemp(Line));
		Layer.StrokeWidth = Width;
		Layer.Color = LayerColor;
		Layer.Opacity = Opacity;
		return Layer;
	}

	FVector2D TrackerMedalCenter()
	{
		return FVector2D(MedalBox * 0.5f, MedalBox * 0.5f);
	}

	/**
	 * The medal's metal: an ink rim and a gunmetal ring lit from the top. Both are rings, open where the glass is, so the
	 * glass alone fades with the UI transparency setting.
	 */
	FPaintedIcon MakeMedalMetal()
	{
		const FVector2D Center = TrackerMedalCenter();
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(MedalBox, MedalBox);
		Icon.Layers.Add(TrackerFill({ TrackerCircle(Center, 16.f), TrackerCircle(Center, 12.f) }, Color::Ink()));
		FPaintLayer Ring = TrackerFill({ TrackerCircle(Center, 14.6f), TrackerCircle(Center, 12.f) }, Color::BarMetalHi());
		Ring.GradientTo = Color::BarMetalLow();
		Ring.GradientStart = Center - FVector2D(0.f, 14.6f);
		Ring.GradientEnd = Center + FVector2D(0.f, 14.6f);
		Icon.Layers.Add(Ring);
		return Icon;
	}

	/** The dark glass inside the ring: a background. */
	FPaintedIcon MakeMedalGlass()
	{
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(MedalBox, MedalBox);
		Icon.Layers.Add(TrackerFill({ TrackerCircle(TrackerMedalCenter(), 12.f) }, Color::ScreenBg().CopyWithNewOpacity(0.9f)));
		return Icon;
	}

	/**
	 * The cyan hairline inside the ring and the ranger's star: orange, five points 10.4 px out with inner corners 4.5 px
	 * in, a 1.5 px ink edge and a light line along its upper-left edges.
	 */
	FPaintedIcon MakeMedalStar()
	{
		const FVector2D Center = TrackerMedalCenter();
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(MedalBox, MedalBox);
		Icon.Layers.Add(TrackerStroke(TrackerClosed(TrackerCircle(Center, 12.4f)), 1.f, Color::Hairline(), 0.6f));
		TArray<FVector2D> Star;
		for (int32 Index = 0; Index < 10; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(-90.f + 36.f * static_cast<float>(Index));
			const float Radius = Index % 2 == 0 ? 10.4f : 4.5f;
			Star.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		Icon.Layers.Add(TrackerFill({ Star }, Color::Accent()));
		Icon.Layers.Add(TrackerStroke(TrackerClosed(Star), 1.5f, Color::Ink()));
		Icon.Layers.Add(TrackerStroke({ Center + FVector2D(-7.6f, -2.6f), Center + FVector2D(-2.1f, -2.9f), Center + FVector2D(0.f, -8.1f) },
			1.f, FLinearColor::White, 0.55f));
		return Icon;
	}

	/** The route: down from under the medal and right to 8 px short of the objective, cyan over a soft dark line. */
	FPaintedIcon MakeRoute()
	{
		const TArray<FVector2D> Route = { FVector2D(17.f, 35.f), FVector2D(17.f, 58.f), FVector2D(38.f, 58.f) };
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(RouteWidth, RouteHeight);
		Icon.Layers.Add(TrackerStroke(Route, 3.f, Color::Ink(), 0.45f));
		Icon.Layers.Add(TrackerStroke(Route, 1.2f, Color::Hairline(), 0.8f));
		return Icon;
	}

	/** The objective's chevron: orange, with a 1.6 px ink edge. */
	FPaintedIcon MakeChevron()
	{
		const TArray<FVector2D> Chevron = { FVector2D(2.f, 1.5f), FVector2D(8.f, 1.5f), FVector2D(14.5f, 8.f), FVector2D(8.f, 14.5f),
			FVector2D(2.f, 14.5f), FVector2D(8.5f, 8.f) };
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(MarkSize, MarkSize);
		Icon.Layers.Add(TrackerFill({ Chevron }, Color::Accent()));
		Icon.Layers.Add(TrackerStroke(TrackerClosed(Chevron), 1.6f, Color::Ink()));
		return Icon;
	}

	/** The tick that replaces it once done: cyan over a thick dark line, in a box 2 px larger all round. */
	FPaintedIcon MakeTick()
	{
		const float Shift = (TickBox - MarkSize) * 0.5f;
		const TArray<FVector2D> Tick = { FVector2D(2.5f + Shift, 8.5f + Shift), FVector2D(6.5f + Shift, 12.5f + Shift), FVector2D(14.f + Shift, 4.f + Shift) };
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(TickBox, TickBox);
		Icon.Layers.Add(TrackerStroke(Tick, 5.f, Color::Ink()));
		Icon.Layers.Add(TrackerStroke(Tick, 2.4f, Color::Hairline()));
		return Icon;
	}

	/** The keycap's outline with its top-right and bottom-left corners cut, Inset in from its box. */
	TArray<FVector2D> KeycapOutline(float Inset)
	{
		// Each cut moves in along its diagonal, so its ends sit where the inset edges meet it, a little further along.
		const float Low = Inset;
		const float High = KeycapSize - Inset;
		const float CutInset = KeycapCut + Inset * (UE_SQRT_2 - 1.f);
		return { FVector2D(Low, Low), FVector2D(KeycapSize - CutInset, Low), FVector2D(High, CutInset), FVector2D(High, High),
			FVector2D(CutInset, High), FVector2D(Low, KeycapSize - CutInset) };
	}

	/** The keycap's plate, dark blue from the top down: a background. */
	FPaintedIcon MakeKeycapPlate()
	{
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(KeycapSize, KeycapSize);
		FPaintLayer Plate = TrackerFill({ KeycapOutline(0.f) }, Color::KeycapTop());
		Plate.GradientTo = Color::KeycapBottom();
		Plate.GradientStart = FVector2D(KeycapSize * 0.5f, 0.f);
		Plate.GradientEnd = FVector2D(KeycapSize * 0.5f, KeycapSize);
		Icon.Layers.Add(Plate);
		return Icon;
	}

	/** The keycap's cyan edge, just inside its outline. */
	FPaintedIcon MakeKeycapEdge()
	{
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(KeycapSize, KeycapSize);
		Icon.Layers.Add(TrackerStroke(TrackerClosed(KeycapOutline(KeycapEdge * 0.5f)), KeycapEdge, Color::Hairline()));
		return Icon;
	}

	const FPaintedIcon& MedalMetalIcon() { static const FPaintedIcon Icon = MakeMedalMetal(); return Icon; }
	const FPaintedIcon& MedalGlassIcon() { static const FPaintedIcon Icon = MakeMedalGlass(); return Icon; }
	const FPaintedIcon& MedalStarIcon() { static const FPaintedIcon Icon = MakeMedalStar(); return Icon; }
	const FPaintedIcon& RouteIcon() { static const FPaintedIcon Icon = MakeRoute(); return Icon; }
	const FPaintedIcon& TrackerChevronIcon() { static const FPaintedIcon Icon = MakeChevron(); return Icon; }
	const FPaintedIcon& TrackerTickIcon() { static const FPaintedIcon Icon = MakeTick(); return Icon; }
	const FPaintedIcon& KeycapPlateIcon() { static const FPaintedIcon Icon = MakeKeycapPlate(); return Icon; }
	const FPaintedIcon& KeycapEdgeIcon() { static const FPaintedIcon Icon = MakeKeycapEdge(); return Icon; }

	/** A keycap picture drawn as a box: its ends as drawn, its middle stretched to the key's width. */
	FSlateBrush KeycapBrush(FName Name, const FPaintedIcon& Icon)
	{
		FSlateBrush Brush = PaintedIconBrush(Name, Icon, TrackerPixelsPerUnit, FVector2D(KeycapSize, KeycapSize));
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(KeycapEndShare, 0.f);
		return Brush;
	}
}

TSharedRef<SWidget> UHudMissionTrackerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UHudMissionTrackerWidget::BuildTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Root;

	// The block, at (Left, Top): everything in it placed as the mockup places it, pictures by their corner, text by its line.
	UCanvasPanel* Layers = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	UCanvasPanelSlot* BlockSlot = Root->AddChildToCanvas(Layers);
	BlockSlot->SetAnchors(FAnchors(0.f, 0.f));
	BlockSlot->SetPosition(FVector2D(Left, Top));
	BlockSlot->SetSize(FVector2D(BlockWidth, BlockHeight));
	Layers->SetRenderOpacity(0.f);
	Layers->SetVisibility(ESlateVisibility::Collapsed);
	Block = Layers;

	auto Place = [Layers](UWidget* Child, float X, float Y, const FVector2D& Size = FVector2D::ZeroVector)
	{
		UCanvasPanelSlot* ChildSlot = Layers->AddChildToCanvas(Child);
		ChildSlot->SetPosition(FVector2D(X, Y));
		if (Size.IsZero())
		{
			ChildSlot->SetAutoSize(true);
		}
		else
		{
			ChildSlot->SetSize(Size);
		}
	};
	// A line of text by its left end and the middle of its line.
	auto PlaceLine = [Layers](UWidget* Child, float X, float Middle)
	{
		UCanvasPanelSlot* ChildSlot = Layers->AddChildToCanvas(Child);
		ChildSlot->SetAlignment(FVector2D(0.f, 0.5f));
		ChildSlot->SetPosition(FVector2D(X, Middle));
		ChildSlot->SetAutoSize(true);
	};

	// Back to front: the route line, the medal (metal, glass, star), the title, the step bar and its number, the objective.
	const FVector2D RouteSize(RouteWidth, RouteHeight);
	Place(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudTrackerRoute"), RouteIcon(), TrackerPixelsPerUnit, RouteSize)), 0.f, 0.f, RouteSize);
	const FVector2D MedalSize(MedalBox, MedalBox);
	Place(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudTrackerMedalMetal"), MedalMetalIcon(), TrackerPixelsPerUnit, MedalSize)), 0.f, 0.f, MedalSize);
	UImage* Glass = MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudTrackerMedalGlass"), MedalGlassIcon(), TrackerPixelsPerUnit, MedalSize));
	MarkBackground(Glass);
	Place(Glass, 0.f, 0.f, MedalSize);
	Place(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudTrackerMedalStar"), MedalStarIcon(), TrackerPixelsPerUnit, MedalSize)), 0.f, 0.f, MedalSize);

	TitleText = MakeFloatingText(WidgetTree, TitleSize, FLinearColor::White, TitleSpacing);
	PlaceLine(TitleText, TitleX, TitleMiddle);

	// The step bar, back to front: the dark track (a background), its ink edge, and the sections (EnsureSections).
	UOverlay* Bar = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UImage* Track = MakeImage(WidgetTree, RectBrush(Color::Track().CopyWithNewOpacity(0.8f)));
	MarkBackground(Track);
	FillOverlaySlot(Bar->AddChildToOverlay(Track), FMargin(BarEdge));
	FillOverlaySlot(Bar->AddChildToOverlay(MakeImage(WidgetTree, RectBrush(FLinearColor::Transparent, Color::Ink(), BarEdge))));
	SectionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	FillOverlaySlot(Bar->AddChildToOverlay(SectionRow), FMargin(BarEdge + BarInset));
	const FVector2D BarSize(BarWidth + BarEdge * 2.f, BarHeight + BarEdge * 2.f);
	USizeBox* BarBox = MakeSized(WidgetTree, Bar, BarSize.X, BarSize.Y);
	BarBox->SetRenderShear(FVector2D(BarSlant, 0.f));
	Place(BarBox, BarX - BarEdge, BarY - BarEdge, BarSize);
	StepBar = BarBox;

	StepText = MakeFloatingText(WidgetTree, StepSize, Color::TextDim(), StepSpacing);
	PlaceLine(StepText, StepX, StepMiddle);

	// The objective row: the chevron (or the tick in its place) on the first line's middle, the short line, the count,
	// all on the first line (a long line wraps downward). A canvas holds the mark, so the tick's wider stroke can reach
	// past the chevron's box.
	LineText = MakeFloatingText(WidgetTree, LineSize, FLinearColor::White);
	LineText->SetWrapTextAt(LineWrap);
	const float RowLine = TrackerLineHeight(LineText->GetFont());
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UCanvasPanel* Mark = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Chevron = MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudTrackerChevron"), TrackerChevronIcon(), TrackerPixelsPerUnit, FVector2D(MarkSize, MarkSize)));
	Mark->AddChildToCanvas(Chevron)->SetSize(FVector2D(MarkSize, MarkSize));
	TickMark = MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudTrackerTick"), TrackerTickIcon(), TrackerPixelsPerUnit, FVector2D(TickBox, TickBox)));
	TickMark->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* TickSlot = Mark->AddChildToCanvas(TickMark);
	TickSlot->SetPosition(FVector2D((MarkSize - TickBox) * 0.5f, (MarkSize - TickBox) * 0.5f));
	TickSlot->SetSize(FVector2D(TickBox, TickBox));
	UHorizontalBoxSlot* MarkSlot = Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, MarkSize, MarkSize));
	MarkSlot->SetVerticalAlignment(VAlign_Top);
	MarkSlot->SetPadding(FMargin(0.f, FMath::Max(0.f, (RowLine - MarkSize) * 0.5f), 0.f, 0.f));

	UHorizontalBoxSlot* LineSlot = Row->AddChildToHorizontalBox(LineText);
	LineSlot->SetVerticalAlignment(VAlign_Top);
	LineSlot->SetPadding(FMargin(RowGap, 0.f, 0.f, 0.f));

	// The count pops from its middle.
	CountText = MakeFloatingText(WidgetTree, LineSize, Color::CyanText());
	CountText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CountText->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBoxSlot* CountSlot = Row->AddChildToHorizontalBox(CountText);
	CountSlot->SetVerticalAlignment(VAlign_Top);
	CountSlot->SetPadding(FMargin(RowGap, 0.f, 0.f, 0.f));

	// The key hint: the keycap (its plate a background, its cyan edge and key solid), then what the key does.
	UOverlay* Keycap = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UImage* Plate = MakeImage(WidgetTree, KeycapBrush(TEXT("HudTrackerKeycapPlate"), KeycapPlateIcon()));
	MarkBackground(Plate);
	FillOverlaySlot(Keycap->AddChildToOverlay(Plate));
	FillOverlaySlot(Keycap->AddChildToOverlay(MakeImage(WidgetTree, KeycapBrush(TEXT("HudTrackerKeycapEdge"), KeycapEdgeIcon()))));
	KeyText = MakeFloatingText(WidgetTree, KeySize, FLinearColor::White);
	UOverlaySlot* KeySlot = Keycap->AddChildToOverlay(KeyText);
	KeySlot->SetHorizontalAlignment(HAlign_Center);
	KeySlot->SetVerticalAlignment(VAlign_Center);
	KeySlot->SetPadding(FMargin(KeycapPadding, 0.f));
	USizeBox* KeycapBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	KeycapBox->SetHeightOverride(KeycapSize);
	KeycapBox->SetMinDesiredWidth(KeycapSize);
	KeycapBox->AddChild(Keycap);

	UHorizontalBox* HintRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	HintRow->AddChildToHorizontalBox(KeycapBox)->SetVerticalAlignment(VAlign_Center);
	HintText = MakeFloatingText(WidgetTree, HintSize, Color::TextDim());
	UHorizontalBoxSlot* WordsSlot = HintRow->AddChildToHorizontalBox(HintText);
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(KeycapGap, 0.f, 0.f, 0.f));
	USizeBox* HintBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	HintBox->SetHeightOverride(HintLine);
	HintBox->AddChild(HintRow);
	HintBox->SetVisibility(ESlateVisibility::Collapsed);
	Hint = HintBox;

	// The row and the hint under it slide in together; the hint moves down when a long line wraps. The row's first line
	// sits on ObjectiveMiddle, the hint's on HintMiddle under a one-line row.
	const float RowTop = ObjectiveMiddle - RowLine * 0.5f;
	const float HintDrop = FMath::Max(0.f, HintMiddle - HintLine * 0.5f - (RowTop + RowLine));
	UVerticalBox* Lower = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Lower->AddChildToVerticalBox(Row)->SetHorizontalAlignment(HAlign_Left);
	UVerticalBoxSlot* HintSlot = Lower->AddChildToVerticalBox(HintBox);
	HintSlot->SetHorizontalAlignment(HAlign_Left);
	HintSlot->SetPadding(FMargin(HintIndent, HintDrop, 0.f, 0.f));
	Place(Lower, ObjectiveX, RowTop);
	Objective = Lower;

	StepBar->SetVisibility(ESlateVisibility::Collapsed);
	StepText->SetVisibility(ESlateVisibility::Collapsed);
}

void UHudMissionTrackerWidget::EnsureSections(int32 Count)
{
	while (SectionRow && Sections.Num() < Count)
	{
		// A section: two flat halves, light over dark, tinted by PaintSteps.
		UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UImage* Upper = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		UImage* Lower = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		Section->AddChildToVerticalBox(Upper)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Section->AddChildToVerticalBox(Lower)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UHorizontalBoxSlot* SectionSlot = SectionRow->AddChildToHorizontalBox(Section);
		SectionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SectionSlot->SetPadding(FMargin(Sections.IsEmpty() ? 0.f : SectionGap, 0.f, 0.f, 0.f));
		Sections.Add(Section);
		SectionUppers.Add(Upper);
		SectionLowers.Add(Lower);
	}
}
