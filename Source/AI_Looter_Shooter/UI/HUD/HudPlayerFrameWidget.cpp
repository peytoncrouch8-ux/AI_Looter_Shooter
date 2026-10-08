// UHudPlayerFrameWidget's health bar: what it shows and how it reacts (a hit's chip, a heal's rise and shine, the
// low-health beat). The tree is built in HudPlayerFrameWidgetLayout.cpp, the bars' pieces painted by
// HudPlayerFrameWidgetStretches.cpp, experience followed in HudPlayerFrameWidgetXP.cpp.

#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudPortraitWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

using namespace LooterUI;

namespace
{
	/** Health changes smaller than this (in points) are rounding, not a hit or a heal. */
	constexpr float HealthStep = 0.01f;
	/** A heal shows only where the bar rises by more than this share of it (health and maximum rising together may not move it). */
	constexpr float FractionStep = 0.002f;
	/** After a hit the lost part lingers this long as the chip, then drains over this long, slowly at first. */
	constexpr float ChipHoldSeconds = 0.45f;
	constexpr float ChipDrainSeconds = 0.6f;
	/** A heal's fill rises over this long; its shine sweeps along the fill over this long. */
	constexpr float HealRiseSeconds = 0.5f;
	constexpr float ShineSeconds = 0.75f;
	/** The "+30" floats this long. */
	constexpr float HealFloatSeconds = 1.2f;
	/** A float starts this far below its spot and rises to this far above it (the mockup's, scaled with the frame). */
	constexpr float FloatStartDrop = 6.8f;
	constexpr float FloatRise = 18.7f;
	/** The share of a float's life it takes to fade in. */
	constexpr float FloatFadeIn = 0.15f;
	/** At the low-health beat's height, the fill is brightened this much (a pale overlay). */
	constexpr float BeatBrightening = 0.3f;

	float HealthEaseOut(float Alpha)
	{
		return FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}
}

float UHudPlayerFrameWidget::LowBeat(const UWorld* World)
{
	// World time, so the frame and the screen's edges beat together (and hold still while the game is paused).
	const double Time = World ? World->GetTimeSeconds() : FPlatformTime::Seconds();
	const double Phase = FMath::Fmod(Time, static_cast<double>(LowBeatSeconds)) / LowBeatSeconds;
	return static_cast<float>(0.5 - 0.5 * FMath::Cos(2.0 * UE_DOUBLE_PI * Phase));
}

void UHudPlayerFrameWidget::PaintFloat(UTextBlock* Text, float Age, float Duration)
{
	if (!Text || Duration <= 0.f)
	{
		return;
	}
	// In quickly, then out over the rest of its life, rising all the way (slowing as it goes).
	const float Life = FMath::Clamp(Age / Duration, 0.f, 1.f);
	const float Opacity = Life < FloatFadeIn ? HealthEaseOut(Life / FloatFadeIn)
		: 1.f - HealthEaseOut((Life - FloatFadeIn) / (1.f - FloatFadeIn));
	Text->SetRenderOpacity(Opacity);
	Text->SetRenderTranslation(FVector2D(0.f, FMath::Lerp(FloatStartDrop, -FloatRise, HealthEaseOut(Life))));
}

