#include "Player/PlayerViewComponent.h"
#include "Player/Animation/LooterClimbPose.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"

// UPlayerViewComponent's cameras: the third-person spring arm, the field of view (the player's setting, the sprint, a
// sight's zoom), the gun held in the third-person body's hands, and where shots start. Aiming and recoil are in
// PlayerViewAim.cpp, the view's kicks in PlayerViewKicks.cpp.

namespace
{
	/** The gun in the body's hands flips this much harder than the first-person gun, so the kick reads from behind. */
	constexpr float ThirdPersonKickScale = 1.35f;
	/**
	 * The eye's lowering (crouches' worth) for the third-person pivot: as it is up to a crouch, then easing toward 1.3 (a
	 * slide's eye goes about 1.5), with no kink where it passes a crouch (its slope is 1 there).
	 */
	float ThirdPersonLowering(float Lowering)
	{
		constexpr float Room = 0.3f;
		const float Past = FMath::Max(Lowering - 1.f, 0.f) / Room;
		return Lowering <= 1.f ? FMath::Max(Lowering, -0.5f) : 1.f + Room * Past / (1.f + Past);
	}

	// Live tuning for the third-person camera (negative = use the component's settings).
	TAutoConsoleVariable<float> CVarArmLength(TEXT("Looter.ThirdPerson.ArmLength"), -1.f, TEXT("Override the third-person camera distance (cm)."));
	TAutoConsoleVariable<float> CVarShoulderRight(TEXT("Looter.ThirdPerson.ShoulderRight"), -1.f, TEXT("Override the third-person camera's offset to the right (cm)."));
	TAutoConsoleVariable<float> CVarShoulderUp(TEXT("Looter.ThirdPerson.ShoulderUp"), -1.f, TEXT("Override the third-person camera's offset upward (cm)."));

	float Tuned(const TAutoConsoleVariable<float>& Variable, float Value)
	{
		const float Override = Variable.GetValueOnGameThread();
		return Override >= 0.f ? Override : Value;
	}

	/** The player's first-person field of view from the settings menu, or Fallback for a character no local player controls. */
	float FirstPersonFieldOfViewSetting(const ACharacter* Owner, float Fallback)
	{
		const APlayerController* OwnerController = Owner ? Cast<APlayerController>(Owner->GetController()) : nullptr;
		const ULocalPlayer* LocalPlayer = OwnerController ? OwnerController->GetLocalPlayer() : nullptr;
		const UGraphicsSettingsSubsystem* Graphics = LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
		return Graphics ? Graphics->GetFirstPersonFieldOfView() : Fallback;
	}

	/** Unzoomed narrowed by a sight's magnification (2x shows half as wide), Aim of the way to the eye. */
	float SightFieldOfView(float Unzoomed, float Zoom, float Aim)
	{
		const float Zoomed = FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(Unzoomed) * 0.5f) / Zoom));
		return FMath::Lerp(Unzoomed, Zoomed, Aim);
	}
}

