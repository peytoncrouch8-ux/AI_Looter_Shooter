// UMissionsWidget: what the 2026-10-13 mission features add to the chosen mission's page: its optional objectives (dimmed,
// "Optional:", done, missed or still to be had), the mission items carried for it, the optional objectives' bonus, and the
// choice of two it ends in (both, and what each gives; once turned in, the one chosen).

#include "UI/Inventory/MissionsWidget.h"
#include "Economy/EconomyRules.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/MissionMarkIcons.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	/** An optional objective's mark: a hollow diamond, the current step's mark drawn in outline. */
	const FVectorIcon& HollowIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Diamond;
			Diamond.ViewBox = FVector2D(12.f, 12.f);
			Diamond.StrokeWidth = 1.4f;
			Diamond.Strokes.Add({ FVector2D(6.0, 1.5), FVector2D(10.5, 6.0), FVector2D(6.0, 10.5), FVector2D(1.5, 6.0), FVector2D(6.0, 1.5) });
			return Diamond;
		}();
		return Icon;
	}

	/** A missed one's mark: a short dash, the chance gone by. */
	const FVectorIcon& DashIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Dash;
			Dash.ViewBox = FVector2D(12.f, 12.f);
			Dash.StrokeWidth = 1.6f;
			Dash.Strokes.Add({ FVector2D(3.0, 6.0), FVector2D(9.0, 6.0) });
			return Dash;
		}();
		return Icon;
	}
}

// ---------------------------------------------------------------------------
// Optional objectives
// ---------------------------------------------------------------------------

void UMissionsWidget::AddPastOptional(const UMissionDefinition& Mission, int32 StepIndex, int32 Index, bool bReady)
{
	const UMissionObjective* Objective = Mission.GetObjective(StepIndex, Index);
	const UMissionRunner* MissionRunner = GetRunner();
	if (!Objective || !MissionRunner)
	{
		return;
	}
	const FName MissionId = Mission.GetMissionId();
	const bool bCompleted = MissionRunner->IsCompleted(MissionId);
	// A finished mission forgets which were done (it paid for them): its optional objectives show as they read.
	if (bCompleted)
	{
		AddOptionalLine(Objective->GetDisplayText(GetWorld()), FString(), /*bDone*/ false, /*bMissed*/ false);
		return;
	}
	const bool bDone = MissionRunner->GetCampaign().HasDoneOptional(MissionId, StepIndex, Index);
	// The last step's, while the mission waits for its turn-in, can still be done (with its progress, when it's running here).
	const bool bStillOpen = bReady && StepIndex == Mission.Steps.Num() - 1;
	FString Progress;
	if (bStillOpen && !bDone)
	{
		for (const FMissionObjectiveView& View : MissionRunner->GetOptionalViews(MissionId))
		{
			if (View.Text == Objective->GetDisplayText(GetWorld()))
			{
				Progress = View.Progress;
				break;
			}
		}
	}
	AddOptionalLine(Objective->GetDisplayText(GetWorld()), Progress, bDone, /*bMissed*/ !bDone && !bStillOpen);
}

void UMissionsWidget::AddOptionalLine(const FString& Text, const FString& Progress, bool bDone, bool bMissed)
{
	if (!OptionalBrush.GetResourceObject())
	{
		OptionalBrush = IconBrush(TEXT("MissionStepOptional"), HollowIcon(), 3.f, FVector2D(12.f, 12.f), Color::Title());
		MissedBrush = IconBrush(TEXT("MissionStepMissed"), DashIcon(), 3.f, FVector2D(12.f, 12.f), Color::TextDim());
	}
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* MarkSlot = Line->AddChildToHorizontalBox(MakeImage(WidgetTree, bDone ? DoneBrush : bMissed ? MissedBrush : OptionalBrush));
	MarkSlot->SetVerticalAlignment(VAlign_Top);
	MarkSlot->SetPadding(FMargin(0.f, 3.f, 10.f, 0.f));
	// "OPTIONAL" small in cyan, then the words a shade dimmer than what the step asks for.
	UHorizontalBoxSlot* TagSlot = Line->AddChildToHorizontalBox(Label(WidgetTree, TEXT("Optional"), 8, bMissed ? Color::TextDim() : Color::Title(), 200));
	TagSlot->SetVerticalAlignment(VAlign_Top);
	TagSlot->SetPadding(FMargin(0.f, 2.f, 8.f, 0.f));
	UTextBlock* Words = MakeText(WidgetTree, Text, 10, Color::TextDim());
	Words->SetAutoWrapText(true);
	Words->SetRenderOpacity(bMissed ? 0.6f : 1.f);
	Line->AddChildToHorizontalBox(Words)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	const FString Right = bDone ? FString(TEXT("Done")) : bMissed ? FString(TEXT("Missed")) : Progress;
	if (!Right.IsEmpty())
	{
		UTextBlock* State = MakeText(WidgetTree, Right, 10, bDone ? Color::Better() : bMissed ? Color::TextDim() : Color::Title());
		State->SetJustification(ETextJustify::Right);
		UHorizontalBoxSlot* StateSlot = Line->AddChildToHorizontalBox(State);
		StateSlot->SetVerticalAlignment(VAlign_Top);
		StateSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	}
	StepsBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 5.f, 8.f, 0.f));
}

