// UPauseMenuWidget's tracked-mission card (E2b, mockups/e.js "mission tag"), under the hero card: its gold seal on the
// card's step, TRACKED MISSION and the step count, the mission's name and its step pips, the step on show and its optional
// ones, as the HUD's tracker shows them (UI/HUD/HudMissionTracker*). The card is painted to fit its words (the in-game type
// is a third bigger than the mockup's), and isn't shown with no mission tracked.

#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/PauseMenuArt.h"
#include "UI/Menus/PauseMenuCardKit.h"
#include "UI/Menus/PauseMenuIcons.h"
#include "UI/Menus/PauseMenuLayout.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Missions/MissionSubsystem.h"
#include "Brushes/SlateNoResource.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

using namespace PauseMenuLayout;
using namespace PauseMenuCardKit;
using namespace LooterUI;

namespace
{
	/** The card's seal: a 40 px gold gem 14 over its top, the mission mark in it; the card's step for it; the pips' gap. */
	constexpr float MissionSeal = 40.f;
	constexpr float MissionSealAbove = 14.f;
	constexpr float MissionSealIcon = 22.f;
	constexpr float MissionNotchHalf = 52.f;
	constexpr float StepGap = 5.f;
	/** The card's line, as the mockup's (3 px at 0.7), and its height before its first fill. */
	constexpr float MissionLine = 3.f;
	constexpr float MissionLineOpacity = 0.7f;
	constexpr float MissionStartHeight = 200.f;
}

void UPauseMenuWidget::MakeMissionCard()
{
	UCanvasPanel* Card = MakeGroup(FVector2D(CardX, MissionY), FVector2D(CardWidth, MissionStartHeight), true);
	Card->SetVisibility(ESlateVisibility::HitTestInvisible);
	MissionGroup = Card;

	// The card's glass (a background) and lines are painted for its height as it's filled (RefreshMission).
	UImage* Glass = MakeImage(WidgetTree, FSlateNoResource());
	MarkBackground(Glass);
	Put(Card, Glass, FVector2D(-LooterReticle::Pad, -LooterReticle::Pad), FVector2D::ZeroVector);
	UImage* Lines = MakeImage(WidgetTree, FSlateNoResource());
	Put(Card, Lines, FVector2D(-LooterReticle::Pad, -LooterReticle::Pad), FVector2D::ZeroVector);
	Mission.Glass = Glass;
	Mission.Lines = Lines;

	// Its seal on the step: a gold gem with the mission mark.
	const FVector2D SealAt(CardWidth * 0.5f - MissionSeal * 0.5f, -MissionSealAbove);
	Put(Card, MakePicture(TEXT("PauseMissionSeal"), PauseMenuArt::GoldGem(MissionSeal, 3.f)), SealAt, FVector2D::ZeroVector);
	UImage* Mark = MakeImage(WidgetTree, PaintedIconBrush(TEXT("PauseIconMission"), PauseMenuIcons::Get(TEXT("Mission")), MissionSealIcon * PictureResolution / 24.f,
		FVector2D(MissionSealIcon, MissionSealIcon), Color::SoulGold()));
	Put(Card, Mark, SealAt + FVector2D((MissionSeal - MissionSealIcon) * 0.5f), FVector2D::ZeroVector);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	// TRACKED MISSION and the step count; then the name with the step pips beside it (the mockup has the pips by the count:
	// in the bigger type that line runs out of room).
	UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Label = Words(WidgetTree, MissionLabelFont, true, Color::SlateFlavor(), MissionLabelSpacing);
	Label->SetText(FText::FromString(TEXT("TRACKED MISSION")));
	AddTo(Top, Label);
	AddFill(WidgetTree, Top);
	UTextBlock* Count = Words(WidgetTree, MissionCountFont, true, Color::SlateLabel());
	AddTo(Top, Count);
	Mission.Count = Count;
	Column->AddChildToVerticalBox(Top);

	UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Name = Words(WidgetTree, MissionNameFont, true, Color::SlateText(), MissionNameSpacing);
	AddTo(NameRow, ShrinkToFit(WidgetTree, Name))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UHorizontalBox* Steps = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	AddTo(NameRow, Steps, 12.f);
	Mission.Name = Name;
	Mission.Steps = Steps;
	Column->AddChildToVerticalBox(NameRow)->SetPadding(FMargin(0.f, MissionLabelGap, 0.f, 0.f));

	// The step on show, then its optional ones, dimmed.
	UTextBlock* Line = Words(WidgetTree, MissionLineFont, false, Color::SlateText());
	Line->SetWrapTextAt(MissionTextWidth);
	Column->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, MissionNameGap, 0.f, 0.f));
	Mission.Line = Line;
	UVerticalBox* Optionals = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(Optionals);
	Mission.Optionals = Optionals;

	Put(Card, Column, FVector2D(MissionPadSide, MissionPadTop), FVector2D(MissionTextWidth, 0.f))->SetAutoSize(true);
}

