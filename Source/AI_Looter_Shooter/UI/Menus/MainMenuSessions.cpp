#include "UI/Menus/MainMenuWidget.h"
#include "Areas/AreaDefinition.h"
#include "Areas/StationBoard.h"
#include "Session/SessionSubsystem.h"
#include "Tutorial/TutorialChoice.h"
#include "UI/Style/LooterButton.h"
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
#include "Misc/Char.h"
#include "Misc/DateTime.h"

using namespace LooterUI;

namespace
{
	const FName ActionPlay(TEXT("Play"));
	const FName ActionDelete(TEXT("Delete"));
	const FName ActionBack(TEXT("Back"));
	const FName ActionCancelDelete(TEXT("CancelDelete"));
	const FName ActionConfirmDelete(TEXT("ConfirmDelete"));
	const FName ActionPlayTutorial(TEXT("PlayTutorial"));
	const FName ActionSkipTutorial(TEXT("SkipTutorial"));
	const FName ActionCancelNewGame(TEXT("CancelNewGame"));

	/** Every card is at least as tall as a saved session's, so the three line up like slots, used or not. */
	constexpr float CardMinHeight = 140.f;
	constexpr float CardActionsWidth = 150.f;
	constexpr float CardGap = 10.f;
	constexpr float PopupWidth = 520.f;

	/** Cards on the picker's glass: a saved session's like the settings menu's rows, an empty slot's fainter. */
	FLinearColor SavedFill() { return Color::Row(); }
	FLinearColor SavedLine() { return Hex(90, 200, 255, 110); }
	FLinearColor EmptyFill() { return Hex(18, 64, 94, 60); }
	FLinearColor EmptyLine() { return Color::RowLine(); }

	/** "Session 2": the slots count from 0, people from 1. */
	FString SessionName(int32 Index)
	{
		return FString::Printf(TEXT("Session %d"), Index + 1);
	}

	/** "no guns", "1 gun", "4 guns", for the middle of a sentence. */
	FString GunsText(int32 Guns)
	{
		if (Guns <= 0)
		{
			return TEXT("no guns");
		}
		return Guns == 1 ? FString(TEXT("1 gun")) : FString::Printf(TEXT("%d guns"), Guns);
	}

	FString Capitalized(FString Words)
	{
		if (!Words.IsEmpty())
		{
			Words[0] = FChar::ToUpper(Words[0]);
		}
		return Words;
	}

	/** Text that ends in "..." rather than running out of its card. */
	UTextBlock* MakeFittedText(UWidgetTree* Tree, const FString& Words, int32 Size, const FLinearColor& TextColor)
	{
		UTextBlock* Block = MakeText(Tree, Words, Size, TextColor);
		Block->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
		return Block;
	}

	/** A small dim label and its value on one line ("PLAYED  3 h 05 min"); the labels share a column. */
	UWidget* MakeFact(UWidgetTree* Tree, const FString& Label, const FString& Value)
	{
		UHorizontalBox* Line = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Line->AddChildToHorizontalBox(MakeSized(Tree, MakeText(Tree, Label, 9, Color::TextDim(), true, 150), 72.f))
			->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* ValueSlot = Line->AddChildToHorizontalBox(MakeFittedText(Tree, Value, 10, Color::Text()));
		ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ValueSlot->SetVerticalAlignment(VAlign_Center);
		return Line;
	}

	/**
	 * A chamfered card: the kit's control shape with its corners cut larger, to suit a card's size. The fill is a
	 * background (it follows the UI transparency setting); the outline always shows.
	 */
	UWidget* MakeCard(UWidgetTree* Tree, UWidget* Content, const FLinearColor& Fill, const FLinearColor& Line)
	{
		auto Shape = [Tree](bool bOutline, const FLinearColor& Tint)
		{
			FSlateBrush Brush = ShapeBrush(EShape::Control, bOutline, Tint);
			Brush.ImageSize *= 1.5f;
			return MakeImage(Tree, Brush);
		};
		UOverlay* Card = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* FillImage = Shape(false, Fill);
		MarkBackground(FillImage);
		FillOverlaySlot(Card->AddChildToOverlay(FillImage));
		FillOverlaySlot(Card->AddChildToOverlay(Shape(true, Line)));
		FillOverlaySlot(Card->AddChildToOverlay(Content), FMargin(12.f, 12.f, 14.f, 12.f));
		return Card;
	}
}

// ---------------------------------------------------------------------------
// The picker
// ---------------------------------------------------------------------------