void UHudPlayerFrameWidget::SetHealth(float Health, float MaxHealth, float DeltaTime)
{
	if (!Cluster || !HealthValue || !HealthMax || !HealFloat || StretchBodies.Num() != static_cast<int32>(EHudFrameStretch::Count))
	{
		return;
	}
	const float Points = FMath::Max(Health, 0.f);
	const float Fraction = MaxHealth > 0.f ? FMath::Clamp(Points / MaxHealth, 0.f, 1.f) : 0.f;

	if (LastHealth < 0.f || (LastHealth <= 0.f && Points > 0.f))
	{
		// First sight, or back from death (a respawn fills the bar): shown as it is, nothing to react to.
		SnapHealth(Fraction);
	}
	else if (Points < LastHealth - HealthStep)
	{
		// A hit: the bar drops at once, and what was showing lingers as the chip, then drains.
		ChipFrom = FMath::Max(ChipShown, FillShown);
		FillShown = Fraction;
		FillFrom = Fraction;
		FillTo = Fraction;
		RiseTime = HealRiseSeconds;
		ChipHold = ChipHoldSeconds;
		ChipDrain = 0.f;
		Activity = ActivityHoldSeconds;
		if (Portrait)
		{
			Portrait->PlayHit();
		}
	}
	else if (Points > LastHealth + HealthStep)
	{
		if (Fraction > LastFraction + FractionStep)
		{
			// A heal: the fill rises with a shine sweeping along it, "+30" rises at its end.
			FillFrom = FillShown;
			FillTo = Fraction;
			RiseTime = 0.f;
			const float Gained = Points - LastHealth;
			HealAmount = HealFloatTime > 0.f ? HealAmount + Gained : Gained;
			if (HealAmount >= 1.f)
			{
				HealFloat->SetText(FText::FromString(FString::Printf(TEXT("+%d"), FMath::RoundToInt32(HealAmount))));
				HealFloat->SetVisibility(ESlateVisibility::HitTestInvisible);
				HealFloatTime = HealFloatSeconds;
				PaintFloat(HealFloat, 0.f, HealFloatSeconds);
				// A shine still near the start keeps going, so a quick run of small heals reads as one sweep.
				if (ShineTime >= ShineSeconds * 0.5f)
				{
					ShineTime = 0.f;
				}
			}
			Activity = ActivityHoldSeconds;
		}
		else if (!FMath::IsNearlyEqual(Fraction, FillTo))
		{
			// Health and the maximum rose together (a level-up) and the bar didn't rise: no heal to show, it follows at once.
			// At full health, full stays full, with no "+8" or shine a second before the level-up banner.
			FillShown = Fraction;
			FillFrom = Fraction;
			FillTo = Fraction;
			RiseTime = HealRiseSeconds;
		}
	}
	else if (!FMath::IsNearlyEqual(Fraction, FillTo))
	{
		// The maximum moved and the health didn't: follow at once.
		FillShown = Fraction;
		FillFrom = Fraction;
		FillTo = Fraction;
		RiseTime = HealRiseSeconds;
	}
	LastHealth = Points;
	LastFraction = Fraction;

	if (RiseTime < HealRiseSeconds)
	{
		RiseTime = FMath::Min(RiseTime + DeltaTime, HealRiseSeconds);
		FillShown = FMath::Lerp(FillFrom, FillTo, HealthEaseOut(RiseTime / HealRiseSeconds));
	}
	if (ChipHold > 0.f)
	{
		ChipHold -= DeltaTime;
		ChipShown = ChipFrom;
	}
	else if (ChipShown > FillShown)
	{
		ChipDrain += DeltaTime;
		ChipShown = FMath::Lerp(ChipFrom, FillShown, FMath::InterpEaseIn(0.f, 1.f, FMath::Min(ChipDrain / ChipDrainSeconds, 1.f), 2.f));
	}
	ChipShown = FMath::Max(ChipShown, FillShown);

	PaintStretch(EHudFrameStretch::HealthChip, ChipShown);
	PaintStretch(EHudFrameStretch::HealthFill, FillShown);

	// Dead (health 0, until the respawn) isn't low health: the bar is simply empty, as the screen's edges stop beating too.
	const bool bLow = Points > 0.f && Fraction <= LowFraction;
	ShowLow(bLow);
	if (bLow)
	{
		// Low: a pale overlay over the fill brightens it on the beat (the portrait's window and the screen's edges pulse).
		PaintStretch(EHudFrameStretch::HealthBeat, FillShown);
		TintStretch(EHudFrameStretch::HealthBeat, Color::HealthEdge().CopyWithNewOpacity(BeatBrightening * LowBeat(GetWorld())));
		bBeatShown = true;
	}
	else if (bBeatShown)
	{
		PaintStretch(EHudFrameStretch::HealthBeat, 0.f);
		bBeatShown = false;
	}

	if (ShineTime < ShineSeconds)
	{
		ShineTime += DeltaTime;
		PaintShine(ShineTime / ShineSeconds, FillShown);
	}
	if (HealFloatTime > 0.f)
	{
		HealFloatTime -= DeltaTime;
		if (HealFloatTime <= 0.f)
		{
			HealFloat->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			PaintFloat(HealFloat, HealFloatSeconds - HealFloatTime, HealFloatSeconds);
		}
	}

	UpdateHealthNumbers(Points, MaxHealth);
}

void UHudPlayerFrameWidget::SnapHealth(float Fraction)
{
	FillShown = Fraction;
	FillFrom = Fraction;
	FillTo = Fraction;
	RiseTime = HealRiseSeconds;
	ChipShown = Fraction;
	ChipFrom = Fraction;
	ChipHold = 0.f;
	ChipDrain = ChipDrainSeconds;
	ShineTime = ShineSeconds;
	PaintShine(1.f, Fraction);
	HealFloatTime = 0.f;
	HealFloat->SetVisibility(ESlateVisibility::Hidden);
}

void UHudPlayerFrameWidget::ShowLow(bool bLow)
{
	if (bLow == bShownLow)
	{
		return;
	}
	bShownLow = bLow;
	HealthValue->SetColorAndOpacity(FSlateColor(bLow ? Color::HealthLowText() : FLinearColor::White));
	if (Portrait)
	{
		Portrait->SetLowHealth(bLow);
	}
}

void UHudPlayerFrameWidget::UpdateHealthNumbers(float Health, float MaxHealth)
{
	// The maximum rounds to the nearest; health rounds up so a sliver of it never reads as 0, but never past the maximum
	// (a maximum of 108.0000076 would read "109 / 108").
	const int32 MaxPoints = FMath::Max(1, FMath::RoundToInt32(MaxHealth));
	const int32 Points = FMath::Min(FMath::CeilToInt32(Health), MaxPoints);
	if (Points != ShownPoints)
	{
		ShownPoints = Points;
		HealthValue->SetText(FText::AsNumber(Points));
	}
	if (MaxPoints != ShownMaxPoints)
	{
		ShownMaxPoints = MaxPoints;
		HealthMax->SetText(FText::FromString(FString::Printf(TEXT("/ %s"), *FText::AsNumber(MaxPoints).ToString())));
	}
}
