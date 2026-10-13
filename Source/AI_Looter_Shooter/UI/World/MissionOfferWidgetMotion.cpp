// UMissionOfferWidget's motion: rising into place as it fades up, its ! popping and its glow breathing, the ACCEPTED stamp
// slammed down and held before the card lifts away, or the card sinking as it's put back; then the HUD takes it away.

#include "UI/World/MissionOfferWidget.h"
#include "Components/Image.h"
#include "Components/Widget.h"

namespace
{
	/** How far the card rises into place as it opens (and on as it's accepted, or sinks as it's put back). */
	constexpr float SlideDistance = 22.f;
	/** The stamp's tilt: pressed by hand. */
	constexpr float StampAngle = -8.f;
	/** The stamp comes down from this much bigger. */
	constexpr float StampDrop = 1.9f;

	float Clamp01(float Value)
	{
		return FMath::Clamp(Value, 0.f, 1.f);
	}

	float EaseOut(float Alpha)
	{
		const float Left = 1.f - Clamp01(Alpha);
		return 1.f - Left * Left * Left;
	}

	float EaseIn(float Alpha)
	{
		const float In = Clamp01(Alpha);
		return In * In * In;
	}
}

void UMissionOfferWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Another menu (or the main menu) may have seen the device change.
	if (LooterUI::GetMenuDevice() != ShownDevice)
	{
		RefreshNavLook();
	}
	Advance(InDeltaTime);
}

void UMissionOfferWidget::Advance(float DeltaSeconds)
{
	Age += DeltaSeconds;
	Nav.Tick(DeltaSeconds);
	if (Mode == EMissionCardMode::Choice)
	{
		// The hold's ring and the columns' light, which a choice made may close the card from.
		AdvanceChoice(DeltaSeconds);
	}
	if (bClosing)
	{
		CloseAge += DeltaSeconds;
		if (CloseAge >= GetCloseSeconds())
		{
			FinishClose();
			return;
		}
	}
	ApplyMotion();
}

void UMissionOfferWidget::ApplyMotion()
{
	// In: rising into place as it fades up.
	const float In = EaseOut(Age / OpenSeconds);
	float Shown = In;
	float Slide = (1.f - In) * SlideDistance;
	if (bClosing && bAccepted)
	{
		// Out after the stamp has been seen: lifting away, taken (a choice's stamp is held a little longer).
		const float Held = Mode == EMissionCardMode::Choice ? ChoiceStampHoldSeconds : StampHoldSeconds;
		const float Out = EaseIn((CloseAge - StampSlamSeconds - Held) / StampOutSeconds);
		Shown *= 1.f - Out;
		Slide -= Out * SlideDistance;
	}
	else if (bClosing)
	{
		// Put back: sinking as it fades.
		const float Out = Clamp01(CloseAge / DeclineSeconds);
		Shown *= 1.f - Out;
		Slide += Out * SlideDistance * 0.5f;
	}
	if (CardBody)
	{
		CardBody->SetRenderOpacity(Shown);
		CardBody->SetRenderTranslation(FVector2D(0.f, Slide));
	}
	if (Backdrop)
	{
		// Over a board's screen, its own dimming is most of it already.
		Backdrop->SetRenderOpacity(Shown * (bOverScreen ? 0.55f : 1.f));
	}

	// The ! pops as the card arrives (overshooting, then settling) and its glow breathes.
	const float Pop = Clamp01((Age - 0.06f) / 0.32f);
	const float MarkScale = Pop < 0.55f ? FMath::Lerp(0.35f, 1.22f, EaseOut(Pop / 0.55f)) : FMath::Lerp(1.22f, 1.f, EaseOut((Pop - 0.55f) / 0.45f));
	if (MarkImage)
	{
		MarkImage->SetRenderScale(FVector2D(MarkScale));
	}
	if (MarkHalo)
	{
		MarkHalo->SetRenderOpacity(Pop * (0.6f + 0.25f * FMath::Sin(Age * 3.2f)));
	}

	// The stamp: slammed down from big, then held.
	if (Stamp)
	{
		const ESlateVisibility Wanted = bAccepted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (Stamp->GetVisibility() != Wanted)
		{
			Stamp->SetVisibility(Wanted);
		}
		if (bAccepted)
		{
			const float Slam = Clamp01(CloseAge / StampSlamSeconds);
			Stamp->SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(FMath::Lerp(StampDrop, 1.f, EaseIn(Slam))),
				FVector2D::ZeroVector, StampAngle));
			Stamp->SetRenderOpacity(FMath::Min(1.f, Slam * 1.6f));
		}
	}
}
