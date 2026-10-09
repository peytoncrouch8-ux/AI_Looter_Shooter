#include "UI/World/DamageNumberWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> UDamageNumberWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		DamageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamageText"));
		WidgetTree->RootWidget = DamageText;
		ApplyDamageText();
	}
	return Super::RebuildWidget();
}

void UDamageNumberWidget::SetDamage(float Damage, bool bCritical)
{
	DisplayedDamage = Damage;
	bDisplayedCritical = bCritical;
	DisplayedHeat = bCritical ? 1.f : 0.f;
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
	DamageText->SetColorAndOpacity(FSlateColor(FMath::Lerp(LooterUI::Color::Accent(), FLinearColor::White, Clamped)));
}

void UDamageNumberWidget::ApplyDamageText()
{
	if (!DamageText)
	{
		return;
	}

	const FText Number = FText::AsNumber(FMath::Max(1, FMath::RoundToInt(DisplayedDamage)));
	if (bDisplayedCritical)
	{
		// Headshots: much bigger, slamming in white-hot and cooling to accent orange, with a "!" so they read instantly
		// in a fight.
		DamageText->SetText(FText::Format(NSLOCTEXT("Looter", "CritDamage", "{0}!"), Number));
		LooterUI::StyleFloatingText(DamageText, CriticalSize, FMath::Lerp(LooterUI::Color::Accent(), FLinearColor::White, FMath::Max(DisplayedHeat, 0.f)));
	}
	else
	{
		DamageText->SetText(Number);
		LooterUI::StyleFloatingText(DamageText, NormalSize, LooterUI::Color::Text());
	}
}
