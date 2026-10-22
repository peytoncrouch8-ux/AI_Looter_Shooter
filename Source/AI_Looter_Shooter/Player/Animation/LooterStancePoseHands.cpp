#include "Player/Animation/LooterStancePoseDetail.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Math/RotationMatrix.h"
#include "ReferenceSkeleton.h"

using namespace LooterStancePoseDetail;

// Either hand read off the reference skeleton, the way the left hand's grip reads its own (LooterStancePoseGrip.cpp): the
// hand's frame (wrist to middle knuckle, the palm's facing) to turn it by, and each finger joint's curl axis and rest bend
// to set its fingers by, from straight. The idle's rests use both (LooterStancePoseIdleHands.cpp).

namespace
{
	constexpr int32 FingerCount = 5;
	constexpr int32 JointCount = 3;
	constexpr int32 ThumbFinger = 0;
	/** The fingers' end joints rest bent this share of their middle joints (no fingertip bones to measure by). */
	constexpr float EndJointRestShare = 0.6f;

	struct FHandRig
	{
		bool bValid = false;
		/** The hand's frame in its bone's space: X from the wrist to the middle knuckle, Y out of the palm. */
		FQuat Frame = FQuat::Identity;
		int32 Index[FingerCount][JointCount] = {};
		FQuat RefTurn[FingerCount][JointCount];
		FVector Axis[FingerCount][JointCount];
		float Rest[FingerCount][JointCount] = {};
	};

	/** Both hands' finger bone names, made once (the rig is read several times a frame). */
	struct FHandNames
	{
		FName Joints[2][FingerCount][JointCount];

		FHandNames()
		{
			const TCHAR* const Fingers[FingerCount] = { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") };
			for (int32 Side = 0; Side < 2; ++Side)
			{
				for (int32 Finger = 0; Finger < FingerCount; ++Finger)
				{
					for (int32 Joint = 0; Joint < JointCount; ++Joint)
					{
						Joints[Side][Finger][Joint] = FName(*FString::Printf(TEXT("%s_%02d_%s"), Fingers[Finger], Joint + 1, Side ? TEXT("r") : TEXT("l")));
					}
				}
			}
		}
	};

	FName JointName(int32 Finger, int32 Joint, bool bRight)
	{
		static const FHandNames Names;
		return Names.Joints[bRight ? 1 : 0][Finger][Joint];
	}

	/** Bone's reference transform in Hand's space; false when Hand isn't above it. */
	bool InHand(const FReferenceSkeleton& Skeleton, int32 Bone, int32 Hand, FTransform& Out)
	{
		const TArray<FTransform>& RefPose = Skeleton.GetRefBonePose();
		Out = FTransform::Identity;
		for (int32 Index = Bone; Index != INDEX_NONE; Index = Skeleton.GetParentIndex(Index))
		{
			if (Index == Hand)
			{
				return true;
			}
			Out = Out * RefPose[Index];
		}
		return false;
	}

