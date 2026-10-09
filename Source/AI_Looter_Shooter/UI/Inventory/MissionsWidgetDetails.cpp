// UMissionsWidget: the chosen mission (its words, steps, objectives and rewards), the keys, tracking, and what the page asks.

#include "UI/Inventory/MissionsWidget.h"
#include "Bestiary/Ledger.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	const FName ActionTrack(TEXT("Track"));

	/** "Main mission", "Side mission", "Tutorial": the details header's first words. */
	FString KindHeading(EMissionKind Kind)
	{
		switch (Kind)
		{
		case EMissionKind::Main:     return TEXT("Main mission");
		case EMissionKind::Side:     return TEXT("Side mission");
		case EMissionKind::Tutorial: break;
		}
		return TEXT("Tutorial");
	}
}

// ---------------------------------------------------------------------------
// The chosen mission
// ---------------------------------------------------------------------------

void UMissionsWidget::RefreshDetails()
{
	if (!DetailsBox)
	{
		return;
	}
	DetailsBox->ClearChildren();
	StepsBox->ClearChildren();
	const UMissionDefinition* Mission = GetSelected();
	if (!Mission)
	{
		DetailsHeader->SetText(FText::FromString(TEXT("MISSIONS")));
		DetailsBox->AddChildToVerticalBox(Label(WidgetTree, TEXT("No missions yet"), 17, Color::TextDim(), 40));
		UTextBlock* Help = MakeText(WidgetTree, TEXT("The jobs you take on, and the story as it unfolds, are written up here."), 10, Color::TextDim());
		Help->SetAutoWrapText(true);
		DetailsBox->AddChildToVerticalBox(Help)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		return;
	}
	const EMissionStatus Status = GetSelectedStatus();
	const FString Heading = Mission->Area.IsNone() ? KindHeading(Mission->Kind) : KindHeading(Mission->Kind) + TEXT(" · ") + AreaName(Mission->Area);
	DetailsHeader->SetText(FText::FromString(Heading.ToUpper()));

	// The card: its name, where it stands, what it's about, and tracking for one running here.
	UTextBlock* NameText = Label(WidgetTree, Mission->Title.ToString(), 17, Color::Text(), 40);
	NameText->SetAutoWrapText(true);
	DetailsBox->AddChildToVerticalBox(NameText);
	const UMissionRunner* MissionRunner = GetRunner();
	const bool bTracked = MissionRunner && MissionRunner->GetTrackedMission() == Mission->GetMissionId();
	FString Standing = DescribeStanding(*Mission, Status);
	if (bTracked)
	{
		Standing += TEXT(" · Tracked: the minimap guides you");
	}
	DetailsBox->AddChildToVerticalBox(Label(WidgetTree, Standing, 9, bTracked ? Color::Accent() : Color::TextDim(), 140))
		->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	if (!Mission->Summary.IsEmpty())
	{
		UTextBlock* Summary = MakeText(WidgetTree, Mission->Summary.ToString(), 10, Color::Text());
		Summary->SetAutoWrapText(true);
		DetailsBox->AddChildToVerticalBox(Summary)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
	if (CanTrackSelected())
	{
		ULooterButton* Track = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		Track->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), ActionTrack, Selected,
			FText::FromString(bTracked ? TEXT("Untrack") : TEXT("Track")), 11, bTracked ? EButtonKind::Normal : EButtonKind::Primary);
		Track->OnButtonClicked.BindUObject(this, &UMissionsWidget::HandleTrackClicked);
		UVerticalBoxSlot* TrackSlot = DetailsBox->AddChildToVerticalBox(MakeSized(WidgetTree, Track, 170.f, 34.f));
		TrackSlot->SetHorizontalAlignment(HAlign_Left);
		TrackSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}

	// Under the card: the steps, then what it gives.
	AddSteps(*Mission, Status);
	AddRewards(*Mission);
}

