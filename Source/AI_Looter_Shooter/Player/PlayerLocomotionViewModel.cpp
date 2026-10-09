#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

// UPlayerLocomotionComponent's first-person motion: the eye dropping into a crouch or a slide and the slide's tip of the
// view, on eased curves that nothing the capsule does can jolt, and the camera-held gun's stance poses, bob, sway and
// kicks.

namespace
{
	// View-model tuning (camera space: X forward, Y right, Z up; cm and degrees).
	const FVector SprintPoseOffset(-4.f, -5.f, -7.f);
	const FRotator SprintPoseRotation(-12.f, -32.f, -22.f);
	const FVector CrouchPoseOffset(-1.f, -1.5f, -1.f);
	constexpr float CrouchPoseRoll = -6.f;
	/** The point the stance poses turn the gun around, from its grip: a little ahead of and above the hand. */
	const FVector GripPivotFromGrip(9.f, 0.f, 2.f);
	/** Ground covered per step at walking and sprinting pace, for the full-size body (the bob follows the feet). */
	constexpr float WalkStepLength = 190.f;
	constexpr float SprintStepLength = 250.f;
	constexpr float KickStiffness = 170.f;
	constexpr float KickDamping = 18.f;
	/** Where a freshly drawn gun starts, low and tipped, before it comes up over its ready time. */
	const FVector DrawPoseOffset(-3.f, 2.f, -16.f);
	const FRotator DrawPoseRotation(-35.f, 8.f, 18.f);
	/**
	 * Aiming: how far in front of the eye the sight's aim point sits. The point goes exactly on the line of sight: the
	 * first-person render (its own field of view and scale) moves anything off the camera's axis, so only points on
	 * it stay on the crosshair, and a scope's reticle lines up only when its axis is the camera's.
	 */
	constexpr float AimSightDistance = 22.f;
	/** How much of the walking bob and look sway aiming takes out. */
	constexpr float AimSteadiness = 0.85f;
	/** While the inventory or pause menu is open the gun is lowered out of view, so it doesn't show through the panels. */
	const FVector MenuPoseOffset(-4.f, 3.f, -30.f);
	const FRotator MenuPoseRotation(-30.f, 0.f, 10.f);
	constexpr float MenuBlendTime = 0.2f;
	/** A slide sits the eye this much lower than a crouch (full-size cm): about seated on the ground. */
	constexpr float SlideEyeDrop = 22.f;
	/** Degrees a slide tips the first-person view (and the gun with it). */
	constexpr float SlideViewRoll = 5.f;

	// How long the eye takes to reach each height (s), on its minimum-jerk curve. Quick enough to sell the drop, never a
	// snap: the slide's 57 cm drop tops out at about 6 cm a frame at 60 fps and eases in and out of it.
	constexpr float SlideDropSeconds = 0.28f;
	constexpr float CrouchEyeSeconds = 0.27f;
	constexpr float StandEyeSeconds = 0.3f;
	/**
	 * As the slide eases out the eye starts rising to the crouch's height, slowly enough that it's still on its way as the
	 * slide ends: standing up out of it then carries straight on, with no stop between.
	 */
	constexpr float SlideRiseSeconds = 0.45f;
	/** Back to its height after the feet jumped under it (a crouch or stand in the air). */
	constexpr float FeetJumpSeconds = 0.25f;
	/** The eye's acceleration at most (cm/s^2): a move that would need a harder turn (a slide cancelled mid-drop) is stretched. */
	constexpr float MaxEyeAcceleration = 4500.f;
	/** How fast the measured rest height of the camera over its rig is followed (it only drifts with the arms' pose). */
	constexpr float EyeRestFollowRate = 4.f;
	constexpr float RollInSeconds = 0.3f;
	constexpr float RollOutSeconds = 0.35f;
	constexpr float MaxRollAcceleration = 400.f;
	/**
	 * The gun rides the camera on a spring (the kick spring): the eye's own acceleration pushes it, so it lags a little
	 * as the view drops or rises and settles after, which is the weight of a stance change without a jolt.
	 */
	constexpr float EyeKickCoupling = 0.1f;
	/**
	 * The climbing pose (a mantle, or 0.6 of it in a vault): the gun drops and turns in, out of the hands' way, so the
	 * climb reads without an animation and the muzzle never pokes into the ledge.
	 */
	const FVector ClimbPoseOffset(-3.f, 2.f, -9.f);
	const FRotator ClimbPoseRotation(-14.f, 8.f, 14.f);
}

