// UCreatureHealthBarWidget: a Warded creature's ward over its health (the traits' soul ward,
// UCreatureTraitComponent::GetWardFraction, fed each frame by UCreatureTierLookComponent::DressTag): shown while it stands
// in the ward's element's colour, a hit's chip draining after the fill, the break's flash and swell. Moved out of
// CreatureHealthBarWidget.cpp to keep it under the size guide; the bar itself is built there with the rest (RebuildWidget).

#include "UI/World/CreatureHealthBarWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"

using namespace LooterUI;

FLinearColor UCreatureHealthBarWidget::DefaultWardColor()
{
	// The kit's pale cyan: a ward with no element reads as the soul's, never as health or a rarity.
	return Color::CyanText();
}

FLinearColor UCreatureHealthBarWidget::WardChipColorFor(const FLinearColor& Fill)
{
	// A paler of the fill, as the health's chip is a pale of its red.
	FLinearColor Chip = FMath::Lerp(Fill, FLinearColor::White, 0.6f);
	Chip.A = 0.85f;
	return Chip;
}

void UCreatureHealthBarWidget::ApplyWardColor()
{
	if (WardFillImage)
	{
		WardFillImage->SetColorAndOpacity(WardColor);
	}
	if (WardChipImage)
	{
		WardChipImage->SetColorAndOpacity(WardChipColorFor(WardColor));
	}
}

void UCreatureHealthBarWidget::SetWard(bool bInWarded, float InFraction, const FLinearColor& InColor)
{
	// A typed ward's colour (a Touched creature's own element, or the roll's); kept through the break so its flash is its own.
	if (bInWarded && !WardColor.Equals(InColor))
	{
		WardColor = InColor;
		ApplyWardColor();
	}
	const float NewFraction = FMath::Clamp(InFraction, 0.f, 1.f);
	if (!bInWarded)
	{
		// Not Warded (its traits changed): no ward bar, at once and with no flash.
		if (bWardShown || IsWardBreaking())
		{
			bWardShown = false;
			WardFraction = WardGhost = WardGhostHold = 0.f;
			WardBreakClock = WardBreakSeconds;
			ApplyWard();
		}
		return;
	}
	if (NewFraction > 0.f)
	{
		if (!bWardShown || IsWardBreaking())
		{
			// Up: seen for the first time, or whole again (a new life): shown as it is at once.
			bWardShown = true;
			WardBreakClock = WardBreakSeconds;
			WardFraction = WardGhost = NewFraction;
			WardGhostHold = 0.f;
			ApplyWard();
			return;
		}
		if (FMath::IsNearlyEqual(NewFraction, WardFraction))
		{
			return;
		}
		if (NewFraction < WardFraction)
		{
			// A hit on the ward: what it took lingers as the chip for a moment.
			WardGhostHold = WardChipHold;
		}
		WardFraction = NewFraction;
		WardGhost = FMath::Max(WardGhost, WardFraction);
		ApplyWard();
		return;
	}
	// Broken while shown: it flashes white-hot and goes. A ward already broken when its tag first shows stays hidden.
	if (bWardShown)
	{
		bWardShown = false;
		WardFraction = WardGhost = WardGhostHold = 0.f;
		WardBreakClock = 0.f;
		ApplyWard();
	}
}

void UCreatureHealthBarWidget::AdvanceWard(float DeltaSeconds)
{
	float Left = FMath::Max(DeltaSeconds, 0.f);
	bool bChanged = false;
	if (IsWardBreaking())
	{
		WardBreakClock = FMath::Min(WardBreakClock + Left, WardBreakSeconds);
		bChanged = true;
	}
	if (WardGhost > WardFraction)
	{
		// The chip holds, then drains down to the fill (whatever of this step the hold didn't use).
		const float Held = FMath::Min(WardGhostHold, Left);
		WardGhostHold -= Held;
		Left -= Held;
		if (Left > 0.f)
		{
			WardGhost = FMath::Max(WardFraction, WardGhost - WardChipDrainRate * Left);
			bChanged = true;
		}
	}
	if (bChanged)
	{
		ApplyWard();
	}
}

float UCreatureHealthBarWidget::WardBreakFlashAt(float Seconds)
{
	if (Seconds < 0.f || Seconds >= WardBreakSeconds)
	{
		return 0.f;
	}
	const float Left = 1.f - Seconds / WardBreakSeconds;
	return Left * Left;
}

void UCreatureHealthBarWidget::ApplyWard()
{
	if (!WardBox)
	{
		return;
	}
	const bool bBreaking = IsWardBreaking();
	WardBox->SetVisibility(bWardShown || bBreaking ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (WardFillBox && WardChipBox)
	{
		WardFillBox->SetWidthOverride(BarWidth * WardFraction);
		WardChipBox->SetWidthOverride(BarWidth * WardGhost);
	}
	// Breaking: the whole bar lit white-hot (a white of its own colour), swelling a little from its middle, fading out; it
	// leans as the health bar does either way.
	const float Flash = bBreaking ? WardBreakFlashAt(WardBreakClock) : 0.f;
	if (WardFlashImage)
	{
		FLinearColor Lit = FMath::Lerp(WardColor, FLinearColor::White, 0.75f);
		Lit.A = Flash;
		WardFlashImage->SetColorAndOpacity(Lit);
		WardFlashImage->SetVisibility(Flash > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	const float Scale = 1.f + (WardBreakSwell - 1.f) * Flash;
	FWidgetTransform Transform;
	Transform.Scale = FVector2D(Scale, Scale);
	// The health bar's own lean (its frame's, set first in RebuildWidget), so the two never drift apart.
	Transform.Shear = FrameBox ? FrameBox->GetRenderTransform().Shear : FVector2D::ZeroVector;
	WardBox->SetRenderTransform(Transform);
}
