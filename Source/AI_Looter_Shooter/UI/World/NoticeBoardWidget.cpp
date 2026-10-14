// UNoticeBoardWidget: the panel, opening and closing, and how each card looks as it's chosen or pointed at. The cards
// themselves and what acting on one does are NoticeBoardWidgetCards.cpp's; choosing and the keys NoticeBoardWidgetInput.cpp's.

#include "UI/World/NoticeBoardWidget.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "UI/Style/MissionMarkIcons.h"
#include "UI/HUD/LooterHUD.h"
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

#define LOCTEXT_NAMESPACE "NoticeBoard"

using namespace LooterUI;

namespace
{
	constexpr float PanelWidth = 760.f;
	constexpr float CardGap = 8.f;
	/** The list scrolls past this height, so a long board still fits a small screen. */
	constexpr float ListMaxHeight = 560.f;

	bool IsFaint(const FNoticeBoardPosting& Posting)
	{
		return Posting.State == ENoticePosting::Done || Posting.State == ENoticePosting::Locked;
	}

	/**
	 * A card's fill and edge by what it is: a chosen one lit orange (a done or locked one more faintly), one under the
	 * mouse brighter (the kit's buttons brighten as they're pointed at), a done or locked one faint.
	 */
	FLinearColor CardFill(const FNoticeBoardPosting& Posting, bool bChosen, bool bHovered)
	{
		if (bChosen)
		{
			return IsFaint(Posting) ? Color::Accent() * FLinearColor(1.f, 1.f, 1.f, 0.12f) : Hex(255, 159, 28, 64);
		}
		if (IsFaint(Posting))
		{
			return Hex(7, 26, 40, 110);
		}
		return bHovered ? Color::Tile() : Color::Row();
	}

	FLinearColor CardLine(const FNoticeBoardPosting& Posting, bool bChosen, bool bHovered)
	{
		if (bChosen)
		{
			return IsFaint(Posting) ? Color::AccentDark() : Color::Accent();
		}
		if (bHovered)
		{
			return IsFaint(Posting) ? Color::RowLine() : Color::TileLine();
		}
		if (IsFaint(Posting))
		{
			return Hex(90, 200, 255, 46);
		}
		// A job up for the taking here is edged in its mark's colour, gold or white, as its ! is.
		if (Posting.State == ENoticePosting::Offered && Posting.bOfferedHere)
		{
			const bool bMain = Posting.Mission && Posting.Mission->Kind == EMissionKind::Main;
			return MissionMarkIcons::Color(bMain) * FLinearColor(1.f, 1.f, 1.f, 0.55f);
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
	Nav.Reset();
	Super::NativeDestruct();
}

void UNoticeBoardWidget::Open(ALooterHUD* InHUD, ANoticeBoard* InBoard)
{
	OwningHUD = InHUD;
	Board = InBoard;
	GivenHere.Reset();
	Selected = INDEX_NONE;
	Hovered = INDEX_NONE;

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
	Hovered = INDEX_NONE;
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

	// What the keys do (filled for the device last used: RefreshPrompts), and a button to close it for the mouse.
	UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Bottom->AddChildToHorizontalBox(PromptBar)->SetVerticalAlignment(VAlign_Center);
	Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	CloseButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	CloseButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), TEXT("Close"), 0, LOCTEXT("Close", "Close"), 12,
		EButtonKind::Normal);
	CloseButton->OnButtonClicked.BindUObject(this, &UNoticeBoardWidget::HandleButton);
	Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, CloseButton, 120.f, 34.f))->SetVerticalAlignment(VAlign_Center);
	Content->AddChildToVerticalBox(Bottom)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	const FText Title = From && !From->Title.IsEmpty() ? From->Title : LOCTEXT("Title", "Notice Board");
	PanelBox->SetContent(MakePanel(WidgetTree, Title.ToString(), Content));

	// What wants doing first: a posting to turn in, then a job to take, else the first that can be tracked.
	Selected = INDEX_NONE;
	for (const ENoticePosting First : { ENoticePosting::Ready, ENoticePosting::Offered })
	{
		for (int32 Index = 0; Index < Postings.Num() && Selected == INDEX_NONE; ++Index)
		{
			Selected = Postings[Index].State == First && CanAct(Index) ? Index : INDEX_NONE;
		}
	}
	for (int32 Index = 0; Index < Postings.Num() && Selected == INDEX_NONE; ++Index)
	{
		Selected = CanAct(Index) ? Index : INDEX_NONE;
	}
	// Nothing to do here: the first card, to read (every card can be chosen).
	if (Selected == INDEX_NONE && !Postings.IsEmpty())
	{
		Selected = 0;
	}
	RebuildNav();
	// The focus look, the keycaps and the prompts for the device last used; every card's look.
	RefreshNavLook();
	SetStatus(FText::GetEmpty(), Color::TextDim());
}

// ---------------------------------------------------------------------------
// How the cards look
// ---------------------------------------------------------------------------

void UNoticeBoardWidget::Restyle()
{
	for (int32 Index = 0; Index < Cards.Num() && Index < Postings.Num(); ++Index)
	{
		const FCard& Card = Cards[Index];
		const bool bChosen = Index == Selected;
		const bool bHovered = Index == Hovered && Card.Button != nullptr;
		if (Card.Fill)
		{
			Card.Fill->SetColorAndOpacity(CardFill(Postings[Index], bChosen, bHovered));
		}
		if (Card.Line)
		{
			Card.Line->SetColorAndOpacity(CardLine(Postings[Index], bChosen, bHovered));
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

void UNoticeBoardWidget::HandleOfferClosed(FName MissionId, bool bAccepted)
{
	// Taken: its card reads Tracked now, and the board says so. Put back: as it was, still up for the taking.
	const FNoticeBoardPosting* Was = Postings.FindByPredicate([MissionId](const FNoticeBoardPosting& Each) { return Each.MissionId == MissionId; });
	const FText Title = Was && Was->Mission ? Was->Mission->Title : FText::FromName(MissionId);
	const bool bMain = Was && Was->Mission && Was->Mission->Kind == EMissionKind::Main;
	Refresh();
	if (bAccepted)
	{
		SetStatus(FText::Format(LOCTEXT("Taken", "{0}: taken. The tracker follows it."), Title), MissionMarkIcons::Color(bMain));
	}
	if (IsInViewport())
	{
		SetKeyboardFocus();
	}
}

#undef LOCTEXT_NAMESPACE