void UMissionsWidget::AddSteps(const UMissionDefinition& Mission, EMissionStatus Status)
{
	const UMissionRunner* MissionRunner = GetRunner();
	const FName MissionId = Mission.GetMissionId();
	const bool bRunning = MissionRunner && MissionRunner->IsRunning(MissionId);
	const bool bReady = MissionRunner && MissionRunner->IsReadyToTurnIn(MissionId);
	const int32 NumSteps = Mission.Steps.Num();

	// Up to the step being played: the steps before it done. A finished mission shows every step done, and so does one
	// waiting for its turn-in (with the turn-in after them); one not begun, none (what it asks is a surprise until it
	// starts).
	int32 Reached = INDEX_NONE;
	if (Status == EMissionStatus::Completed || bReady)
	{
		Reached = NumSteps;
	}
	else if (bRunning)
	{
		Reached = MissionRunner->GetStep(MissionId);
	}
	else if (Status == EMissionStatus::Active && MissionRunner)
	{
		// The campaign's mission, waiting in its own area: its saved step.
		Reached = FMath::Clamp(MissionRunner->GetCampaign().ActiveMissionStep, 0, FMath::Max(NumSteps - 1, 0));
	}

	StepsBox->AddChildToVerticalBox(Label(WidgetTree, TEXT("Objectives"), 10, Color::Accent(), 300))->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	if (Reached == INDEX_NONE)
	{
		UTextBlock* Waiting = MakeText(WidgetTree, Status == EMissionStatus::Available
			? TEXT("Its objectives show once it starts.") : TEXT("Finish what comes before it first."), 10, Color::TextDim());
		Waiting->SetAutoWrapText(true);
		StepsBox->AddChildToVerticalBox(Waiting);
		return;
	}

	for (int32 StepIndex = 0; StepIndex < FMath::Min(Reached, NumSteps); ++StepIndex)
	{
		for (const TObjectPtr<UMissionObjective>& Objective : Mission.Steps[StepIndex].Objectives)
		{
			if (Objective)
			{
				AddStepLine(Objective->GetDisplayText(GetWorld()), /*bDone*/ true, /*bCurrent*/ false);
			}
		}
	}
	if (Reached >= NumSteps)
	{
		// Every objective done: what's left is the turn-in, "Ready to turn in: talk to Delia".
		if (bReady)
		{
			AddObjectiveLine(Mission.GetTurnInText(), FString(), 1, 0.f, false);
		}
		return;
	}

	// The step being played: each objective with how far along it is (running here), or as it reads (elsewhere).
	const TArray<FMissionObjectiveView> Views = bRunning ? MissionRunner->GetObjectiveViews(MissionId) : TArray<FMissionObjectiveView>();
	if (!Views.IsEmpty())
	{
		for (const FMissionObjectiveView& View : Views)
		{
			AddObjectiveLine(View.Text, View.Progress, View.Required, View.Fraction, View.bDone);
		}
	}
	else
	{
		for (const TObjectPtr<UMissionObjective>& Objective : Mission.Steps[Reached].Objectives)
		{
			if (Objective)
			{
				AddObjectiveLine(Objective->GetDisplayText(GetWorld()), FString(), 1, 0.f, false);
			}
		}
	}
	const int32 Ahead = NumSteps - Reached - 1;
	if (Ahead > 0)
	{
		StepsBox->AddChildToVerticalBox(Label(WidgetTree, Ahead == 1 ? FString(TEXT("One more step after this")) : FString::Printf(TEXT("%d more steps after this"), Ahead),
			8, Color::TextDim(), 150))->SetPadding(FMargin(22.f, 10.f, 0.f, 0.f));
	}
}

void UMissionsWidget::AddStepLine(const FString& Text, bool bDone, bool bCurrent)
{
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* MarkSlot = Line->AddChildToHorizontalBox(MakeImage(WidgetTree, bDone ? DoneBrush : CurrentBrush));
	MarkSlot->SetVerticalAlignment(VAlign_Top);
	MarkSlot->SetPadding(FMargin(0.f, 3.f, 10.f, 0.f));
	UTextBlock* Words = MakeText(WidgetTree, Text, 10, bCurrent ? Color::Text() : Color::TextDim());
	Words->SetAutoWrapText(true);
	Line->AddChildToHorizontalBox(Words)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	StepsBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 4.f, 8.f, 0.f));
}

