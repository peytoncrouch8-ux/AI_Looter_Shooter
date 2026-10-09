#include "Player/PlayerViewComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

// UPlayerViewComponent's aiming down the sights and recoil on the aim. Its camera, field of view and held gun are in
// PlayerViewCamera.cpp, its view kicks in PlayerViewKicks.cpp.

// ---------------------------------------------------------------------------
// Aiming down the sights
// ---------------------------------------------------------------------------

void UPlayerViewComponent::HandleAimPressed()
{
	// Toggle mode: press to aim, press again to stop; hold mode: aim while held.
	bAimWanted = bAimToggle ? !bAimWanted : true;
}

void UPlayerViewComponent::HandleAimReleased()
{
	if (!bAimToggle)
	{
		bAimWanted = false;
	}
}

void UPlayerViewComponent::UpdateAim(float DeltaTime)
{
	// Only with a gun in hand, and not while it's being reloaded or carried low in a sprint (aiming stops a sprint first).
	const UWeaponManagerComponent* Manager = WeaponManager.Get();
	const AWeaponBase* Weapon = Manager ? Manager->GetActiveWeapon() : nullptr;
	const bool bSprinting = Locomotion.IsValid() && Locomotion->GetSprintAlpha() > 0.5f;
	const bool bCanAim = Weapon && !Weapon->IsReloading() && !bSprinting && Mode != EPlayerViewMode::ThirdPersonFront;
	if (!Weapon)
	{
		bAimWanted = false;
	}
	// Quicker with better handling: a Handling 1 gun takes BaseAimSeconds to come up to the eye.
	const float Speed = (Weapon ? Weapon->GetStats().Handling : 1.f) / BaseAimSeconds;
	const float WasAimAlpha = AimAlpha;
	AimAlpha = FMath::FInterpConstantTo(AimAlpha, bAimWanted && bCanAim ? 1.f : 0.f, DeltaTime, Speed);
	AimZoom = Weapon ? FMath::Max(Weapon->GetStats().Zoom, MinAimZoom) : AimZoom;
	// The sights coming up: the gun's aim-in sound, once as aiming starts.
	if (WasAimAlpha <= 0.f && AimAlpha > 0.f && Weapon)
	{
		Weapon->PlayAimIn();
	}
}

float UPlayerViewComponent::GetAimSpreadMultiplier() const
{
	return FMath::Lerp(1.f, AimSpreadMultiplier, GetAimAlpha());
}

float UPlayerViewComponent::GetLookSensitivityMultiplier() const
{
	// Through a sight the view turns slower, so the crosshair moves across the target as it did unzoomed.
	return FMath::Lerp(1.f, 1.f / AimZoom, GetAimAlpha());
}

float UPlayerViewComponent::GetAimAlpha() const
{
	// Eased, so the gun settles into the sight instead of stopping dead.
	return FMath::InterpEaseInOut(0.f, 1.f, AimAlpha, 2.f);
}

// ---------------------------------------------------------------------------
// Recoil
// ---------------------------------------------------------------------------

void UPlayerViewComponent::BindFiringWeapon(AWeaponBase* Weapon)
{
	if (AWeaponBase* Old = FiringWeapon.Get())
	{
		Old->OnFired.RemoveDynamic(this, &UPlayerViewComponent::HandleWeaponFired);
	}
	FiringWeapon = Weapon;
	if (Weapon)
	{
		Weapon->OnFired.AddUniqueDynamic(this, &UPlayerViewComponent::HandleWeaponFired);
	}
}

void UPlayerViewComponent::HandleWeaponFired()
{
	if (const AWeaponBase* Weapon = FiringWeapon.Get())
	{
		Recoil.AddShot(Weapon->GetRecoilProfile(), RecoilRandom);
		// The view's own kick on top of the recoil (only what the player sees; the aim takes the recoil above).
		AddShotKick(*Weapon);
	}
}

void UPlayerViewComponent::UpdateRecoil(float DeltaTime)
{
	AController* Controller = Character->GetController();
	// Whatever moved the view since our change last tick was the player (pulling down against the kick uses it up).
	const float PlayerPitch = Controller && bHaveLastViewPitch ? FRotator::NormalizeAxis(Controller->GetControlRotation().Pitch - LastViewPitch) : 0.f;
	const FRotator AimChange = Recoil.Tick(DeltaTime, PlayerPitch);

	// Only a player's own view kicks (the camera manager clamps the pitch on its next update).
	if (Controller && Controller->IsLocalPlayerController() && !AimChange.IsNearlyZero(1.e-4f))
	{
		FRotator View = Controller->GetControlRotation();
		View.Pitch = FRotator::ClampAxis(View.Pitch + AimChange.Pitch);
		View.Yaw = FRotator::ClampAxis(View.Yaw + AimChange.Yaw);
		Controller->SetControlRotation(View);
	}
	bHaveLastViewPitch = Controller != nullptr;
	LastViewPitch = Controller ? Controller->GetControlRotation().Pitch : 0.f;
}
