// UStationBoardWidget: the confirm, and leaving.

#include "UI/World/StationBoardWidget.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "StationBoard"

using namespace LooterUI;

namespace
{
	const FName ActionNotYet(TEXT("NotYet"));
	const FName ActionGo(TEXT("Go"));

	constexpr float ConfirmWidth = 540.f;

	/**
	 * The board behind an open confirm, faded as a whole: its lines would otherwise read through the confirm's glass (the
	 * dimming backdrop is a background, which the UI transparency setting thins).
	 */
	constexpr float BoardOpacityBehindConfirm = 0.15f;
}

void UStationBoardWidget::OpenConfirm(int32 Index, bool bAuto)
{
	if (!PopupLayer || !Lines.IsValidIndex(Index))
	{
		return;
	}
	const FStationBoardLine& Line = Lines[Index];
	ConfirmIndex = Index;
	bConfirmAuto = bAuto;

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* Body = MakeText(WidgetTree, ConfirmBody(Line).ToString(), 14, Color::Text());
	Body->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(Body);

	// Not yet on the left; the trip, the call to action, on the right.
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* NotYetSlot = Buttons->AddChildToHorizontalBox(MakeButton(ActionNotYet, Index, LOCTEXT("NotYet", "Not yet"), 14, false));
	NotYetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NotYetSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	Buttons->AddChildToHorizontalBox(MakeButton(ActionGo, Index, Words.Depart, 14, true))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Content->AddChildToVerticalBox(MakeSized(WidgetTree, Buttons, 0.f, 44.f))->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));

	// The board behind dims further and takes every click, so nothing under it can be pressed while it asks.
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Backdrop->SetBrush(RectBrush(Color::Backdrop()));
	Backdrop->SetVisibility(ESlateVisibility::Visible);
	Backdrop->SetHorizontalAlignment(HAlign_Center);
	Backdrop->SetVerticalAlignment(VAlign_Center);
	MarkBackground(Backdrop);
	Backdrop->SetContent(MakeSized(WidgetTree, MakePanel(WidgetTree, ConfirmTitle(Line).ToString(), Content), ConfirmWidth));

	PopupLayer->ClearChildren();
	FillOverlaySlot(PopupLayer->AddChildToOverlay(Backdrop));
	PopupLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (PanelBox)
	{
		PanelBox->SetRenderOpacity(BoardOpacityBehindConfirm);
	}
}

void UStationBoardWidget::CloseConfirm()
{
	ConfirmIndex = INDEX_NONE;
	bConfirmAuto = false;
	if (PopupLayer)
	{
		PopupLayer->ClearChildren();
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PanelBox)
	{
		PanelBox->SetRenderOpacity(1.f);
	}
}

void UStationBoardWidget::Decline()
{
	// The first cast-off's confirm opened with the board: Not yet puts both away.
	const bool bCloseBoard = bConfirmAuto;
	CloseConfirm();
	if (bCloseBoard)
	{
		Close();
	}
}

void UStationBoardWidget::HandleConfirmButton(ULooterButton* Button)
{
	if (Button->Action == ActionGo)
	{
		ConfirmTrip();
	}
	else if (Button->Action == ActionNotYet)
	{
		Decline();
	}
}

void UStationBoardWidget::ConfirmTrip()
{
	if (!Lines.IsValidIndex(ConfirmIndex))
	{
		CloseConfirm();
		return;
	}
	// Kept apart from the board, which is put away first: the game has its input back before the ride or the fade.
	const FStationBoardLine Line = Lines[ConfirmIndex];
	const FStationBoardWords Spoken = Words;
	ALooterHUD* HUD = OwningHUD.Get();
	AActor* Origin = From.Get();
	Close();

	UAreaTravelSubsystem* Travel = UAreaTravelSubsystem::Get(this);
	if (Travel && Travel->Depart(Line, Origin))
	{
		return;
	}
	// Nobody could go (a trip already under way, no game to leave): the board again, saying so.
	UE_LOG(LogLooter, Warning, TEXT("Station board: the trip to %s couldn't go."), *Line.Name.ToString());
	if (HUD && HUD->OpenStationBoard(Origin, Spoken))
	{
		CloseConfirm();
		SetStatus(LOCTEXT("CantLeave", "You can't leave right now."), Color::Worse());
	}
}

FText UStationBoardWidget::ConfirmTitle(const FStationBoardLine& Line) const
{
	if (Line.bFirstCastOff)
	{
		return FText::Format(LOCTEXT("LeaveHere", "Leave {0}?"), HereName.IsEmpty() ? LOCTEXT("Skyreach", "Skyreach") : HereName);
	}
	return FText::Format(LOCTEXT("LeaveFor", "Leave for {0}?"), Line.Name);
}

FText UStationBoardWidget::ConfirmBody(const FStationBoardLine& Line) const
{
	if (Line.bFirstCastOff)
	{
		return LOCTEXT("StoryBegins", "Your story begins. You can come back to practice any time.");
	}
	if (Line.Kind == EStationLine::Practice)
	{
		return LOCTEXT("PracticeBody", "Practice: its creatures give no experience and drop only ammo. Its jetty brings you back.");
	}
	return LOCTEXT("TripBody", "You'll arrive at its station.");
}

#undef LOCTEXT_NAMESPACE
