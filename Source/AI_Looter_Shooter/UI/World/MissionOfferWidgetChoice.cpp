// UMissionOfferWidget's choice mode: a mission turned in with a choice of two (FMissionTurnIn::Choices, Back Pay). The
// question, what comes either way, and the two side by side, each with what it means and what it gives; the focused one is
// held down until its ring fills, so a choice that can't be undone is never made by a stray key; then its stamp, its words.

#include "UI/World/MissionOfferWidget.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPointComponent.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterProgressRingWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/MissionMarkIcons.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Actor.h"

#define LOCTEXT_NAMESPACE "MissionOffer"

using namespace LooterUI;
using namespace MenuNav;

namespace
{
	const FName ActionChoice(TEXT("Choice"));

	/** A column's least height, so two choices of different lengths stand level. */
	constexpr float ColumnMinHeight = 300.f;
	constexpr float ColumnGap = 16.f;
	constexpr float RewardLabelWidth = 92.f;
	/** A ring let go drains back this many times faster than it fills. */
	constexpr float DrainRate = 2.5f;
	/** How far a held column leans toward the player as its ring fills. */
	constexpr float HeldGrow = 0.018f;
}

// ---------------------------------------------------------------------------
// Opening
// ---------------------------------------------------------------------------

void UMissionOfferWidget::OpenChoice(ALooterHUD* InHUD, const UMissionDefinition* InMission, AActor* InGiver, bool bInOverScreen)
{
	Card = InMission ? FMissionOfferCard::MakeChoice(*InMission, InGiver, GetWorld()) : FMissionOfferCard();
	OpenAs(EMissionCardMode::Choice, InHUD, InMission, InGiver, bInOverScreen);
	LooterSound::Play2D(this, LooterSoundCue::MissionFeatures::ChoiceOpen);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void UMissionOfferWidget::BuildChoiceContent(UVerticalBox* Content)
{
	// The question, in the giver's voice or the card's own words.
	UTextBlock* Question = MakeText(WidgetTree, Card.Question.ToString(), 15, Color::Text());
	Question->SetFont(Font(15, false));
	Question->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(Question)->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));

	// What comes whichever is chosen: one line, so the two below differ only in what differs.
	if (!Card.EitherWay.IsEmpty())
	{
		UHorizontalBox* Either = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* HeadSlot = Either->AddChildToHorizontalBox(LoadoutParts::Label(WidgetTree,
			LOCTEXT("EitherWay", "Either way").ToString(), 9, Color::TextDim(), 260));
		HeadSlot->SetVerticalAlignment(VAlign_Center);
		HeadSlot->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
		for (const FMissionOfferReward& Reward : Card.EitherWay)
		{
			UHorizontalBoxSlot* LabelSlot = Either->AddChildToHorizontalBox(LoadoutParts::Label(WidgetTree, Reward.Label.ToString(), 8,
				Color::TextDim(), 200));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
			LabelSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			UHorizontalBoxSlot* ValueSlot = Either->AddChildToHorizontalBox(MakeText(WidgetTree, Reward.Value.ToString(), 14, Reward.Color, false, 20));
			ValueSlot->SetVerticalAlignment(VAlign_Center);
			ValueSlot->SetPadding(FMargin(0.f, 0.f, 22.f, 0.f));
		}
		Content->AddChildToVerticalBox(Either)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	}

	// The two side by side, the same height, each a button holding its card.
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 Index = 0; Index < Card.Choices.Num(); ++Index)
	{
		UHorizontalBoxSlot* ColumnSlot = Columns->AddChildToHorizontalBox(MakeChoiceColumn(Index));
		ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ColumnSlot->SetVerticalAlignment(VAlign_Fill);
		ColumnSlot->SetPadding(FMargin(Index > 0 ? ColumnGap * 0.5f : 0.f, 0.f, Index + 1 < Card.Choices.Num() ? ColumnGap * 0.5f : 0.f, 0.f));
	}
	Content->AddChildToVerticalBox(Columns);
}

