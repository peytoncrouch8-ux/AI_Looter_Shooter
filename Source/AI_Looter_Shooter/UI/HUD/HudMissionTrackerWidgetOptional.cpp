// UHudMissionTrackerWidget: the step's optional objectives, under the objective and its hint. Smaller and dimmed, a dashed
// ring for a mark (in the marks' column under the medal) and "Optional:" before each, so they never read as the job; one
// done is checked where it stands (the check pops in over the ring, the line flashes cyan and settles dim, its own small
// sound) and stays checked until the step ends.

#include "UI/HUD/HudMissionTrackerWidget.h"
#include "UI/HUD/HudMissionTrackerShapes.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "HudMissionTracker"

using namespace LooterUI;

namespace
{
	/** The rows sit under the hint, their marks in the column under the objective's circle, their words ending where its words end. */
	constexpr float OptionalTop = 6.f;
	constexpr float OptionalRowGap = 3.f;
	constexpr float OptionalMarkColumn = HudTrackerShapes::MedalBox;
	constexpr float OptionalMarkHeight = 18.f;
	constexpr float OptionalWordsGap = 10.f;
	constexpr int32 OptionalLabelSize = 12;
	constexpr int32 OptionalLabelSpacing = 120;
	constexpr int32 OptionalLineSize = 15;
	/** A long optional line wraps rather than run across the screen, inside the tracker's MaxWidth with its label and count. */
	constexpr float OptionalWrap = 330.f;
	/** The row's nudge as it's struck off (toward the screen's edge, the tracker's side). */
	constexpr float RowNudge = 8.f;

	/** Out fast, overshooting a touch, settling (a pop's ease). */
	float EaseOutBack(float Share)
	{
		const float T = FMath::Clamp(Share, 0.f, 1.f) - 1.f;
		constexpr float Overshoot = 1.7f;
		return 1.f + T * T * ((Overshoot + 1.f) * T + Overshoot);
	}

	/** The check's pop: from this big, overshooting a little as it settles. */
	constexpr float CheckPopFrom = 1.9f;
}

UWidget* UHudMissionTrackerWidget::BuildOptionals()
{
	OptionalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	OptionalBox->SetVisibility(ESlateVisibility::Collapsed);
	OptionalBox->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
	UVerticalBox* Holder = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UVerticalBoxSlot* BoxSlot = Holder->AddChildToVerticalBox(OptionalBox);
	BoxSlot->SetHorizontalAlignment(HAlign_Right);
	BoxSlot->SetPadding(FMargin(0.f, OptionalTop, 0.f, 0.f));
	return Holder;
}

void UHudMissionTrackerWidget::EnsureOptionalRows(int32 Count)
{
	while (OptionalBox && OptionalRows.Num() < Count)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		// "OPTIONAL:" small and cyan, then the line dimmed (a long one wraps, right-aligned), then its count.
		UTextBlock* Label = MakeFloatingText(WidgetTree, OptionalLabelSize, Color::Hairline(), OptionalLabelSpacing);
		Label->SetText(LOCTEXT("Optional", "OPTIONAL:"));
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		UTextBlock* Line = MakeFloatingText(WidgetTree, OptionalLineSize, Color::TextDim(), 0, ETextJustify::Right);
		Line->SetWrapTextAt(OptionalWrap);
		Row->AddChildToHorizontalBox(Line)->SetVerticalAlignment(VAlign_Center);
		UTextBlock* CountWords = MakeFloatingText(WidgetTree, OptionalLineSize, Color::CyanText());
		UHorizontalBoxSlot* CountSlot = Row->AddChildToHorizontalBox(CountWords);
		CountSlot->SetVerticalAlignment(VAlign_Center);
		CountSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));

		// The mark in the column: the dashed ring, the check over it once done (it pops about its middle).
		UOverlay* Mark = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* Ring = MakeImage(WidgetTree, HudTrackerShapes::MarkBrush(EHudTrackerMark::Optional));
		UOverlaySlot* RingSlot = Mark->AddChildToOverlay(Ring);
		RingSlot->SetHorizontalAlignment(HAlign_Center);
		RingSlot->SetVerticalAlignment(VAlign_Center);
		UImage* Check = MakeImage(WidgetTree, HudTrackerShapes::MarkBrush(EHudTrackerMark::OptionalDone));
		Check->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Check->SetVisibility(ESlateVisibility::Collapsed);
		UOverlaySlot* CheckSlot = Mark->AddChildToOverlay(Check);
		CheckSlot->SetHorizontalAlignment(HAlign_Center);
		CheckSlot->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* MarkSlot = Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, OptionalMarkColumn, OptionalMarkHeight));
		MarkSlot->SetVerticalAlignment(VAlign_Center);
		MarkSlot->SetPadding(FMargin(OptionalWordsGap, 0.f, 0.f, 0.f));

		UVerticalBoxSlot* RowSlot = OptionalBox->AddChildToVerticalBox(Row);
		RowSlot->SetHorizontalAlignment(HAlign_Right);
		RowSlot->SetPadding(FMargin(0.f, OptionalRows.IsEmpty() ? 0.f : OptionalRowGap, 0.f, 0.f));
		OptionalRows.Add(Row);
		OptionalMarks.Add(Ring);
		OptionalChecks.Add(Check);
		OptionalLines.Add(Line);
		OptionalCounts.Add(CountWords);
		OptionalTickAges.Add(OptionalTickSeconds);
	}
}

