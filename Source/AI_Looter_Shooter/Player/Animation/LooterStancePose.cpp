#include "Player/Animation/LooterStancePose.h"
#include "AI_Looter_Shooter.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "TwoBoneIK.h"
#include <atomic>

using namespace LooterStancePoseDetail;

// The stance pose's body: lean, aim, crouch and slide. The weapon's part (recoil, a melee strike's jab, the left hand on
// the foregrip) is in LooterStancePoseWeapon.cpp, the mantle's and vault's in LooterStancePoseClimb.cpp.

bool LooterStancePose::Apply(FCompactPose& Output, const FLooterStanceInput& Stance)
{
	const FBoneContainer& Bones = Output.GetBoneContainer();
	const FCompactPoseBoneIndex Pelvis = FindBone(Bones, TEXT("pelvis"));
	const FCompactPoseBoneIndex Head = FindBone(Bones, TEXT("head"));
	const FCompactPoseBoneIndex Neck = FindBone(Bones, TEXT("neck_01"));
	const FCompactPoseBoneIndex Spine[] = { FindBone(Bones, TEXT("spine_01")), FindBone(Bones, TEXT("spine_03")), FindBone(Bones, TEXT("spine_05")) };
	const float SpineShare[] = { 0.45f, 0.35f, 0.2f };

	const FLegBones Legs[] = {
		{ FindBone(Bones, TEXT("thigh_l")), FindBone(Bones, TEXT("calf_l")), FindBone(Bones, TEXT("foot_l")), -1.f },
		{ FindBone(Bones, TEXT("thigh_r")), FindBone(Bones, TEXT("calf_r")), FindBone(Bones, TEXT("foot_r")), 1.f } };

	bool bHasLegs = true;
	for (const FLegBones& Leg : Legs)
	{
		bHasLegs &= Leg.Thigh.IsValid() && Leg.Calf.IsValid() && Leg.Foot.IsValid();
	}
	if (!Pelvis.IsValid() || !Head.IsValid() || !bHasLegs)
	{
		// A skeleton without mannequin bone names (or a LOD that strips them): leave the pose alone, say so once.
		static std::atomic<bool> bWarned = false;
		if (!bWarned.exchange(true))
		{
			UE_LOG(LogLooter, Warning, TEXT("Stance pose skipped: the skeleton lacks pelvis/head/leg bones."));
		}
		return false;
	}

	FCSPose<FCompactPose> Pose;
	Pose.InitPose(Output);

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
	// taking over from the crouch's forward lean as it comes in; a mantle or vault leans into the climb, taking over from
	// the sprint's (a vault is taken at a run, and the two together would fold the body in half).
	const float Lean = Stance.CrouchTorsoLean * Crouch * (1.f - Slide) + Stance.SprintTorsoLean * Stance.SprintAlpha * (1.f - Stance.Climb.Lean)
		+ Stance.SlideTorsoLean * Slide + Stance.Climb.TorsoLean * Stance.Climb.Lean;
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
			const FLegBones& Leg = Legs[Index];
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

	// A mantle or vault tucks the legs on top of whatever they were doing.
	if (Stance.Climb.Legs > UE_KINDA_SMALL_NUMBER)
	{
		ApplyClimbLegs(Pose, Legs, Pelvis, Stance, Forward, Right);
	}

	// A melee strike drives the right hand (and the gun in it) forward; the left hand finds the foregrip after it.
	if (!FMath::IsNearlyZero(Stance.MeleeJab))
	{
		ApplyMeleeJab(Pose, Bones, Stance, Forward, Right);
	}

	if (Stance.bHandOnForegrip)
	{
		// The kick moves the right hand (and the gun in it) first; the left hand then finds the foregrip where it went.
		if (Stance.RecoilBack > 0.01f || FMath::Abs(Stance.RecoilPitch) > 0.01f)
		{
			ApplyRecoil(Pose, Bones, Stance, Right);
		}
		ApplyLeftHandOnForegrip(Pose, Bones, Stance, Right);
	}

	// The hands last: they blend away from the foregrip toward the obstacle.
	if (Stance.Climb.Hands > UE_KINDA_SMALL_NUMBER)
	{
		ApplyClimbHands(Pose, Bones, Stance, Forward, Right);
	}

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Pose), Output);
	return true;
}
