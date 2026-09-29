#include "UI/CreatureHealthBarWidget.h"
#include "UI/LooterUIStyle.h"
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

		NameText = MakeText(WidgetTree, CreatureName.ToString(), 11, Color::Text(), true, 120);
		NameText->SetJustification(ETextJustify::Center);
		Box->AddChildToVerticalBox(NameText)->SetHorizontalAlignment(HAlign_Center);

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

void UCreatureHealthBarWidget::SetCreatureName(const FText& InName)
{
	if (!CreatureName.EqualTo(InName))
	{
		CreatureName = InName;
		if (NameText)
		{
			NameText->SetText(FText::FromString(CreatureName.ToString().ToUpper()));
		}
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
