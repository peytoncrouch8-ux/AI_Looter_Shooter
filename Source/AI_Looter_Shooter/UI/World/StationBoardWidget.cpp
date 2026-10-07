// UStationBoardWidget: the panel, its lines and keys.

#include "UI/World/StationBoardWidget.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Session/SessionSubsystem.h"
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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "StationBoard"

using namespace LooterUI;

namespace
{
	const FName ActionLine(TEXT("Line"));
	const FName ActionClose(TEXT("Close"));

	constexpr float PanelWidth = 640.f;
	constexpr float LineMinHeight = 64.f;
	constexpr float LineGap = 8.f;

	/** A line's card by what it is and whether it's chosen: a chosen destination lit orange, here and the blank line faint. */
	FLinearColor CardFill(const FStationBoardLine& Line, bool bChosen)
	{
		if (!Line.IsDestination())
		{
			return Hex(7, 26, 40, 110);
		}
		return bChosen ? Hex(255, 159, 28, 64) : Color::Row();
	}

	FLinearColor CardLine(const FStationBoardLine& Line, bool bChosen)
	{
		if (!Line.IsDestination())
		{
			return Hex(90, 200, 255, 46);
		}
		return bChosen ? Color::Accent() : Color::RowLine();
	}
}

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

void UStationBoardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// The board holds the keyboard (Escape closes it); its buttons never take it.
	SetIsFocusable(true);
}

void UStationBoardWidget::Open(ALooterHUD* InHUD, AActor* InFrom, const FStationBoardWords& InWords)
{
	OwningHUD = InHUD;
	From = InFrom;
	Words = InWords;

	// Read every time it opens: the story may have moved on since (an area opened, the first cast-off).
	TArray<UAreaDefinition*> Found;
	Lines = UAreaTravelSubsystem::GatherBoardLines(GetWorld(), Found);
	Areas.Reset();
	HereName = FText::GetEmpty();
	const FString Map = USessionSubsystem::MapOf(GetWorld());
	for (UAreaDefinition* Area : Found)
	{
		Areas.Add(Area);
		if (Area && !Map.IsEmpty() && Area->GetMapPackage().Equals(Map, ESearchCase::IgnoreCase))
		{
			HereName = StationBoard::AreaName(*Area);
		}
	}
	ConfirmIndex = INDEX_NONE;
	bConfirmAuto = false;

	// Opened before it was first shown, the panel is built as the widget is.
	if (WidgetTree && WidgetTree->RootWidget)
	{
		BuildPanel();
	}
}

void UStationBoardWidget::Close()
{
	CloseConfirm();
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseStationBoard();
	}
	else
	{
		RemoveFromParent();
	}
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UStationBoardWidget::RebuildWidget()
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

		// Over the panel: the confirm, while it's open.
		PopupLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Popup"));
		FillOverlaySlot(Root->AddChildToOverlay(PopupLayer));
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);

		BuildPanel();
	}
	return Super::RebuildWidget();
}

