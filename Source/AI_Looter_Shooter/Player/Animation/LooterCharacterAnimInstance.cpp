#include "Player/Animation/LooterCharacterAnimInstance.h"
#include "AI_Looter_Shooter.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "TwoBoneIK.h"
#include "UObject/UnrealType.h"
#include <atomic>

namespace
{
	FCompactPoseBoneIndex FindBone(const FBoneContainer& Bones, const TCHAR* Name)
	{
		FBoneReference Reference(Name);
		Reference.Initialize(Bones);
		return Reference.GetCompactPoseIndex(Bones);
	}

	void SetBone(FCSPose<FCompactPose>& Pose, FCompactPoseBoneIndex Bone, const FTransform& Transform)
	{
		const FBoneTransform Single[] = { FBoneTransform(Bone, Transform) };
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Single), 1.f);
	}

	/** Rotates a bone about its own pivot, in component space; its children follow. */
	void RotateBone(FCSPose<FCompactPose>& Pose, FCompactPoseBoneIndex Bone, const FQuat& Rotation)
	{
		if (Bone.IsValid())
		{
			FTransform Transform = Pose.GetComponentSpaceTransform(Bone);
			Transform.SetRotation(Rotation * Transform.GetRotation());
			SetBone(Pose, Bone, Transform);
		}
	}
}

// ---------------------------------------------------------------------------
// Game thread
// ---------------------------------------------------------------------------

