#include "Scenes/SceneSkipPromptWidget.h"
#include "UI/Style/LooterProgressRingWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/** The words, as the HUD's interact prompt sets them. */
	constexpr int32 TextSize = 15;
	constexpr int32 TextSpacing = 100;

	/** In from the screen's bottom right corner, clear of where the HUD's ammo sits. */
	constexpr float ScreenMargin = 56.f;

	/** Seconds to fade fully in or out. */
	constexpr float FadeSeconds = 0.15f;

	/** The lead word of the variant that is held. */
	constexpr const TCHAR* HoldLead = TEXT("HOLD");

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

bool USceneSkipPromptWidget::IsHoldLead(const FString& Lead)
{
	return Lead.Equals(HoldLead, ESearchCase::IgnoreCase);
}

TSharedRef<SWidget> USceneSkipPromptWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);

		// One line: the lead word, then the key (in brackets in the accent color for a press, or inside its progress ring
		// for a hold), then the words. The ring is taller than the words: everything sits on the line's middle.
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Prompt"));
		Line->SetVisibility(ESlateVisibility::Collapsed);
		Line->SetRenderOpacity(0.f);
		LeadText = MakeFloatingText(WidgetTree, TextSize, Color::Text(), TextSpacing);
		UHorizontalBoxSlot* LeadSlot = Line->AddChildToHorizontalBox(LeadText);
		LeadSlot->SetVerticalAlignment(VAlign_Center);
		LeadSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		KeyText = MakeFloatingText(WidgetTree, TextSize, Color::Accent(), TextSpacing);
		UHorizontalBoxSlot* KeySlot = Line->AddChildToHorizontalBox(KeyText);
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeySlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		ULooterProgressRing* Ring = nullptr;
		UTextBlock* Key = nullptr;
		HoldCap = MakeHoldKeyCap(WidgetTree, Ring, Key);
		HoldRing = Ring;
		HoldKey = Key;
		HoldCap->SetVisibility(ESlateVisibility::Collapsed);
		UHorizontalBoxSlot* CapSlot = Line->AddChildToHorizontalBox(HoldCap);
		CapSlot->SetVerticalAlignment(VAlign_Center);
		CapSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		WordsText = MakeFloatingText(WidgetTree, TextSize, Color::Text(), TextSpacing);
		Line->AddChildToHorizontalBox(WordsText)->SetVerticalAlignment(VAlign_Center);

		UOverlaySlot* PromptSlot = Root->AddChildToOverlay(Line);
		PromptSlot->SetHorizontalAlignment(HAlign_Right);
		PromptSlot->SetVerticalAlignment(VAlign_Bottom);
		PromptSlot->SetPadding(FMargin(0.f, 0.f, ScreenMargin, ScreenMargin));

		Prompt = Line;
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
	const bool bHold = IsHoldLead(Lead);
	if (bShown)
	{
		SetShown(LeadText, !Lead.IsEmpty());
		SetTextIfChanged(LeadText, Lead);
		SetTextIfChanged(WordsText, Words);
		// The hold wears its key inside the ring (which shows its empty track until the key is held); a press, its key in
		// brackets.
		SetShown(HoldCap, bHold);
		SetShown(KeyText, !bHold && !Key.IsEmpty());
		if (bHold)
		{
			SetHoldKeyLabel(HoldKey, Key);
		}
		else
		{
			SetTextIfChanged(KeyText, Key);
		}

		// The hold's end pops the ring once, with its sound; the key let go lets it start over.
		if (bHold && Hold >= 1.f && !bHoldDone)
		{
			bHoldDone = true;
			LooterSound::Play2D(this, LooterSoundCue::Hold::Complete);
			HoldRing->Pop();
		}
		else if (Hold < 0.5f)
		{
			bHoldDone = false;
		}
	}

	// The ring runs every frame: its fill, and the pop (which outlasts the prompt's reason to show, as the scene is skipped).
	UpdateHoldKeyCap(HoldRing, HoldKey, bShown && bHold ? Hold : 0.f, DeltaSeconds);

	// Quick fades; the last words stay while it fades out, and a pop is seen out first.
	const bool bPopping = HoldRing && HoldRing->IsPopping();
	const bool bShow = bShown || bPopping;
	const float Target = bShow ? 1.f : 0.f;
	if (Opacity != Target)
	{
		Opacity = FMath::Clamp(Opacity + (bShow ? 1.f : -1.f) * DeltaSeconds / FadeSeconds, 0.f, 1.f);
		Prompt->SetRenderOpacity(Opacity);
	}
	SetShown(Prompt, Opacity > 0.f);
}

USceneSkipPromptWidget::FShown USceneSkipPromptWidget::GetShown() const
{
	FShown Shown;
	if (!Prompt)
	{
		return Shown;
	}
	Shown.bVisible = Prompt->GetVisibility() != ESlateVisibility::Collapsed;
	Shown.bRing = Shown.bVisible && HoldCap && HoldCap->GetVisibility() != ESlateVisibility::Collapsed;
	if (HoldRing)
	{
		Shown.RingProgress = HoldRing->GetProgress();
		Shown.bRingPopping = HoldRing->IsPopping();
	}
	Shown.Key = Shown.bRing && HoldKey ? HoldKey->GetText().ToString() : (KeyText ? KeyText->GetText().ToString() : FString());
	return Shown;
}
