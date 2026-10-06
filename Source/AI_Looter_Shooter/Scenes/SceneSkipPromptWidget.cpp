#include "Scenes/SceneSkipPromptWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;

namespace
{
	/** The words, as the HUD's interact prompt sets them. */
	constexpr int32 TextSize = 15;
	constexpr int32 TextSpacing = 100;

	/** The hold bar: as slim and slanted as the HUD's other bars. */
	constexpr int32 BarSegments = 16;
	constexpr float BarWidth = 170.f;
	constexpr float BarHeight = 6.f;
	constexpr float BarSlant = 16.f;
	constexpr float SegmentGap = 2.f;

	/** In from the screen's bottom right corner, clear of where the HUD's ammo sits. */
	constexpr float ScreenMargin = 56.f;

	/** Seconds to fade fully in or out. */
	constexpr float FadeSeconds = 0.15f;

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

TSharedRef<SWidget> USceneSkipPromptWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);

		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Prompt"));
		Box->SetVisibility(ESlateVisibility::Collapsed);
		Box->SetRenderOpacity(0.f);

		// One line: the lead word, the key in brackets in the accent color, the words.
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		LeadText = MakeFloatingText(WidgetTree, TextSize, Color::Text(), TextSpacing);
		Line->AddChildToHorizontalBox(LeadText)->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));
		KeyText = MakeFloatingText(WidgetTree, TextSize, Color::Accent(), TextSpacing);
		Line->AddChildToHorizontalBox(KeyText)->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));
		WordsText = MakeFloatingText(WidgetTree, TextSize, Color::Text(), TextSpacing);
		Line->AddChildToHorizontalBox(WordsText);
		Box->AddChildToVerticalBox(Line)->SetHorizontalAlignment(HAlign_Right);

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
		BarSlot->SetHorizontalAlignment(HAlign_Right);
		BarSlot->SetPadding(FMargin(0.f, 5.f, 4.f, 0.f));

		UOverlaySlot* PromptSlot = Root->AddChildToOverlay(Box);
		PromptSlot->SetHorizontalAlignment(HAlign_Right);
		PromptSlot->SetVerticalAlignment(VAlign_Bottom);
		PromptSlot->SetPadding(FMargin(0.f, 0.f, ScreenMargin, ScreenMargin));

		Prompt = Box;
		WidgetTree->RootWidget = Root;
	}
	return Super::RebuildWidget();
}

void USceneSkipPromptWidget::Update(bool bShown, const FString& Lead, const FString& Key, const FString& Words, float Hold, float DeltaSeconds)
{
	if (!Prompt)
	{
		return;
	}
	if (bShown)
	{
		SetShown(LeadText, !Lead.IsEmpty());
		SetTextIfChanged(LeadText, Lead);
		SetTextIfChanged(KeyText, Key);
		SetTextIfChanged(WordsText, Words);
		// The bar keeps its room under the line, and fills while the key is held.
		SetShown(HoldBar, Hold > 0.f, ESlateVisibility::Hidden);
		if (Hold > 0.f)
		{
			const int32 Lit = FMath::Clamp(FMath::CeilToInt32(Hold * BarSegments), 0, BarSegments);
			for (int32 Index = 0; Index < HoldSegments.Num(); ++Index)
			{
				HoldSegments[Index]->SetColorAndOpacity(Index < Lit ? Color::Accent() : Color::SegmentOff());
			}
		}
	}

	// Quick fades; the last words stay while it fades out.
	const float Target = bShown ? 1.f : 0.f;
	if (Opacity != Target)
	{
		Opacity = FMath::Clamp(Opacity + (bShown ? 1.f : -1.f) * DeltaSeconds / FadeSeconds, 0.f, 1.f);
		Prompt->SetRenderOpacity(Opacity);
	}
	SetShown(Prompt, Opacity > 0.f);
}
