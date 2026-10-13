#include "UI/HUD/HudInteractPromptWidget.h"
#include "UI/Style/LooterProgressRingWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;

namespace
{
	/** The words: big enough to read at a glance under the crosshair, small enough not to crowd the aim. */
	constexpr int32 TextSize = 15;
	constexpr int32 TextSpacing = 100;
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

		TapLine = MakeTapLine();
		Box->AddChildToVerticalBox(TapLine)->SetHorizontalAlignment(HAlign_Center);
		HoldLine = MakeHoldLine();
		UVerticalBoxSlot* HoldSlot = Box->AddChildToVerticalBox(HoldLine);
		HoldSlot->SetHorizontalAlignment(HAlign_Center);
		HoldSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

		Lines = Box;
		WidgetTree->RootWidget = Box;
	}
	return Super::RebuildWidget();
}

UWidget* UHudInteractPromptWidget::MakeTapLine()
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TapKey = MakeFloatingText(WidgetTree, TextSize, Color::Accent(), TextSpacing);
	Row->AddChildToHorizontalBox(TapKey)->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));
	TapWords = MakeFloatingText(WidgetTree, TextSize, Color::Text(), TextSpacing);
	Row->AddChildToHorizontalBox(TapWords);
	return Row;
}

UWidget* UHudInteractPromptWidget::MakeHoldLine()
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// The ring is taller than the words: everything sits on the row's middle.
	UTextBlock* Lead = MakeFloatingText(WidgetTree, TextSize, Color::Text(), TextSpacing);
	Lead->SetText(FText::FromString(TEXT("HOLD")));
	UHorizontalBoxSlot* LeadSlot = Row->AddChildToHorizontalBox(Lead);
	LeadSlot->SetVerticalAlignment(VAlign_Center);
	LeadSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	ULooterProgressRing* Ring = nullptr;
	UTextBlock* Key = nullptr;
	UWidget* Cap = MakeHoldKeyCap(WidgetTree, Ring, Key);
	HoldRing = Ring;
	HoldKey = Key;
	UHorizontalBoxSlot* CapSlot = Row->AddChildToHorizontalBox(Cap);
	CapSlot->SetVerticalAlignment(VAlign_Center);
	CapSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	HoldWords = MakeFloatingText(WidgetTree, TextSize, Color::Text(), TextSpacing);
	Row->AddChildToHorizontalBox(HoldWords)->SetVerticalAlignment(VAlign_Center);
	return Row;
}

void UHudInteractPromptWidget::Listen(const UInteractionComponent* Interaction)
{
	// No Interaction is the loot card's turn: keep listening, since a hold on loot completes through it too.
	if (!Interaction || Listening.Get() == Interaction)
	{
		return;
	}
	// The prompt only reads the component it is given; hearing what it does isn't changing it.
	if (UInteractionComponent* Old = const_cast<UInteractionComponent*>(Listening.Get()))
	{
		Old->OnInteracted.RemoveDynamic(this, &UHudInteractPromptWidget::HandleInteracted);
	}
	Listening = Interaction;
	const_cast<UInteractionComponent*>(Interaction)->OnInteracted.AddUniqueDynamic(this, &UHudInteractPromptWidget::HandleInteracted);
}

void UHudInteractPromptWidget::HandleInteracted(AActor* Target, bool bHeld)
{
	if (!bHeld)
	{
		return;
	}
	// Every hold that completes gets its sound (the loot card's hold to equip too); the ring pops only if it's on show.
	LooterSound::Play2D(this, LooterSoundCue::Hold::Complete);
	if (HoldRing && bHoldLineUp && Opacity > 0.f)
	{
		HoldRing->Pop();
	}
}

void UHudInteractPromptWidget::Update(const UInteractionComponent* Interaction, const FString& Key, float DeltaSeconds)
{
	if (!Lines)
	{
		return;
	}
	Listen(Interaction);

	// The words for each way of using it; a line shows only when it has some.
	const AActor* Focused = Interaction ? Interaction->GetFocusedActor() : nullptr;
	const FInteractionOptions& Options = Interaction ? Interaction->GetFocusedOptions() : FInteractionOptions::None();
	const bool bTapLine = Focused && Options.bTap && !Options.TapPrompt.IsEmpty();
	const bool bHoldLine = Focused && Options.bHold && !Options.HoldPrompt.IsEmpty();
	const bool bWanted = bTapLine || bHoldLine;

	float Hold = 0.f;
	if (bWanted)
	{
		SetShown(TapLine, bTapLine);
		if (bTapLine)
		{
			SetTextIfChanged(TapKey, FString::Printf(TEXT("[%s]"), *Key));
			SetTextIfChanged(TapWords, Options.TapPrompt.ToString().ToUpper());
		}
		SetShown(HoldLine, bHoldLine);
		if (bHoldLine)
		{
			SetHoldKeyLabel(HoldKey, Key);
			SetTextIfChanged(HoldWords, Options.HoldPrompt.ToString().ToUpper());
			// The ring wears the hold while the key is held, and shows its empty track otherwise, keeping its room.
			Hold = Interaction->GetHoldProgress();
		}
		bHoldLineUp = bHoldLine;
	}

	// The ring keeps its place every frame: it fills with the hold, and a completed one pops (HandleInteracted).
	UpdateHoldKeyCap(HoldRing, HoldKey, Hold, DeltaSeconds);

	// Quick fades; the words of the last thing stay while it fades out, and the pop of a hold that just completed is
	// seen out (the thing may not be usable again at once, which would fade the prompt before the pop).
	const bool bPopping = HoldRing && HoldRing->IsPopping();
	const bool bShow = bWanted || bPopping;
	const float Target = bShow ? 1.f : 0.f;
	if (Opacity != Target)
	{
		Opacity = FMath::Clamp(Opacity + (bShow ? 1.f : -1.f) * DeltaSeconds / FadeSeconds, 0.f, 1.f);
		Lines->SetRenderOpacity(Opacity);
	}
	if (Opacity <= 0.f)
	{
		bHoldLineUp = false;
	}
	SetShown(Lines, Opacity > 0.f);
}

UHudInteractPromptWidget::FShown UHudInteractPromptWidget::GetShown() const
{
	FShown Shown;
	if (!Lines)
	{
		return Shown;
	}
	Shown.bVisible = Lines->GetVisibility() != ESlateVisibility::Collapsed;
	Shown.bTapLine = Shown.bVisible && TapLine->GetVisibility() != ESlateVisibility::Collapsed;
	Shown.bHoldLine = Shown.bVisible && HoldLine->GetVisibility() != ESlateVisibility::Collapsed;
	if (HoldRing)
	{
		Shown.RingProgress = HoldRing->GetProgress();
		Shown.bRingPopping = HoldRing->IsPopping();
	}
	Shown.RingKey = HoldKey ? HoldKey->GetText().ToString() : FString();
	Shown.HoldWords = HoldWords ? HoldWords->GetText().ToString() : FString();
	return Shown;
}
