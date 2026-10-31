// UHudMissionTrackerWidget's widgets: top-left, a little under the corner, left-aligned (the user's call, 2026-10-31; it was
// right-aligned under the minimap). Its lines start after a column of marks under the medal: the medal then the title, the
// objective's big circle then its line (the user: the task on the circle's right), the key hint and the optional
// objectives under it. No step row (the user: no "3 / 4", just the next objective). The pictures are
// HudMissionTrackerShapes.cpp's; how the tracker follows the tracked mission is in HudMissionTrackerWidget.cpp.

#include "UI/HUD/HudMissionTrackerWidget.h"
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

	/** The marks' column on the left (as wide as the medal, so every mark centres under it), and the gap after it. */
	constexpr float MarkColumn = HudTrackerShapes::MedalBox;
	constexpr float TextGap = 10.f;
	/** The text's left edge, in from the block's: everything that isn't a mark lines up on it. */
	constexpr float TextLeft = MarkColumn + TextGap;

	/** The objective row this far under the title; its count this far after its line. */
	constexpr float ObjectiveTop = 8.f;
	constexpr float CountGap = 9.f;
	/**
	 * A long line (a mission without a short one) wraps rather than run across the screen: room for the line, its count
	 * ("12 s / 30 s" is the longest) and the column, inside MaxWidth.
	 */
	constexpr float CountRoom = 120.f;
	constexpr float LineWrap = UHudMissionTrackerWidget::MaxWidth - TextLeft - CountGap - CountRoom;

	/** The key hint under the row: its line's height and the gap above it. */
	constexpr float HintGap = 4.f;
	constexpr float HintLine = 26.f;
	/** The keycap's key 4 px from its sides, the words 10 px after it. */
	constexpr float KeycapPadding = 4.f;
	constexpr float KeycapGap = 10.f;

	// --- Type: the mockup's sizes, as every HUD widget sets them; letter spacing in thousandths of an em ---

	constexpr int32 TitleSize = 17;
	constexpr int32 TitleSpacing = 94;   // 1.6 px at 17
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

	// The block hangs from its top-left corner, a little under the screen's.
	UVerticalBox* Layers = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BlockSlot = Root->AddChildToCanvas(Layers);
	BlockSlot->SetAnchors(FAnchors(0.f, 0.f));
	BlockSlot->SetAlignment(FVector2D(0.f, 0.f));
	BlockSlot->SetAutoSize(true);
	BlockSlot->SetPosition(FVector2D(Left, Top));
	Layers->SetRenderOpacity(0.f);
	Layers->SetVisibility(ESlateVisibility::Collapsed);
	Block = Layers;

	// Each line hugs the left, starting at the text's edge (TextLeft in from the block's) or, for a mark, in the column.
	auto AddLeft = [Layers](UWidget* Child, float RowTop, float LeftPad)
	{
		UVerticalBoxSlot* ChildSlot = Layers->AddChildToVerticalBox(Child);
		ChildSlot->SetHorizontalAlignment(HAlign_Left);
		ChildSlot->SetPadding(FMargin(LeftPad, RowTop, 0.f, 0.f));
	};

	// The medal (its metal, its glass (a background) and the star), then the title.
	{
		UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UOverlay* Medal = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Medal->AddChildToOverlay(MakeImage(WidgetTree, HudTrackerShapes::MedalMetalBrush()));
		UImage* Glass = MakeImage(WidgetTree, HudTrackerShapes::MedalGlassBrush());
		MarkBackground(Glass);
		Medal->AddChildToOverlay(Glass);
		Medal->AddChildToOverlay(MakeImage(WidgetTree, HudTrackerShapes::MedalStarBrush()));
		TitleRow->AddChildToHorizontalBox(MakeSized(WidgetTree, Medal, MarkColumn, MarkColumn))->SetVerticalAlignment(VAlign_Center);
		TitleText = MakeFloatingText(WidgetTree, TitleSize, FLinearColor::White, TitleSpacing, ETextJustify::Left);
		TitleText->SetWrapTextAt(UHudMissionTrackerWidget::MaxWidth - TextLeft);
		UHorizontalBoxSlot* TitleSlot = TitleRow->AddChildToHorizontalBox(TitleText);
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		TitleSlot->SetPadding(FMargin(TextGap, 0.f, 0.f, 0.f));
		AddLeft(TitleRow, 0.f, 0.f);
	}

	// The objective row: its circle in the column, centred on the first line's middle (the check lies over the ring,
	// shown once done), then the short line (a long one wraps) and its count.
	LineText = MakeFloatingText(WidgetTree, LineSize, FLinearColor::White, 0, ETextJustify::Left);
	LineWrapCounted = LineWrap;
	LineWrapAlone = LineWrap + CountGap + CountRoom;
	LineText->SetWrapTextAt(LineWrap);
	const float RowLine = TrackerLineHeight(LineText->GetFont());
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	UOverlay* Mark = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	ObjectiveRing = MakeMark(WidgetTree, EHudTrackerMark::Objective);
	CentreInOverlay(Mark, ObjectiveRing);
	ObjectiveCheck = MakeMark(WidgetTree, EHudTrackerMark::ObjectiveDone);
	ObjectiveCheck->SetVisibility(ESlateVisibility::Collapsed);
	CentreInOverlay(Mark, ObjectiveCheck);
	const float MarkHeight = FMath::Max(RowLine, static_cast<float>(HudTrackerShapes::MarkSize(EHudTrackerMark::ObjectiveDone).Y));
	Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, MarkColumn, MarkHeight))->SetVerticalAlignment(VAlign_Top);

	// The line sits on the mark's middle: a mark taller than the line pushes it down by half the difference.
	const float LineDrop = FMath::Max(0.f, (MarkHeight - RowLine) * 0.5f);
	UHorizontalBoxSlot* LineSlot = Row->AddChildToHorizontalBox(LineText);
	LineSlot->SetVerticalAlignment(VAlign_Top);
	LineSlot->SetPadding(FMargin(TextGap, LineDrop, 0.f, 0.f));

	// The count pops from its middle.
	CountText = MakeFloatingText(WidgetTree, LineSize, Color::CyanText());
	CountText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CountText->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBoxSlot* CountSlot = Row->AddChildToHorizontalBox(CountText);
	CountSlot->SetVerticalAlignment(VAlign_Top);
	CountSlot->SetPadding(FMargin(CountGap, LineDrop, 0.f, 0.f));

	// The key hint: the keycap (its plate a background, its cyan edge and key solid), then what the key does, starting at
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
	Lower->AddChildToVerticalBox(Row)->SetHorizontalAlignment(HAlign_Left);
	UVerticalBoxSlot* HintSlot = Lower->AddChildToVerticalBox(HintBox);
	HintSlot->SetHorizontalAlignment(HAlign_Left);
	HintSlot->SetPadding(FMargin(TextLeft, HintGap, 0.f, 0.f));
	Lower->AddChildToVerticalBox(BuildOptionals())->SetHorizontalAlignment(HAlign_Left);
	Objective = Lower;
	AddLeft(Lower, ObjectiveTop, 0.f);
}
