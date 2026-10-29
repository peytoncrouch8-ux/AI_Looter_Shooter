#include "Player/PlayerViewComponent.h"
#include "Combat/HealthComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Weapons/GunfeelKickTuning.h"
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
	// Swimming lowers the gun out of view: no sights.
	const bool bSwimming = Locomotion.IsValid() && Locomotion->IsSwimming();
	// No sights on a gun going down or coming up in a swap.
	// Dead (until the respawn resets health) the sights drop and a toggled aim is let go.
	const UHealthComponent* Health = OwnerHealth.Get();
	const bool bDead = Health && Health->IsDead();
	const bool bCanAim = Weapon && !bDead && !Weapon->IsReloading() && !bSprinting && !bSwimming && !Manager->IsSwapping()
		&& Mode != EPlayerViewMode::ThirdPersonFront;
	if (!Weapon || bDead || bSwimming)
	{
		bAimWanted = false;
	}
	// Quicker with better handling: a Handling 1 gun takes BaseAimSeconds to come up to the eye, times the kind's own
	// (the shotgun's sights a touch slower, the six-gun's quicker: Looter.Gunfeel.Aim.Time.*).
	const float AimSeconds = BaseAimSeconds * (Weapon ? GunfeelKick::AimTimeScale(GunfeelKick::KindOf(Weapon)) : 1.f);
	const float Speed = (Weapon ? Weapon->GetStats().Handling : 1.f) / AimSeconds;
	const float WasAimAlpha = AimAlpha;
	AimAlpha = FMath::FInterpConstantTo(AimAlpha, bAimWanted && bCanAim ? 1.f : 0.f, DeltaTime, Speed);
	AimZoom = Weapon ? FMath::Max(Weapon->GetStats().Zoom, MinAimZoom) : AimZoom;
	// Each kind comes up its own way (gun identity): a light gun snaps to the eye, a heavy one is hefted and eased in.
	if (Weapon)
	{
		AimCurve = GunfeelKick::KindFeel(GunfeelKick::KindOf(Weapon)).AimCurve;
	}
	// The sights coming up: the gun's aim-in sound, once as aiming starts.
	if (WasAimAlpha <= 0.f && AimAlpha > 0.f && Weapon)
	{
		Weapon->PlayAimIn();
	}
	// ...and arriving: the view settles once as they reach the eye after being under half way (no settle on a sight that
	// only dipped a little).
	if (AimAlpha < 0.5f)
	{
		bAimSettleArmed = true;
	}
	else if (bAimSettleArmed && AimAlpha >= 1.f && Weapon)
	{
		bAimSettleArmed = false;
		AddAimSettleKick(*Weapon);
	}
	// ...and let go with the sights still up: the gun's aim-out sound (not for a reload, a sprint or a swap taking them).
	if (bAimWantedLast && !bAimWanted && AimAlpha > 0.f && Weapon && !bDead && !bSwimming)
	{
		Weapon->PlayAimOut();
	}
	bAimWantedLast = bAimWanted;
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
	// Eased, so the gun settles into the sight instead of stopping dead; by its kind's curve, whose ends are the same for all
	// (the spread, the look's speed and the crosshair's fade all read this).
	return FMath::InterpEaseInOut(0.f, 1.f, AimAlpha, AimCurve);
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
		// The gun model kicks less through the sight (AddShot blends by how far the sights are up); the aim then takes the
		// kind's pattern on top: a rifle's drift, an SMG's shimmy, an LMG's sway, by the shot's place in the burst.
		// The LMG's dig-in reaches the pattern too (the profile already has it).
		const EWeaponKind Kind = GunfeelKick::KindOf(Weapon);
		Recoil.AddShot(Weapon->GetRecoilProfile(), RecoilRandom, GetAimAlpha());
		Recoil.AddAimYaw(GunfeelKick::PatternSide(Kind, Recoil.GetShotInBurst()) * GunfeelKick::ShotRecoil(Weapon) * Weapon->GetKickDigIn());
		// A bolt gun's kick holds while its bolt is worked, then the world settles (none for any other gun).
		Recoil.HoldRecovery(Weapon->GetRecoilHoldSeconds());
		// The view's own kick on top of the recoil (only what the player sees; the aim takes the recoil above).
		AddShotKick(*Weapon);
	}
}

void UPlayerViewComponent::UpdateRecoil(float DeltaTime)
{
	AController* Controller = Character->GetController();
	// Whatever moved the view since our change last tick was the player (pulling down against the kick uses it up), less
	// what the gun's own sway through the sights moved it. Counted as the player's, every dip of the sway ate the kick's
	// recovery and its rise never gave it back, so a long aimed burst ratcheted the view up a little each sway.
	const AWeaponBase* Weapon = FiringWeapon.Get();
	const float SwayPitch = Weapon ? static_cast<float>(Weapon->GetSwayApplied().Pitch) : 0.f;
	const float ViewMoved = Controller && bHaveLastViewPitch ? FRotator::NormalizeAxis(Controller->GetControlRotation().Pitch - LastViewPitch) : 0.f;
	const float PlayerPitch = bHaveLastViewPitch ? ViewMoved - (SwayPitch - LastSwayPitch) : 0.f;
	LastSwayPitch = SwayPitch;
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
