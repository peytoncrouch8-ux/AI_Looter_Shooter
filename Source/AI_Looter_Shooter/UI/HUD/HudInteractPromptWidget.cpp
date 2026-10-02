#include "UI/HUD/HudInteractPromptWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Interaction/InteractionComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;

namespace
{
	/** The words: big enough to read at a glance under the crosshair, small enough not to crowd the aim. */
	constexpr int32 TextSize = 15;
	constexpr int32 TextSpacing = 100;
	/** The hold bar: as slim and slanted as the HUD's other bars (the loot card's hold bar, the experience bar). */
	constexpr int32 BarSegments = 16;
	constexpr float BarWidth = 170.f;
	constexpr float BarHeight = 6.f;
	constexpr float BarSlant = 16.f;
	constexpr float SegmentGap = 2.f;
	/** Seconds to fade fully in or out: quick, since it follows the eyes. */
	constexpr float FadeSeconds = 0.12f;

	void SetTextIfChanged(UTextBlock* Text, const FString& Value)
	{
		if (!Text->GetText().ToString().Equals(Value))
		{
			Text->SetText(FText::FromString(Value));
		}
	}

	void SetShown(UWidget* Widget, bool bShown, ESlateVisibility WhenHidden = ESlateVisibility::Collapsed)
	{
		const ESlateVisibility Wanted = bShown ? ESlateVisibility::HitTestInvisible : WhenHidden;
		if (Widget->GetVisibility() != Wanted)
		{
			Widget->SetVisibility(Wanted);
		}
	}
}

TSharedRef<SWidget> UHudInteractPromptWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Lines"));
		Box->SetVisibility(ESlateVisibility::Collapsed);
		Box->SetRenderOpacity(0.f);

		TapLine = MakeLine(nullptr, TapKey, TapWords);
		Box->AddChildToVerticalBox(TapLine)->SetHorizontalAlignment(HAlign_Center);
		HoldLine = MakeLine(TEXT("HOLD"), HoldKey, HoldWords);
		UVerticalBoxSlot* HoldSlot = Box->AddChildToVerticalBox(HoldLine);
		HoldSlot->SetHorizontalAlignment(HAlign_Center);
		HoldSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

		// The hold bar: segments on a faint dark backing, sheared like the HUD's other bars.
		UBorder* Backing = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		FLinearColor BackingColor = Color::Backdrop();
		BackingColor.A *= 0.6f;
		Backing->SetBrush(RectBrush(BackingColor));
		MarkBackground(Backing);
		Backing->SetPadding(FMargin(2.f));
		UHorizontalBox* Segments = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		HoldSegments.Reset();
		for (int32 Index = 0; Index < BarSegments; ++Index)
		{
			// White, tinted per segment as the hold fills.
			UImage* Segment = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
			Segment->SetColorAndOpacity(Color::SegmentOff());
			UHorizontalBoxSlot* SegmentSlot = Segments->AddChildToHorizontalBox(Segment);
			SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < BarSegments ? SegmentGap : 0.f, 0.f));
			HoldSegments.Add(Segment);
		}
		Backing->SetContent(Segments);
		USizeBox* BarBox = MakeSized(WidgetTree, Backing, BarWidth, BarHeight);
		BarBox->SetRenderShear(FVector2D(BarSlant, 0.f));
		BarBox->SetVisibility(ESlateVisibility::Hidden);
		HoldBar = BarBox;
		UVerticalBoxSlot* BarSlot = Box->AddChildToVerticalBox(BarBox);
		BarSlot->SetHorizontalAlignment(HAlign_Center);
		BarSlot->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));

		Lines = Box;
		WidgetTree->RootWidget = Box;
	}
	return Super::RebuildWidget();
}

UWidget* UHudInteractPromptWidget::MakeLine(const TCHAR* Lead, TObjectPtr<UTextBlock>& OutKey, TObjectPtr<UTextBlock>& OutWords)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (Lead)
	{
		UTextBlock* LeadText = MakeFloatingText(WidgetTree, TextSize, Color::Text(), TextSpacing);
		LeadText->SetText(FText::FromString(Lead));
		Row->AddChildToHorizontalBox(LeadText)->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));
	}
	OutKey = MakeFloatingText(WidgetTree, TextSize, Color::Accent(), TextSpacing);
	Row->AddChildToHorizontalBox(OutKey)->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));
	OutWords = MakeFloatingText(WidgetTree, TextSize, Color::Text(), TextSpacing);
	Row->AddChildToHorizontalBox(OutWords);
	return Row;
}

void UHudInteractPromptWidget::Update(const UInteractionComponent* Interaction, const FString& Key, float DeltaSeconds)
{
	if (!Lines)
	{
		return;
	}

	// The words for each way of using it; a line shows only when it has some.
	const AActor* Focused = Interaction ? Interaction->GetFocusedActor() : nullptr;
	const FInteractionOptions& Options = Interaction ? Interaction->GetFocusedOptions() : FInteractionOptions::None();
	const bool bTapLine = Focused && Options.bTap && !Options.TapPrompt.IsEmpty();
	const bool bHoldLine = Focused && Options.bHold && !Options.HoldPrompt.IsEmpty();
	const bool bWanted = bTapLine || bHoldLine;

	if (bWanted)
	{
		const FString Bracketed = FString::Printf(TEXT("[%s]"), *Key);
		SetShown(TapLine, bTapLine);
		if (bTapLine)
		{
			SetTextIfChanged(TapKey, Bracketed);
			SetTextIfChanged(TapWords, Options.TapPrompt.ToString().ToUpper());
		}
		SetShown(HoldLine, bHoldLine);
		if (bHoldLine)
		{
			SetTextIfChanged(HoldKey, Bracketed);
			SetTextIfChanged(HoldWords, Options.HoldPrompt.ToString().ToUpper());
		}

		// The bar keeps its room under a hold line, and fills while the key is held.
		const float Hold = bHoldLine ? Interaction->GetHoldProgress() : 0.f;
		SetShown(HoldBar, Hold > 0.f, bHoldLine ? ESlateVisibility::Hidden : ESlateVisibility::Collapsed);
		if (Hold > 0.f)
		{
			const int32 Lit = FMath::Clamp(FMath::CeilToInt32(Hold * BarSegments), 0, BarSegments);
			for (int32 Index = 0; Index < HoldSegments.Num(); ++Index)
			{
				HoldSegments[Index]->SetColorAndOpacity(Index < Lit ? Color::Accent() : Color::SegmentOff());
			}
		}
	}

	// Quick fades; the words of the last thing stay while it fades out.
	const float Target = bWanted ? 1.f : 0.f;
	if (Opacity != Target)
	{
		Opacity = FMath::Clamp(Opacity + (bWanted ? 1.f : -1.f) * DeltaSeconds / FadeSeconds, 0.f, 1.f);
		Lines->SetRenderOpacity(Opacity);
	}
	SetShown(Lines, Opacity > 0.f);
}