void ULooterCharacterAnimInstance::RefreshStanceInput(float DeltaSeconds)
{
	const APawn* Pawn = TryGetPawnOwner();
	// Blueprint-added components don't exist yet when the mesh first initializes its animation, so keep looking.
	if (!Locomotion.IsValid() && Pawn)
	{
		Locomotion = Pawn->FindComponentByClass<UPlayerLocomotionComponent>();
	}

	const UPlayerLocomotionComponent* Loco = Locomotion.Get();
	CrouchAlpha = Loco ? Loco->GetCrouchAlpha() : 0.f;
	SprintAlpha = Loco ? Loco->GetSprintAlpha() : 0.f;
	SlideAlpha = Loco ? Loco->GetSlideAlpha() : 0.f;

	StanceInput.CrouchAlpha = CrouchAlpha;
	StanceInput.SprintAlpha = SprintAlpha;
	StanceInput.SlideAlpha = SlideAlpha;
	StanceInput.CrouchedHeadHeight = Loco ? Loco->GetCrouchedHeadHeight() : 120.f;
	StanceInput.CrouchTorsoLean = CrouchTorsoLean;
	StanceInput.SprintTorsoLean = SprintTorsoLean;
	StanceInput.CrouchHipsBack = CrouchHipsBack;
	StanceInput.MaxHipDrop = MaxHipDrop;
	StanceInput.KneeSplay = KneeSplay;
	StanceInput.SlideTorsoLean = SlideTorsoLean;
	StanceInput.SlideHipHeight = SlideHipHeight;
	StanceInput.SlideLegReach = SlideLegReach;

	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	BodyScale = Mesh ? FMath::Max(static_cast<float>(Mesh->GetComponentScale().Z), UE_KINDA_SMALL_NUMBER) : 1.f;
	if (Pawn && Mesh)
	{
		const FVector Forward = Mesh->GetComponentTransform().InverseTransformVectorNoScale(Pawn->GetActorForwardVector()).GetSafeNormal2D();
		StanceInput.Forward = Forward.IsNearlyZero() ? FVector::YAxisVector : Forward;
	}

	// Armed: aim the torso with the view and put the left hand on the gun actually held.
	if (!WeaponManager.IsValid() && Pawn)
	{
		WeaponManager = Pawn->FindComponentByClass<UWeaponManagerComponent>();
	}
	const AWeaponBase* Weapon = bHoldsWeapon && WeaponManager.IsValid() ? WeaponManager->GetActiveWeapon() : nullptr;
	if (!View.IsValid() && Pawn)
	{
		View = Pawn->FindComponentByClass<UPlayerViewComponent>();
	}
	StanceInput.bAimWithTorso = bHoldsWeapon && Pawn;
	// The player's aim is where they face, whichever way the camera looks; sprinting carries the gun low instead.
	const float AimPitch = View.IsValid() ? View->GetAimRotation().Pitch : (Pawn ? FRotator::NormalizeAxis(Pawn->GetBaseAimRotation().Pitch) : 0.f);
	StanceInput.AimPitch = FMath::Clamp(AimPitch, -MaxTorsoAimPitch, MaxTorsoAimPitch) * (1.f - SprintAlpha);
	StanceInput.WeaponGrip = Weapon ? Weapon->GetGripPoint() : FVector::ZeroVector;
	StanceInput.WeaponForegrip = Weapon ? Weapon->GetForegripPoint() : FVector::ZeroVector;

	// The hand socket the gun hangs from (the same one the weapon manager attaches to in third person).
	const FName HoldSocket = WeaponManager.IsValid() ? WeaponManager->ThirdPersonAttachSocket : NAME_None;
	StanceInput.HoldBone = Mesh && Weapon ? Mesh->GetSocketBoneName(HoldSocket) : NAME_None;
	StanceInput.bHandOnForegrip = Weapon && StanceInput.HoldBone != NAME_None && Mesh->DoesSocketExist(HoldSocket);
	StanceInput.HoldSocketLocal = StanceInput.bHandOnForegrip ? Mesh->GetSocketTransform(HoldSocket, RTS_ParentBoneSpace) : FTransform::Identity;
	const FQuat WeaponWorld = View.IsValid() ? View->GetHeldWeaponRotation() : (Pawn ? Pawn->GetActorQuat() : FQuat::Identity);
	StanceInput.WeaponRotation = Mesh ? Mesh->GetComponentQuat().Inverse() * WeaponWorld : FQuat::Identity;
	StanceInput.RecoilBack = Weapon && View.IsValid() ? View->GetKickBack() : 0.f;
	StanceInput.RecoilPitch = Weapon && View.IsValid() ? View->GetKickRotation().Pitch : 0.f;

	// No pawn (the loadout screen's stand-in): hold the gun it was given, standing still and aiming level.
	if (!Pawn && Mesh && StandaloneHold.IsSet() && bHoldsWeapon)
	{
		const FStandaloneHold& Hold = StandaloneHold.GetValue();
		const FVector Forward = Mesh->GetComponentTransform().InverseTransformVectorNoScale(Hold.Facing.GetForwardVector()).GetSafeNormal2D();
		StanceInput.Forward = Forward.IsNearlyZero() ? FVector::YAxisVector : Forward;
		StanceInput.bAimWithTorso = true;
		StanceInput.AimPitch = 0.f;
		StanceInput.WeaponGrip = Hold.Grip;
		StanceInput.WeaponForegrip = Hold.Foregrip;
		StanceInput.HoldBone = Mesh->GetSocketBoneName(Hold.Socket);
		StanceInput.bHandOnForegrip = StanceInput.HoldBone != NAME_None && Mesh->DoesSocketExist(Hold.Socket);
		StanceInput.HoldSocketLocal = StanceInput.bHandOnForegrip ? Mesh->GetSocketTransform(Hold.Socket, RTS_ParentBoneSpace) : FTransform::Identity;
		StanceInput.WeaponRotation = Mesh->GetComponentQuat().Inverse() * Hold.Facing;
	}

	// Reloads play the reload animation on the upper body, stretched to the weapon's actual reload time.
	const float ReloadProgress = Weapon ? Weapon->GetReloadProgress() : -1.f;
	StanceInput.ReloadAnimation = ReloadAnimation;
	StanceInput.ReloadWeight = ReloadAnimation && ReloadProgress >= 0.f ? Weapon->GetReloadBlend() : 0.f;
	StanceInput.ReloadTime = ReloadAnimation ? FMath::Max(ReloadProgress, 0.f) * ReloadAnimation->GetPlayLength() : 0.f;

	// The held upper-body pose loops on its own clock (breathing, small sway), independent of the legs.
	StanceInput.UpperBodyPose = bHoldsWeapon ? UpperBodyPose.Get() : nullptr;
	if (StanceInput.UpperBodyPose)
	{
		const float Length = FMath::Max(static_cast<float>(StanceInput.UpperBodyPose->GetPlayLength()), UE_KINDA_SMALL_NUMBER);
		UpperBodyTime = FMath::Fmod(UpperBodyTime + DeltaSeconds, Length);
	}
	StanceInput.UpperBodyTime = UpperBodyTime;
}

