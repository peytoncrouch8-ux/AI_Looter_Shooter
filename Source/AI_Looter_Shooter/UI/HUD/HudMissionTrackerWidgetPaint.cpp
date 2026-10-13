// UHudMissionTrackerWidget: painting what it shows (only when it changes): the title, the step bar and its number, the
// objective row and the key hint. The optional objectives under them are HudMissionTrackerWidgetOptional.cpp's.

#include "UI/HUD/HudMissionTrackerWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

using namespace LooterUI;

namespace
{
	/** A step bar section's two flat halves, light over dark. */
	struct FTrackerSectionColors
	{
		FLinearColor Upper;
		FLinearColor Lower;
	};

	/** A step done: the experience bar's cyan; the current one: orange; one still to come: a faint cyan on the dark track. */
	FTrackerSectionColors SectionColorsFor(int32 Index, int32 DoneCount, int32 Current)
	{
		if (Index < DoneCount)
		{
			return { Color::XPLight(), Color::XPDark() };
		}
		if (Index == Current)
		{
			return { Color::AccentLight(), Color::Accent() };
		}
		const FLinearColor Faint = Color::Hairline().CopyWithNewOpacity(0.17f);
		return { Faint, Faint };
	}
}

void UHudMissionTrackerWidget::PaintTitle(const FText& Title)
{
	if (TitleText)
	{
		TitleText->SetText(Title.ToUpper());
	}
}

void UHudMissionTrackerWidget::PaintSteps(int32 DoneCount, int32 Current, int32 StepCount, int32 Number)
{
	if (!StepBar || !StepText)
	{
		return;
	}
	// A mission of one step has no bar.
	const bool bBar = StepCount > 1;
	const ESlateVisibility BarVisibility = bBar ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	StepBar->SetVisibility(BarVisibility);
	StepText->SetVisibility(BarVisibility);
	if (!bBar)
	{
		return;
	}
	EnsureSections(StepCount);
	for (int32 Index = 0; Index < Sections.Num(); ++Index)
	{
		if (Index >= StepCount)
		{
			Sections[Index]->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		Sections[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
		const FTrackerSectionColors Colors = SectionColorsFor(Index, DoneCount, Current);
		SectionUppers[Index]->SetColorAndOpacity(Colors.Upper);
		SectionLowers[Index]->SetColorAndOpacity(Colors.Lower);
	}
	StepText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::Clamp(Number, 1, StepCount), StepCount)));
}

void UHudMissionTrackerWidget::PaintObjective()
{
	if (!LineText || !CountText || !Chevron || !TickMark)
	{
		return;
	}
	// Done: the chevron turns into the tick and the words dim; the count keeps its cyan.
	LineText->SetText(FText::FromString(Shown.Line));
	LineText->SetColorAndOpacity(FSlateColor(Shown.bDone ? Color::TextDim() : FLinearColor::White));
	Chevron->SetVisibility(Shown.bDone ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	TickMark->SetVisibility(Shown.bDone ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	CountText->SetText(FText::FromString(Shown.Count));
	CountText->SetVisibility(Shown.Count.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
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
