// UHudMissionTrackerWidget: painting what it shows (only when it changes): the title, the step circles and their number,
// the objective row with its ring or check, and the key hint; and the checks popping in. The optional objectives under
// them are HudMissionTrackerWidgetOptional.cpp's.

#include "UI/HUD/HudMissionTrackerWidget.h"
#include "UI/HUD/HudMissionTrackerShapes.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

using namespace LooterUI;

namespace
{
	/** A check pops in from this big, overshooting a little as it settles over CheckPopSeconds. */
	constexpr float CheckPopFrom = 1.8f;
	constexpr float CheckPopSeconds = 0.4f;

	/** Out fast, overshooting a touch, settling (a pop's ease). */
	float CheckEaseOutBack(float Share)
	{
		const float T = FMath::Clamp(Share, 0.f, 1.f) - 1.f;
		constexpr float Overshoot = 1.7f;
		return 1.f + T * T * ((Overshoot + 1.f) * T + Overshoot);
	}

	/** A check DoneShare of the way through its pop: its scale and opacity (it's solid by a third of the way). */
	void ApplyCheckPop(UWidget* Check, float DoneShare)
	{
		if (Check)
		{
			Check->SetRenderScale(FVector2D(FMath::Lerp(CheckPopFrom, 1.f, CheckEaseOutBack(DoneShare))));
			Check->SetRenderOpacity(FMath::Min(1.f, DoneShare * 3.f));
		}
	}
}

EHudTrackerMark UHudMissionTrackerWidget::ObjectiveMark(bool bDone, bool bOptional)
{
	if (bOptional)
	{
		return bDone ? EHudTrackerMark::OptionalDone : EHudTrackerMark::Optional;
	}
	return bDone ? EHudTrackerMark::ObjectiveDone : EHudTrackerMark::Objective;
}

EHudTrackerMark UHudMissionTrackerWidget::StepMark(int32 Index, int32 DoneCount, int32 Current)
{
	if (Index < DoneCount)
	{
		return EHudTrackerMark::StepDone;
	}
	return Index == Current ? EHudTrackerMark::StepNow : EHudTrackerMark::StepToCome;
}

EHudTrackerMark UHudMissionTrackerWidget::GetShownObjectiveMark() const
{
	return ObjectiveMark(Shown.bDone, false);
}

TArray<EHudTrackerMark> UHudMissionTrackerWidget::GetShownStepMarks() const
{
	// Only the circles on show: a one-step mission has none.
	TArray<EHudTrackerMark> Marks;
	if (StepRow && StepRow->GetVisibility() != ESlateVisibility::Collapsed)
	{
		for (int32 Index = 0; Index < FMath::Min(Shown.StepCount, StepMarks.Num()); ++Index)
		{
			Marks.Add(StepMarks[Index]);
		}
	}
	return Marks;
}

TArray<EHudTrackerMark> UHudMissionTrackerWidget::GetShownOptionalMarks() const
{
	TArray<EHudTrackerMark> Marks;
	for (const FMissionTrackerOptional& Optional : Shown.Optionals)
	{
		Marks.Add(ObjectiveMark(Optional.bDone, true));
	}
	return Marks;
}

void UHudMissionTrackerWidget::PaintTitle(const FText& Title)
{
	if (TitleText)
	{
		TitleText->SetText(Title.ToUpper());
	}
}

void UHudMissionTrackerWidget::PaintSteps(int32 DoneCount, int32 Current, int32 StepCount, int32 Number, bool bPopChecks)
{
	if (!StepRow || !StepText)
	{
		return;
	}
	// A mission of one step has no row.
	const bool bRow = StepCount > 1;
	StepRow->SetVisibility(bRow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bRow)
	{
		return;
	}
	EnsureSteps(StepCount);
	for (int32 Index = 0; Index < StepImages.Num(); ++Index)
	{
		UImage* Circle = StepImages[Index];
		if (Index >= StepCount)
		{
			Circle->SetVisibility(ESlateVisibility::Collapsed);
			StepPopAges[Index] = CheckPopSeconds;
			continue;
		}
		Circle->SetVisibility(ESlateVisibility::HitTestInvisible);
		const EHudTrackerMark Mark = StepMark(Index, DoneCount, Current);
		if (Mark != StepMarks[Index])
		{
			// A step just done is checked with a pop, as the objective's check pops; anything else (a mission shown anew
			// with steps done already) is just shown.
			const bool bChecked = bPopChecks && Mark == EHudTrackerMark::StepDone && StepMarks[Index] != EHudTrackerMark::StepDone;
			StepMarks[Index] = Mark;
			Circle->SetBrush(HudTrackerShapes::MarkBrush(Mark));
			StepPopAges[Index] = bChecked ? 0.f : CheckPopSeconds;
			ApplyCheckPop(Circle, bChecked ? 0.f : 1.f);
		}
	}
	StepText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::Clamp(Number, 1, StepCount), StepCount)));
}

void UHudMissionTrackerWidget::PaintObjective()
{
	if (!LineText || !CountText || !ObjectiveRing || !ObjectiveCheck)
	{
		return;
	}
	// Done: the ring turns into the check and the words dim; the count keeps its cyan.
	LineText->SetText(FText::FromString(Shown.Line));
	LineText->SetColorAndOpacity(FSlateColor(Shown.bDone ? Color::TextDim() : FLinearColor::White));
	ObjectiveRing->SetVisibility(Shown.bDone ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	ObjectiveCheck->SetVisibility(Shown.bDone ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!Shown.bDone)
	{
		// The next objective's ring comes with no check popping behind it.
		CheckPopAge = CheckPopSeconds;
		ApplyCheckPop(ObjectiveCheck.Get(), 1.f);
	}
	CountText->SetText(FText::FromString(Shown.Count));
	CountText->SetVisibility(Shown.Count.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	// A line without a count takes the count's room, so a plain line stays on one row more often.
	LineText->SetWrapTextAt(Shown.Count.IsEmpty() ? LineWrapAlone : LineWrapCounted);
}

void UHudMissionTrackerWidget::PaintHint()
{
	if (!Hint || !KeyText || !HintText)
	{
		return;
	}
	// Only for objectives that teach a key.
	const bool bHint = !Shown.HintKey.IsEmpty();
	Hint->SetVisibility(bHint ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bHint)
	{
		KeyText->SetText(FText::FromString(Shown.HintKey));
		HintText->SetText(FText::FromString(Shown.HintText));
	}
}

void UHudMissionTrackerWidget::StartCheckPop()
{
	CheckPopAge = 0.f;
	ApplyCheckPop(ObjectiveCheck.Get(), 0.f);
}

void UHudMissionTrackerWidget::AnimateChecks(float DeltaTime)
{
	if (CheckPopAge < CheckPopSeconds)
	{
		CheckPopAge = FMath::Min(CheckPopAge + DeltaTime, CheckPopSeconds);
		ApplyCheckPop(ObjectiveCheck.Get(), CheckPopAge / CheckPopSeconds);
	}
	for (int32 Index = 0; Index < FMath::Min(StepPopAges.Num(), StepImages.Num()); ++Index)
	{
		float& Age = StepPopAges[Index];
		if (Age < CheckPopSeconds)
		{
			Age = FMath::Min(Age + DeltaTime, CheckPopSeconds);
			ApplyCheckPop(StepImages[Index].Get(), Age / CheckPopSeconds);
		}
	}
}