void UStationBoardWidget::BuildPanel()
{
	if (!PanelBox)
	{
		return;
	}
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (!HereName.IsEmpty())
	{
		Content->AddChildToVerticalBox(MakeText(WidgetTree, FText::Format(LOCTEXT("YouAreAt", "You are at {0}"), HereName).ToString(), 11,
			Color::TextDim(), true, 150));
	}
	Content->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("Destinations", "Destinations").ToString()))
		->SetPadding(FMargin(0.f, HereName.IsEmpty() ? 0.f : 12.f, 0.f, 0.f));

	LinesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(LinesBox)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	Cards.Reset();
	Cards.SetNum(Lines.Num());
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		LinesBox->AddChildToVerticalBox(MakeLineCard(Index))->SetPadding(FMargin(0.f, Index > 0 ? LineGap : 0.f, 0.f, 0.f));
	}
	if (Lines.IsEmpty())
	{
		UTextBlock* Nowhere = MakeText(WidgetTree, LOCTEXT("Nowhere", "Nowhere to go from here yet.").ToString(), 13, Color::TextDim());
		Nowhere->SetAutoWrapText(true);
		LinesBox->AddChildToVerticalBox(Nowhere);
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
		{ TEXT("E"), LOCTEXT("HintChoose", "Choose") },
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
	Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeButton(ActionClose, 0, LOCTEXT("Close", "Close"), 12, false), 120.f, 34.f))
		->SetVerticalAlignment(VAlign_Center);
	Content->AddChildToVerticalBox(Bottom)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	PanelBox->SetContent(MakePanel(WidgetTree, Words.Title.ToString(), Content));

	// The first line that goes somewhere is chosen to begin with.
	Selected = Lines.IndexOfByPredicate([](const FStationBoardLine& Each) { return Each.IsDestination(); });
	Restyle();
	SetStatus(FText::GetEmpty(), Color::TextDim());
	CloseConfirm();

	// Before the first cast-off the board's one trip asks at once (or says why it can't go yet).
	if (Lines.Num() == 1 && Lines[0].bFirstCastOff && Lines[0].IsDestination())
	{
		if (Lines[0].bLevelBuilt)
		{
			OpenConfirm(0, /*bAuto*/ true);
		}
		else
		{
			SetStatus(StationBoard::NotOpenText(Words, Lines[0]), Color::Worse());
		}
	}
}

UWidget* UStationBoardWidget::MakeLineCard(int32 Index)
{
	const FStationBoardLine& Line = Lines[Index];
	const bool bGoes = Line.IsDestination();
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// A strip down the left side: orange for a destination, faint for where the board stands and the blank line.
	Row->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(bGoes ? Color::Accent() : Color::RowLine())), 3.f));

	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	const bool bBlank = Line.Kind == EStationLine::NextMission;
	UTextBlock* NameText = MakeText(WidgetTree, Line.Name.ToString(), bBlank ? 13 : 17, bBlank || Line.bHere ? Color::TextDim() : Color::Title(), false, 40);
	NameText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	Info->AddChildToVerticalBox(NameText);
	const FText Note = DescribeLine(Line);
	if (!Note.IsEmpty())
	{
		const FLinearColor NoteColor = bGoes && !Line.bLevelBuilt ? Color::Worse() : Line.bFirstCastOff ? Color::Accent() : Color::TextDim();
		Info->AddChildToVerticalBox(MakeText(WidgetTree, Note.ToString(), 10, NoteColor, true, 150))->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	}
	UHorizontalBoxSlot* InfoSlot = Row->AddChildToHorizontalBox(Info);
	InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InfoSlot->SetVerticalAlignment(VAlign_Center);
	InfoSlot->SetPadding(FMargin(12.f, 0.f, 12.f, 0.f));

	FLineCard& Card = Cards[Index];
	UOverlay* CardBox = LoadoutParts::MakeCard(WidgetTree, Row, FMargin(10.f, 8.f, 14.f, 8.f), 1.5f, Card.Fill, Card.Line);
	USizeBox* Sized = MakeSized(WidgetTree, CardBox, 0.f);
	Sized->SetMinDesiredHeight(LineMinHeight);
	if (!bGoes)
	{
		// Here and the blank line go nowhere: no button.
		return Sized;
	}
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->SetupContent(Sized, ActionLine, Index, EButtonKind::Bare);
	Button->OnButtonClicked.BindUObject(this, &UStationBoardWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &UStationBoardWidget::HandleHovered);
	Card.Button = Button;
	return Button;
}

FText UStationBoardWidget::DescribeLine(const FStationBoardLine& Line) const
{
	if (Line.Kind == EStationLine::NextMission)
	{
		return LOCTEXT("NoteNext", "Opens the next destination");
	}
	if (Line.bHere)
	{
		return LOCTEXT("NoteHere", "You are here");
	}
	if (!Line.bLevelBuilt)
	{
		return LOCTEXT("NoteClosed", "Not open yet");
	}
	if (Line.bFirstCastOff)
	{
		return LOCTEXT("NoteStory", "Your story begins");
	}
	if (Line.Kind == EStationLine::Practice)
	{
		return LOCTEXT("NotePractice", "Practice: no experience, ammo only");
	}
	const UAreaDefinition* Area = Line.Area.Get();
	if (Area && Area->HasLevelBand())
	{
		return FText::Format(LOCTEXT("NoteBand", "Creatures level {0}-{1}"), FText::AsNumber(Area->MinLevel), FText::AsNumber(Area->GetBandTop()));
	}
	return FText::GetEmpty();
}

