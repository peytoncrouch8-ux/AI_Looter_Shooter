// UPlayerHUDWidget: the silver readout at the top of the weapon column (Docs/Progression.md, "Economy"): the Inked silver
// coin and the amount in floating type with no panel, counting up on a find while a "+N" pops in left of it and fades.

#include "UI/HUD/PlayerHUDWidget.h"
#include "Economy/EconomyRules.h"
#include "Economy/WalletSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Vendor/SilverUI.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/** The amount's type and the "+N"'s (the grenade count's 20 is the column's loudest; silver sits under it). */
	constexpr int32 MoneySize = 16;
	constexpr int32 GainSize = 15;

	/** The "+N": popping in, held, faded out (s), and how far it slides in from the right (px). */
	constexpr float GainIn = 0.15f;
	constexpr float GainHold = 1.6f;
	constexpr float GainOut = 0.6f;
	constexpr float GainSlide = 12.f;

	/** How fast the amount counts to a find, and how long the amount glows cyan after one (s). */
	constexpr float CountSpeed = 8.f;
	constexpr float GlowSeconds = 0.6f;

	/** A find brings the weapon column forward from its idle fade for this long (its activity hold). */
	constexpr float FindActivity = 3.f;
}

UWidget* UPlayerHUDWidget::BuildMoneyReadout()
{
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	MoneyGainText = MakeFloatingText(WidgetTree, GainSize, Color::CyanText(), 40, ETextJustify::Right);
	MoneyGainText->SetVisibility(ESlateVisibility::Hidden);
	UHorizontalBoxSlot* GainSlot = Line->AddChildToHorizontalBox(MoneyGainText);
	GainSlot->SetVerticalAlignment(VAlign_Center);
	GainSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	UTextBlock* Amount = nullptr;
	Line->AddChildToHorizontalBox(SilverUI::MakeAmount(WidgetTree, 0, MoneySize, Color::Text(), /*bFloating*/ true, &Amount))
		->SetVerticalAlignment(VAlign_Center);
	MoneyText = Amount;
	Line->SetVisibility(ESlateVisibility::Collapsed);
	MoneyReadout = Line;
	return Line;
}

void UPlayerHUDWidget::HandleSilverChanged(int32 Total, int32 Delta, ESilverChange Why)
{
	MoneyTarget = Total;
	const bool bFind = Delta > 0 && (Why == ESilverChange::Found || Why == ESilverChange::Reward || Why == ESilverChange::Sold);
	if (!bFind)
	{
		// Spent at a counter (the screen showed it) or put back by a load: no fanfare, the amount just is.
		ShownMoney = static_cast<float>(Total);
		return;
	}
	// Finds close together add up into one "+N".
	const bool bShowing = MoneyGainAge >= 0.f && MoneyGainAge < GainIn + GainHold;
	MoneyGain = bShowing ? MoneyGain + Delta : Delta;
	MoneyGainAge = bShowing ? FMath::Min(MoneyGainAge, GainIn) : 0.f;
	if (MoneyGainText)
	{
		MoneyGainText->SetText(FText::FromString(TEXT("+") + LooterEconomy::Format(MoneyGain)));
	}
	WeaponActivity = FMath::Max(WeaponActivity, FindActivity);
}

void UPlayerHUDWidget::UpdateMoney(float DeltaTime)
{
	if (!MoneyReadout || !MoneyText)
	{
		return;
	}

	// The level's wallet: a new one with each level, so follow whichever is here now.
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(GetOwningPlayer());
	if (BoundWallet.Get() != Wallet)
	{
		if (UWalletSubsystem* Old = BoundWallet.Get())
		{
			Old->OnSilverChanged.Remove(SilverChangedHandle);
		}
		SilverChangedHandle = Wallet ? Wallet->OnSilverChanged.AddUObject(this, &UPlayerHUDWidget::HandleSilverChanged) : FDelegateHandle();
		BoundWallet = Wallet;
		MoneyTarget = Wallet ? Wallet->GetSilver() : 0;
		ShownMoney = static_cast<float>(MoneyTarget);
		MoneyText->SetText(FText::FromString(LooterEconomy::Format(MoneyTarget)));
	}

	// Shown once there's been any silver at all.
	bMoneySeen |= MoneyTarget > 0 || MoneyGainAge >= 0.f;
	const ESlateVisibility Wanted = bMoneySeen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (MoneyReadout->GetVisibility() != Wanted)
	{
		MoneyReadout->SetVisibility(Wanted);
	}
	if (!bMoneySeen)
	{
		return;
	}

	// The amount counts to its target, glowing cyan through a find's first moment.
	const int32 Before = FMath::RoundToInt32(ShownMoney);
	ShownMoney = FMath::FInterpTo(ShownMoney, static_cast<float>(MoneyTarget), DeltaTime, CountSpeed);
	if (FMath::Abs(ShownMoney - static_cast<float>(MoneyTarget)) < 0.5f)
	{
		ShownMoney = static_cast<float>(MoneyTarget);
	}
	const int32 Shown = FMath::RoundToInt32(ShownMoney);
	if (Shown != Before)
	{
		MoneyText->SetText(FText::FromString(LooterEconomy::Format(Shown)));
	}
	const float Glow = MoneyGainAge >= 0.f ? FMath::Clamp(1.f - MoneyGainAge / GlowSeconds, 0.f, 1.f) : 0.f;
	MoneyText->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Text(), Color::CyanText(), Glow)));

	// The "+N": in with a pop from the right, held, faded out.
	if (MoneyGainAge < 0.f || !MoneyGainText)
	{
		return;
	}
	MoneyGainAge += DeltaTime;
	if (MoneyGainAge >= GainIn + GainHold + GainOut)
	{
		MoneyGainAge = -1.f;
		MoneyGain = 0;
		MoneyGainText->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	const float In = FMath::Clamp(MoneyGainAge / GainIn, 0.f, 1.f);
	const float Out = FMath::Clamp((MoneyGainAge - GainIn - GainHold) / GainOut, 0.f, 1.f);
	const float EaseIn = 1.f - FMath::Square(1.f - In);
	MoneyGainText->SetVisibility(ESlateVisibility::HitTestInvisible);
	MoneyGainText->SetRenderOpacity(EaseIn * (1.f - Out));
	MoneyGainText->SetRenderScale(FVector2D(1.f + 0.3f * (1.f - EaseIn)));
	MoneyGainText->SetRenderTranslation(FVector2D(GainSlide * (1.f - EaseIn), 0.f));
}