void ULooterCharacterAnimInstance::SetStandaloneHold(const FVector& Grip, const FVector& Foregrip, FName HoldSocket, const FQuat& Facing)
{
	StandaloneHold = FStandaloneHold{ Grip, Foregrip, HoldSocket, Facing };
}

void ULooterCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	const FNumericProperty* Property = CastField<FNumericProperty>(GetClass()->FindPropertyByName(GroundSpeedVariable));
	GroundSpeedProperty = Property && Property->IsFloatingPoint() ? Property : nullptr;
	WrittenGroundSpeed = -1.0;
	UE_CLOG(!GroundSpeedProperty, LogLooter, Verbose, TEXT("%s has no float %s: its blend spaces get the speed in world units."),
		*GetClass()->GetName(), *GroundSpeedVariable.ToString());
}

void ULooterCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	if (!GroundSpeedProperty || FMath::IsNearlyEqual(BodyScale, 1.f))
	{
		return;
	}
	// Only a value the event graph wrote this update, so a frame it skipped can't be divided twice.
	void* Value = GroundSpeedProperty->ContainerPtrToValuePtr<void>(this);
	const double Speed = GroundSpeedProperty->GetFloatingPointPropertyValue(Value);
	if (Speed != WrittenGroundSpeed)
	{
		GroundSpeedProperty->SetFloatingPointPropertyValue(Value, Speed / BodyScale);
		// Read back: a single-precision variable rounds what it's given.
		WrittenGroundSpeed = GroundSpeedProperty->GetFloatingPointPropertyValue(Value);
	}
}

FAnimInstanceProxy* ULooterCharacterAnimInstance::CreateAnimInstanceProxy()
{
	return new FLooterCharacterAnimInstanceProxy(this);
}

void ULooterCharacterAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FLooterCharacterAnimInstanceProxy*>(InProxy);
}

// ---------------------------------------------------------------------------
// Animation thread
// ---------------------------------------------------------------------------

void FLooterCharacterAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	// Still on the game thread: gather this frame's stance (the locomotion component has already ticked) and copy it over.
	ULooterCharacterAnimInstance* Instance = CastChecked<ULooterCharacterAnimInstance>(InAnimInstance);
	Instance->RefreshStanceInput(DeltaSeconds);
	Stance = Instance->GetStanceInput();
}

bool FLooterCharacterAnimInstanceProxy::Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode)
{
	EvaluateAnimationNode_WithRoot(Output, InRootNode);

	// Only the final output of the main graph (not linked layers) gets the stance on top.
	if (InRootNode == GetRootNode() && Stance.NeedsLayer())
	{
		if (Stance.UpperBodyPose)
		{
			ApplyUpperBodyPose(Output);
		}
		if (Stance.ReloadWeight > UE_KINDA_SMALL_NUMBER && Stance.ReloadAnimation)
		{
			ApplyReloadOverlay(Output);
		}
		ApplyStance(Output);
	}
	return true;
}

void FLooterCharacterAnimInstanceProxy::ApplyUpperBodyPose(FPoseContext& Output) const
{
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const FCompactPoseBoneIndex UpperBodyRoot = FindBone(Bones, TEXT("spine_01"));
	if (!UpperBodyRoot.IsValid())
	{
		return;
	}

	FCompactPose HeldPose;
	HeldPose.SetBoneContainer(&Bones);
	FBlendedCurve HeldCurve;
	HeldCurve.InitFrom(Output.Curve);
	UE::Anim::FStackAttributeContainer HeldAttributes;
	FAnimationPoseData HeldData(HeldPose, HeldCurve, HeldAttributes);
	Stance.UpperBodyPose->GetAnimationPose(HeldData, FAnimExtractContext(static_cast<double>(Stance.UpperBodyTime)));

	// Where the held pose points the base of the spine relative to the whole mesh, not the hips (they sway every step).
	FCSPose<FCompactPose> HeldInMesh;
	HeldInMesh.InitPose(HeldPose);
	const FQuat SpineInMesh = HeldInMesh.GetComponentSpaceTransform(UpperBodyRoot).GetRotation();

	// The spine and everything above it take the held pose...
	for (const FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
	{
		if (Bone == UpperBodyRoot || Bones.BoneIsChildOf(Bone, UpperBodyRoot))
		{
			Output.Pose[Bone] = HeldPose[Bone];
		}
	}

	// ...with the base of the spine held in mesh space, so the chest and gun ride the steps without rocking with the hips.
	FCSPose<FCompactPose> Pose;
	Pose.InitPose(Output.Pose);
	FTransform Spine = Pose.GetComponentSpaceTransform(UpperBodyRoot);
	Spine.SetRotation(SpineInMesh);
	SetBone(Pose, UpperBodyRoot, Spine);
	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Pose), Output.Pose);
}

