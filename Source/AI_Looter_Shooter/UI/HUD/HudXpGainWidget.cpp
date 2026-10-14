#include "UI/HUD/HudXpGainWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/World/XpPopupWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/** Bigger than the bar's own numbers (13 px), so it reads as news; letter-spaced a little, as the kill's "+XP". */
	constexpr int32 GainSize = 16;
	constexpr int32 GainSpacing = 40;
	/** It pops in from this big, overshooting a touch as it settles. */
	constexpr float GainPopFrom = 1.6f;
	/** It's solid by this share of its pop. */
	constexpr float GainFadeInShare = 0.35f;
	/** It rises this far (pixels) as it fades. */
	constexpr float GainFadeRise = 8.f;

	/** Out fast, overshooting a touch, settling (a pop's ease). */
	float GainEaseOutBack(float Share)
	{
		const float T = FMath::Clamp(Share, 0.f, 1.f) - 1.f;
		constexpr float Overshoot = 1.4f;
		return 1.f + T * T * ((Overshoot + 1.f) * T + Overshoot);
	}
}

TSharedRef<SWidget> UHudXpGainWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		AmountText = MakeFloatingText(WidgetTree, GainSize, Color::CyanText(), GainSpacing);
		// It pops from its left end, growing away from the numbers before it rather than over them.
		AmountText->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
		WidgetTree->RootWidget = AmountText;
		Apply();
	}
	return Super::RebuildWidget();
}

float UHudXpGainWidget::OpacityAt(float Age)
{
	if (Age < 0.f || Age >= HoldSeconds + FadeSeconds)
	{
		return 0.f;
	}
	if (Age < PopSeconds * GainFadeInShare)
	{
		return Age / (PopSeconds * GainFadeInShare);
	}
	if (Age <= HoldSeconds)
	{
		return 1.f;
	}
	const float Share = (Age - HoldSeconds) / FadeSeconds;
	return 1.f - Share * Share;
}

float UHudXpGainWidget::ScaleAt(float Age)
{
	if (Age < 0.f || Age >= PopSeconds)
	{
		return 1.f;
	}
	return FMath::Lerp(GainPopFrom, 1.f, GainEaseOutBack(Age / PopSeconds));
}

float UHudXpGainWidget::RiseAt(float Age)
{
	if (Age <= HoldSeconds)
	{
		return 0.f;
	}
	const float Share = FMath::Clamp((Age - HoldSeconds) / FadeSeconds, 0.f, 1.f);
	return GainFadeRise * (1.f - (1.f - Share) * (1.f - Share));
}

void UHudXpGainWidget::AddGain(int64 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	// Kills close together read as one number: still on screen, it adds up, stays solid (no blink as it starts over) and
	// pops again; gone, it fades in fresh.
	const bool bShowing = IsShowing();
	Total = bShowing ? Total + Amount : Amount;
	ShowAge = bShowing ? PopSeconds * GainFadeInShare : 0.f;
	PopAge = 0.f;
	Apply();
}

void UHudXpGainWidget::Advance(float DeltaSeconds, bool bBarFilling)
{
	if (!IsShowing())
	{
		return;
	}
	// While the bar is still filling up to the gain, the number waits at full strength beside it.
	const float Next = ShowAge + DeltaSeconds;
	ShowAge = bBarFilling && Next > HoldSeconds ? FMath::Max(ShowAge, HoldSeconds) : Next;
	PopAge = FMath::Min(PopAge + DeltaSeconds, PopSeconds);
	Apply();
}

void UHudXpGainWidget::Apply()
{
	if (!AmountText)
	{
		return;
	}
	const float Opacity = OpacityAt(ShowAge);
	// Hidden, not collapsed: its place in the line stays, so the numbers before it never move.
	const ESlateVisibility Wanted = Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	if (AmountText->GetVisibility() != Wanted)
	{
		AmountText->SetVisibility(Wanted);
	}
	if (Opacity <= 0.f)
	{
		return;
	}
	AmountText->SetText(FText::FromString(UXpPopupWidget::Words(Total)));
	AmountText->SetRenderOpacity(Opacity);
	AmountText->SetRenderScale(FVector2D(ScaleAt(PopAge)));
	AmountText->SetRenderTranslation(FVector2D(0.f, -RiseAt(ShowAge)));
	// White as it pops, settling into the experience's cyan.
	const float Settled = FMath::Clamp(PopAge / PopSeconds, 0.f, 1.f);
	AmountText->SetColorAndOpacity(FSlateColor(FMath::Lerp(FLinearColor::White, Color::CyanText(), Settled)));
}

FString UHudXpGainWidget::GetShownText() const
{
	return IsShowing() ? UXpPopupWidget::Words(Total) : FString();
}
