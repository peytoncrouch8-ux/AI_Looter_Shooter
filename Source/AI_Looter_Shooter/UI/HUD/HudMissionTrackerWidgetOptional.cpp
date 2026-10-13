// UHudMissionTrackerWidget: the step's optional objectives, under the objective and its hint. Smaller and dimmed, a hollow
// diamond for a mark and "Optional:" before each, so they never read as the job; one done ticks where it stands (the tick
// pops in over the diamond, the line flashes cyan and settles dim, its own small sound) and stays ticked until the step ends.

#include "UI/HUD/HudMissionTrackerWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "HudMissionTracker"

using namespace LooterUI;

namespace
{
	/** The rows sit under the hint, their marks under the objective's chevron, their words under its line. */
	constexpr float OptionalTop = 6.f;
	constexpr float OptionalRowGap = 3.f;
	constexpr float OptionalMarkSize = 12.f;
	constexpr float OptionalTickBox = 16.f;
	constexpr float OptionalMarkInset = 2.f;
	constexpr float OptionalWordsGap = 11.f;
	constexpr int32 OptionalLabelSize = 12;
	constexpr int32 OptionalLabelSpacing = 120;
	constexpr int32 OptionalLineSize = 15;
	/** A long optional line wraps rather than run across the screen. */
	constexpr float OptionalWrap = 470.f;
	/** The tick's pop: from this big, overshooting a little as it settles; the row's nudge from the right. */
	constexpr float TickPopFrom = 1.9f;
	constexpr float RowNudge = 8.f;
	constexpr float PixelsPerUnit = 2.f;

	FPaintLayer Stroke(TArray<FVector2D> Line, float Width, const FLinearColor& LayerColor, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Strokes.Add(MoveTemp(Line));
		Layer.StrokeWidth = Width;
		Layer.Color = LayerColor;
		Layer.Opacity = Opacity;
		return Layer;
	}

	/** The hollow diamond: a cyan outline over a soft dark one, so it reads over sky as the chevron does. */
	const FPaintedIcon& DiamondPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const float Half = OptionalMarkSize * 0.5f;
			const TArray<FVector2D> Diamond = { FVector2D(Half, 1.5f), FVector2D(OptionalMarkSize - 1.5f, Half), FVector2D(Half, OptionalMarkSize - 1.5f),
				FVector2D(1.5f, Half), FVector2D(Half, 1.5f) };
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(OptionalMarkSize);
			Result.Layers.Add(Stroke(Diamond, 3.2f, Color::Ink(), 0.7f));
			Result.Layers.Add(Stroke(Diamond, 1.4f, Color::Hairline(), 0.85f));
			return Result;
		}();
		return Picture;
	}

	/** The tick that takes its place once done: the objective's tick, a size down. */
	const FPaintedIcon& TickPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const TArray<FVector2D> Tick = { FVector2D(3.f, 8.5f), FVector2D(6.5f, 12.f), FVector2D(13.f, 4.f) };
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(OptionalTickBox);
			Result.Layers.Add(Stroke(Tick, 4.4f, Color::Ink()));
			Result.Layers.Add(Stroke(Tick, 2.1f, Color::Hairline()));
			return Result;
		}();
		return Picture;
	}

	/** Out fast, overshooting a touch, settling (a pop's ease). */
	float EaseOutBack(float Share)
	{
		const float T = FMath::Clamp(Share, 0.f, 1.f) - 1.f;
		constexpr float Overshoot = 1.7f;
		return 1.f + T * T * ((Overshoot + 1.f) * T + Overshoot);
	}
}

UWidget* UHudMissionTrackerWidget::BuildOptionals()
{
	OptionalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	OptionalBox->SetVisibility(ESlateVisibility::Collapsed);
	OptionalBox->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	UVerticalBox* Holder = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Holder->AddChildToVerticalBox(OptionalBox)->SetPadding(FMargin(0.f, OptionalTop, 0.f, 0.f));
	return Holder;
}