void UPlayerViewComponent::UpdateBoom(float DeltaTime)
{
	const ACharacter* Owner = Character.Get();
	if (IsFirstPerson() || !Boom)
	{
		return;
	}

	// The pivot tracks the feet at shoulder height, lowered as far as the first-person eye is (a share of a crouch's
	// drop, eased on its curve), so crouching or sliding lowers the camera smoothly even though the capsule itself
	// shrinks in a single frame, and a crouch or stand in the air (which moves the feet) doesn't jolt it. A slide dips it
	// a little lower than a crouch, not all the way. The heights and distances are for the full-size body: the pivot sits
	// in the capsule's own (scaled) space, and the arm, which works in the world's, is scaled to match, so a smaller
	// character keeps the framing the view was tuned with.
	const UPlayerLocomotionComponent* Loco = Locomotion.Get();
	const float Lowering = Loco ? ThirdPersonLowering(Loco->GetViewLowering()) : (Owner->bIsCrouched ? 1.f : 0.f);
	const float HalfHeight = Owner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	Boom->SetRelativeLocation(FVector(0.f, 0.f, FMath::Lerp(PivotHeight, CrouchedPivotHeight, Lowering) - HalfHeight));
	const float Scale = static_cast<float>(Owner->GetActorScale3D().Z);

	PullOut = FMath::FInterpConstantTo(PullOut, 1.f, DeltaTime, 1.f / PullOutTime);
	const float Out = 1.f - FMath::Pow(1.f - PullOut, 3.f); // ease out

	if (Mode == EPlayerViewMode::ThirdPerson)
	{
		const FVector Shoulder(ShoulderOffset.X, Tuned(CVarShoulderRight, ShoulderOffset.Y), Tuned(CVarShoulderUp, ShoulderOffset.Z));
		Boom->TargetArmLength = Tuned(CVarArmLength, ArmLength) * Scale * Out;
		Boom->SocketOffset = Shoulder * Scale * Out;
	}
	else
	{
		// Front view: the arm points back at the character from where they're looking.
		const FRotator Control = Owner->GetControlRotation();
		Boom->SetWorldRotation(FRotator(-FRotator::NormalizeAxis(Control.Pitch), Control.Yaw + 180.f, 0.f));
		Boom->TargetArmLength = FrontArmLength * Scale * Out;
		Boom->SocketOffset = FVector(0.f, 0.f, 10.f) * Scale * Out;
	}
}

void UPlayerViewComponent::UpdateFieldOfView()
{
	const float Offset = Locomotion.IsValid() ? Locomotion->GetFieldOfViewOffset() : 0.f;
	UCameraComponent* Active = IsFirstPerson() ? FirstPersonCamera.Get() : ThirdPersonCamera.Get();
	// First person uses the player's setting, read every frame so a change in the settings menu shows without a restart.
	// Aiming narrows the view by the sight's magnification (2x shows half as wide) from there, so a sight zooms the same
	// whatever the setting; its aim point sits on the camera's axis, which no field of view moves, so it stays true.
	const float Aim = GetAimAlpha();
	const float Unzoomed = (IsFirstPerson() ? FirstPersonFieldOfViewSetting(Character.Get(), FirstPersonFieldOfView) : ThirdPersonFieldOfView) + Offset;
	const float Wanted = SightFieldOfView(Unzoomed, AimZoom, Aim);
	if (Active && !FMath::IsNearlyEqual(Active->FieldOfView, Wanted, 0.01f))
	{
		Active->SetFieldOfView(Wanted);
	}

	// The setting widens the world, not the gun: what the camera draws as first person (its view model, through the
	// engine's first-person rendering) keeps the angle the view had before the setting existed, sprint and sight zoom
	// included, so the gun keeps its size and still grows through a sight as it always has. Only meshes drawn as first
	// person follow it (AWeaponBase::AttachToHolder decides that for the gun).
	UCameraComponent* Camera = FirstPersonCamera.Get();
	if (Camera && IsFirstPerson())
	{
		const float ViewModel = SightFieldOfView(FirstPersonFieldOfView + Offset, AimZoom, Aim);
		Camera->SetEnableFirstPersonFieldOfView(true);
		if (!FMath::IsNearlyEqual(Camera->FirstPersonFieldOfView, ViewModel, 0.01f))
		{
			Camera->SetFirstPersonFieldOfView(ViewModel);
		}
	}
}

FRotator UPlayerViewComponent::GetAimRotation() const
{
	const ACharacter* Owner = Character.Get();
	if (!Owner)
	{
		return FRotator::ZeroRotator;
	}
	const FRotator Control = Owner->GetControlRotation();
	return FRotator(FRotator::NormalizeAxis(Control.Pitch), Control.Yaw, 0.f);
}

