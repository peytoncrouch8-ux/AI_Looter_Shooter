#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "TwoBoneIK.h"

using namespace LooterStancePoseDetail;

// The stance pose's mantle and vault: a code pose, since the mannequin set has no climb animation. It only shows from
// behind (third person); in first person the body isn't drawn, only its shadow. Heights and offsets are the full-size
// mannequin's, in the mesh's component space. The left leg and hand lead throughout: the gun sits in the right hand, and a
// raised right knee would meet it.

void LooterStancePoseDetail::ApplyClimbLegs(FCSPose<FCompactPose>& Pose, const FLegBones (&Legs)[2], FCompactPoseBoneIndex Pelvis,
	const FLooterStanceInput& Stance, const FVector& Forward, const FVector& Right)
{
	const FLooterClimbInput& Climb = Stance.Climb;
	const FVector Up = FVector::UpVector;
	const FVector Hips = Pose.GetComponentSpaceTransform(Pelvis).GetLocation();

	for (const FLegBones& Leg : Legs)
	{
		FTransform Thigh = Pose.GetComponentSpaceTransform(Leg.Thigh);
		FTransform Calf = Pose.GetComponentSpaceTransform(Leg.Calf);
		FTransform Foot = Pose.GetComponentSpaceTransform(Leg.Foot);
		const FVector ThighAt = Thigh.GetLocation();
		const bool bLead = Leg.Side < 0.f;

		FVector FootTarget;
		FVector KneeTarget;
		if (Climb.bVault)
		{
			// Both knees up and forward, the shins folded under, the feet swung a little to the right (the planted hand is the
			// left): a hurdle over the obstacle.
			FootTarget = bLead ? Hips + Forward * 18.f + Right * -4.f - Up * 50.f : Hips + Forward * 12.f + Right * 10.f - Up * 56.f;
			KneeTarget = ThighAt + Forward * 48.f + Right * (Leg.Side * 12.f) + Up * (bLead ? 12.f : 8.f);
		}
		else if (bLead)
		{
			// The leading knee comes up to the ledge and the foot reaches for its top (as far as the hip lets it).
			const float Reach = FMath::Clamp(Climb.LedgeAhead, 16.f, 34.f);
			FootTarget = Hips + Forward * Reach + Right * -6.f;
			FootTarget.Z = FMath::Clamp(static_cast<double>(Climb.LedgeUp), Hips.Z - 56.0, Hips.Z - 14.0);
			KneeTarget = ThighAt + Forward * 45.f + Right * -12.f + Up * 18.f;
		}
		else
		{
			// The trailing leg hangs bent below the hips.
			FootTarget = Hips - Forward * 10.f + Right * 8.f - Up * 60.f;
			KneeTarget = ThighAt + Forward * 38.f + Right * 14.f - Up * 6.f;
		}

		AnimationCore::SolveTwoBoneIK(Thigh, Calf, Foot, KneeTarget, FootTarget,
			/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
		const FBoneTransform Chain[] = { FBoneTransform(Leg.Thigh, Thigh), FBoneTransform(Leg.Calf, Calf), FBoneTransform(Leg.Foot, Foot) };
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), Climb.Legs);
	}
}

void LooterStancePoseDetail::ApplyClimbHands(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FVector& Forward, const FVector& Right)
{
	const FLooterClimbInput& Climb = Stance.Climb;
	// The ledge lies along the move's heading, which a ledge met on a diagonal makes different from the way the body faces.
	const FVector Way = Climb.Heading.IsNearlyZero() ? Forward : Climb.Heading;
	const FVector Across = FVector::CrossProduct(FVector::UpVector, Way).GetSafeNormal();
	struct FArm
	{
		const TCHAR* Upper;
		const TCHAR* Lower;
		const TCHAR* Hand;
		float Side; // -1 left, +1 right
	};
	const FArm Arms[] = { { TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l"), -1.f }, { TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r"), 1.f } };

	for (const FArm& Arm : Arms)
	{
		// A gun is held in the right hand, which keeps it; a vault plants one hand only.
		if (Arm.Side > 0.f && (Stance.bAimWithTorso || Climb.bVault))
		{
			continue;
		}
		const FCompactPoseBoneIndex UpperArm = FindBone(Bones, Arm.Upper);
		const FCompactPoseBoneIndex LowerArm = FindBone(Bones, Arm.Lower);
		const FCompactPoseBoneIndex Hand = FindBone(Bones, Arm.Hand);
		if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
		{
			continue;
		}

		FTransform Shoulder = Pose.GetComponentSpaceTransform(UpperArm);
		FTransform Elbow = Pose.GetComponentSpaceTransform(LowerArm);
		FTransform Wrist = Pose.GetComponentSpaceTransform(Hand);

		// On the ledge's top (or the rail's) a shoulder's width apart; out of reach, the arm just points at it.
		FVector Target = Way * Climb.LedgeAhead + Across * (Arm.Side * 13.f);
		Target.Z = Climb.LedgeUp;
		// The elbows go out and down, the way they do on a ledge.
		const FVector ElbowHint = Elbow.GetLocation() + Right * (Arm.Side * 22.f) - FVector::UpVector * 10.f;

		AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Wrist, ElbowHint, Target,
			/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
		const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), Climb.Hands);
	}
}