void UHudMissionTrackerWidget::EnsureOptionalRows(int32 Count)
{
	while (OptionalBox && OptionalRows.Num() < Count)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		// The mark: a hollow diamond, the tick drawn over it once done (a canvas, so the tick's pop can reach past the box).
		UCanvasPanel* Mark = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		UImage* Diamond = MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudTrackerOptionalMark"), DiamondPicture(), PixelsPerUnit,
			FVector2D(OptionalMarkSize)));
		Mark->AddChildToCanvas(Diamond)->SetSize(FVector2D(OptionalMarkSize));
		UImage* Tick = MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudTrackerOptionalTick"), TickPicture(), PixelsPerUnit,
			FVector2D(OptionalTickBox)));
		Tick->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Tick->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* TickSlot = Mark->AddChildToCanvas(Tick);
		TickSlot->SetPosition(FVector2D((OptionalMarkSize - OptionalTickBox) * 0.5f));
		TickSlot->SetSize(FVector2D(OptionalTickBox));
		UHorizontalBoxSlot* MarkSlot = Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, OptionalMarkSize, OptionalMarkSize));
		MarkSlot->SetVerticalAlignment(VAlign_Center);
		MarkSlot->SetPadding(FMargin(OptionalMarkInset, 0.f, 0.f, 0.f));

		// "OPTIONAL:" small and cyan, then the line dimmed, then its count.
		UTextBlock* Label = MakeFloatingText(WidgetTree, OptionalLabelSize, Color::Hairline(), OptionalLabelSpacing);
		Label->SetText(LOCTEXT("Optional", "OPTIONAL:"));
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetPadding(FMargin(OptionalWordsGap, 0.f, 6.f, 0.f));
		UTextBlock* Line = MakeFloatingText(WidgetTree, OptionalLineSize, Color::TextDim());
		Line->SetWrapTextAt(OptionalWrap);
		Row->AddChildToHorizontalBox(Line)->SetVerticalAlignment(VAlign_Center);
		UTextBlock* CountWords = MakeFloatingText(WidgetTree, OptionalLineSize, Color::CyanText());
		UHorizontalBoxSlot* CountSlot = Row->AddChildToHorizontalBox(CountWords);
		CountSlot->SetVerticalAlignment(VAlign_Center);
		CountSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));

		OptionalBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, OptionalRows.IsEmpty() ? 0.f : OptionalRowGap, 0.f, 0.f));
		OptionalRows.Add(Row);
		OptionalMarks.Add(Diamond);
		OptionalTicks.Add(Tick);
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
		OptionalTicks[Index]->SetVisibility(Optional.bDone ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		// At rest (not mid-tick): dim, a done one a shade dimmer still.
		if (OptionalTickAges[Index] >= OptionalTickSeconds)
		{
			OptionalLines[Index]->SetColorAndOpacity(FSlateColor(Color::TextDim()));
			OptionalLines[Index]->SetRenderOpacity(Optional.bDone ? 0.7f : 1.f);
			OptionalTicks[Index]->SetRenderScale(FVector2D(1.f));
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
		if (Age >= OptionalTickSeconds || !OptionalTicks.IsValidIndex(Index))
		{
			continue;
		}
		Age = FMath::Min(Age + DeltaTime, OptionalTickSeconds);
		const float Share = Age / OptionalTickSeconds;
		// The tick slams in big and settles with a little overshoot, in the first half.
		const float Pop = FMath::Clamp(Share * 2.f, 0.f, 1.f);
		OptionalTicks[Index]->SetRenderScale(FVector2D(FMath::Lerp(TickPopFrom, 1.f, EaseOutBack(Pop))));
		OptionalTicks[Index]->SetRenderOpacity(FMath::Min(1.f, Pop * 3.f));
		// The line flashes white, then cyan, then settles dim with the rest.
		const FLinearColor Flash = Share < 0.2f ? FMath::Lerp(FLinearColor::White, Color::CyanText(), Share / 0.2f)
			: FMath::Lerp(Color::CyanText(), Color::TextDim(), (Share - 0.2f) / 0.8f);
		OptionalLines[Index]->SetColorAndOpacity(FSlateColor(Flash));
		OptionalLines[Index]->SetRenderOpacity(FMath::Lerp(1.f, 0.7f, Share));
		// The row gives a small nudge, as if struck off.
		OptionalRows[Index]->SetRenderTranslation(FVector2D(RowNudge * (1.f - EaseOutBack(Pop)) * -0.5f, 0.f));
	}
}

#undef LOCTEXT_NAMESPACE
