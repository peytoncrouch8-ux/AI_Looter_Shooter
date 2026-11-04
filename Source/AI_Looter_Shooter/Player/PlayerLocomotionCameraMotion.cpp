#include "Player/PlayerLocomotionComponent.h"
#include "Player/CameraShakeModifier.h"
#include "Player/PlayerCameraMotion.h"
#include "Player/PlayerViewComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Weapons/GunfeelKickKinds.h"
#include "Weapons/WeaponBase.h"
#include "Camera/CameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

// The camera answering the movement (core-loop audit change 6, Player/PlayerCameraMotion.h): the locomotion's half. The turns
// (a landing's dip, the jump's kick, each footfall's nod and sprint roll, a mantle's or vault's landing) go to the camera
// modifier as FViewKick, which scales them by the Camera shake setting like every kick; the eye's dip and the strafe lean can't
// be kicks (the first is centimetres, the second held), so they come back from UpdateCameraMotion to UpdateCamera, scaled
// by the same setting here. Everything is for the local player's own view: a character with no local controller (a test world's)
// moves nothing. Through the sights it all keeps AimedShare, so the reticle stays on what it is on.

namespace
{
	/** The local player's controller, else null. */
	APlayerController* MotionPlayer(const ACharacter* Owner)
	{
		APlayerController* Player = Owner ? Cast<APlayerController>(Owner->GetController()) : nullptr;
		return Player && Player->IsLocalController() ? Player : nullptr;
	}

	/** The Camera shake setting (1 without a local player, as the modifier reads it). */
	float ShakeSetting(const APlayerController* Player)
	{
		const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
		const UControlSettingsSubsystem* Controls = LocalPlayer ? LocalPlayer->GetSubsystem<UControlSettingsSubsystem>() : nullptr;
		return Controls ? Controls->GetCameraShake() : 1.f;
	}

	float SightsShare(const UPlayerViewComponent* View, const CameraMotion::FTuning& Tuning)
	{
		return CameraMotion::AimedShare(View ? View->GetAimAlpha() : 0.f, Tuning);
	}

	/**
	 * Aimed, a kick turns about the sight (FViewKick::Pivot), as the shot and settle kicks do (PlayerViewKicks.cpp), so the
	 * reticle keeps the shot's line: about the eye the 0.3 degree jump kick took the raised sight 0.03 cm off it
	 * (Looter.Weapons.Aim.CrouchAligned, 2026-11-03).
	 */
	void PivotOnSight(FViewKick& Kick, const UPlayerViewComponent* View, const UCameraComponent* Camera, const ACharacter* Owner)
	{
		if (!View || View->GetAimAlpha() <= 0.f || !Owner)
		{
			return;
		}
		const UWeaponManagerComponent* Guns = Owner->FindComponentByClass<UWeaponManagerComponent>();
		if (const AWeaponBase* Weapon = Guns ? Guns->GetActiveWeapon() : nullptr)
		{
			Kick.Pivot = GunfeelKick::SightGap(Camera, *Weapon);
		}
	}
}

FCameraMotionPose UPlayerLocomotionComponent::UpdateCameraMotion(float DeltaTime)
{
	// The mantle's and vault's landing listener, bound the first frame this runs (the component listens to itself, so it dies with it).
	if (!TraversalLandedHandle.IsValid())
	{
		TraversalLandedHandle = OnTraversalLanded.AddUObject(this, &UPlayerLocomotionComponent::HandleTraversalLanded);
	}

	FCameraMotionPose Pose;
	const ACharacter* Owner = Character.Get();
	const UCharacterMovementComponent* Move = Movement.Get();
	if (!Owner || !Move)
	{
		return Pose;
	}
	const APlayerController* Player = MotionPlayer(Owner);
	const CameraMotion::FTuning Tuning = CameraMotion::Live();
	const float Shake = ShakeSetting(Player);

	ViewMotion.Advance(DeltaTime);
	if (!Player || Tuning.Master <= 0.f)
	{
		// No local view, or the master off: no dips, and a lean already under way eases out rather than snapping.
		ViewMotion.ClearDips();
		Pose.Roll = ViewMotion.EaseLean(0.f, Tuning.StrafeRate, DeltaTime) * Shake;
		return Pose;
	}

	// The lean follows the sideways speed on the ground, into the strafe; a slide has its own tip, and in the air it eases out.
	const float Lateral = static_cast<float>(FVector::DotProduct(Owner->GetVelocity(), Owner->GetActorRightVector()));
	float Target = Move->IsMovingOnGround() ? CameraMotion::StrafeTarget(Lateral, BaseWalkSpeed, Tuning) : 0.f;
	Target *= (1.f - SlideAlpha) * SightsShare(PlayerView.Get(), Tuning);
	ViewMotion.EaseLean(Target, Tuning.StrafeRate, DeltaTime);

	Pose.EyeDrop = ViewMotion.GetEyeDrop() * Shake;
	Pose.Roll = ViewMotion.GetLean() * Shake;
	return Pose;
}

