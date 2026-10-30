// UCreatureHealthBarWidget: the element statuses on a creature, as a row of small icons under its health bar (Elements
// matter, 2026-10-29; fed each frame by UCreatureTierLookComponent::DressTag from UElementStatusComponent::GetStatusIcons).
// Each icon is the element's glyph (GunCardIcons::ElementBrush, the gun card's own) on a dark disc inside the kit's progress
// ring in the element's colour, draining clockwise as the status runs down, with its stack count beside it past one; it
// pops in as the status starts, so a new burn reads at a glance. The bar itself is built in CreatureHealthBarWidget.cpp.

#include "UI/World/CreatureHealthBarWidget.h"
#include "Elements/ElementRules.h"
#include "UI/Cards/GunCardIcons.h"
#include "UI/Style/LooterProgressRingWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

using namespace LooterUI;

namespace
{
	/** How dark the disc behind a glyph is: the HUD's faint backings' (the hold key cap's is the same). */
	constexpr float DiscAlpha = 0.6f;
	/** The disc sits just inside the ring's line (the ring keeps 3.5 px free outside it). */
	constexpr float DiscInset = 3.5f;
	/** The ring's track: a faint of the element's own colour, the time already spent. */
	constexpr float TrackAlpha = 0.25f;

	/** Where an element sits in the row: the enum's own order (Ember first, Rot last). */
	int32 OrderOf(ELooterElement Element)
	{
		return static_cast<int32>(Element);
	}
}

void UCreatureHealthBarWidget::ArrangeStatuses(TConstArrayView<FCreatureStatusIcon> In, TArray<FCreatureStatusIcon>& Out)
{
	Out.Reset();
	for (const FCreatureStatusIcon& Icon : In)
	{
		if (Icon.Element == ELooterElement::None || Icon.TimeShare <= 0.f)
		{
			continue;
		}
		// One icon an element: a second report of it (two burns counted apart) keeps the longest time and the most stacks.
		FCreatureStatusIcon* Same = Out.FindByPredicate([&Icon](const FCreatureStatusIcon& Kept) { return Kept.Element == Icon.Element; });
		if (Same)
		{
			Same->TimeShare = FMath::Max(Same->TimeShare, Icon.TimeShare);
			Same->Stacks = FMath::Max(Same->Stacks, Icon.Stacks);
			continue;
		}
		FCreatureStatusIcon& Added = Out.Add_GetRef(Icon);
		Added.TimeShare = FMath::Clamp(Icon.TimeShare, 0.f, 1.f);
		Added.Stacks = FMath::Max(Icon.Stacks, 1);
	}
	Out.Sort([](const FCreatureStatusIcon& A, const FCreatureStatusIcon& B) { return OrderOf(A.Element) < OrderOf(B.Element); });
	if (Out.Num() > MaxStatusIcons)
	{
		Out.SetNum(MaxStatusIcons);
	}
}

FText UCreatureHealthBarWidget::StacksLabel(int32 Stacks)
{
	return Stacks > 1 ? FText::AsNumber(Stacks) : FText::GetEmpty();
}

float UCreatureHealthBarWidget::StatusPopScaleAt(float Seconds)
{
	if (Seconds < 0.f || Seconds >= StatusPopSeconds)
	{
		return 1.f;
	}
	// Big at once and settling fast: a pop, not a swell (the rank word's ease).
	const float Left = 1.f - Seconds / StatusPopSeconds;
	return FMath::Lerp(1.f, StatusPopScale, Left * Left * Left);
}

float UCreatureHealthBarWidget::GetStatusScale(int32 Index) const
{
	return Index >= 0 && Index < Statuses.Num() ? StatusPopScaleAt(IconAges[Index]) : 1.f;
}

UWidget* UCreatureHealthBarWidget::BuildStatusRow()
{
	StatusRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	StatusBoxes.Reset();
	StatusRings.Reset();
	StatusGlyphs.Reset();
	StatusStacks.Reset();
	for (int32 Index = 0; Index < MaxStatusIcons; ++Index)
	{
		UOverlay* Icon = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		// A faint dark disc so the glyph reads over sky and grass; a background, so it fades with the transparency setting.
		FLinearColor DiscColor = Color::Backdrop();
		DiscColor.A *= DiscAlpha;
		UImage* Disc = MakeImage(WidgetTree, CircleBrush(DiscColor));
		MarkBackground(Disc);
		FillOverlaySlot(Icon->AddChildToOverlay(Disc), FMargin(DiscInset));

		ULooterProgressRing* Ring = WidgetTree->ConstructWidget<ULooterProgressRing>(ULooterProgressRing::StaticClass());
		Ring->Configure(StatusIconSize, StatusRingThickness);
		FillOverlaySlot(Icon->AddChildToOverlay(Ring));

		// The glyph's brush is set when its element is (ApplyStatuses).
		UImage* Glyph = MakeImage(WidgetTree, RectBrush(FLinearColor::Transparent));
		UOverlaySlot* GlyphSlot = Icon->AddChildToOverlay(MakeSized(WidgetTree, Glyph, StatusGlyphSize, StatusGlyphSize));
		GlyphSlot->SetHorizontalAlignment(HAlign_Center);
		GlyphSlot->SetVerticalAlignment(VAlign_Center);

		USizeBox* IconBox = MakeSized(WidgetTree, Icon, StatusIconSize, StatusIconSize);
		IconBox->SetRenderTransformPivot(FVector2D(0.5, 0.5));
		IconBox->SetVisibility(ESlateVisibility::Collapsed);

		// The stack count beside the icon's lower right, in the element's colour, outlined like the HUD's floating text.
		UHorizontalBox* Pair = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Pair->AddChildToHorizontalBox(IconBox)->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Stacks = MakeFloatingText(WidgetTree, StatusStackTextSize, Color::Text(), 0);
		Stacks->SetVisibility(ESlateVisibility::Collapsed);
		UHorizontalBoxSlot* StacksSlot = Pair->AddChildToHorizontalBox(Stacks);
		StacksSlot->SetVerticalAlignment(VAlign_Bottom);
		StacksSlot->SetPadding(FMargin(-3.f, 0.f, 0.f, 0.f));

		UHorizontalBoxSlot* PairSlot = StatusRow->AddChildToHorizontalBox(Pair);
		PairSlot->SetVerticalAlignment(VAlign_Center);
		PairSlot->SetPadding(FMargin(Index > 0 ? StatusIconGap : 0.f, 0.f, 0.f, 0.f));

		StatusBoxes.Add(IconBox);
		StatusRings.Add(Ring);
		StatusGlyphs.Add(Glyph);
		StatusStacks.Add(Stacks);
	}
	StatusRow->SetVisibility(ESlateVisibility::Collapsed);
	return StatusRow;
}

