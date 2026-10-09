// UNoticeBoardWidget: the panel, opening and closing, choosing among the cards and the keys. The cards themselves and what
// acting on one does are NoticeBoardWidgetCards.cpp's.

#include "UI/World/NoticeBoardWidget.h"
#include "Missions/MissionRunner.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "NoticeBoard"

using namespace LooterUI;

namespace
{
	const FName ActionClose(TEXT("Close"));

	constexpr float PanelWidth = 760.f;
	constexpr float CardGap = 8.f;
	/** The list scrolls past this height, so a long board still fits a small screen. */
	constexpr float ListMaxHeight = 560.f;

	/** A card's fill and edge by what it is and whether it's chosen: a chosen one lit orange, a done or locked one faint. */
	FLinearColor CardFill(const FNoticeBoardPosting& Posting, bool bChosen)
	{
		if (Posting.State == ENoticePosting::Done || Posting.State == ENoticePosting::Locked)
		{
			return Hex(7, 26, 40, 110);
		}
		return bChosen ? Hex(255, 159, 28, 64) : Color::Row();
	}

	FLinearColor CardLine(const FNoticeBoardPosting& Posting, bool bChosen)
	{
		if (Posting.State == ENoticePosting::Done || Posting.State == ENoticePosting::Locked)
		{
			return Hex(90, 200, 255, 46);
		}
		if (bChosen)
		{
			return Color::Accent();
		}
		return Posting.State == ENoticePosting::Ready ? Color::AccentDark() : Color::RowLine();
	}
}

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

void UNoticeBoardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// The board holds the keyboard (Escape closes it); its buttons never take it.
	SetIsFocusable(true);
}

void UNoticeBoardWidget::NativeDestruct()
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnMissionCompleted.Remove(CompletedHandle);
	}
	BoundRunner.Reset();
	CompletedHandle.Reset();
	Super::NativeDestruct();
}

void UNoticeBoardWidget::Open(ALooterHUD* InHUD, ANoticeBoard* InBoard)
{
	OwningHUD = InHUD;
	Board = InBoard;
	GivenHere.Reset();
	Selected = INDEX_NONE;

	// What a turn-in gives is heard from the runner as it's given, for the card to say.
	UMissionRunner* Runner = GetRunner();
	if (Runner != BoundRunner.Get())
	{
		if (UMissionRunner* Old = BoundRunner.Get())
		{
			Old->OnMissionCompleted.Remove(CompletedHandle);
		}
		BoundRunner = Runner;
		CompletedHandle = Runner ? Runner->OnMissionCompleted.AddUObject(this, &UNoticeBoardWidget::HandleMissionCompleted) : FDelegateHandle();
	}

	// Opened before it was first shown, the panel is built as the widget is.
	if (WidgetTree && WidgetTree->RootWidget)
	{
		Refresh();
	}
	else
	{
		const ANoticeBoard* From = Board.Get();
		Postings = From && Runner ? From->GatherPostings(*Runner) : TArray<FNoticeBoardPosting>();
	}
}

void UNoticeBoardWidget::Close()
{
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseNoticeBoard();
	}
	else
	{
		RemoveFromParent();
	}
}

UMissionRunner* UNoticeBoardWidget::GetRunner() const
{
	return UMissionRunner::Get(this);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UNoticeBoardWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		// The whole screen answers the mouse, so a click beside the board lands on it and the keyboard stays with it.
		Root->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Root;

		// The world stays in view behind the board, dimmed.
		UImage* Backdrop = MakeImage(WidgetTree, RectBrush(Color::Backdrop()));
		MarkBackground(Backdrop);
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		PanelBox = MakeSized(WidgetTree, nullptr, PanelWidth);
		UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelBox);
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);

		BuildPanel();
	}
	return Super::RebuildWidget();
}

