// UHudMissionTrackerWidget's widgets: under the minimap, right-aligned to its column (the user's call, 2026-10-14). Its lines
// end at a column of marks under the medal: the title ends in the medal, the step circles and their "4 / 6" sit under the
// title, the objective's line ends in its circle, the key hint and the optional objectives under it. The pictures are
// HudMissionTrackerShapes.cpp's; how the tracker follows the tracked mission is in HudMissionTrackerWidget.cpp.

#include "UI/HUD/HudMissionTrackerWidget.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/HUD/HudMissionTrackerShapes.h"
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
	// --- The column: 1080p pixels ---

	/** The marks' column on the right (as wide as the medal, so every mark centres under it), and the gap before it. */
	constexpr float MarkColumn = HudTrackerShapes::MedalBox;
	constexpr float TextGap = 10.f;
	/** The text's right edge, in from the block's: everything that isn't a mark lines up on it. */
	constexpr float TextRight = MarkColumn + TextGap;

	/** The step row under the title: this far down, the circles this far apart, "4 / 6" this far after them. */
	constexpr float StepTop = 1.f;
	constexpr float StepSpacing = 4.f;
	constexpr float StepCountGap = 8.f;

	/** The objective row this far under the step row; its count this far after its line. */
	constexpr float ObjectiveTop = 6.f;
	constexpr float CountGap = 9.f;
	/**
	 * A long line (a mission without a short one) wraps rather than run across the screen: room for the line, its count
	 * ("12 s / 30 s" is the longest) and the column, inside MaxWidth.
	 */
	constexpr float CountRoom = 120.f;
	constexpr float LineWrap = UHudMissionTrackerWidget::MaxWidth - TextRight - CountGap - CountRoom;

	/** The key hint under the row: its line's height and the gap above it. */
	constexpr float HintGap = 4.f;
	constexpr float HintLine = 26.f;
	/** The keycap's key 4 px from its sides, the words 10 px after it. */
	constexpr float KeycapPadding = 4.f;
	constexpr float KeycapGap = 10.f;

	// --- Type: the mockup's sizes, as every HUD widget sets them; letter spacing in thousandths of an em ---

	constexpr int32 TitleSize = 17;
	constexpr int32 TitleSpacing = 94;   // 1.6 px at 17
	constexpr int32 StepSize = 12;
	constexpr int32 StepLetterSpacing = 125;   // 1.5 px at 12
	constexpr int32 LineSize = 18;
	constexpr int32 KeySize = 13;
	constexpr int32 HintSize = 14;

	/** How tall Slate sets one line of Font (its tallest characters and its outline), to centre a mark on a line's middle. */
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

	/** A mark's picture, centred in its own box size (its pops scale it about its middle). */
	UImage* MakeMark(UWidgetTree* Tree, EHudTrackerMark Mark)
	{
		UImage* Image = MakeImage(Tree, HudTrackerShapes::MarkBrush(Mark));
		Image->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		return Image;
	}

	/** Content centred in an overlay (the marks over one another in their column). */
	void CentreInOverlay(UOverlay* Overlay, UWidget* Content)
	{
		UOverlaySlot* ContentSlot = Overlay->AddChildToOverlay(Content);
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}
}