UWidget* UMainMenuWidget::MakePicker()
{
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Choose a session"), 12, Color::TextDim(), true, 150));

	SessionList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(SessionList)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	// The status line keeps its room when empty, so the Back button doesn't jump when a message shows.
	StatusText = MakeText(WidgetTree, TEXT(""), 12, Color::TextDim());
	StatusText->SetAutoWrapText(true);
	USizeBox* StatusBox = MakeSized(WidgetTree, StatusText, 0.f);
	StatusBox->SetMinDesiredHeight(20.f);
	Content->AddChildToVerticalBox(StatusBox)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	UVerticalBoxSlot* BackSlot = Content->AddChildToVerticalBox(MakeSized(WidgetTree,
		MakeButton(ActionBack, 0, TEXT("Back"), 14, EButtonKind::Normal), 180.f, 40.f));
	BackSlot->SetHorizontalAlignment(HAlign_Left);
	BackSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

	UWidget* Panel = MakePanel(WidgetTree, TEXT("Single Player"), Content);
	Panel->SetVisibility(ESlateVisibility::Collapsed);
	return Panel;
}

void UMainMenuWidget::RefreshSessions()
{
	if (!SessionList)
	{
		return;
	}
	SessionList->ClearChildren();
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	if (!Sessions)
	{
		UTextBlock* Missing = MakeText(WidgetTree, TEXT("Sessions can't be read right now."), 12, Color::Worse());
		Missing->SetAutoWrapText(true);
		SessionList->AddChildToVerticalBox(Missing);
		return;
	}
	for (int32 Index = 0; Index < USessionSubsystem::MaxSessions; ++Index)
	{
		SessionList->AddChildToVerticalBox(MakeSessionCard(Index, Sessions->GetSummary(Index)))
			->SetPadding(FMargin(0.f, Index > 0 ? CardGap : 0.f, 0.f, 0.f));
	}
}

UWidget* UMainMenuWidget::MakeSessionCard(int32 Index, const FSessionSummary& Summary)
{
	const bool bSaved = Summary.bExists;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// A strip down the left side: orange for a saved session, faint for an empty slot.
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(bSaved ? Color::Accent() : Color::RowLine())), 3.f));

	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (bSaved)
	{
		// The session, its level (large), the area the player is in (Skyreach, Ransom's Rest) and the guns they carry,
		// how long it's been played and when it was saved.
		Info->AddChildToVerticalBox(MakeSection(WidgetTree, SessionName(Index)));
		Info->AddChildToVerticalBox(MakeText(WidgetTree, FString::Printf(TEXT("Level %d"), Summary.Level), 20, Color::Title(), true, 80))
			->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		const FString Guns = Capitalized(GunsText(Summary.Weapons));
		const FString Where = Summary.Place.IsEmpty() ? Guns : FString::Printf(TEXT("%s  ·  %s"), *Summary.Place, *Guns);
		Info->AddChildToVerticalBox(MakeFittedText(WidgetTree, Where, 11, Color::Text()))->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		Info->AddChildToVerticalBox(MakeFact(WidgetTree, TEXT("Played"), USessionSubsystem::FormatPlayTime(Summary.PlayedSeconds)))
			->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		Info->AddChildToVerticalBox(MakeFact(WidgetTree, TEXT("Saved"), USessionSubsystem::FormatSavedTime(Summary.Saved, FDateTime::Now())))
			->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	}
	else
	{
		Info->AddChildToVerticalBox(MakeText(WidgetTree, SessionName(Index), 12, Color::TextDim(), true, 300));
		Info->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Empty"), 16, Color::TextDim()))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	UHorizontalBoxSlot* InfoSlot = Line->AddChildToHorizontalBox(Info);
	InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InfoSlot->SetVerticalAlignment(VAlign_Center);
	InfoSlot->SetPadding(FMargin(12.f, 0.f, 12.f, 0.f));

	// Its buttons on the right: Continue and a smaller Delete for a saved session; New Game for an empty slot, which
	// has nothing to delete.
	UVerticalBox* Actions = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Actions->AddChildToVerticalBox(MakeSized(WidgetTree,
		MakeButton(ActionPlay, Index, bSaved ? TEXT("Continue") : TEXT("New Game"), 13, EButtonKind::Primary), 0.f, 40.f));
	if (bSaved)
	{
		Actions->AddChildToVerticalBox(MakeSized(WidgetTree, MakeButton(ActionDelete, Index, TEXT("Delete"), 10, EButtonKind::Danger), 0.f, 28.f))
			->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Actions, CardActionsWidth))->SetVerticalAlignment(VAlign_Center);

	USizeBox* Card = MakeSized(WidgetTree, MakeCard(WidgetTree, Line, bSaved ? SavedFill() : EmptyFill(), bSaved ? SavedLine() : EmptyLine()), 0.f);
	Card->SetMinDesiredHeight(CardMinHeight);
	return Card;
}

