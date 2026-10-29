// UPlayerHUDWidget's crosshair: the gun kind's shape (HudCrosshairRules), its gap following the gun's spread, the kick of
// each shot by its kind, the new shape growing in as another kind comes into hand, its colour from the settings, and when it
// shows. Aiming down the sights it fades out, with any gun in any view (the user's call, 2026-10-14: no crosshair while
// aiming), and comes back as the gun comes down; the centre dot stays only through a third-person aim, which shows no sight.

#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/HudCrosshairRules.h"
#include "UI/HUD/HudCrosshairWidget.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Components/SizeBox.h"
#include "Components/Widget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Actor.h"

namespace
{
	/** The centre dot's own opacity (it's drawn a little soft), and how solid it stays through a third-person aim. */
	constexpr float AimDotRest = 0.8f;
	constexpr float AimDotThirdPersonAim = 0.95f;

	EWeaponKind KindOf(const AWeaponBase* Gun)
	{
		const UWeaponDefinition* Definition = Gun ? Gun->GetInstance().Definition.Get() : nullptr;
		return Definition ? Definition->Kind : EWeaponKind::None;
	}
}

void UPlayerHUDWidget::UpdateCrosshair(const AWeaponBase* Active, float DeltaTime)
{
	if (!CrosshairBox || !CrosshairLines)
	{
		return;
	}

	// The gun in hand's shape; another kind grows in and fades up, so the change reads (the first shape just shows).
	const EWeaponKind Kind = KindOf(Active);
	if (!bCrosshairKindShown || Kind != CrosshairKind)
	{
		CrosshairSwitchAge = bCrosshairKindShown ? 0.f : HudCrosshair::SwitchSeconds;
		CrosshairKind = Kind;
		bCrosshairKindShown = true;
		CrosshairLines->SetKind(Kind);
		if (CrosshairDotShape)
		{
			CrosshairDotShape->SetKind(Kind);
		}
		// A kick from the last gun's shot doesn't carry over to the new shape.
		bCrosshairKicking = false;
	}
	const HudCrosshair::FShape& Shape = HudCrosshair::ShapeFor(Kind);

	// The colour picked in the settings, read live so a change shows the moment the game resumes.
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UGraphicsSettingsSubsystem* Settings = LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
	const FLinearColor Colour = HudCrosshair::ColourOf(Settings ? Settings->GetCrosshairColour() : ECrosshairColour::White);
	CrosshairLines->SetColour(Colour);
	if (CrosshairDotShape)
	{
		CrosshairDotShape->SetColour(Colour);
	}

	// The gap tracks the weapon's accuracy: tight for rifles, wide for shotguns, tighter still when crouched. The big shapes
	// open less, so none reaches the prompt under it.
	const float Spread = Active ? Active->GetEffectiveSpread() : 1.f;
	const float Wanted = HudCrosshair::GapBoxFor(Spread);
	if (!FMath::IsNearlyEqual(Wanted, CrosshairSize, 0.25f))
	{
		CrosshairSize = CrosshairSize <= 0.f ? Wanted : FMath::FInterpTo(CrosshairSize, Wanted, DeltaTime, 10.f);
	}
	CrosshairLines->SetOpen(FMath::Min(HudCrosshair::OpenOf(CrosshairSize), HudCrosshair::OpenLimitFor(Shape)));

	// Each shot kicks it out by its kind's amount and it settles back, so every shot is felt where the eyes are. The frame of
	// the shot shows the full kick. A new shape grows in on top.
	float Scale = 1.f;
	if (bCrosshairKicking)
	{
		const HudCrosshair::FKick Kick = HudCrosshair::KickFor(Kind);
		Scale = HudCrosshair::KickScaleAt(Kick, CrosshairKickAge);
		CrosshairKickAge += DeltaTime;
		bCrosshairKicking = CrosshairKickAge < Kick.Seconds;
	}
	const float SwitchFade = HudCrosshair::SwitchFadeAt(CrosshairSwitchAge);
	Scale *= HudCrosshair::SwitchScaleAt(CrosshairSwitchAge);
	CrosshairSwitchAge = FMath::Min(CrosshairSwitchAge + DeltaTime, HudCrosshair::SwitchSeconds);
	if (!FMath::IsNearlyEqual(static_cast<float>(CrosshairBox->GetRenderTransform().Scale.X), Scale, 0.001f))
	{
		CrosshairBox->SetRenderScale(FVector2D(Scale));
	}

	// No aiming while the gun is down in the sprint pose, and no crosshair when the camera faces the character. Aiming down
	// the sights, the lines fade as the gun comes up and return as it's lowered, in every view; the dot stays only through
	// a third-person aim.
	const AActor* Holder = Active ? Active->GetOwner() : nullptr;
	const UPlayerLocomotionComponent* Locomotion = Holder ? Holder->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	const UPlayerViewComponent* View = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	const bool bFacingCamera = View && View->GetViewMode() == EPlayerViewMode::ThirdPersonFront;
	const float Aim = View ? View->GetAimAlpha() : 0.f;
	const float Sprint = Locomotion ? Locomotion->GetSprintAlpha() : 0.f;
	const float Opacity = CrosshairOpacity(Aim, Sprint, bFacingCamera) * SwitchFade;
	if (!FMath::IsNearlyEqual(CrosshairBox->GetRenderOpacity(), Opacity, 0.01f))
	{
		CrosshairBox->SetRenderOpacity(Opacity);
	}
	const float DotOpacity = AimDotOpacity(Aim, Sprint, bFacingCamera, !View || View->IsFirstPerson(), Shape.bHipDot) * SwitchFade;
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

float UPlayerHUDWidget::AimDotOpacity(float AimAlpha, float SprintAlpha, bool bFacingCamera, bool bFirstPerson, bool bHipDot)
{
	// A shape drawn with no dot shows none beside its lines; through a third-person aim it still gets the aim point.
	const float WithTicks = bHipDot ? CrosshairOpacity(AimAlpha, SprintAlpha, bFacingCamera) * AimDotRest : 0.f;
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
