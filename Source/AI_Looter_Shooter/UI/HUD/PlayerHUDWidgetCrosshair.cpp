// UPlayerHUDWidget's crosshair: its gap following the gun's spread, the kick of each shot, and when it shows. Aiming down
// the sights it fades out, with any gun in any view (the user's call, 2026-10-14: no crosshair while aiming), and comes
// back as the gun comes down; the centre dot stays only through a third-person aim, which shows no sight.

#include "UI/HUD/PlayerHUDWidget.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponBase.h"
#include "Components/SizeBox.h"
#include "Components/Widget.h"
#include "GameFramework/Actor.h"

namespace
{
	/** The centre dot's own opacity (it's drawn a little soft), and how solid it stays through a third-person aim. */
	constexpr float AimDotRest = 0.8f;
	constexpr float AimDotThirdPersonAim = 0.95f;
	/** Each shot kicks the crosshair out to this size, settling back over CrosshairKickTime seconds (easing out). */
	constexpr float CrosshairKickScale = 1.45f;
	constexpr float CrosshairKickTime = 0.16f;
}

void UPlayerHUDWidget::UpdateCrosshair(const AWeaponBase* Active, float DeltaTime)
{
	// The gap tracks the weapon's accuracy: tight for rifles, wide for shotguns, tighter still when crouched.
	const float Spread = Active ? Active->GetEffectiveSpread() : 1.f;
	const float Wanted = FMath::Clamp(22.f + Spread * 7.f, 24.f, 80.f);
	if (!FMath::IsNearlyEqual(Wanted, CrosshairSize, 0.25f))
	{
		CrosshairSize = CrosshairSize <= 0.f ? Wanted : FMath::FInterpTo(CrosshairSize, Wanted, DeltaTime, 10.f);
		CrosshairBox->SetWidthOverride(CrosshairSize);
		CrosshairBox->SetHeightOverride(CrosshairSize);
	}

	// Each shot kicks it out and it settles back, so every shot is felt where the eyes are. The frame of the shot shows
	// the full kick.
	if (bCrosshairKicking)
	{
		const float Settled = FMath::Min(CrosshairKickAge / CrosshairKickTime, 1.f);
		const float Kick = 1.f - FMath::InterpEaseOut(0.f, 1.f, Settled, 2.f);
		CrosshairBox->SetRenderScale(FVector2D(1.f + (CrosshairKickScale - 1.f) * Kick));
		CrosshairKickAge += DeltaTime;
		bCrosshairKicking = Settled < 1.f;
	}

	// No aiming while the gun is down in the sprint pose, and no crosshair when the camera faces the character. Aiming down
	// the sights, the ticks fade as the gun comes up and return as it's lowered, in every view; the dot stays only through
	// a third-person aim.
	const AActor* Holder = Active ? Active->GetOwner() : nullptr;
	const UPlayerLocomotionComponent* Locomotion = Holder ? Holder->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	const UPlayerViewComponent* View = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	const bool bFacingCamera = View && View->GetViewMode() == EPlayerViewMode::ThirdPersonFront;
	const float Aim = View ? View->GetAimAlpha() : 0.f;
	const float Sprint = Locomotion ? Locomotion->GetSprintAlpha() : 0.f;
	const float Opacity = CrosshairOpacity(Aim, Sprint, bFacingCamera);
	if (!FMath::IsNearlyEqual(CrosshairBox->GetRenderOpacity(), Opacity, 0.01f))
	{
		CrosshairBox->SetRenderOpacity(Opacity);
	}
	const float DotOpacity = AimDotOpacity(Aim, Sprint, bFacingCamera, !View || View->IsFirstPerson());
	if (CrosshairDot && !FMath::IsNearlyEqual(CrosshairDot->GetRenderOpacity(), DotOpacity, 0.01f))
	{
		CrosshairDot->SetRenderOpacity(DotOpacity);
	}
}

float UPlayerHUDWidget::CrosshairOpacity(float AimAlpha, float SprintAlpha, bool bFacingCamera)
{
	if (bFacingCamera)
	{
		return 0.f;
	}
	const float SightFade = FMath::Clamp(1.f - AimAlpha / CrosshairGoneAtAim, 0.f, 1.f);
	return FMath::Clamp(1.f - SprintAlpha, 0.f, 1.f) * SightFade;
}

float UPlayerHUDWidget::AimDotOpacity(float AimAlpha, float SprintAlpha, bool bFacingCamera, bool bFirstPerson)
{
	const float WithTicks = CrosshairOpacity(AimAlpha, SprintAlpha, bFacingCamera) * AimDotRest;
	if (bFirstPerson || bFacingCamera)
	{
		return WithTicks;
	}
	// Third person: the dot firms up as the ticks go, so the aim point never drops out mid-raise.
	const float Raised = FMath::Clamp(AimAlpha / CrosshairGoneAtAim, 0.f, 1.f);
	return FMath::Max(WithTicks, Raised * AimDotThirdPersonAim * FMath::Clamp(1.f - SprintAlpha, 0.f, 1.f));
}

void UPlayerHUDWidget::HandleFired()
{
	CrosshairKickAge = 0.f;
	bCrosshairKicking = true;
}