void UMissionsWidget::AddObjectiveLine(const FString& Text, const FString& Progress, int32 Required, float Fraction, bool bDone)
{
	// An objective of the current step: its words and count, and a bar under them when it counts several things.
	UVerticalBox* Block = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* MarkSlot = Line->AddChildToHorizontalBox(MakeImage(WidgetTree, bDone ? DoneBrush : CurrentBrush));
	MarkSlot->SetVerticalAlignment(VAlign_Top);
	MarkSlot->SetPadding(FMargin(0.f, 3.f, 10.f, 0.f));
	UTextBlock* Words = MakeText(WidgetTree, Text, 11, bDone ? Color::TextDim() : Color::Text());
	Words->SetAutoWrapText(true);
	Line->AddChildToHorizontalBox(Words)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	if (!Progress.IsEmpty() || bDone)
	{
		UTextBlock* Count = MakeText(WidgetTree, bDone ? FString(TEXT("Done")) : Progress, 11, bDone ? Color::Better() : Color::Accent());
		Count->SetJustification(ETextJustify::Right);
		UHorizontalBoxSlot* CountSlot = Line->AddChildToHorizontalBox(Count);
		CountSlot->SetVerticalAlignment(VAlign_Top);
		CountSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	}
	Block->AddChildToVerticalBox(Line);
	if (Required > 1 && !bDone)
	{
		Block->AddChildToVerticalBox(MakeSegmentBar(WidgetTree, FMath::Clamp(Required, 2, 12), Fraction, Color::Accent(), 4.f))
			->SetPadding(FMargin(22.f, 6.f, 0.f, 0.f));
	}
	StepsBox->AddChildToVerticalBox(Block)->SetPadding(FMargin(0.f, 6.f, 8.f, 0.f));
}

void UMissionsWidget::AddRewards(const UMissionDefinition& Mission)
{
	const TArray<FString> Lines = MissionRewards::Describe(Mission.Rewards);
	if (Lines.IsEmpty())
	{
		return;
	}
	const bool bGiven = GetSelectedStatus() == EMissionStatus::Completed;
	StepsBox->AddChildToVerticalBox(Label(WidgetTree, bGiven ? TEXT("Rewarded") : TEXT("Rewards"), 10, Color::Accent(), 300))
		->SetPadding(FMargin(0.f, 22.f, 0.f, 4.f));
	for (const FString& Reward : Lines)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* BulletSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Accent())), 5.f, 5.f));
		BulletSlot->SetVerticalAlignment(VAlign_Top);
		BulletSlot->SetPadding(FMargin(3.f, 6.f, 13.f, 0.f));
		UTextBlock* Words = MakeText(WidgetTree, Reward, 10, bGiven ? Color::TextDim() : Color::Text());
		Words->SetAutoWrapText(true);
		Line->AddChildToHorizontalBox(Words)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		StepsBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 4.f, 8.f, 0.f));
	}
}

// ---------------------------------------------------------------------------
// Keys and tracking
// ---------------------------------------------------------------------------

void UMissionsWidget::RefreshPrompts()
{
	if (!PromptBar)
	{
		return;
	}
	PromptBar->ClearChildren();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const UMissionRunner* MissionRunner = GetRunner();
	const UMissionDefinition* Mission = GetSelected();
	const bool bTracked = Mission && MissionRunner && MissionRunner->GetTrackedMission() == Mission->GetMissionId();
	struct FPrompt
	{
		FString Key;
		FString Text;
	};
	TArray<FPrompt, TInlineAllocator<6>> Prompts;
	Prompts.Add({ TEXT("W / S"), TEXT("Browse") });
	if (CanTrackSelected())
	{
		Prompts.Add({ TEXT("E"), bTracked ? TEXT("Untrack") : TEXT("Track") });
	}
	Prompts.Add({ TEXT("1"), TEXT("Loadout") });
	Prompts.Add({ TEXT("2"), Ledger::BookName(Ledger::IsOpenIn(this)) });
	Prompts.Add({ TEXT("4"), TEXT("Map") });
	Prompts.Add({ Bindings ? Bindings->GetKey(TEXT("Inventory")).GetDisplayName().ToString() : FString(TEXT("Tab")), TEXT("Close") });
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		PromptBar->AddChildToHorizontalBox(MakeKeyHint(WidgetTree, Prompts[Index].Key, Prompts[Index].Text, Index == 0))
			->SetPadding(FMargin(Index > 0 ? 26.f : 0.f, 0.f, 0.f, 0.f));
	}
}

bool UMissionsWidget::CanTrackSelected() const
{
	// Only a mission running in this level has a waypoint for the minimap to guide to.
	const UMissionRunner* MissionRunner = GetRunner();
	const UMissionDefinition* Mission = GetSelected();
	return MissionRunner && Mission && MissionRunner->IsRunning(Mission->GetMissionId());
}