void UCreatureHealthBarWidget::SetStatuses(TConstArrayView<FCreatureStatusIcon> Icons)
{
	TArray<FCreatureStatusIcon> Arranged;
	ArrangeStatuses(Icons, Arranged);
	// What changed: the elements shown (an icon's glyph and colour, its pop) or only the rings and counts.
	bool bElementsChanged = Arranged.Num() != Statuses.Num();
	bool bAnyChanged = bElementsChanged;
	for (int32 Index = 0; Index < Arranged.Num() && !bElementsChanged; ++Index)
	{
		bElementsChanged = Arranged[Index].Element != Statuses[Index].Element;
		bAnyChanged = bAnyChanged || bElementsChanged || Arranged[Index].Stacks != Statuses[Index].Stacks
			|| !FMath::IsNearlyEqual(Arranged[Index].TimeShare, Statuses[Index].TimeShare, 0.002f);
	}
	if (!bAnyChanged)
	{
		return;
	}
	if (bElementsChanged)
	{
		// An element new to the row pops in; one that only moved along (another ran out before it) keeps its clock.
		float NewAges[MaxStatusIcons] = {};
		for (int32 Index = 0; Index < Arranged.Num(); ++Index)
		{
			NewAges[Index] = 0.f;
			for (int32 Old = 0; Old < Statuses.Num(); ++Old)
			{
				if (Statuses[Old].Element == Arranged[Index].Element)
				{
					NewAges[Index] = IconAges[Old];
					break;
				}
			}
		}
		for (int32 Index = 0; Index < MaxStatusIcons; ++Index)
		{
			IconAges[Index] = Index < Arranged.Num() ? NewAges[Index] : StatusPopSeconds;
		}
	}
	Statuses = MoveTemp(Arranged);
	ApplyStatuses(bElementsChanged);
}

void UCreatureHealthBarWidget::AdvanceStatuses(float DeltaSeconds)
{
	const float Step = FMath::Max(DeltaSeconds, 0.f);
	for (int32 Index = 0; Index < Statuses.Num(); ++Index)
	{
		if (IconAges[Index] < StatusPopSeconds)
		{
			IconAges[Index] = FMath::Min(IconAges[Index] + Step, StatusPopSeconds);
			ApplyStatusScale(Index);
		}
	}
}

void UCreatureHealthBarWidget::ApplyStatusScale(int32 Index)
{
	if (StatusBoxes.IsValidIndex(Index) && StatusBoxes[Index])
	{
		const float Scale = GetStatusScale(Index);
		StatusBoxes[Index]->SetRenderScale(FVector2D(Scale, Scale));
	}
}

void UCreatureHealthBarWidget::ApplyStatuses(bool bElementsChanged)
{
	if (!StatusRow)
	{
		return;
	}
	StatusRow->SetVisibility(Statuses.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	for (int32 Index = 0; Index < MaxStatusIcons && Index < StatusBoxes.Num(); ++Index)
	{
		USizeBox* IconBox = StatusBoxes[Index];
		ULooterProgressRing* Ring = StatusRings[Index];
		UImage* Glyph = StatusGlyphs[Index];
		UTextBlock* Stacks = StatusStacks[Index];
		if (!IconBox || !Ring || !Glyph || !Stacks)
		{
			continue;
		}
		if (!Statuses.IsValidIndex(Index))
		{
			IconBox->SetVisibility(ESlateVisibility::Collapsed);
			Stacks->SetVisibility(ESlateVisibility::Collapsed);
			IconElements[Index] = ELooterElement::None;
			continue;
		}
		const FCreatureStatusIcon& Icon = Statuses[Index];
		const FLinearColor Tint = ElementRules::UiColor(Icon.Element);
		if (bElementsChanged || IconElements[Index] != Icon.Element)
		{
			// The glyph and the colours only when the element in this place changes (the brush is shared, made once a size).
			IconElements[Index] = Icon.Element;
			FSlateBrush GlyphBrush;
			GunCardIcons::ElementBrush(Icon.Element, FVector2D(StatusGlyphSize, StatusGlyphSize), Tint, GlyphBrush);
			Glyph->SetBrush(GlyphBrush);
			FLinearColor Track = Tint;
			Track.A = TrackAlpha;
			Ring->SetColors(Tint, Track);
			Stacks->SetColorAndOpacity(FSlateColor(Tint));
			IconBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		// The ring drains as the status runs down (full as it starts).
		Ring->SetProgress(Icon.TimeShare);
		const FText Label = StacksLabel(Icon.Stacks);
		if (!Stacks->GetText().EqualTo(Label))
		{
			Stacks->SetText(Label);
		}
		Stacks->SetVisibility(Label.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		ApplyStatusScale(Index);
	}
}