void UMainMenuWidget::HandleSessionButton(ULooterButton* Button)
{
	if (Button->Action == ActionPlay)
	{
		// A saved session continues; an empty slot asks first whether to play the tutorial.
		const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
		if (Sessions && !Sessions->GetSummary(Button->Index).bExists)
		{
			OpenNewGame(Button->Index);
		}
		else
		{
			StartSession(Button->Index);
		}
	}
	else if (Button->Action == ActionPlayTutorial || Button->Action == ActionSkipTutorial)
	{
		const int32 Index = NewGameIndex;
		CloseNewGame();
		if (Index != INDEX_NONE)
		{
			StartSession(Index, Button->Action == ActionSkipTutorial);
		}
	}
	else if (Button->Action == ActionCancelNewGame)
	{
		CloseNewGame();
	}
	else if (Button->Action == ActionDelete)
	{
		OpenDeleteConfirm(Button->Index);
	}
	else if (Button->Action == ActionConfirmDelete)
	{
		ConfirmDelete();
	}
	else if (Button->Action == ActionCancelDelete)
	{
		CloseDeleteConfirm();
	}
	else if (Button->Action == ActionBack)
	{
		ShowMainButtons();
	}
}

void UMainMenuWidget::StartSession(int32 Index, bool bSkipTutorial)
{
	USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	if (!Sessions)
	{
		SetStatus(TEXT("Sessions can't be read right now."), Color::Worse());
		return;
	}
	// The level opens and the menu goes with this world. Until then a second click would start a second trip.
	bLoading = true;
	SetStatus(FString::Printf(TEXT("Loading %s..."), *SessionName(Index)), Color::Accent());
	RefreshFooter();
	const bool bStarted = bSkipTutorial ? Sessions->PlaySessionSkippingTutorial(Index) : Sessions->PlaySession(Index);
	if (!bStarted)
	{
		bLoading = false;
		SetStatus(FString::Printf(TEXT("%s couldn't be started."), *SessionName(Index)), Color::Worse());
		RefreshFooter();
	}
}

// ---------------------------------------------------------------------------
// A new game: the tutorial, or skip it
// ---------------------------------------------------------------------------

void UMainMenuWidget::OpenNewGame(int32 Index)
{
	if (!PopupLayer)
	{
		StartSession(Index);
		return;
	}
	NewGameIndex = Index;
	const FString SessionLabel = SessionName(Index);
	// The places by their areas' names (the session picker shows them too).
	const UAreaDefinition* Practice = UAreaDefinition::FindByName(TEXT("Skyreach"));
	const UAreaDefinition* First = UAreaDefinition::FindByName(StationBoard::FirstAreaId().ToString());
	const FText PracticeName = Practice ? StationBoard::AreaName(*Practice) : FText::FromString(TEXT("Skyreach"));
	const FText FirstName = First ? StationBoard::AreaName(*First) : FText::FromString(TEXT("Ransom's Rest"));
	const bool bCanSkip = USessionSubsystem::CanSkipTutorial();

	// The choice's words are the tutorial's (TutorialChoice), so the tests read the same.
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* Body = MakeText(WidgetTree, TutorialChoice::Explain(PracticeName, FirstName).ToString(), 13, Color::Text());
	Body->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(Body);
	if (!bCanSkip)
	{
		UTextBlock* Closed = MakeText(WidgetTree, TutorialChoice::SkipClosed(FirstName).ToString(), 12, Color::Worse());
		Closed->SetAutoWrapText(true);
		Content->AddChildToVerticalBox(Closed)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	// Skyreach first, the call to action; the skip under it; Cancel last.
	Content->AddChildToVerticalBox(MakeSized(WidgetTree, MakeButton(ActionPlayTutorial, Index, TutorialChoice::StartLabel(PracticeName).ToString(), 14,
		EButtonKind::Primary), 0.f, 44.f))->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
	ULooterButton* Skip = MakeButton(ActionSkipTutorial, Index, TutorialChoice::SkipLabel(FirstName).ToString(), 13, EButtonKind::Normal);
	Skip->SetIsEnabled(bCanSkip);
	Content->AddChildToVerticalBox(MakeSized(WidgetTree, Skip, 0.f, 40.f))->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	UVerticalBoxSlot* CancelSlot = Content->AddChildToVerticalBox(MakeSized(WidgetTree,
		MakeButton(ActionCancelNewGame, Index, TEXT("Cancel"), 11, EButtonKind::Mini), 140.f, 30.f));
	CancelSlot->SetHorizontalAlignment(HAlign_Right);
	CancelSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	// The whole screen dims behind it and takes every click, as behind the delete confirmation.
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Backdrop->SetBrush(RectBrush(Color::Backdrop()));
	Backdrop->SetVisibility(ESlateVisibility::Visible);
	Backdrop->SetHorizontalAlignment(HAlign_Center);
	Backdrop->SetVerticalAlignment(VAlign_Center);
	MarkBackground(Backdrop);
	Backdrop->SetContent(MakeSized(WidgetTree, MakePanel(WidgetTree, FString::Printf(TEXT("New game in %s"), *SessionLabel), Content), PopupWidth));

	PopupLayer->ClearChildren();
	FillOverlaySlot(PopupLayer->AddChildToOverlay(Backdrop));
	PopupLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshFooter();
}

void UMainMenuWidget::CloseNewGame()
{
	if (NewGameIndex == INDEX_NONE)
	{
		return;
	}
	NewGameIndex = INDEX_NONE;
	if (PopupLayer)
	{
		PopupLayer->ClearChildren();
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);
	}
	RefreshFooter();
}