FQuat UPlayerViewComponent::GetHeldWeaponRotation() const
{
	const FRotator Aim = GetAimRotation();
	// Each shot flips the muzzle up (and twists it a little) about the grip in the hand, a bit harder than in first
	// person so it reads from behind the character.
	FQuat Aiming = Aim.Quaternion() * (Recoil.GetKickRotation() * ThirdPersonKickScale).Quaternion();

	// Reloading tips the gun down and rolls it toward the body, where the reload animation's hands work on it.
	const AWeaponBase* Weapon = WeaponManager.IsValid() ? WeaponManager->GetActiveWeapon() : nullptr;
	if (const float Reload = Weapon ? Weapon->GetReloadBlend() : 0.f; Reload > 0.f)
	{
		Aiming = FQuat::Slerp(Aiming, Aiming * FRotator(-20.f, 15.f, 45.f).Quaternion(), Reload);
	}

	// Sprinting swings the gun down across the chest (muzzle low and to the left), like the first-person sprint pose.
	const float Sprint = Locomotion.IsValid() ? Locomotion->GetSprintAlpha() : 0.f;
	FQuat Held = Aiming;
	if (Sprint > 0.f)
	{
		Held = FQuat::Slerp(Aiming, FRotator(-35.f, Aim.Yaw - 50.f, 25.f).Quaternion(), Sprint);
	}
	// A mantle or vault carries it up and out to the right while the hands reach for the ledge. Held along the aim it
	// pointed into the ledge the body climbs; swung low across the chest like a sprint, it ran through the leading left
	// knee coming up. (A vault's pose alpha tops out at its share: that's a full carry.)
	const float Climb = Locomotion.IsValid() ? FMath::Min(Locomotion->GetTraversalAlpha() / LooterClimbPose::VaultAlphaShare, 1.f) : 0.f;
	if (Climb > 0.f)
	{
		Held = FQuat::Slerp(Held, FRotator(60.f, Aim.Yaw + 35.f, -15.f).Quaternion(), Climb);
	}
	return Held;
}

void UPlayerViewComponent::UpdateHeldWeapon()
{
	const ACharacter* Owner = Character.Get();
	const UWeaponManagerComponent* Manager = WeaponManager.Get();
	USkeletalMeshComponent* Body = Owner ? Owner->GetMesh() : nullptr;
	AWeaponBase* Weapon = Manager ? Manager->GetActiveWeapon() : nullptr;
	if (IsFirstPerson() || !Body || !Weapon || !Manager->IsThirdPersonHold() || Weapon->GetRootComponent()->GetAttachParent() != Body)
	{
		return;
	}

	// The animation moves the hand; the gun keeps its grip in it but points exactly where the player aims, so the
	// barrel lines up with the crosshair whatever the pose. (The left hand is solved onto it in the stance layer.)
	const FTransform Socket = Body->GetSocketTransform(Manager->ThirdPersonAttachSocket);
	const FQuat Relative = Socket.GetRotation().Inverse() * GetHeldWeaponRotation();
	Weapon->SetActorRelativeTransform(FTransform(Relative, Relative.RotateVector(-Weapon->GetGripPoint())));
}

void UPlayerViewComponent::GetAimViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const ACharacter* Owner = Character.Get();
	const AController* Controller = Owner ? Owner->GetController() : nullptr;
	if (!Owner)
	{
		OutLocation = FVector::ZeroVector;
		OutRotation = FRotator::ZeroRotator;
		return;
	}
	if (Mode == EPlayerViewMode::ThirdPersonFront || !Controller)
	{
		Owner->GetActorEyesViewPoint(OutLocation, OutRotation);
		return;
	}

	Controller->GetPlayerViewPoint(OutLocation, OutRotation);
	if (Mode == EPlayerViewMode::ThirdPerson)
	{
		// Same line as the crosshair, but starting level with the character's eyes.
		const FVector Direction = OutRotation.Vector();
		OutLocation += Direction * FMath::Max(FVector::DotProduct(Owner->GetPawnViewLocation() - OutLocation, Direction), 0.f);
	}
}