void UPlayerLocomotionComponent::RefreshBodyTransform()
{
	ACharacter* Owner = Character.Get();
	USkeletalMeshComponent* Body = Owner ? Owner->GetMesh() : nullptr;
	const USceneComponent* Parent = Body ? Body->GetAttachParent() : nullptr;
	if (!Parent || Body->IsUsingAbsoluteLocation())
	{
		return;
	}
	// Where its relative location puts it, against where it is. They differ after the engine's crouch or stand until the
	// capsule next moves: a one-frame pop of a quarter metre as a slide stood up into a sprint, and a body (and view) left
	// floating or sunk while the player stood still.
	const FVector Expected = Parent->GetSocketTransform(Body->GetAttachSocketName()).TransformPosition(Body->GetRelativeLocation());
	if (!Body->GetComponentLocation().Equals(Expected, 0.05))
	{
		Body->UpdateComponentToWorld(EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
	}
}

float UPlayerLocomotionComponent::GetViewLowering() const
{
	const float CrouchDrop = StandingEye - GetCrouchedHeadHeight() * GetBodyScale();
	if (!bHaveEye || CrouchDrop < 1.f)
	{
		return CrouchAlpha;
	}
	return (StandingEye - GetEyeHeight()) / CrouchDrop;
}

void UPlayerLocomotionComponent::UpdateCamera(float DeltaTime)
{
	UCameraComponent* View = Camera.Get();
	USceneComponent* Rig = EyeRig.Get();
	const ACharacter* Owner = Character.Get();
	if (!View || !Rig || !Owner)
	{
		return;
	}
	const float Dt = FMath::Min(DeltaTime, 0.1f);
	const float Scale = GetBodyScale();
	const double Feet = GetFeetHeight();
	const float HalfHeight = Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// The capsule changed size since the eye last looked, by the movement's crouch or stand. On the ground the engine
	// keeps the feet where they were; in the air it keeps the capsule's middle, so the feet jump under the eye (tucked up,
	// or dropped) and the eye's height above them takes the opposite jump, which leaves the view where it was. (A resize
	// this component did itself, StandUpNow, noted its own jump.)
	if (LastHalfHeight > 0.f && !FMath::IsNearlyEqual(HalfHeight, LastHalfHeight, 0.01f) && !bLastResizeKeptFeet)
	{
		PendingFeetJump += LastHalfHeight - HalfHeight;
	}
	LastHalfHeight = HalfHeight;
	bLastResizeKeptFeet = Movement->bCrouchMaintainsBaseLocation;

	// The eye with nothing lowered, measured afresh: the rig's parent (the body, put in place by RefreshBodyTransform)
	// above the feet, plus how high the camera sits over the rig's parent once the rig's current lowering is taken off.
	// Whatever the capsule and the body did this frame is in it, so the camera lands exactly on the eye's curve.
	const USceneComponent* RigParent = Rig->GetAttachParent();
	const float ParentScale = RigParent ? FMath::Max(static_cast<float>(RigParent->GetComponentScale().Z), UE_KINDA_SMALL_NUMBER) : 1.f;
	const double ParentHeight = RigParent ? RigParent->GetSocketLocation(Rig->GetAttachSocketName()).Z : Feet;
	const float Lowered = static_cast<float>(EyeRigBaseLocation.Z - Rig->GetRelativeLocation().Z) * ParentScale;
	const float Rest = static_cast<float>(View->GetComponentLocation().Z - ParentHeight) + Lowered;
	EyeRest = bHaveEye ? FMath::FInterpTo(EyeRest, Rest, Dt, EyeRestFollowRate) : Rest;
	const float Natural = static_cast<float>(ParentHeight - Feet) + EyeRest;
	if (!Owner->bIsCrouched || !bHaveEye)
	{
		StandingEye = Natural;
	}

	if (Traversal.IsActive() && bHaveEye)
	{
		// Through a mantle or vault the eye rides its own world path (FPlayerTraversal), smooth however sharply the capsule
		// hops under it: its height over the feet is whatever that leaves. Its own curve takes over again as the move ends.
		TraversalEyeHeight = Traversal.GetEyeZ() - static_cast<float>(Feet);
		PendingFeetJump = 0.f;
	}
	else
	{
		// Where the eye heads: down to the slide's height while it holds its speed, back up to the crouch's as it eases out,
		// the crouch's while crouched, else standing. Heights are the full-size body's, shrunk by the character's scale.
		const float Crouched = GetCrouchedHeadHeight() * Scale;
		float Target = StandingEye;
		float Seconds = StandEyeSeconds;
		if (Slide.IsActive())
		{
			Target = Slide.IsEasingOut() ? Crouched : Crouched - SlideEyeDrop * Scale;
			Seconds = Slide.IsEasingOut() ? SlideRiseSeconds : SlideDropSeconds;
		}
		else if (Owner->bIsCrouched)
		{
			Target = Crouched;
			Seconds = CrouchEyeSeconds;
		}
		if (!bHaveEye)
		{
			EyeHeight.Reset(Target);
			bHaveEye = true;
		}
		// The standing height is re-measured all the time: a drift of a few millimetres doesn't start a new move.
		if (!FMath::IsNearlyEqual(Target, EyeHeight.GetTarget(), 0.25f))
		{
			EyeHeight.SetTarget(Target, Seconds, MaxEyeAcceleration);
		}
		if (!FMath::IsNearlyZero(PendingFeetJump, 0.01f))
		{
			EyeHeight.Shift(-PendingFeetJump, FeetJumpSeconds, MaxEyeAcceleration);
		}
		PendingFeetJump = 0.f;
		EyeHeight.Advance(Dt);
	}

	// Lower the rig (the arms the camera rides, or the camera itself) so the camera sits at the eye's height. The drop is
	// in world cm, scaled into the rig's parent's space. Every height the eye heads for is under the capsule's top (the
	// crouch's keeps CrouchHeadClearance), so a low ceiling the capsule fits under never cuts the view; on its way down
	// into a crouch it's where the standing capsule just was, which was clear. (A traversal's eye stays under the top too:
	// its plan checks it.)
	const FVector Wanted = EyeRigBaseLocation - FVector(0.f, 0.f, (Natural - GetEyeHeight()) / ParentScale);
	if (!Rig->GetRelativeLocation().Equals(Wanted, 0.01f))
	{
		Rig->SetRelativeLocation(Wanted);
	}

	// A slide tips the view a little, eased in as it drops and out as it slows: an offset on the camera itself, so the
	// controller's rotation (the aim) stays level.
	const bool bTipped = Slide.IsActive() && !Slide.IsEasingOut();
	ViewRoll.SetTarget(bTipped ? SlideViewRoll : 0.f, bTipped ? RollInSeconds : RollOutSeconds, MaxRollAcceleration);
	ViewRoll.Advance(Dt);
	const float Roll = ViewRoll.GetValue();
	if (!FMath::IsNearlyEqual(Roll, AppliedViewRoll, 0.01f))
	{
		View->ClearAdditiveOffset();
		if (!FMath::IsNearlyZero(Roll, 0.01f))
		{
			View->AddAdditiveOffset(FTransform(FRotator(0.f, 0.f, Roll)), 0.f);
		}
		AppliedViewRoll = Roll;
	}
}

void UPlayerLocomotionComponent::HandleLanded(const FHitResult& Hit)
{
	// Harder landings push the gun down further.
	KickVelocity -= FMath::Clamp(FallSpeed / 12.f, 10.f, 90.f);
	FallSpeed = 0.f;
}

void UPlayerLocomotionComponent::UpdateViewModel(float DeltaTime)
{
	const ACharacter* Owner = Character.Get();
	const UCharacterMovementComponent* Move = Movement.Get();
	const float Dt = FMath::Min(DeltaTime, 0.05f);

	// Jumps and falls feed the kick spring.
	const bool bFalling = Move->IsFalling();
	if (bFalling)
	{
		FallSpeed = FMath::Max(FallSpeed, -Owner->GetVelocity().Z);
		if (!bWasFalling && Owner->GetVelocity().Z > 50.f)
		{
			KickVelocity -= 18.f; // the gun lags as you jump
		}
	}
	bWasFalling = bFalling;
	// The eye's own easing pushes it too: dropping into a crouch or a slide, the gun lags a touch high and settles; rising,
	// a touch low; heaved up a ledge, it lags low and swings up over the top. (The eye's acceleration never jumps, so
	// neither does this.)
	const float EyeAcceleration = Traversal.IsActive() ? Traversal.GetEyeAcceleration() : EyeHeight.GetAcceleration();
	KickVelocity -= EyeAcceleration * EyeKickCoupling * Dt;
	KickVelocity += (-KickStiffness * KickOffset - KickDamping * KickVelocity) * Dt;
	KickOffset += KickVelocity * Dt;

	// The gun lags behind fast mouse movement and springs back.
	const FRotator Control = Owner->GetControlRotation();
	const FRotator Turn = (Control - LastControlRotation).GetNormalized();
	LastControlRotation = Control;
	const float TurnScale = 0.012f / FMath::Max(Dt, UE_KINDA_SMALL_NUMBER);
	const FVector2D SwayTarget(FMath::Clamp(-Turn.Yaw * TurnScale, -4.f, 4.f), FMath::Clamp(-Turn.Pitch * TurnScale, -3.f, 3.f));
	LookSway = FMath::Vector2DInterpTo(LookSway, SwayTarget, Dt, 9.f);

	// Step cycle: one dip per footstep, one side-to-side sway per stride. A smaller body takes shorter steps; a slide
	// takes none.
	const float Speed = Owner->GetVelocity().Size2D() * (1.f - SlideAlpha);
	const bool bGrounded = Move->IsMovingOnGround();
	const float StepLength = FMath::Lerp(WalkStepLength, SprintStepLength, SprintAlpha) * FMath::Lerp(1.f, 0.75f, CrouchAlpha) * GetBodyScale();
	if (bGrounded)
	{
		StepPhase = FMath::Fmod(StepPhase + Dt * Speed / StepLength * UE_PI, 2.f * UE_PI);
	}
	MoveWeight = FMath::FInterpTo(MoveWeight, bGrounded ? FMath::Clamp(Speed / FMath::Max(BaseWalkSpeed, 1.f), 0.f, 1.6f) : 0.f, Dt, 6.f);
	IdleTime += Dt;

	const float Sprint = SprintAlpha;
	const float Crouch = CrouchAlpha;
	const float StepSide = FMath::Sin(StepPhase);
	const float StepDip = StepSide * StepSide;
	const float BobDepth = FMath::Lerp(0.5f, 1.3f, Sprint) * MoveWeight;
	const float BobSide = FMath::Lerp(0.7f, 2.f, Sprint) * MoveWeight;
	const float BobRoll = FMath::Lerp(0.6f, 3.f, Sprint) * MoveWeight;
	const float Breath = 1.f - FMath::Min(MoveWeight, 1.f);

	FVector Offset(0.f, StepSide * BobSide, -StepDip * BobDepth);
	FRotator Rotation(-StepDip * BobDepth * 0.6f, 0.f, StepSide * BobRoll);

	Offset.Z += FMath::Sin(IdleTime * 1.6f) * 0.15f * Breath;
	Rotation.Pitch += FMath::Sin(IdleTime * 1.6f + 0.6f) * 0.25f * Breath;

	Rotation.Yaw += LookSway.X;
	Rotation.Pitch += LookSway.Y;
	Offset.Y += LookSway.X * 0.35f;
	Offset.Z += LookSway.Y * 0.25f;

	Offset += SprintPoseOffset * Sprint + CrouchPoseOffset * Crouch + ClimbPoseOffset * TraversalAlpha;
	Rotation += SprintPoseRotation * Sprint + ClimbPoseRotation * TraversalAlpha;
	Rotation.Roll += CrouchPoseRoll * Crouch;

	Offset.Z += KickOffset;
	Rotation.Pitch += KickOffset * 0.8f;

	// Apply on top of the manager's hold offset, rotating around the grip rather than the back of the gun.
	const UWeaponManagerComponent* Manager = WeaponManager.Get();
	AWeaponBase* Weapon = Manager ? Manager->GetActiveWeapon() : nullptr;
	if (!Weapon || Weapon->GetAttachParentActor() != Owner || Manager->IsThirdPersonHold())
	{
		// In third person the gun is in the body's hands and moves with the animation instead.
		return;
	}

	// Reloading brings the gun in to work the magazine or pump (the weapon moves the part itself, on the same timeline).
	FVector ReloadOffset;
	FRotator ReloadRotation;
	LooterReload::ViewModelPose(Weapon->GetReloadPart(), Weapon->GetReloadProgress(), ReloadOffset, ReloadRotation);
	Offset += ReloadOffset;
	Rotation += ReloadRotation;

	// Recoil: each shot jumps the gun back toward you and flips its muzzle up, springing back into place.
	if (const UPlayerViewComponent* ViewComponent = PlayerView.Get())
	{
		Offset.X -= ViewComponent->GetKickBack();
		Rotation += ViewComponent->GetKickRotation();
	}

	// Lowered while a menu is open.
	const APlayerController* Player = Cast<APlayerController>(Owner->GetController());
	const ALooterHUD* Hud = Player ? Player->GetHUD<ALooterHUD>() : nullptr;
	MenuLinear = FMath::FInterpConstantTo(MenuLinear, Hud && Hud->IsMenuOpen() ? 1.f : 0.f, Dt, 1.f / MenuBlendTime);
	const float Menu = FMath::SmoothStep(0.f, 1.f, MenuLinear);
	Offset += MenuPoseOffset * Menu;
	Rotation += MenuPoseRotation * Menu;

	// A freshly drawn gun comes up from below over its ready time (quicker with better handling).
	const float Ready = FMath::InterpEaseOut(0.f, 1.f, Weapon->GetReadyAlpha(), 2.f);
	Offset += DrawPoseOffset * (1.f - Ready);
	Rotation += DrawPoseRotation * (1.f - Ready);

	// Aiming: the hold moves from the hip to straight in front of the eye, with the sight's aim point on the line of
	// sight; walking bob and look sway mostly settle so the sight stays on target.
	const float Aim = PlayerView.IsValid() ? PlayerView->GetAimAlpha() : 0.f;
	FTransform Hold = Manager->GetFirstPersonHold(Weapon);
	if (Aim > 0.f)
	{
		const FVector AimLocation = FVector(AimSightDistance, 0.f, 0.f) - Weapon->GetAimPoint() * Hold.GetScale3D();
		Hold.SetLocation(FMath::Lerp(Hold.GetLocation(), AimLocation, Aim));
		Hold.SetRotation(FQuat::Slerp(Hold.GetRotation(), FQuat::Identity, Aim));
		const float Settle = 1.f - AimSteadiness * Aim;
		Offset *= Settle;
		Rotation *= Settle;
	}

	FQuat FinalRotation = Rotation.Quaternion() * Hold.GetRotation();
	// The stance poses turn the gun around (roughly) its grip, wherever this gun has it.
	const FVector Pivot = (Weapon->GetGripPoint().IsZero() ? Manager->AttachGrip : Weapon->GetGripPoint()) + GripPivotFromGrip;
	FVector FinalLocation = Hold.GetLocation() + Offset + Hold.GetRotation().RotateVector(Pivot) - FinalRotation.RotateVector(Pivot);

	// The slide's tipped view (UpdateCamera) takes the gun with it, turning about the line of sight so a sight stays on it.
	if (!FMath::IsNearlyZero(AppliedViewRoll))
	{
		const FQuat Tilt = FRotator(0.f, 0.f, AppliedViewRoll).Quaternion();
		FinalLocation = Tilt.RotateVector(FinalLocation);
		FinalRotation = Tilt * FinalRotation;
	}
	Weapon->SetActorRelativeTransform(FTransform(FinalRotation, FinalLocation, Hold.GetScale3D()));
}
