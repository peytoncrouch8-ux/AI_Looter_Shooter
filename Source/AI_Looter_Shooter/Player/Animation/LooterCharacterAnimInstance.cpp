#include "Player/Animation/LooterCharacterAnimInstance.h"
#include "AI_Looter_Shooter.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"

using namespace LooterStancePoseDetail;

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
		// A mantle or vault tells where its obstacle's top is as it starts; the hands reach for that.
		if (UPlayerLocomotionComponent* Found = Locomotion.Get())
		{
			Found->OnTraversalStarted.AddUObject(this, &ULooterCharacterAnimInstance::HandleTraversalStarted);
		}
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
	RefreshClimbInput();

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
	const UWeaponDefinition* HeldDefinition = Weapon ? Weapon->GetInstance().Definition.Get() : nullptr;
	StanceInput.HoldReach = HeldDefinition ? HeldDefinition->HoldReach : 0.f;

	// The hand socket the gun hangs from (the same one the weapon manager attaches to in third person).
	const FName HoldSocket = WeaponManager.IsValid() ? WeaponManager->ThirdPersonAttachSocket : NAME_None;
	StanceInput.HoldBone = Mesh && Weapon ? Mesh->GetSocketBoneName(HoldSocket) : NAME_None;
	StanceInput.bHandOnForegrip = Weapon && StanceInput.HoldBone != NAME_None && Mesh->DoesSocketExist(HoldSocket);
	StanceInput.HoldSocketLocal = StanceInput.bHandOnForegrip ? Mesh->GetSocketTransform(HoldSocket, RTS_ParentBoneSpace) : FTransform::Identity;
	const FQuat WeaponWorld = View.IsValid() ? View->GetHeldWeaponRotation() : (Pawn ? Pawn->GetActorQuat() : FQuat::Identity);
	StanceInput.WeaponRotation = Mesh ? Mesh->GetComponentQuat().Inverse() * WeaponWorld : FQuat::Identity;
	StanceInput.RecoilBack = Weapon && View.IsValid() ? View->GetKickBack() : 0.f;
	StanceInput.RecoilPitch = Weapon && View.IsValid() ? View->GetKickRotation().Pitch : 0.f;

	// A melee strike jabs the body (the stock or a fist) as the first-person view swings, on the strike's own clock.
	if (!Melee.IsValid() && Pawn)
	{
		Melee = Pawn->FindComponentByClass<UPlayerMeleeComponent>();
	}
	StanceInput.MeleeJab = Melee.IsValid() && Melee->IsSwinging() ? LooterStancePose::MeleeJab(Melee->GetSwingTime()) : 0.f;

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

void ULooterCharacterAnimInstance::NativeUninitializeAnimation()
{
	// This instance goes (a weapon change swaps the Anim Blueprint): stop listening to the player's climbs.
	if (UPlayerLocomotionComponent* Loco = Locomotion.Get())
	{
		Loco->OnTraversalStarted.RemoveAll(this);
	}
	Locomotion.Reset();
	Super::NativeUninitializeAnimation();
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
		LooterStancePose::Apply(Output.Pose, Stance);
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