ULooterButton* UStationBoardWidget::MakeButton(FName Action, int32 Index, const FText& Label, int32 FontSize, bool bPrimary)
{
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), Action, Index, Label, FontSize,
		bPrimary ? EButtonKind::Primary : EButtonKind::Normal);
	Button->OnButtonClicked.BindUObject(this, &UStationBoardWidget::HandleButton);
	return Button;
}

// ---------------------------------------------------------------------------
// Choosing
// ---------------------------------------------------------------------------

void UStationBoardWidget::Select(int32 Index)
{
	if (Lines.IsValidIndex(Index) && Lines[Index].IsDestination() && Index != Selected)
	{
		Selected = Index;
		Restyle();
	}
}

void UStationBoardWidget::Step(int32 Direction)
{
	if (Lines.IsEmpty())
	{
		return;
	}
	// Past here and the blank line to the next line that goes somewhere, round the ends.
	int32 Index = Selected == INDEX_NONE ? (Direction > 0 ? -1 : 0) : Selected;
	for (int32 Tries = 0; Tries < Lines.Num(); ++Tries)
	{
		Index = (Index + Direction + Lines.Num()) % Lines.Num();
		if (Lines[Index].IsDestination())
		{
			Select(Index);
			return;
		}
	}
}

void UStationBoardWidget::Restyle()
{
	for (int32 Index = 0; Index < Cards.Num() && Index < Lines.Num(); ++Index)
	{
		const FLineCard& Card = Cards[Index];
		const bool bChosen = Index == Selected;
		if (Card.Fill)
		{
			Card.Fill->SetColorAndOpacity(CardFill(Lines[Index], bChosen));
		}
		if (Card.Line)
		{
			Card.Line->SetColorAndOpacity(CardLine(Lines[Index], bChosen));
		}
	}
}

void UStationBoardWidget::Choose(int32 Index)
{
	if (!Lines.IsValidIndex(Index) || !Lines[Index].IsDestination())
	{
		return;
	}
	Select(Index);
	const FStationBoardLine& Line = Lines[Index];
	if (!Line.bLevelBuilt)
	{
		// Until the level is built nobody goes, and the board says so ("The line to the Lily isn't open yet.").
		SetStatus(StationBoard::NotOpenText(Words, Line), Color::Worse());
		return;
	}
	OpenConfirm(Index, /*bAuto*/ false);
}

void UStationBoardWidget::SetStatus(const FText& Message, const FLinearColor& TextColor)
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

void UStationBoardWidget::HandleButton(ULooterButton* Button)
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
	if (Button->Action == ActionLine)
	{
		if (ConfirmIndex == INDEX_NONE)
		{
			Choose(Button->Index);
		}
	}
	else
	{
		HandleConfirmButton(Button);
	}
	// Clicking handed the keyboard to the game viewport (LooterButton); take it back so the keys keep reaching the board.
	if (IsInViewport())
	{
		SetKeyboardFocus();
	}
}

void UStationBoardWidget::HandleHovered(ULooterButton* Button)
{
	if (Button && ConfirmIndex == INDEX_NONE)
	{
		Select(Button->Index);
	}
}

FReply UStationBoardWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		// One step back at a time: the confirm, then the board.
		if (ConfirmIndex != INDEX_NONE)
		{
			Decline();
		}
		else
		{
			Close();
		}
		return FReply::Handled();
	}

	// The Interact key held to open the board repeats: only a fresh press takes a line or confirms.
	const bool bTake = !InKeyEvent.IsRepeat()
		&& (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom);
	if (ConfirmIndex != INDEX_NONE)
	{
		if (bTake)
		{
			ConfirmTrip();
		}
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
	if (bTake)
	{
		Choose(Selected);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UStationBoardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A click on no button ends here rather than in the game viewport, which would take the keyboard from the board.
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