void UPlayerLocomotionComponent::CameraMotionLanded(float LandingSpeed)
{
	APlayerController* Player = MotionPlayer(Character.Get());
	if (!Player)
	{
		return;
	}
	const CameraMotion::FTuning Tuning = CameraMotion::Live();
	if (!CameraMotion::LandingFelt(LandingSpeed, Tuning))
	{
		return;
	}
	// The view dips by the fall (a spring on the modifier), and the eye with it on a gentler curve of its own that starts at
	// rest (UpdateCameraMotion hands it to UpdateCamera): the whole rig, arms and gun with it, lowers a moment.
	const float Share = SightsShare(PlayerView.Get(), Tuning);
	FViewKick Kick = CameraMotion::LandingKick(LandingSpeed, Tuning) * Share;
	PivotOnSight(Kick, PlayerView.Get(), Camera.Get(), Character.Get());
	UCameraShakeModifier::Kick(Player, Kick);
	ViewMotion.AddEyeDip(CameraMotion::LandingDrop(LandingSpeed, Tuning) * Share, Tuning.LandDropSeconds);
}

void UPlayerLocomotionComponent::CameraMotionJumped()
{
	APlayerController* Player = MotionPlayer(Character.Get());
	if (!Player)
	{
		return;
	}
	const CameraMotion::FTuning Tuning = CameraMotion::Live();
	FViewKick Kick = CameraMotion::JumpKick(Tuning) * SightsShare(PlayerView.Get(), Tuning);
	if (!Kick.IsNone())
	{
		PivotOnSight(Kick, PlayerView.Get(), Camera.Get(), Character.Get());
		UCameraShakeModifier::Kick(Player, Kick);
	}
}

void UPlayerLocomotionComponent::CameraMotionStepped(float DeltaTime)
{
	APlayerController* Player = MotionPlayer(Character.Get());
	if (!Player)
	{
		return;
	}
	const CameraMotion::FTuning Tuning = CameraMotion::Live();
	// The gun bob's own clock (StepPhase), so the nod and the gun's dip land together and never drift apart. Read every frame,
	// even when the nod is off, so it is current the moment it comes on.
	float Side = 0.f;
	if (!ViewMotion.StepFootfall(StepPhase, DeltaTime, CameraMotion::PeakSeconds(Tuning.StepHertz, Tuning.StepDamping), Side))
	{
		return;
	}
	FViewKick Kick = CameraMotion::StepKick(SprintAlpha, MoveWeight, Side, Tuning) * SightsShare(PlayerView.Get(), Tuning);
	if (!Kick.IsNone())
	{
		PivotOnSight(Kick, PlayerView.Get(), Camera.Get(), Character.Get());
		UCameraShakeModifier::Kick(Player, Kick);
	}
}

void UPlayerLocomotionComponent::HandleTraversalLanded(ETraversalKind Kind, float /*Height*/)
{
	APlayerController* Player = MotionPlayer(Character.Get());
	if (!Player)
	{
		return;
	}
	// The eye already gives through a move and rises back (FPlayerTraversal); the view nods as the feet meet the top or the floor.
	const CameraMotion::FTuning Tuning = CameraMotion::Live();
	FViewKick Kick = CameraMotion::TraversalKick(Kind == ETraversalKind::Vault, Tuning) * SightsShare(PlayerView.Get(), Tuning);
	if (!Kick.IsNone())
	{
		PivotOnSight(Kick, PlayerView.Get(), Camera.Get(), Character.Get());
		UCameraShakeModifier::Kick(Player, Kick);
	}
}
