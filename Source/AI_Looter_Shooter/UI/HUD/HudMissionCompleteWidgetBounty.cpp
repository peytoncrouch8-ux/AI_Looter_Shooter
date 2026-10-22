// UHudMissionCompleteWidget's bounty variant: a bounty paid at the board shows as "BOUNTY PAID" with the mark's name, the
// purse line (PURSE and the silver) and the experience, both counting up from nothing. The show's timing and the rest of the
// words are HudMissionCompleteWidget.cpp's.

#include "UI/HUD/HudMissionCompleteWidget.h"
#include "Missions/MissionDefinition.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/** The numbers count up from this long into the show (the rewards row is in by 0.9) to that, easing out so they land softly. */
	constexpr float CountFrom = 0.6f;
	constexpr float CountTo = 1.7f;

	/** Where a bounty's title starts ("Bounty: Old Stilts"): the bounty board's (BountyRules::TitleFor). */
	const TCHAR* const BountyPrefix = TEXT("Bounty:");
	const TCHAR* const BountyIdPrefix = TEXT("Bounty");

	constexpr int32 PurseSize = 14;
	constexpr int32 PurseSpacing = 220;
	constexpr float PurseGap = 12.f;

	FString Plus(int64 Amount, const TCHAR* Unit)
	{
		return FString::Printf(TEXT("+%s %s"), *FText::AsNumber(Amount).ToString(), Unit);
	}
}

bool UHudMissionCompleteWidget::IsBountyMission(const UMissionDefinition& Mission)
{
	// The board names its ending Bounty_<serial>; a made-up mission that is titled as a bounty counts as well (it is never a
	// story asset: those are never transient).
	if (Mission.Id.ToString().StartsWith(BountyIdPrefix))
	{
		return true;
	}
	return Mission.HasAnyFlags(RF_Transient) && Mission.Title.ToString().StartsWith(BountyPrefix);
}

FText UHudMissionCompleteWidget::BountyMarkName(const UMissionDefinition& Mission)
{
	FString Title = Mission.Title.ToString();
	if (Title.StartsWith(BountyPrefix))
	{
		Title.RightChopInline(FCString::Strlen(BountyPrefix));
		Title.TrimStartInline();
	}
	return Title.IsEmpty() ? Mission.Title : FText::FromString(Title);
}

void UHudMissionCompleteWidget::ApplyBountyRewards()
{
	const FMissionRewardsGiven& Given = Shown.Given;
	if (Given.Silver > 0)
	{
		// The purse line: PURSE, dim, then the silver in the readout's white.
		UTextBlock* Label = MakeFloatingText(WidgetTree, PurseSize, Color::TextDim(), PurseSpacing, ETextJustify::Center);
		Label->SetText(NSLOCTEXT("LooterHUD", "BountyPurse", "PURSE"));
		UHorizontalBoxSlot* LabelSlot = RewardsRow->AddChildToHorizontalBox(Label);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetPadding(FMargin(0.f, 0.f, PurseGap, 0.f));
		SilverTarget = Given.Silver;
		SilverCountText = AddReward(Plus(0, TEXT("SILVER")), Color::Text());
		// The words kept for the tests are the whole sum.
		RewardWords.Last() = Plus(Given.Silver, TEXT("SILVER"));
	}
	if (Given.Experience > 0)
	{
		XPTarget = Given.Experience;
		XPCountText = AddReward(Plus(0, TEXT("XP")), Color::CyanText());
		RewardWords.Last() = Plus(Given.Experience, TEXT("XP"));
	}
}

void UHudMissionCompleteWidget::PaintCounts()
{
	if (!Shown.bBounty || Time < 0.f)
	{
		return;
	}
	const float Alpha = FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp((Time - CountFrom) / (CountTo - CountFrom), 0.f, 1.f), 2.f);
	if (SilverCountText)
	{
		const int64 Now = FMath::RoundToInt64(static_cast<double>(SilverTarget) * Alpha);
		if (Now != SilverCounted)
		{
			SilverCounted = Now;
			SilverCountText->SetText(FText::FromString(Plus(Now, TEXT("SILVER"))));
		}
	}
	if (XPCountText)
	{
		const int64 Now = FMath::RoundToInt64(static_cast<double>(XPTarget) * Alpha);
		if (Now != XPCounted)
		{
			XPCounted = Now;
			XPCountText->SetText(FText::FromString(Plus(Now, TEXT("XP"))));
		}
	}
}

FString UHudMissionCompleteWidget::GetShownTitle() const
{
	return TitleText ? TitleText->GetText().ToString() : FString();
}

FString UHudMissionCompleteWidget::GetCountedSilver() const
{
	return SilverCountText ? SilverCountText->GetText().ToString() : FString();
}

FString UHudMissionCompleteWidget::GetCountedExperience() const
{
	return XPCountText ? XPCountText->GetText().ToString() : FString();
}
