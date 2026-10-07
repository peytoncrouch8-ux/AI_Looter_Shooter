#include "Scenes/GraveClawPromptWidget.h"
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

using namespace LooterUI;

namespace
{
	/** The words, a little larger than the skip prompt's: it's the one thing to do. */
	constexpr int32 TextSize = 18;
	constexpr int32 TextSpacing = 120;

	/** The pips: slim and slanted like the HUD's bars, with gaps between them. */
	constexpr float PipWidth = 46.f;
	constexpr float PipHeight = 6.f;
	constexpr float PipGap = 8.f;
	constexpr float PipSlant = 16.f;

	/** Below the middle of the screen, above where the captions sit (Slate units: pixels at 1080p). */
	constexpr float BelowMiddle = 170.f;

	/** Seconds to fade fully in or out. */
	constexpr float FadeSeconds = 0.25f;

	void SetTextIfChanged(UTextBlock* Text, const FString& Value)
	{
		if (!Text->GetText().ToString().Equals(Value))
		{
			Text->SetText(FText::FromString(Value));
		}
	}
}

TSharedRef<SWidget> UGraveClawPromptWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);

		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Prompt"));
		Box->SetVisibility(ESlateVisibility::Collapsed);
		Box->SetRenderOpacity(0.f);

		// One line: PRESS, the key in brackets in the accent color, TO CLAW OUT.
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Lead = MakeFloatingText(WidgetTree, TextSize, Color::Text(), TextSpacing);
		Lead->SetText(NSLOCTEXT("LooterScenes", "ClawLead", "PRESS"));
		Line->AddChildToHorizontalBox(Lead)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		KeyText = MakeFloatingText(WidgetTree, TextSize, Color::Accent(), TextSpacing);
		Line->AddChildToHorizontalBox(KeyText)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		UTextBlock* Words = MakeFloatingText(WidgetTree, TextSize, Color::Text(), TextSpacing);
		Words->SetText(NSLOCTEXT("LooterScenes", "ClawWords", "TO CLAW OUT"));
		Line->AddChildToHorizontalBox(Words);
		Box->AddChildToVerticalBox(Line)->SetHorizontalAlignment(HAlign_Center);

		// A pip a claw: plain slanted bars with no backing panel, dim until lit.
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		PipImages.Reset();
		for (int32 Index = 0; Index < Pips; ++Index)
		{
			UImage* Pip = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
			Pip->SetColorAndOpacity(Color::SegmentOff());
			USizeBox* Sized = MakeSized(WidgetTree, Pip, PipWidth, PipHeight);
			Sized->SetRenderShear(FVector2D(PipSlant, 0.f));
			UHorizontalBoxSlot* PipSlot = Row->AddChildToHorizontalBox(Sized);
			PipSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < Pips ? PipGap : 0.f, 0.f));
			PipImages.Add(Pip);
		}
		UVerticalBoxSlot* RowSlot = Box->AddChildToVerticalBox(Row);
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

		UOverlaySlot* PromptSlot = Root->AddChildToOverlay(Box);
		PromptSlot->SetHorizontalAlignment(HAlign_Center);
		PromptSlot->SetVerticalAlignment(VAlign_Center);
		PromptSlot->SetPadding(FMargin(0.f, BelowMiddle * 2.f, 0.f, 0.f));

		Prompt = Box;
		WidgetTree->RootWidget = Root;
	}
	return Super::RebuildWidget();
}

void UGraveClawPromptWidget::Update(bool bShown, const FString& Key, int32 Lit, float DeltaSeconds)
{
	if (!Prompt)
	{
		return;
	}
	if (bShown)
	{
		SetTextIfChanged(KeyText, FString::Printf(TEXT("[%s]"), *Key));
	}
	if (Lit != ShownLit)
	{
		ShownLit = Lit;
		for (int32 Index = 0; Index < PipImages.Num(); ++Index)
		{
			PipImages[Index]->SetColorAndOpacity(Index < Lit ? Color::Accent() : Color::SegmentOff());
		}
	}

	// Quick fades; the last words stay while it fades out.
	const float Target = bShown ? 1.f : 0.f;
	if (Opacity != Target)
	{
		Opacity = FMath::Clamp(Opacity + (bShown ? 1.f : -1.f) * DeltaSeconds / FadeSeconds, 0.f, 1.f);
		Prompt->SetRenderOpacity(Opacity);
	}
	const ESlateVisibility Wanted = Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (Prompt->GetVisibility() != Wanted)
	{
		Prompt->SetVisibility(Wanted);
	}
}
