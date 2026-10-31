// UHudFlaskWidget's plain rules (Looter.UI.Flask.Rules): how full each pip is, which flask changes flash, the icon's tip,
// a pip's pour and rise, the refusal's shake; and the flask's Inked icon. The row itself is HudFlaskWidget.cpp's.

#include "UI/HUD/HudFlaskWidget.h"
#include "Player/PlayerFlaskRules.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/InkedIconData.inl"

using namespace LooterUI;

namespace
{
	/** A pip filling with soul eases up at this FInterpTo speed (quick, but the rise is seen). */
	constexpr float SoulRiseSpeed = 9.f;
	/** The refusal's shake: this far each way (px) at its start, this fast (radians a second), as the grenade counter's. */
	constexpr float DeniedShake = 4.f;
	constexpr float ShakeSpeed = 55.f;
}

// ---------------------------------------------------------------------------
// The plain rules
// ---------------------------------------------------------------------------

float UHudFlaskWidget::PipTarget(int32 Index, int32 Charges, float SoulShare)
{
	if (Index < Charges)
	{
		return 1.f;
	}
	// Only the next charge fills: soul isn't kept with the flask full (the flask says 0 then), and never two at once.
	return Index == Charges ? FMath::Clamp(SoulShare, 0.f, 1.f) : 0.f;
}

bool UHudFlaskWidget::FlashFor(EFlaskChange Why, EHudFlaskFlash& OutFlash)
{
	switch (Why)
	{
	case EFlaskChange::Drunk: OutFlash = EHudFlaskFlash::Drunk; return true;
	case EFlaskChange::Filled: OutFlash = EHudFlaskFlash::Filled; return true;
	case EFlaskChange::Refilled: OutFlash = EHudFlaskFlash::Refilled; return true;
	case EFlaskChange::DeniedEmpty: OutFlash = EHudFlaskFlash::DeniedEmpty; return true;
	case EFlaskChange::DeniedFull: OutFlash = EHudFlaskFlash::DeniedFull; return true;
	default: return false;
	}
}

float UHudFlaskWidget::TipAngle(float DrinkAlpha)
{
	// Tipped up to the mouth and back down over the drink: the most at its middle, where the gulp is. Toward the left,
	// where the player frame's portrait (the drinker) is.
	const float Alpha = FMath::Clamp(DrinkAlpha, 0.f, 1.f);
	return -TipDegrees * FMath::Sin(UE_PI * Alpha);
}

float UHudFlaskWidget::StepLevel(float Shown, float Target, float DeltaTime)
{
	if (Target < Shown)
	{
		// A pour: drains steadily, a whole pip in DrunkSeconds, so it reads as the liquid going.
		return FMath::Max(Target, Shown - DeltaTime / DrunkSeconds);
	}
	const float Next = FMath::FInterpTo(Shown, Target, DeltaTime, SoulRiseSpeed);
	return Target - Next <= UHudFlaskWidget::LevelSnap ? Target : Next;
}

float UHudFlaskWidget::ShakeOffset(float Elapsed)
{
	if (Elapsed < 0.f || Elapsed >= DeniedSeconds)
	{
		return 0.f;
	}
	return FMath::Sin(Elapsed * ShakeSpeed) * DeniedShake * (1.f - Elapsed / DeniedSeconds);
}

const FInkedIcon& UHudFlaskWidget::Icon()
{
	// The soul flask as an Inked icon, in the ammo icons' box and ink line: a round-bellied flask with a leather band, a
	// crooked neck, a tin collar and a cork, the soul's level cut across it (Art/Icons/InkedIcons.py makes it).
	static const FInkedIcon Flask = InkedIconData::SoulFlask();
	return Flask;
}