UWidget* UMissionOfferWidget::MakeChoiceColumn(int32 Index)
{
	const FMissionChoiceCard& Choice = Card.Choices[Index];
	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Words->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, FText::Format(LOCTEXT("ChoiceNumber", "Choice {0}"), Index + 1).ToString(), 9,
		MissionMarkIcons::Color(Card.bMain), 260));
	UTextBlock* Title = MakeText(WidgetTree, Choice.Title.ToString(), 21, Color::Text(), false, 20);
	Title->SetAutoWrapText(true);
	Words->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	if (!Choice.Line.IsEmpty())
	{
		UTextBlock* Line = MakeText(WidgetTree, Choice.Line.ToString(), 13, Color::Text());
		Line->SetFont(Font(13, false));
		Line->SetAutoWrapText(true);
		Words->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
	UImage* Rule = MakeImage(WidgetTree, RectBrush(Color::ScreenLine()));
	Words->AddChildToVerticalBox(MakeSized(WidgetTree, Rule, 0.f, 1.f))->SetPadding(FMargin(0.f, 14.f, 0.f, 10.f));
	Words->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("YouGet", "You get").ToString(), 9, Color::Accent(), 260));
	for (const FMissionOfferReward& Reward : Choice.Rewards)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* LabelSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree,
			LoadoutParts::Label(WidgetTree, Reward.Label.ToString(), 8, Color::TextDim(), 200), RewardLabelWidth));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Value = MakeText(WidgetTree, Reward.Value.ToString(), 16, Reward.Color, false, 20);
		Value->SetAutoWrapText(true);
		UHorizontalBoxSlot* ValueSlot = Line->AddChildToHorizontalBox(Value);
		ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ValueSlot->SetVerticalAlignment(VAlign_Center);
		Words->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}
	// Room between the reward and the hold, so the hold sits at the column's foot in both.
	Words->AddChildToVerticalBox(MakeSized(WidgetTree, nullptr, 0.f, 8.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// The hold: the ring round the key, and the words. Shown on the focused one only.
	UHorizontalBox* HoldRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	ULooterProgressRing* Ring = nullptr;
	UTextBlock* Key = nullptr;
	HoldRow->AddChildToHorizontalBox(MakeHoldKeyCap(WidgetTree, Ring, Key))->SetVerticalAlignment(VAlign_Center);
	SetHoldKeyLabel(Key, GetInteractKey().GetDisplayName().ToString());
	UTextBlock* HoldWords = LoadoutParts::Label(WidgetTree, LOCTEXT("HoldToChoose", "Hold to choose").ToString(), 10, Color::Accent(), 220);
	UHorizontalBoxSlot* HoldWordsSlot = HoldRow->AddChildToHorizontalBox(HoldWords);
	HoldWordsSlot->SetVerticalAlignment(VAlign_Center);
	HoldWordsSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	HoldRow->SetVisibility(ESlateVisibility::Hidden);
	Words->AddChildToVerticalBox(HoldRow)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	// The column: its glass (a background), its edge (lit when focused), the words inside.
	UOverlay* Column = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UImage* Fill = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
	Fill->SetColorAndOpacity(LoadoutParts::Colors::CardFill());
	MarkBackground(Fill);
	FillOverlaySlot(Column->AddChildToOverlay(Fill));
	UImage* Edge = MakeImage(WidgetTree, RectBrush(FLinearColor::Transparent, FLinearColor::White, 1.5f));
	Edge->SetColorAndOpacity(LoadoutParts::Colors::CardLine());
	FillOverlaySlot(Column->AddChildToOverlay(Edge));
	FillOverlaySlot(Column->AddChildToOverlay(Words), FMargin(18.f, 16.f));
	USizeBox* Sized = MakeSized(WidgetTree, Column, 0.f, 0.f);
	Sized->SetMinDesiredHeight(ColumnMinHeight);
	Sized->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->SetupContent(Sized, ActionChoice, Index, EButtonKind::Bare);
	// The card sounds its own pointing and choosing.
	Button->bPlaysSounds = false;
	Button->OnButtonClicked.BindUObject(this, &UMissionOfferWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &UMissionOfferWidget::HandleHovered);
	// The mouse holds a choice down as a key does: pressed on it, let go off it.
	if (Index == 0)
	{
		Button->OnPressed.AddDynamic(this, &UMissionOfferWidget::HandleFirstChoicePressed);
	}
	else
	{
		Button->OnPressed.AddDynamic(this, &UMissionOfferWidget::HandleSecondChoicePressed);
	}
	Button->OnReleased.AddDynamic(this, &UMissionOfferWidget::HandleChoiceReleased);

	ChoiceButtons.Add(Button);
	ChoiceBodies.Add(Sized);
	ChoiceEdges.Add(Edge);
	ChoiceFills.Add(Fill);
	ChoiceHoldRows.Add(HoldRow);
	ChoiceRings.Add(Ring);
	ChoiceKeys.Add(Key);
	ChoiceHoldWords.Add(HoldWords);
	return Button;
}

// ---------------------------------------------------------------------------
// Holding a choice down
// ---------------------------------------------------------------------------

int32 UMissionOfferWidget::GetFocusedChoice() const
{
	const UWidget* Focused = Nav.GetFocusedWidget();
	for (int32 Index = 0; Index < ChoiceButtons.Num(); ++Index)
	{
		if (ChoiceButtons[Index] && ChoiceButtons[Index].Get() == Focused)
		{
			return Index;
		}
	}
	// Without the widgets (a test), the first.
	return ChoiceButtons.IsEmpty() && Card.Choices.Num() > 0 ? 0 : INDEX_NONE;
}

void UMissionOfferWidget::BeginHold(int32 Choice)
{
	if (Mode != EMissionCardMode::Choice || bClosing || Chosen != INDEX_NONE || !Card.Choices.IsValidIndex(Choice) || !IsArmed())
	{
		return;
	}
	// Another one held before: its ring starts afresh on this one.
	if (HoldChoice != Choice)
	{
		HoldProgress = 0.f;
	}
	HoldChoice = Choice;
	bHoldDown = true;
	SetStatus(FText::GetEmpty());
	LooterSound::Stop(HoldSound.Get());
	HoldSound = LooterSound::Start(this, LooterSoundCue::MissionFeatures::ChoiceHold);
}

void UMissionOfferWidget::EndHold()
{
	bHoldDown = false;
	HoldKey = FKey();
	// The swell fades as the ring drains back.
	LooterSound::Stop(HoldSound.Get(), 0.2f);
	HoldSound.Reset();
}

bool UMissionOfferWidget::HandleChoiceKeyDown(const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	// Esc and moving between the two are the card's usual keys (MissionOfferWidgetInput.cpp).
	if (IsBack(Key) || IsStickKey(Key) || MoveOf(Key) != EMove::None)
	{
		return false;
	}
	if (Key == GetInteractKey() || Key == EKeys::E || IsAccept(Key))
	{
		RevealNav(Key);
		if (!InKeyEvent.IsRepeat() && !bHoldDown)
		{
			BeginHold(GetFocusedChoice());
			if (bHoldDown)
			{
				HoldKey = Key;
			}
		}
		return true;
	}
	return false;
}

bool UMissionOfferWidget::HandleChoiceKeyUp(const FKeyEvent& InKeyEvent)
{
	if (bHoldDown && HoldKey.IsValid() && InKeyEvent.GetKey() == HoldKey)
	{
		EndHold();
		return true;
	}
	return false;
}

void UMissionOfferWidget::HandleFirstChoicePressed()
{
	if (ChoiceButtons.IsValidIndex(0) && !bClosing)
	{
		Nav.FocusOn(ChoiceButtons[0].Get());
		BeginHold(0);
	}
}

void UMissionOfferWidget::HandleSecondChoicePressed()
{
	if (ChoiceButtons.IsValidIndex(1) && !bClosing)
	{
		Nav.FocusOn(ChoiceButtons[1].Get());
		BeginHold(1);
	}
}

void UMissionOfferWidget::HandleChoiceReleased()
{
	// Only the mouse's hold: a key's is let go by its own key.
	if (bHoldDown && !HoldKey.IsValid())
	{
		EndHold();
	}
}

void UMissionOfferWidget::AdvanceChoice(float DeltaSeconds)
{
	const int32 Focused = GetFocusedChoice();
	// A key's hold follows the focus: moved to the other one, it lets go.
	if (bHoldDown && HoldKey.IsValid() && HoldChoice != Focused)
	{
		EndHold();
	}
	if (Chosen == INDEX_NONE && !bClosing)
	{
		if (bHoldDown)
		{
			HoldProgress = FMath::Min(HoldProgress + DeltaSeconds / ChoiceHoldSeconds, 1.f);
			if (HoldProgress >= 1.f)
			{
				Choose(HoldChoice);
			}
		}
		else
		{
			HoldProgress = FMath::Max(HoldProgress - DeltaSeconds * DrainRate / ChoiceHoldSeconds, 0.f);
			if (HoldProgress <= 0.f)
			{
				HoldChoice = INDEX_NONE;
			}
		}
	}

	// The columns: the focused one lit (its edge orange, brightening as it fills, leaning in a touch), the other dimmed;
	// once chosen, the chosen one bright and the other faded back.
	for (int32 Index = 0; Index < ChoiceButtons.Num(); ++Index)
	{
		const bool bThisFocused = Chosen == INDEX_NONE ? Index == Focused : Index == Chosen;
		const float Held = Index == HoldChoice ? HoldProgress : 0.f;
		if (ChoiceEdges.IsValidIndex(Index) && ChoiceEdges[Index])
		{
			ChoiceEdges[Index]->SetColorAndOpacity(bThisFocused ? FMath::Lerp(Color::Accent(), Color::AccentLight(), Held)
				: LoadoutParts::Colors::CardLine());
		}
		if (ChoiceFills.IsValidIndex(Index) && ChoiceFills[Index])
		{
			ChoiceFills[Index]->SetColorAndOpacity(bThisFocused ? Color::Tile() : LoadoutParts::Colors::CardFill());
		}
		if (ChoiceBodies.IsValidIndex(Index) && ChoiceBodies[Index])
		{
			const float Faded = Chosen != INDEX_NONE ? 0.3f : 0.72f;
			ChoiceBodies[Index]->SetRenderOpacity(bThisFocused ? 1.f : Faded);
			ChoiceBodies[Index]->SetRenderScale(FVector2D(1.f + HeldGrow * Held));
		}
		if (ChoiceHoldRows.IsValidIndex(Index) && ChoiceHoldRows[Index])
		{
			const bool bHoldShown = bThisFocused && (Chosen == INDEX_NONE || Index == Chosen);
			const ESlateVisibility Wanted = bHoldShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
			if (ChoiceHoldRows[Index]->GetVisibility() != Wanted)
			{
				ChoiceHoldRows[Index]->SetVisibility(Wanted);
			}
		}
		if (ChoiceRings.IsValidIndex(Index))
		{
			UpdateHoldKeyCap(ChoiceRings[Index], ChoiceKeys.IsValidIndex(Index) ? ChoiceKeys[Index].Get() : nullptr,
				Index == Chosen ? 1.f : Held, DeltaSeconds);
		}
	}
}

// ---------------------------------------------------------------------------
// Choosing
// ---------------------------------------------------------------------------

bool UMissionOfferWidget::Choose(int32 Choice)
{
	if (Mode != EMissionCardMode::Choice || bClosing || Chosen != INDEX_NONE || !Card.Choices.IsValidIndex(Choice))
	{
		return false;
	}
	UMissionRunner* Runner = UMissionRunner::Get(this);
	const UMissionDefinition* TurnedIn = Mission.Get();
	if (!Runner || !TurnedIn || !Runner->TurnInWithChoice(TurnedIn->GetMissionId(), Choice))
	{
		LooterSound::Play2D(this, LooterSoundCue::Denied);
		SetStatus(LOCTEXT("CantChoose", "This can't be settled here now."));
		EndHold();
		HoldProgress = 0.f;
		return false;
	}
	Chosen = Choice;
	HoldChoice = Choice;
	HoldProgress = 1.f;
	bAccepted = true;
	bClosing = true;
	CloseAge = 0.f;
	Nav.SetShown(false);
	EndHold();
	SetStatus(FText::GetEmpty());
	if (ChoiceRings.IsValidIndex(Choice) && ChoiceRings[Choice])
	{
		ChoiceRings[Choice]->Pop();
	}
	if (StampWord)
	{
		StampWord->SetText(Card.Choices[Choice].Stamp.ToUpper());
	}
	LooterSound::Play2D(this, LooterSoundCue::MissionFeatures::ChoiceMade);

	// The choice's own words, said once the card is gone (the captions wait under it), in the giver's voice where a line
	// names no speaker.
	const FMissionTurnInChoice* Made = TurnedIn->GetTurnInChoice(Choice);
	if (Made && !Made->Lines.IsEmpty())
	{
		const AActor* At = Giver.Get();
		const USpeakerPointComponent* Point = At ? At->FindComponentByClass<USpeakerPointComponent>() : nullptr;
		const FText Voice = Point && !Point->SpeakerName.IsEmpty() ? Point->SpeakerName : TurnedIn->GetGiverName();
		TArray<FStoryLine> Said = Made->Lines;
		for (FStoryLine& Line : Said)
		{
			if (Line.Speaker.IsEmpty())
			{
				Line.Speaker = Voice;
			}
		}
		if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
		{
			Captions->Play(Said, ECaptionPlay::Interrupt);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Choice card: %s, %s."), *TurnedIn->GetMissionId().ToString(), *Card.Choices[Choice].Title.ToString());
	ApplyMotion();
	return true;
}

#undef LOCTEXT_NAMESPACE