	float AngleAbout(const FVector& From, const FVector& To, const FVector& Axis)
	{
		return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(From, To), Axis), FVector::DotProduct(From, To))));
	}

	FHandRig ReadHand(const FReferenceSkeleton& Skeleton, bool bRight, bool bFingers)
	{
		FHandRig Rig;
		const int32 Hand = Skeleton.FindBoneIndex(bRight ? FName(TEXT("hand_r")) : FName(TEXT("hand_l")));
		if (Hand == INDEX_NONE)
		{
			return Rig;
		}
		FTransform Joint[FingerCount][JointCount];
		bool bHas[FingerCount][JointCount];
		for (int32 Finger = 0; Finger < FingerCount; ++Finger)
		{
			for (int32 Step = 0; Step < JointCount; ++Step)
			{
				Rig.Index[Finger][Step] = Skeleton.FindBoneIndex(JointName(Finger, Step, bRight));
				bHas[Finger][Step] = Rig.Index[Finger][Step] != INDEX_NONE && InHand(Skeleton, Rig.Index[Finger][Step], Hand, Joint[Finger][Step]);
				if (!bHas[Finger][Step])
				{
					Rig.Index[Finger][Step] = INDEX_NONE;
				}
			}
		}

		// The palm's facing from the line across the knuckles, pinky to index: a left hand's is that line cross the fingers'
		// way, a right hand's the other way round (mirror images). A hand without a pinky (Gauge's three fingers) uses its last.
		constexpr int32 IndexFinger = 1;
		constexpr int32 MiddleFinger = 2;
		const int32 Outer = bHas[4][0] ? 4 : 3;
		if (!bHas[MiddleFinger][0] || !bHas[IndexFinger][0] || !bHas[Outer][0])
		{
			return Rig;
		}
		const FVector Fingers = Joint[MiddleFinger][0].GetLocation().GetSafeNormal();
		const FVector Across = (Joint[IndexFinger][0].GetLocation() - Joint[Outer][0].GetLocation()).GetSafeNormal();
		const FVector Crossed = bRight ? FVector::CrossProduct(Fingers, Across) : FVector::CrossProduct(Across, Fingers);
		const FVector Palm = FVector::VectorPlaneProject(Crossed, Fingers).GetSafeNormal();
		if (Fingers.IsNearlyZero() || Palm.IsNearlyZero())
		{
			return Rig;
		}
		Rig.Frame = FRotationMatrix::MakeFromXY(Fingers, Palm).ToQuat();
		Rig.bValid = true;
		if (!bFingers)
		{
			return Rig;
		}

		// Each joint curls about the line square to its bone and the palm's facing (toward the palm whichever way the bone's
		// own axes run); its rest bend measured the same way against the bone before it.
		const TArray<FTransform>& RefPose = Skeleton.GetRefBonePose();
		const FVector KnuckleLine = FVector::CrossProduct(Fingers, Palm);
		for (int32 Finger = 0; Finger < FingerCount; ++Finger)
		{
			for (int32 Step = 0; Step < JointCount; ++Step)
			{
				if (!bHas[Finger][Step])
				{
					continue;
				}
				const FVector Here = Joint[Finger][Step].GetLocation();
				FVector Along = Fingers;
				if (Step + 1 < JointCount && bHas[Finger][Step + 1])
				{
					Along = Joint[Finger][Step + 1].GetLocation() - Here;
				}
				else if (Step > 0 && bHas[Finger][Step - 1])
				{
					Along = Here - Joint[Finger][Step - 1].GetLocation();
				}
				Along = Along.GetSafeNormal(UE_SMALL_NUMBER, Fingers);
				// The thumb lies across the side of the palm, so turning it toward the palm's facing swung it out, away from the
				// fingers (Looter.Pose.Idle.Curl): its axis is the other way round, so a bend carries it in across the palm.
				const FVector Toward = FVector::CrossProduct(Along, Palm).GetSafeNormal(UE_SMALL_NUMBER, KnuckleLine);
				const FVector Axis = Finger == ThumbFinger ? -Toward : Toward;
				const FQuat Turn = Joint[Finger][Step].GetRotation();
				Rig.Axis[Finger][Step] = Turn.UnrotateVector(Axis).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
				Rig.RefTurn[Finger][Step] = RefPose[Rig.Index[Finger][Step]].GetRotation();

				float Rest = 0.f;
				if (Step == 0 && Finger != ThumbFinger)
				{
					const int32 Parent = Skeleton.GetParentIndex(Rig.Index[Finger][Step]);
					FTransform Before;
					const FVector Previous = Parent != Hand && Parent != INDEX_NONE && InHand(Skeleton, Parent, Hand, Before)
						? (Here - Before.GetLocation()).GetSafeNormal(UE_SMALL_NUMBER, Fingers) : Fingers;
					Rest = AngleAbout(Previous, Along, Axis);
				}
				else if (Step == 1 && bHas[Finger][0])
				{
					Rest = AngleAbout((Here - Joint[Finger][0].GetLocation()).GetSafeNormal(UE_SMALL_NUMBER, Along), Along, Axis);
				}
				Rig.Rest[Finger][Step] = FMath::Clamp(Rest, -30.f, 90.f);
			}
			Rig.Rest[Finger][2] = Rig.Rest[Finger][1] * EndJointRestShare;
		}
		return Rig;
	}
}