float UHudMissionTrackerWidget::TopFor(float MinimapScale, bool bMinimapShown)
{
	if (!bMinimapShown)
	{
		return UHudMinimapWidget::Margin;
	}
	return UHudMinimapWidget::Margin + UHudMinimapWidget::FrameBottom(MinimapScale) + TopGap;
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

	// The block hangs from its top-right corner under the minimap; UpdatePlacement moves it with the minimap's size.
	UVerticalBox* Layers = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BlockSlot = Root->AddChildToCanvas(Layers);
	BlockSlot->SetAnchors(FAnchors(1.f, 0.f));
	BlockSlot->SetAlignment(FVector2D(1.f, 0.f));
	BlockSlot->SetAutoSize(true);
	PlacedTop = TopFor(1.f, true);
	BlockSlot->SetPosition(FVector2D(-RightInset, PlacedTop));
	Layers->SetRenderOpacity(0.f);
	Layers->SetVisibility(ESlateVisibility::Collapsed);
	Block = Layers;

	// Each line hugs the right, ending at the text's edge (TextRight in from the block's) or, for a mark, in the column.
	auto AddRight = [Layers](UWidget* Child, float Top, float Right)
	{
		UVerticalBoxSlot* ChildSlot = Layers->AddChildToVerticalBox(Child);
		ChildSlot->SetHorizontalAlignment(HAlign_Right);
		ChildSlot->SetPadding(FMargin(0.f, Top, Right, 0.f));
	};

	// The title, ending in the medal: its metal, its glass (a background) and the star.
	{
		UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		TitleText = MakeFloatingText(WidgetTree, TitleSize, FLinearColor::White, TitleSpacing, ETextJustify::Right);
		TitleText->SetWrapTextAt(UHudMissionTrackerWidget::MaxWidth - TextRight);
		TitleRow->AddChildToHorizontalBox(TitleText)->SetVerticalAlignment(VAlign_Center);
		UOverlay* Medal = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Medal->AddChildToOverlay(MakeImage(WidgetTree, HudTrackerShapes::MedalMetalBrush()));
		UImage* Glass = MakeImage(WidgetTree, HudTrackerShapes::MedalGlassBrush());
		MarkBackground(Glass);
		Medal->AddChildToOverlay(Glass);
		Medal->AddChildToOverlay(MakeImage(WidgetTree, HudTrackerShapes::MedalStarBrush()));
		UHorizontalBoxSlot* MedalSlot = TitleRow->AddChildToHorizontalBox(MakeSized(WidgetTree, Medal, MarkColumn, MarkColumn));
		MedalSlot->SetVerticalAlignment(VAlign_Center);
		MedalSlot->SetPadding(FMargin(TextGap, 0.f, 0.f, 0.f));
		AddRight(TitleRow, 0.f, 0.f);
	}

	// Under the title, ending at the text's edge: the step circles (EnsureSteps makes them), then "4 / 6".
	{
		UHorizontalBox* Steps = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		StepCircles = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Steps->AddChildToHorizontalBox(StepCircles)->SetVerticalAlignment(VAlign_Center);
		StepText = MakeFloatingText(WidgetTree, StepSize, Color::TextDim(), StepLetterSpacing, ETextJustify::Right);
		UHorizontalBoxSlot* CountSlot = Steps->AddChildToHorizontalBox(StepText);
		CountSlot->SetVerticalAlignment(VAlign_Center);
		CountSlot->SetPadding(FMargin(StepCountGap, 0.f, 0.f, 0.f));
		Steps->SetVisibility(ESlateVisibility::Collapsed);
		StepRow = Steps;
		AddRight(Steps, StepTop, TextRight);
	}

	// The objective row: the short line (a long one wraps, its lines right-aligned), its count, and its circle in the
	// column, centred on the first line's middle; the check lies over the ring, shown once done.
	LineText = MakeFloatingText(WidgetTree, LineSize, FLinearColor::White, 0, ETextJustify::Right);
	LineWrapCounted = LineWrap;
	LineWrapAlone = LineWrap + CountGap + CountRoom;
	LineText->SetWrapTextAt(LineWrap);
	const float RowLine = TrackerLineHeight(LineText->GetFont());
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Row->AddChildToHorizontalBox(LineText)->SetVerticalAlignment(VAlign_Top);

	// The count pops from its middle.
	CountText = MakeFloatingText(WidgetTree, LineSize, Color::CyanText());
	CountText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CountText->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBoxSlot* CountSlot = Row->AddChildToHorizontalBox(CountText);
	CountSlot->SetVerticalAlignment(VAlign_Top);
	CountSlot->SetPadding(FMargin(CountGap, 0.f, 0.f, 0.f));

	UOverlay* Mark = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	ObjectiveRing = MakeMark(WidgetTree, EHudTrackerMark::Objective);
	CentreInOverlay(Mark, ObjectiveRing);
	ObjectiveCheck = MakeMark(WidgetTree, EHudTrackerMark::ObjectiveDone);
	ObjectiveCheck->SetVisibility(ESlateVisibility::Collapsed);
	CentreInOverlay(Mark, ObjectiveCheck);
	UHorizontalBoxSlot* MarkSlot = Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, MarkColumn, FMath::Max(RowLine,
		static_cast<float>(HudTrackerShapes::MarkSize(EHudTrackerMark::ObjectiveDone).Y))));
	MarkSlot->SetVerticalAlignment(VAlign_Top);
	MarkSlot->SetPadding(FMargin(TextGap, 0.f, 0.f, 0.f));

	// The key hint: the keycap (its plate a background, its cyan edge and key solid), then what the key does, ending at
	// the text's edge.
	UOverlay* Keycap = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UImage* Plate = MakeImage(WidgetTree, HudTrackerShapes::KeycapPlateBrush());
	MarkBackground(Plate);
	FillOverlaySlot(Keycap->AddChildToOverlay(Plate));
	FillOverlaySlot(Keycap->AddChildToOverlay(MakeImage(WidgetTree, HudTrackerShapes::KeycapEdgeBrush())));
	KeyText = MakeFloatingText(WidgetTree, KeySize, FLinearColor::White);
	UOverlaySlot* KeySlot = Keycap->AddChildToOverlay(KeyText);
	KeySlot->SetHorizontalAlignment(HAlign_Center);
	KeySlot->SetVerticalAlignment(VAlign_Center);
	KeySlot->SetPadding(FMargin(KeycapPadding, 0.f));
	USizeBox* KeycapBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	KeycapBox->SetHeightOverride(HudTrackerShapes::KeycapSize);
	KeycapBox->SetMinDesiredWidth(HudTrackerShapes::KeycapSize);
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

	// The row, the hint and the step's optional objectives (HudMissionTrackerWidgetOptional.cpp) slide in together.
	UVerticalBox* Lower = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Lower->AddChildToVerticalBox(Row)->SetHorizontalAlignment(HAlign_Right);
	UVerticalBoxSlot* HintSlot = Lower->AddChildToVerticalBox(HintBox);
	HintSlot->SetHorizontalAlignment(HAlign_Right);
	HintSlot->SetPadding(FMargin(0.f, HintGap, TextRight, 0.f));
	Lower->AddChildToVerticalBox(BuildOptionals())->SetHorizontalAlignment(HAlign_Right);
	Objective = Lower;
	AddRight(Lower, ObjectiveTop, 0.f);
}

void UHudMissionTrackerWidget::EnsureSteps(int32 Count)
{
	while (StepCircles && StepImages.Num() < Count)
	{
		// A circle, its picture set by PaintSteps; it pops about its middle when it's checked.
		UImage* Circle = MakeMark(WidgetTree, EHudTrackerMark::StepToCome);
		UHorizontalBoxSlot* CircleSlot = StepCircles->AddChildToHorizontalBox(Circle);
		CircleSlot->SetVerticalAlignment(VAlign_Center);
		CircleSlot->SetPadding(FMargin(StepImages.IsEmpty() ? 0.f : StepSpacing, 0.f, 0.f, 0.f));
		StepImages.Add(Circle);
		StepMarks.Add(EHudTrackerMark::StepToCome);
		StepPopAges.Add(1000.f);
	}
}
