#include "UI/World/XpPopupWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

namespace
{
	/** Letter spacing (thousandths of an em): a little open small, wider big, as the banners' reward lines. */
	constexpr int32 XpPopupSmallSpacing = 40;
	constexpr int32 XpPopupBigSpacing = 120;
}

TSharedRef<SWidget> UXpPopupWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		AmountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmountText"));
		WidgetTree->RootWidget = AmountText;
		ApplyText();
	}
	return Super::RebuildWidget();
}

FString UXpPopupWidget::Words(int64 Amount)
{
	return FString::Printf(TEXT("+%s XP"), *FText::AsNumber(FMath::Max<int64>(Amount, 0)).ToString());
}

void UXpPopupWidget::SetAmount(int64 InAmount, bool bInBig, bool bInTier, const FLinearColor& TierColor)
{
	Amount = InAmount;
	bBig = bInBig;
	// A turn-in's is never a tier's: only a kill has a tier.
	bTier = bInTier && !bInBig;
	TextColor = bTier ? TierColor : LooterUI::Color::CyanText();
	ApplyText();
}

void UXpPopupWidget::ApplyText()
{
	if (!AmountText)
	{
		return;
	}
	AmountText->SetText(FText::FromString(Words(Amount)));
	// The experience's cyan, as the frame's "+XP" and the mission banner's reward: outlined floating text, no plate. A top
	// tier's kill in its own colour, a size up.
	const int32 Size = bBig ? BigSize : (bTier ? TierSize : SmallSize);
	const FLinearColor Shown = bTier ? TextColor : LooterUI::Color::CyanText();
	LooterUI::StyleFloatingText(AmountText, Size, Shown, bBig ? XpPopupBigSpacing : XpPopupSmallSpacing);
}