bool LooterStancePoseDetail::HandTurnFor(const FBoneContainer& Bones, bool bRight, const FVector& Fingers, const FVector& Palm, FQuat& OutTurn)
{
	const FHandRig Rig = ReadHand(Bones.GetReferenceSkeleton(), bRight, /*bFingers*/ false);
	const FVector Way = Fingers.GetSafeNormal();
	const FVector Facing = FVector::VectorPlaneProject(Palm, Way).GetSafeNormal();
	if (!Rig.bValid || Way.IsNearlyZero() || Facing.IsNearlyZero())
	{
		return false;
	}
	// The bone turned so its own frame lands on the wanted one.
	OutTurn = (FRotationMatrix::MakeFromXY(Way, Facing).ToQuat() * Rig.Frame.Inverse()).GetNormalized();
	return true;
}

void LooterStancePoseDetail::CurlHand(FCompactPose& Pose, bool bRight, const FHandCurl& Curl, float Weight, float Flex)
{
	const float Share = FMath::Clamp(Weight, 0.f, 1.f);
	if (Share <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FBoneContainer& Bones = Pose.GetBoneContainer();
	const FHandRig Rig = ReadHand(Bones.GetReferenceSkeleton(), bRight, /*bFingers*/ true);
	if (!Rig.bValid)
	{
		return;
	}
	for (int32 Finger = 0; Finger < FingerCount; ++Finger)
	{
		for (int32 Step = 0; Step < JointCount; ++Step)
		{
			if (Rig.Index[Finger][Step] == INDEX_NONE)
			{
				continue;
			}
			const FCompactPoseBoneIndex Bone = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Rig.Index[Finger][Step]));
			if (!Bone.IsValid())
			{
				continue;
			}
			// From straight (the reference turn less its rest bend) to the rest's bend, a fist's spread toward the pinky, a
			// finger's own extra (a card's ripple), the flex on top; the thumb its own.
			float Degrees = Finger == ThumbFinger ? Curl.Thumb[Step]
				: Curl.Fingers[Step] + (Step < 2 ? Curl.Spread * static_cast<float>(Finger - 1) : 0.f) + Curl.PerFinger[Finger - 1] + Flex;
			Degrees = FMath::Max(Degrees, -15.f);
			const FQuat Rested = (Rig.RefTurn[Finger][Step] * FQuat(Rig.Axis[Finger][Step], FMath::DegreesToRadians(Degrees - Rig.Rest[Finger][Step]))).GetNormalized();
			FTransform& Local = Pose[Bone];
			Local.SetRotation(FQuat::Slerp(Local.GetRotation(), Rested, Share).GetNormalized());
		}
	}
}

void LooterStancePoseDetail::FlexHand(FCompactPose& Pose, bool bRight, float Degrees)
{
	if (FMath::Abs(Degrees) <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FBoneContainer& Bones = Pose.GetBoneContainer();
	const FHandRig Rig = ReadHand(Bones.GetReferenceSkeleton(), bRight, /*bFingers*/ true);
	if (!Rig.bValid)
	{
		return;
	}
	// The four fingers a little further toward the palm than the animation has them, about each joint's own axis.
	for (int32 Finger = 1; Finger < FingerCount; ++Finger)
	{
		for (int32 Step = 0; Step < JointCount; ++Step)
		{
			const FCompactPoseBoneIndex Bone = Rig.Index[Finger][Step] == INDEX_NONE ? FCompactPoseBoneIndex(INDEX_NONE)
				: Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Rig.Index[Finger][Step]));
			if (Bone.IsValid())
			{
				FTransform& Local = Pose[Bone];
				Local.SetRotation((Local.GetRotation() * FQuat(Rig.Axis[Finger][Step], FMath::DegreesToRadians(Degrees))).GetNormalized());
			}
		}
	}
}