void UHudMissionTrackerWidget::PaintOptionals()
{
	if (!OptionalBox)
	{
		return;
	}
	const int32 Count = Shown.Optionals.Num();
	EnsureOptionalRows(Count);
	OptionalBox->SetVisibility(Count > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	while (OptionalTickAges.Num() < OptionalRows.Num())
	{
		OptionalTickAges.Add(OptionalTickSeconds);
	}
	for (int32 Index = 0; Index < OptionalRows.Num(); ++Index)
	{
		if (Index >= Count)
		{
			OptionalRows[Index]->SetVisibility(ESlateVisibility::Collapsed);
			OptionalTickAges[Index] = OptionalTickSeconds;
			continue;
		}
		const FMissionTrackerOptional& Optional = Shown.Optionals[Index];
		OptionalRows[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
		OptionalLines[Index]->SetText(FText::FromString(Optional.Line));
		OptionalCounts[Index]->SetText(FText::FromString(Optional.Count));
		OptionalCounts[Index]->SetVisibility(Optional.Count.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		OptionalMarks[Index]->SetVisibility(Optional.bDone ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
		OptionalChecks[Index]->SetVisibility(Optional.bDone ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		// At rest (not mid-tick): dim, a done one a shade dimmer still.
		if (OptionalTickAges[Index] >= OptionalTickSeconds)
		{
			OptionalLines[Index]->SetColorAndOpacity(FSlateColor(Color::TextDim()));
			OptionalLines[Index]->SetRenderOpacity(Optional.bDone ? 0.7f : 1.f);
			OptionalChecks[Index]->SetRenderScale(FVector2D(1.f));
			OptionalChecks[Index]->SetRenderOpacity(1.f);
			OptionalRows[Index]->SetRenderTranslation(FVector2D::ZeroVector);
		}
	}
}

void UHudMissionTrackerWidget::StartOptionalTick(int32 Index)
{
	if (!OptionalTickAges.IsValidIndex(Index))
	{
		return;
	}
	OptionalTickAges[Index] = 0.f;
	AnimateOptionals(0.f);
}

void UHudMissionTrackerWidget::AnimateOptionals(float DeltaTime)
{
	for (int32 Index = 0; Index < OptionalTickAges.Num(); ++Index)
	{
		float& Age = OptionalTickAges[Index];
		if (Age >= OptionalTickSeconds || !OptionalChecks.IsValidIndex(Index))
		{
			continue;
		}
		Age = FMath::Min(Age + DeltaTime, OptionalTickSeconds);
		const float Share = Age / OptionalTickSeconds;
		// The check slams in big and settles with a little overshoot, in the first half.
		const float Pop = FMath::Clamp(Share * 2.f, 0.f, 1.f);
		OptionalChecks[Index]->SetRenderScale(FVector2D(FMath::Lerp(CheckPopFrom, 1.f, EaseOutBack(Pop))));
		OptionalChecks[Index]->SetRenderOpacity(FMath::Min(1.f, Pop * 3.f));
		// The line flashes white, then cyan, then settles dim with the rest.
		const FLinearColor Flash = Share < 0.2f ? FMath::Lerp(FLinearColor::White, Color::CyanText(), Share / 0.2f)
			: FMath::Lerp(Color::CyanText(), Color::TextDim(), (Share - 0.2f) / 0.8f);
		OptionalLines[Index]->SetColorAndOpacity(FSlateColor(Flash));
		OptionalLines[Index]->SetRenderOpacity(FMath::Lerp(1.f, 0.7f, Share));
		// The row gives a small nudge toward the edge, as if struck off.
		OptionalRows[Index]->SetRenderTranslation(FVector2D(RowNudge * (1.f - EaseOutBack(Pop)) * 0.5f, 0.f));
	}
}

#undef LOCTEXT_NAMESPACE
