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
	ApplyDamageText();
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
		// Headshots: bigger, accent orange, with a "!" so they read instantly in a fight.
		DamageText->SetText(FText::Format(NSLOCTEXT("Looter", "CritDamage", "{0}!"), Number));
		LooterUI::StyleText(DamageText, 32, LooterUI::Color::Accent());
	}
	else
	{
		DamageText->SetText(Number);
		LooterUI::StyleText(DamageText, 22, LooterUI::Color::Text());
	}
}