void FLooterCharacterAnimInstanceProxy::ApplyReloadOverlay(FPoseContext& Output) const
{
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const FCompactPoseBoneIndex UpperBodyRoot = FindBone(Bones, TEXT("spine_01"));
	if (!UpperBodyRoot.IsValid())
	{
		return;
	}

	FCompactPose ReloadPose;
	ReloadPose.SetBoneContainer(&Bones);
	FBlendedCurve ReloadCurve;
	ReloadCurve.InitFrom(Output.Curve);
	UE::Anim::FStackAttributeContainer ReloadAttributes;
	FAnimationPoseData ReloadData(ReloadPose, ReloadCurve, ReloadAttributes);
	Stance.ReloadAnimation->GetAnimationPose(ReloadData, FAnimExtractContext(static_cast<double>(Stance.ReloadTime)));

	// Layered per bone in local space: the spine and everything above it take the reload, the legs keep moving.
	for (const FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
	{
		if (Bone == UpperBodyRoot || Bones.BoneIsChildOf(Bone, UpperBodyRoot))
		{
			Output.Pose[Bone].BlendWith(ReloadPose[Bone], Stance.ReloadWeight);
		}
	}
}

void FLooterCharacterAnimInstanceProxy::ApplyStance(FPoseContext& Output) const
{
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const FCompactPoseBoneIndex Pelvis = FindBone(Bones, TEXT("pelvis"));
	const FCompactPoseBoneIndex Head = FindBone(Bones, TEXT("head"));
	const FCompactPoseBoneIndex Neck = FindBone(Bones, TEXT("neck_01"));
	const FCompactPoseBoneIndex Spine[] = { FindBone(Bones, TEXT("spine_01")), FindBone(Bones, TEXT("spine_03")), FindBone(Bones, TEXT("spine_05")) };
	const float SpineShare[] = { 0.45f, 0.35f, 0.2f };

	struct FLeg
	{
		FCompactPoseBoneIndex Thigh, Calf, Foot;
		float Side; // -1 left, +1 right
	};
	const FLeg Legs[] = {
		{ FindBone(Bones, TEXT("thigh_l")), FindBone(Bones, TEXT("calf_l")), FindBone(Bones, TEXT("foot_l")), -1.f },
		{ FindBone(Bones, TEXT("thigh_r")), FindBone(Bones, TEXT("calf_r")), FindBone(Bones, TEXT("foot_r")), 1.f } };

	bool bHasLegs = true;
	for (const FLeg& Leg : Legs)
	{
		bHasLegs &= Leg.Thigh.IsValid() && Leg.Calf.IsValid() && Leg.Foot.IsValid();
	}
	if (!Pelvis.IsValid() || !Head.IsValid() || !bHasLegs)
	{
		// A skeleton without mannequin bone names (or a LOD that strips them): leave the pose alone, say so once.
		static std::atomic<bool> bWarned = false;
		if (!bWarned.exchange(true))
		{
			UE_LOG(LogLooter, Warning, TEXT("Stance pose skipped: %s lacks pelvis/head/leg bones."), *GetNameSafe(GetSkelMeshComponent()));
		}
		return;
	}

	FCSPose<FCompactPose> Pose;
	Pose.InitPose(Output.Pose);

	const FVector Up = FVector::UpVector;
	const FVector Forward = Stance.Forward;
	const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal(); // rotating about this tips the torso forward
	const float Crouch = Stance.CrouchAlpha;
	const float Slide = Stance.SlideAlpha;

	// Feet stay where the graph planted them.
	FTransform FootTargets[UE_ARRAY_COUNT(Legs)];
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Legs); ++Index)
	{
		FootTargets[Index] = Pose.GetComponentSpaceTransform(Legs[Index].Foot);
	}

	// Torso lean, spread down the spine; the neck takes most of it back so the head stays upright. A slide leans back,
	// taking over from the crouch's forward lean as it comes in.
	const float Lean = Stance.CrouchTorsoLean * Crouch * (1.f - Slide) + Stance.SprintTorsoLean * Stance.SprintAlpha + Stance.SlideTorsoLean * Slide;
	if (!FMath::IsNearlyZero(Lean))
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Spine); ++Index)
		{
			RotateBone(Pose, Spine[Index], FQuat(Right, FMath::DegreesToRadians(Lean * SpineShare[Index])));
		}
		RotateBone(Pose, Neck, FQuat(Right, FMath::DegreesToRadians(-Lean * 0.7f)));
	}

	// Armed: the torso (and with it the arms and gun) pitches to where the player aims, mostly from the chest up.
	if (Stance.bAimWithTorso && !FMath::IsNearlyZero(Stance.AimPitch))
	{
		const float AimShare[] = { 0.2f, 0.35f, 0.45f };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Spine); ++Index)
		{
			RotateBone(Pose, Spine[Index], FQuat(Right, FMath::DegreesToRadians(-Stance.AimPitch * AimShare[Index])));
		}
	}

	if (Crouch > UE_KINDA_SMALL_NUMBER || Slide > UE_KINDA_SMALL_NUMBER)
	{
		// Hips drop until the head reaches the crouched head height (the capsule's top minus clearance); a slide sits them
		// down near the ground instead.
		const float HeadHeight = Pose.GetComponentSpaceTransform(Head).GetLocation().Z;
		const float CrouchDrop = FMath::Min(FMath::Max(HeadHeight - Stance.CrouchedHeadHeight, 0.f), Stance.MaxHipDrop) * Crouch;
		FTransform PelvisTransform = Pose.GetComponentSpaceTransform(Pelvis);
		const float SlideDrop = FMath::Max(static_cast<float>(PelvisTransform.GetLocation().Z) - Stance.SlideHipHeight, 0.f);
		const float Drop = FMath::Lerp(CrouchDrop, SlideDrop, Slide);
		PelvisTransform.AddToTranslation(-Up * Drop - Forward * (Stance.CrouchHipsBack * Crouch * (1.f - Slide)));
		SetBone(Pose, Pelvis, PelvisTransform);
		const FVector Hips = PelvisTransform.GetLocation();

		// Legs bend to reach the planted feet again, knees forward and a little outward. In a slide the right leg reaches
		// out ahead, heel near the ground, and the left folds under it with its knee out to the side (a code pose: no
		// slide animation exists).
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Legs); ++Index)
		{
			const FLeg& Leg = Legs[Index];
			FTransform Thigh = Pose.GetComponentSpaceTransform(Leg.Thigh);
			FTransform Calf = Pose.GetComponentSpaceTransform(Leg.Calf);
			FTransform Foot = Pose.GetComponentSpaceTransform(Leg.Foot);
			FVector KneeTarget = Calf.GetLocation() + Forward * 60.f + Right * (Leg.Side * Stance.KneeSplay);
			FVector FootTarget = FootTargets[Index].GetLocation();
			if (Slide > UE_KINDA_SMALL_NUMBER)
			{
				const bool bLeading = Leg.Side > 0.f;
				FVector SlideFoot = bLeading ? Hips + Forward * Stance.SlideLegReach + Right * 12.f : Hips + Forward * 22.f + Right * 4.f;
				SlideFoot.Z = bLeading ? 13.f : 10.f;
				const FVector SlideKnee = bLeading ? Thigh.GetLocation() + Forward * 50.f + Up * 25.f
					: Thigh.GetLocation() + Forward * 15.f + Right * (Leg.Side * 40.f) - Up * 10.f;
				FootTarget = FMath::Lerp(FootTarget, SlideFoot, Slide);
				KneeTarget = FMath::Lerp(KneeTarget, SlideKnee, Slide);
			}

			AnimationCore::SolveTwoBoneIK(Thigh, Calf, Foot, KneeTarget, FootTarget,
				/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
			Foot.SetRotation(FootTargets[Index].GetRotation());

			const FBoneTransform Chain[] = { FBoneTransform(Leg.Thigh, Thigh), FBoneTransform(Leg.Calf, Calf), FBoneTransform(Leg.Foot, Foot) };
			Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
		}
	}

	if (Stance.bHandOnForegrip)
	{
		// The kick moves the right hand (and the gun in it) first; the left hand then finds the foregrip where it went.
		if (Stance.RecoilBack > 0.01f || FMath::Abs(Stance.RecoilPitch) > 0.01f)
		{
			ApplyRecoil(Pose, Bones, Right);
		}
		ApplyLeftHandOnForegrip(Pose, Bones, Right);
	}

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Pose), Output.Pose);
}