FString UMissionsWidget::DescribeOptionalBonus(const UMissionDefinition& Mission) const
{
	if (Mission.CountOptional() <= 0 || Mission.Rewards.OptionalSilver <= 0)
	{
		return FString();
	}
	// At the player's level now, as the turn-in will pay it.
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	const int32 Each = LooterEconomy::AtLevel(Mission.Rewards.OptionalSilver, Progression ? FMath::Max(Progression->GetLevel(), 1) : 1);
	const UMissionRunner* MissionRunner = GetRunner();
	const int32 Done = MissionRunner ? MissionRunner->CountOptionalDone(Mission.GetMissionId()) : 0;
	FString Words = FString::Printf(TEXT("Optional objectives: +%s silver each"), *LooterEconomy::Format(Each));
	if (Done > 0)
	{
		Words += FString::Printf(TEXT(" (%d done)"), Done);
	}
	return Words;
}

// ---------------------------------------------------------------------------
// Carried
// ---------------------------------------------------------------------------

void UMissionsWidget::AddCarried(const UMissionDefinition& Mission)
{
	const UMissionRunner* MissionRunner = GetRunner();
	const TArray<FMissionItemInfo> Carried = MissionRunner ? MissionRunner->GetCarriedItems(Mission.GetMissionId()) : TArray<FMissionItemInfo>();
	if (Carried.IsEmpty())
	{
		return;
	}
	StepsBox->AddChildToVerticalBox(Label(WidgetTree, TEXT("Carried"), 10, Color::Accent(), 300))->SetPadding(FMargin(0.f, 22.f, 0.f, 4.f));
	for (const FMissionItemInfo& Item : Carried)
	{
		// Its name bright, its line under it dim: a thing in the pocket, not a step.
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* BulletSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree,
			RectBrush(MissionMarkIcons::Color(Mission.Kind == EMissionKind::Main))), 6.f, 6.f));
		BulletSlot->SetVerticalAlignment(VAlign_Top);
		BulletSlot->SetPadding(FMargin(3.f, 6.f, 12.f, 0.f));
		UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		const FString Name = Item.Name.IsEmpty() ? FName::NameToDisplayString(Item.Id.ToString(), false) : Item.Name.ToString();
		Words->AddChildToVerticalBox(MakeText(WidgetTree, Name, 11, Color::Text()));
		if (!Item.Line.IsEmpty())
		{
			UTextBlock* About = MakeText(WidgetTree, Item.Line.ToString(), 9, Color::TextDim());
			About->SetFont(Font(9, false));
			About->SetAutoWrapText(true);
			Words->AddChildToVerticalBox(About)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		}
		Line->AddChildToHorizontalBox(Words)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		StepsBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 6.f, 8.f, 0.f));
	}
}

// ---------------------------------------------------------------------------
// The choice it ends in
// ---------------------------------------------------------------------------

void UMissionsWidget::AddChoice(const UMissionDefinition& Mission, EMissionStatus Status)
{
	if (!Mission.HasTurnInChoice())
	{
		return;
	}
	const UMissionRunner* MissionRunner = GetRunner();
	const int32 Chosen = MissionRunner ? MissionRunner->GetTurnInChoice(Mission.GetMissionId()) : INDEX_NONE;
	const bool bSettled = Status == EMissionStatus::Completed && Chosen != INDEX_NONE;
	StepsBox->AddChildToVerticalBox(Label(WidgetTree, bSettled ? TEXT("You chose") : TEXT("Your call at the turn-in"), 10, Color::Accent(), 300))
		->SetPadding(FMargin(0.f, 22.f, 0.f, 4.f));
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const FMissionTurnInChoice* Choice = Mission.GetTurnInChoice(Index);
		if (!Choice || (bSettled && Index != Chosen))
		{
			continue;
		}
		const FString Gives = MissionRewards::DescribeShort(Choice->Rewards);
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* BulletSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Title())), 5.f, 5.f));
		BulletSlot->SetVerticalAlignment(VAlign_Top);
		BulletSlot->SetPadding(FMargin(3.f, 6.f, 13.f, 0.f));
		UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Words->AddChildToVerticalBox(MakeText(WidgetTree, Gives.IsEmpty() ? Choice->Title.ToString() : FString::Printf(TEXT("%s: %s"),
			*Choice->Title.ToString(), *Gives), 10, bSettled ? Color::TextDim() : Color::Text()));
		if (!Choice->Line.IsEmpty() && !bSettled)
		{
			UTextBlock* About = MakeText(WidgetTree, Choice->Line.ToString(), 9, Color::TextDim());
			About->SetFont(Font(9, false));
			About->SetAutoWrapText(true);
			Words->AddChildToVerticalBox(About)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		}
		Line->AddChildToHorizontalBox(Words)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		StepsBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 4.f, 8.f, 0.f));
	}
}