void UMissionsWidget::ToggleTrack()
{
	UMissionRunner* MissionRunner = GetRunner();
	const UMissionDefinition* Mission = GetSelected();
	if (!MissionRunner || !Mission || !CanTrackSelected())
	{
		return;
	}
	const FName MissionId = Mission->GetMissionId();
	MissionRunner->TrackMission(MissionRunner->GetTrackedMission() == MissionId ? FName(NAME_None) : MissionId);
	// The runner tells the page, which redraws on its next tick; the button and prompts follow at once.
	RefreshDetails();
	RefreshPrompts();
}

void UMissionsWidget::HandleTrackClicked(ULooterButton* Button)
{
	ToggleTrack();
	// Clicking handed keyboard focus to the game viewport (LooterButton); take it back for the page's keys.
	SetKeyboardFocus();
}

FReply UMissionsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;

	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || (Bindings ? Bindings->IsInventoryKey(Key) : (Key == EKeys::Tab || Key == EKeys::I)))
	{
		Close();
		return FReply::Handled();
	}
	if (Key == EKeys::One)
	{
		ShowPage(EInventoryPage::Loadout);
		return FReply::Handled();
	}
	if (Key == EKeys::Two || Key == EKeys::Gamepad_LeftShoulder)
	{
		ShowPage(EInventoryPage::Bestiary);
		return FReply::Handled();
	}
	if (Key == EKeys::Four || Key == EKeys::Gamepad_RightShoulder)
	{
		ShowPage(EInventoryPage::Map);
		return FReply::Handled();
	}
	if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up)
	{
		Select(Selected - 1, true);
		return FReply::Handled();
	}
	if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down)
	{
		Select(Selected + 1, true);
		return FReply::Handled();
	}
	const bool bInteractKey = Bindings && Bindings->GetKey(TEXT("Interact")) == Key;
	if (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom || bInteractKey)
	{
		ToggleTrack();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

UMissionRunner* UMissionsWidget::GetRunner() const
{
	if (UMissionRunner* Followed = Runner.Get())
	{
		return Followed;
	}
	return GetWorld() ? GetWorld()->GetSubsystem<UMissionRunner>() : nullptr;
}

const UMissionDefinition* UMissionsWidget::GetSelected() const
{
	return Listed.IsValidIndex(Selected) ? Listed[Selected].Get() : nullptr;
}

EMissionStatus UMissionsWidget::GetSelectedStatus() const
{
	return Statuses.IsValidIndex(Selected) ? Statuses[Selected] : EMissionStatus::Locked;
}

FString UMissionsWidget::AreaName(FName AreaId) const
{
	const FString* Found = AreaNames.Find(AreaId);
	return Found ? *Found : FName::NameToDisplayString(AreaId.ToString(), /*bIsBool*/ false);
}

FString UMissionsWidget::DescribeStanding(const UMissionDefinition& Mission, EMissionStatus Status) const
{
	const UMissionRunner* MissionRunner = GetRunner();
	const FName MissionId = Mission.GetMissionId();
	switch (Status)
	{
	case EMissionStatus::Active:
		// Its objectives done, waiting for its giver: here, or in its own area.
		if (MissionRunner && MissionRunner->IsReadyToTurnIn(MissionId))
		{
			return MissionRunner->IsRunning(MissionId) || Mission.Area.IsNone() ? Mission.GetTurnInText()
				: FString::Printf(TEXT("%s in %s"), *Mission.GetTurnInText(), *AreaName(Mission.Area));
		}
		if (MissionRunner && MissionRunner->IsRunning(MissionId))
		{
			return FString::Printf(TEXT("Step %d of %d"), MissionRunner->GetStep(MissionId) + 1, Mission.Steps.Num());
		}
		// The campaign's mission, waiting in its own area.
		return Mission.Area.IsNone() ? FString(TEXT("Waiting")) : FString::Printf(TEXT("Continue in %s"), *AreaName(Mission.Area));
	case EMissionStatus::Available:
		if (!Mission.Area.IsNone() && MissionRunner && Mission.Area != MissionRunner->GetAreaHere())
		{
			return FString::Printf(TEXT("Ready in %s"), *AreaName(Mission.Area));
		}
		return Mission.Start == EMissionStart::OnEvent ? FString(TEXT("Ready to pick up")) : FString(TEXT("Ready"));
	case EMissionStatus::Completed:
		return TEXT("Completed");
	case EMissionStatus::Locked:
		break;
	}
	return TEXT("Locked");
}