void UNoticeBoardWidget::Refresh()
{
	const FName Chosen = Postings.IsValidIndex(Selected) ? Postings[Selected].MissionId : NAME_None;
	const ANoticeBoard* From = Board.Get();
	const UMissionRunner* Runner = GetRunner();
	Postings = From && Runner ? From->GatherPostings(*Runner) : TArray<FNoticeBoardPosting>();
	BuildPanel();
	// The same card stays chosen while it can still be acted on.
	const int32 Again = Postings.IndexOfByPredicate([Chosen](const FNoticeBoardPosting& Each) { return Each.MissionId == Chosen; });
	if (!Chosen.IsNone() && CanAct(Again))
	{
		Select(Again);
	}
}

void UNoticeBoardWidget::BuildPanel()
{
	if (!PanelBox)
	{
		return;
	}
	const ANoticeBoard* From = Board.Get();
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (From && !From->Subtitle.IsEmpty())
	{
		Content->AddChildToVerticalBox(MakeText(WidgetTree, From->Subtitle.ToString(), 11, Color::TextDim(), true, 150));
	}

	// The postings, pinned up one under another.
	CardsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Cards.Reset();
	Cards.SetNum(Postings.Num());
	for (int32 Index = 0; Index < Postings.Num(); ++Index)
	{
		CardsBox->AddChildToVerticalBox(MakeCard(Index))->SetPadding(FMargin(0.f, Index > 0 ? CardGap : 0.f, 0.f, 0.f));
	}
	if (Postings.IsEmpty())
	{
		UTextBlock* Nothing = MakeText(WidgetTree, LOCTEXT("Nothing", "Nothing posted.").ToString(), 13, Color::TextDim());
		CardsBox->AddChildToVerticalBox(Nothing);
	}
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	StyleScrollBox(Scroll);
	Scroll->AddChild(CardsBox);
	USizeBox* List = MakeSized(WidgetTree, Scroll, 0.f);
	List->SetMaxDesiredHeight(ListMaxHeight);
	Content->AddChildToVerticalBox(List)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	// While postings are still locked the board says why (Skyreach's: bring a gun).
	const bool bAnyLocked = Postings.ContainsByPredicate([](const FNoticeBoardPosting& Each) { return Each.State == ENoticePosting::Locked; });
	if (bAnyLocked && From && !From->LockedNote.IsEmpty())
	{
		UTextBlock* Note = MakeText(WidgetTree, From->LockedNote.ToString(), 13, Color::Accent());
		Note->SetAutoWrapText(true);
		Content->AddChildToVerticalBox(Note)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}

	// The status line keeps its room when empty, so nothing under it jumps when a word shows.
	StatusText = MakeText(WidgetTree, FString(), 12, Color::TextDim());
	StatusText->SetAutoWrapText(true);
	USizeBox* StatusBox = MakeSized(WidgetTree, StatusText, 0.f);
	StatusBox->SetMinDesiredHeight(20.f);
	Content->AddChildToVerticalBox(StatusBox)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	// What the keys do, and a button to close it for the mouse.
	UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	struct FKeyHint
	{
		const TCHAR* KeyName;
		FText Does;
	};
	const FKeyHint Hints[] = {
		{ TEXT("W/S"), LOCTEXT("HintSelect", "Select") },
		{ TEXT("E"), LOCTEXT("HintAct", "Track or turn in") },
		{ TEXT("Esc"), LOCTEXT("HintClose", "Close") },
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Hints); ++Index)
	{
		UHorizontalBoxSlot* HintSlot = Bottom->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, Hints[Index].KeyName,
			Hints[Index].Does.ToString(), Index == 1));
		HintSlot->SetVerticalAlignment(VAlign_Center);
		HintSlot->SetPadding(FMargin(Index > 0 ? 16.f : 0.f, 0.f, 0.f, 0.f));
	}
	Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ULooterButton* CloseButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	CloseButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), ActionClose, 0, LOCTEXT("Close", "Close"), 12,
		EButtonKind::Normal);
	CloseButton->OnButtonClicked.BindUObject(this, &UNoticeBoardWidget::HandleButton);
	Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, CloseButton, 120.f, 34.f))->SetVerticalAlignment(VAlign_Center);
	Content->AddChildToVerticalBox(Bottom)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	const FText Title = From && !From->Title.IsEmpty() ? From->Title : LOCTEXT("Title", "Notice Board");
	PanelBox->SetContent(MakePanel(WidgetTree, Title.ToString(), Content));

	// What wants doing first: a posting to turn in, else the first that can be tracked.
	Selected = INDEX_NONE;
	for (int32 Index = 0; Index < Postings.Num() && Selected == INDEX_NONE; ++Index)
	{
		Selected = Postings[Index].State == ENoticePosting::Ready && CanAct(Index) ? Index : INDEX_NONE;
	}
	for (int32 Index = 0; Index < Postings.Num() && Selected == INDEX_NONE; ++Index)
	{
		Selected = CanAct(Index) ? Index : INDEX_NONE;
	}
	Restyle();
	SetStatus(FText::GetEmpty(), Color::TextDim());
}