void FLooterCharacterAnimInstanceProxy::ApplyRecoil(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FVector& Right) const
{
	// The chest rocks back with the kick, mostly high up. Seen from behind the character a gun's few cm of kick would be
	// lost, so the body sells it bigger than the first-person view does.
	const FCompactPoseBoneIndex Chest[] = { FindBone(Bones, TEXT("spine_03")), FindBone(Bones, TEXT("spine_05")) };
	const float ChestShare[] = { 0.4f, 0.6f };
	const float ChestPitch = FMath::Min(Stance.RecoilPitch * 0.8f + Stance.RecoilBack * 1.5f, 14.f);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Chest); ++Index)
	{
		RotateBone(Pose, Chest[Index], FQuat(Right, FMath::DegreesToRadians(-ChestPitch * ChestShare[Index])));
	}

	// The right arm gives: the hand is pushed back along the barrel, the elbow bending down and out.
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_r"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_r"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_r"));
	if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid() || Stance.RecoilBack <= 0.f)
	{
		return;
	}
	FTransform Shoulder = Pose.GetComponentSpaceTransform(UpperArm);
	FTransform Elbow = Pose.GetComponentSpaceTransform(LowerArm);
	FTransform Wrist = Pose.GetComponentSpaceTransform(Hand);
	const FQuat HandRotation = Wrist.GetRotation();
	const FVector Barrel = Stance.WeaponRotation.GetForwardVector();
	const FVector WristTarget = Wrist.GetLocation() - Barrel * FMath::Min(Stance.RecoilBack * 2.5f, 14.f);
	const FVector ElbowHint = Elbow.GetLocation() - FVector::UpVector * 15.f + Right * 10.f;

	AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Wrist, ElbowHint, WristTarget,
		/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
	Wrist.SetRotation(HandRotation);

	const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
}

