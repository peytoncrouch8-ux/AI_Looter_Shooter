#include "UI/World/CreatureHealthBarWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;

namespace
{
	constexpr int32 SegmentCount = 12;
}

TSharedRef<SWidget> UCreatureHealthBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		// "LV 1  BROWN SPIDER": the level in the accent color, so it reads at a glance.
		UHorizontalBox* Label = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		LevelText = MakeText(WidgetTree, TEXT(""), 11, Color::Accent(), true, 120);
		Label->AddChildToHorizontalBox(LevelText)->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		NameText = MakeText(WidgetTree, TEXT(""), 11, Color::Text(), true, 120);
		Label->AddChildToHorizontalBox(NameText);
		Box->AddChildToVerticalBox(Label)->SetHorizontalAlignment(HAlign_Center);
		ApplyLabel();

		USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		BarSize->SetWidthOverride(150.f);
		BarSize->SetHeightOverride(7.f);
		UHorizontalBox* Bar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Segments.Reset();
		for (int32 Index = 0; Index < SegmentCount; ++Index)
		{
			UImage* Segment = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Segment->SetBrush(RectBrush(Color::Health()));
			UHorizontalBoxSlot* SegmentSlot = Bar->AddChildToHorizontalBox(Segment);
			SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < SegmentCount ? 2.f : 0.f, 0.f));
			Segments.Add(Segment);
		}
		BarSize->SetContent(Bar);
		Box->AddChildToVerticalBox(BarSize)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

		WidgetTree->RootWidget = MakePlate(WidgetTree, Box, FMargin(10.f, 5.f, 10.f, 7.f));
		ShownLit = INDEX_NONE;
		ApplyHealth();
	}
	return Super::RebuildWidget();
}

void UCreatureHealthBarWidget::SetCreature(const FText& InName, int32 InLevel)
{
	if (!CreatureName.EqualTo(InName) || CreatureLevel != InLevel)
	{
		CreatureName = InName;
		CreatureLevel = InLevel;
		ApplyLabel();
	}
}

void UCreatureHealthBarWidget::ApplyLabel()
{
	if (LevelText && NameText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("LV %d"), CreatureLevel)));
		NameText->SetText(FText::FromString(CreatureName.ToString().ToUpper()));
	}
}

void UCreatureHealthBarWidget::SetHealthFraction(float InFraction)
{
	Fraction = FMath::Clamp(InFraction, 0.f, 1.f);
	ApplyHealth();
}

void UCreatureHealthBarWidget::ApplyHealth()
{
	// Any damage at all takes a segment, so a single chip shot still reads as progress.
	const int32 Lit = FMath::Clamp(FMath::CeilToInt(Fraction * SegmentCount), 0, SegmentCount);
	if (Lit == ShownLit || Segments.Num() != SegmentCount)
	{
		return;
	}
	ShownLit = Lit;
	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		Segments[Index]->SetBrush(RectBrush(Index < Lit ? Color::Health() : Color::SegmentOff()));
	}
}