// ---------------------------------------------------------------------------
// Choosing
// ---------------------------------------------------------------------------

void UNoticeBoardWidget::Select(int32 Index)
{
	if (CanAct(Index) && Index != Selected)
	{
		Selected = Index;
		Restyle();
	}
}

void UNoticeBoardWidget::Step(int32 Direction)
{
	if (Postings.IsEmpty())
	{
		return;
	}
	// Past the done and locked cards to the next that can be acted on, round the ends.
	int32 Index = Selected == INDEX_NONE ? (Direction > 0 ? -1 : 0) : Selected;
	for (int32 Tries = 0; Tries < Postings.Num(); ++Tries)
	{
		Index = (Index + Direction + Postings.Num()) % Postings.Num();
		if (CanAct(Index))
		{
			Select(Index);
			return;
		}
	}
}

void UNoticeBoardWidget::Restyle()
{
	for (int32 Index = 0; Index < Cards.Num() && Index < Postings.Num(); ++Index)
	{
		const FCard& Card = Cards[Index];
		const bool bChosen = Index == Selected;
		if (Card.Fill)
		{
			Card.Fill->SetColorAndOpacity(CardFill(Postings[Index], bChosen));
		}
		if (Card.Line)
		{
			Card.Line->SetColorAndOpacity(CardLine(Postings[Index], bChosen));
		}
	}
}

void UNoticeBoardWidget::SetStatus(const FText& Message, const FLinearColor& TextColor)
{
	if (StatusText)
	{
		StatusText->SetText(Message);
		StatusText->SetColorAndOpacity(FSlateColor(TextColor));
	}
}

// ---------------------------------------------------------------------------
// Buttons and keys
// ---------------------------------------------------------------------------

void UNoticeBoardWidget::HandleButton(ULooterButton* Button)
{
	if (!Button)
	{
		return;
	}
	if (Button->Action == ActionClose)
	{
		Close();
		return;
	}
	Act(Button->Index);
	// Clicking handed the keyboard to the game viewport (LooterButton); take it back so the keys keep reaching the board.
	if (IsInViewport())
	{
		SetKeyboardFocus();
	}
}

void UNoticeBoardWidget::HandleHovered(ULooterButton* Button)
{
	if (Button)
	{
		Select(Button->Index);
	}
}

FReply UNoticeBoardWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		Close();
		return FReply::Handled();
	}
	if (Key == EKeys::W || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		Step(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::S || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		Step(1);
		return FReply::Handled();
	}
	// The Interact key that read the board may still be held and repeating: only a fresh press acts.
	const bool bAct = !InKeyEvent.IsRepeat()
		&& (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom);
	if (bAct)
	{
		Act(Selected);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UNoticeBoardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A click on no button ends here rather than in the game viewport, which would take the keyboard from the board.
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