void FLooterCharacterAnimInstanceProxy::ApplyLeftHandOnForegrip(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FVector& Right) const
{
	const FCompactPoseBoneIndex HoldBone = FindBone(Bones, *Stance.HoldBone.ToString());
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_l"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_l"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_l"));
	if (!HoldBone.IsValid() || !UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
	{
		return;
	}

	// The gun's grip sits in the right-hand socket and the gun points along the aim, so its foregrip is here:
	const FVector GripLocation = (Stance.HoldSocketLocal * Pose.GetComponentSpaceTransform(HoldBone)).GetLocation();
	const FVector Foregrip = GripLocation + Stance.WeaponRotation.RotateVector(Stance.WeaponForegrip - Stance.WeaponGrip);

	FTransform Shoulder = Pose.GetComponentSpaceTransform(UpperArm);
	FTransform Elbow = Pose.GetComponentSpaceTransform(LowerArm);
	FTransform Wrist = Pose.GetComponentSpaceTransform(Hand);

	// Aim the palm (the mannequin's HandGrip_L point) at the foregrip, keeping the animation's hand orientation.
	static const FVector PalmFromWrist(7.5f, -2.5f, 0.f);
	const FQuat HandRotation = Wrist.GetRotation();
	const FVector WristTarget = Foregrip - HandRotation.RotateVector(PalmFromWrist);
	// Keep the elbow bending down and out, the way it already is.
	const FVector ElbowHint = Elbow.GetLocation() - FVector::UpVector * 20.f + Right * -10.f;

	AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Wrist, ElbowHint, WristTarget,
		/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
	Wrist.SetRotation(HandRotation);

	// During a reload the left hand is busy with the magazine, so it lets go of the foregrip.
	const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f - Stance.ReloadWeight);
}