void UPauseMenuWidget::RefreshMission()
{
	const UWorld* World = GetWorld();
	const UMissionSubsystem* Missions = World ? World->GetSubsystem<UMissionSubsystem>() : nullptr;
	const FMission* Tracked = Missions ? Missions->GetTracked() : nullptr;
	if (MissionGroup)
	{
		MissionGroup->SetVisibility(Tracked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!Tracked)
	{
		return;
	}

	// As the HUD's tracker shows it: the step being played of how many (all done while it waits for its turn-in), the short
	// line with its count, the step's optional objectives.
	const FMissionTrackerParts& Parts = Tracked->Tracker;
	const int32 StepCount = FMath::Max(1, Parts.StepCount);
	const int32 Step = FMath::Clamp(Parts.Step, 0, StepCount);
	SetWords(Mission.Count, FString::Printf(TEXT("%d / %d"), FMath::Min(Step + 1, StepCount), StepCount));
	SetWords(Mission.Name, Tracked->Title.ToString().ToUpper());
	FString Line = Parts.Line.IsEmpty() ? Tracked->Objective.ToString() : Parts.Line;
	if (!Parts.Count.IsEmpty())
	{
		Line += FString::Printf(TEXT("  %s"), *Parts.Count);
	}
	Line = FString::Printf(TEXT("• %s"), *Line);
	SetWords(Mission.Line, Line);

	// The step pips: done ones salt, the current one gold, the rest hollow.
	if (UHorizontalBox* Steps = Mission.Steps.Get())
	{
		Steps->ClearChildren();
		const TCHAR* const PipNames[] = { TEXT("PauseStepDone"), TEXT("PauseStepNow"), TEXT("PauseStepAhead") };
		for (int32 Index = 0; Index < StepCount; ++Index)
		{
			const int32 State = Index < Step ? 0 : Index == Step ? 1 : 2;
			AddTo(Steps, MakePicture(PipNames[State], PauseMenuArt::StepPip(State)), Index > 0 ? StepGap : 0.f);
		}
	}

	TArray<FString> OptionalLines;
	if (UVerticalBox* Box = Mission.Optionals.Get())
	{
		Box->ClearChildren();
		for (const FMissionTrackerOptional& Optional : Parts.Optionals)
		{
			FString Text = FString::Printf(TEXT("• Optional: %s"), *Optional.Line);
			if (!Optional.Count.IsEmpty())
			{
				Text += FString::Printf(TEXT("  %s"), *Optional.Count);
			}
			if (Optional.bDone)
			{
				Text += TEXT(" (done)");
			}
			UTextBlock* Row = Words(WidgetTree, MissionOptionalFont, false, Optional.bDone ? Color::SlateIcon() : Color::SlateLabel());
			Row->SetText(FText::FromString(Text));
			Row->SetWrapTextAt(MissionTextWidth);
			Box->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, MissionOptionalGap, 0.f, 0.f));
			OptionalLines.Add(Text);
		}
	}

	// The card fitted to its words: painted again only when its height changes (each height's texture is kept for the
	// session; the heights come in whole lines, so there are few).
	const float Height = MissionHeight(Line, OptionalLines);
	if (!FMath::IsNearlyEqual(Height, Mission.Height))
	{
		Mission.Height = Height;
		if (UImage* Glass = Mission.Glass.Get())
		{
			Glass->SetBrush(PictureBrush(TEXT("PauseMissionGlass"),
				PauseMenuArt::TagCard(CardWidth, Height, MissionNotchHalf, 0.f, MissionLine, MissionLineOpacity, PauseMenuArt::EPart::Fill)));
		}
		if (UImage* Lines = Mission.Lines.Get())
		{
			Lines->SetBrush(PictureBrush(TEXT("PauseMissionLines"),
				PauseMenuArt::TagCard(CardWidth, Height, MissionNotchHalf, 0.f, MissionLine, MissionLineOpacity, PauseMenuArt::EPart::Lines)));
		}
		if (UCanvasPanelSlot* CardSlot = MissionGroup ? Cast<UCanvasPanelSlot>(MissionGroup->Slot) : nullptr)
		{
			CardSlot->SetSize(FVector2D(CardWidth, Height));
		}
	}
}

bool UPauseMenuWidget::IsMissionCardShown() const
{
	return MissionGroup && MissionGroup->GetVisibility() != ESlateVisibility::Collapsed;
}