// ---------------------------------------------------------------------------
// The delete confirmation
// ---------------------------------------------------------------------------

void UMainMenuWidget::OpenDeleteConfirm(int32 Index)
{
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	if (!PopupLayer || !Sessions)
	{
		return;
	}
	// Read the slot again, so the popup says exactly what goes.
	const FSessionSummary Summary = Sessions->GetSummary(Index);
	if (!Summary.bExists)
	{
		RefreshSessions();
		return;
	}
	DeleteIndex = Index;
	const FString SessionLabel = SessionName(Index);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	const FString Where = Summary.Place.IsEmpty() ? FString() : FString::Printf(TEXT(" in %s"), *Summary.Place);
	UTextBlock* Body = MakeText(WidgetTree, FString::Printf(TEXT("Everything saved in %s will be deleted for good: level %d%s, %s, %s played."),
		*SessionLabel, Summary.Level, *Where, *GunsText(Summary.Weapons), *USessionSubsystem::FormatPlayTime(Summary.PlayedSeconds)), 13, Color::Text());
	Body->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(Body);
	Content->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("This can't be undone."), 13, Color::Worse()))->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* CancelSlot = Buttons->AddChildToHorizontalBox(MakeButton(ActionCancelDelete, Index, TEXT("Cancel"), 14, EButtonKind::Normal));
	CancelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	CancelSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	Buttons->AddChildToHorizontalBox(MakeButton(ActionConfirmDelete, Index, TEXT("Delete Session"), 14, EButtonKind::Danger))
		->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Content->AddChildToVerticalBox(MakeSized(WidgetTree, Buttons, 0.f, 42.f))->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));

	// The whole screen dims behind it and takes every click, so nothing under it can be pressed while it's open.
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Backdrop->SetBrush(RectBrush(Color::Backdrop()));
	Backdrop->SetVisibility(ESlateVisibility::Visible);
	Backdrop->SetHorizontalAlignment(HAlign_Center);
	Backdrop->SetVerticalAlignment(VAlign_Center);
	MarkBackground(Backdrop);
	Backdrop->SetContent(MakeSized(WidgetTree, MakePanel(WidgetTree, FString::Printf(TEXT("Delete %s?"), *SessionLabel), Content), PopupWidth));

	PopupLayer->ClearChildren();
	FillOverlaySlot(PopupLayer->AddChildToOverlay(Backdrop));
	PopupLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshFooter();
}

void UMainMenuWidget::CloseDeleteConfirm()
{
	DeleteIndex = INDEX_NONE;
	if (PopupLayer)
	{
		PopupLayer->ClearChildren();
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);
	}
	RefreshFooter();
}

void UMainMenuWidget::ConfirmDelete()
{
	const int32 Index = DeleteIndex;
	CloseDeleteConfirm();
	USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	if (Index == INDEX_NONE || !Sessions)
	{
		return;
	}
	const bool bDeleted = Sessions->DeleteSession(Index);
	// The cards show what's saved now, whichever way it went.
	RefreshSessions();
	if (bDeleted)
	{
		SetStatus(FString::Printf(TEXT("%s deleted."), *SessionName(Index)), Color::TextDim());
	}
	else
	{
		SetStatus(FString::Printf(TEXT("%s couldn't be deleted."), *SessionName(Index)), Color::Worse());
	}
}
