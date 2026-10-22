#include "UI/World/DamageNumberWidget.h"
#include "UI/World/DamageNumberActor.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> UDamageNumberWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		DamageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamageText"));
		TagText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TagText"));
		if (UHorizontalBoxSlot* NumberSlot = Row->AddChildToHorizontalBox(DamageText))
		{
			NumberSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* TagSlot = Row->AddChildToHorizontalBox(TagText))
		{
			// Beside the number, a little apart, level with its middle; the pop grows from its near edge.
			TagSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
			TagSlot->SetVerticalAlignment(VAlign_Center);
		}
		TagText->SetVisibility(ESlateVisibility::Collapsed);
		TagText->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
		WidgetTree->RootWidget = Row;
		ApplyDamageText();
	}
	return Super::RebuildWidget();
}

int32 UDamageNumberWidget::NumberSize(bool bCritical, const FElementNumberLook& Look)
{
	return FMath::RoundToInt((bCritical ? CriticalSize : NormalSize) * FMath::Max(Look.SizeScale, 0.1f));
}

void UDamageNumberWidget::SetDamage(float Damage, bool bCritical, const FElementNumberLook& Look)
{
	DisplayedDamage = Damage;
	bDisplayedCritical = bCritical;
	DisplayedLook = Look;
	DisplayedHeat = bCritical ? 1.f : 0.f;
	DisplayedTagScale = DamageNumberMotion::TagScaleAt(0.f);
	ApplyDamageText();
}

void UDamageNumberWidget::SetHeat(float Heat)
{
	const float Clamped = FMath::Clamp(Heat, 0.f, 1.f);
	if (!DamageText || !bDisplayedCritical || FMath::IsNearlyEqual(Clamped, DisplayedHeat, 0.01f))
	{
		return;
	}
	DisplayedHeat = Clamped;
	DamageText->SetColorAndOpacity(FSlateColor(ElementCallout::NumberColor(DisplayedLook, true, Clamped)));
}

void UDamageNumberWidget::SetTagScale(float Scale)
{
	DisplayedTagScale = Scale;
	if (TagText && DisplayedLook.Callout != EElementAffinity::Neutral)
	{
		FWidgetTransform Transform;
		Transform.Scale = FVector2D(Scale);
		TagText->SetRenderTransform(Transform);
	}
}

void UDamageNumberWidget::ApplyDamageText()
{
	if (!DamageText)
	{
		return;
	}

	const FText Number = FText::AsNumber(FMath::Max(1, FMath::RoundToInt(DisplayedDamage)));
	const int32 Size = NumberSize(bDisplayedCritical, DisplayedLook);
	if (bDisplayedCritical)
	{
		// Headshots: much bigger, slamming in white-hot and cooling to accent orange (tinted toward the element's colour if it
		// carried one), with a "!" so they read instantly in a fight.
		DamageText->SetText(FText::Format(NSLOCTEXT("Looter", "CritDamage", "{0}!"), Number));
		LooterUI::StyleFloatingText(DamageText, Size, ElementCallout::NumberColor(DisplayedLook, true, FMath::Max(DisplayedHeat, 0.f)));
	}
	else
	{
		DamageText->SetText(Number);
		LooterUI::StyleFloatingText(DamageText, Size, ElementCallout::NumberColor(DisplayedLook, false, 0.f));
	}

	if (TagText)
	{
		if (DisplayedLook.Callout == EElementAffinity::Neutral)
		{
			TagText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			TagText->SetText(ElementCallout::TagText(DisplayedLook.Callout));
			LooterUI::StyleFloatingText(TagText, TagSize, ElementCallout::TagColor(DisplayedLook.Element, DisplayedLook.Callout), 2);
			TagText->SetVisibility(ESlateVisibility::HitTestInvisible);
			SetTagScale(DisplayedTagScale);
		}
	}
}
