#include "Player/FirstPersonHandsRig.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "ReferenceSkeleton.h"

// The hands themselves: each grip's finger curls (code, no animation assets), and FFirstPersonHandsRig's hands: their
// frames and grip points read off the skeleton, and the finger joints curled toward the palm from straight (whatever
// bend the mesh rests in), the thumb swung in across the palm at its base.

namespace
{
	const TCHAR* const FingerNames[FHandFingers::Count] = { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") };
	const TCHAR* const HandSuffix[2] = { TEXT("l"), TEXT("r") };
	/** The mannequin's grip points (the middle of what a hand holds), which the third-person guns sit in too. */
	const TCHAR* const GripSockets[2] = { TEXT("HandGrip_L"), TEXT("HandGrip_R") };

	/**
	 * A hand without a grip socket (SK_FP_Arms): its grip point out along the knuckles' line by this share of the wrist's
	 * distance to the middle knuckle, off the palm (FirstPersonHands::GripOffPalm), and a little toward the pinky (cm):
	 * Intermediate/FPArms/FPArms_rig.json's GripPoint, measured in the hand's frame (6.2, 3.6 and 0.66 cm on its hand).
	 */
	constexpr float HoldAlongKnuckles = 0.645f;
	constexpr float HoldTowardPinky = 0.65f;
	/** The fingers' end joints rest bent this share of their middle joints (the skeleton has no fingertip to measure it by). */
	constexpr float EndJointRestShare = 0.6f;

	void SetFinger(FHandFingers& Hand, int32 Finger, float Knuckle, float Middle, float Tip)
	{
		Hand.Curl[Finger][0] = Knuckle;
		Hand.Curl[Finger][1] = Middle;
		Hand.Curl[Finger][2] = Tip;
	}

	/** Every finger but the thumb alike, each a little more bent than the one before it toward the pinky (as a real fist). */
	FHandFingers Fist(float Knuckle, float Middle, float Tip, float Spread, float ThumbSwing, float ThumbMiddle, float ThumbTip)
	{
		FHandFingers Hand;
		SetFinger(Hand, FHandFingers::Thumb, ThumbSwing, ThumbMiddle, ThumbTip);
		for (int32 Finger = 1; Finger < FHandFingers::Count; ++Finger)
		{
			const float More = Spread * static_cast<float>(Finger - 1);
			SetFinger(Hand, Finger, Knuckle + More, Middle + More, Tip);
		}
		return Hand;
	}

	/** The signed angle (degrees) from From to To about Axis. */
	float AngleAbout(const FVector& From, const FVector& To, const FVector& Axis)
	{
		return FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(From, To), Axis), FVector::DotProduct(From, To))));
	}
}

FHandFingers FHandFingers::Of(EHandGrip Grip)
{
	// From straight. The fingers round a grip close as the look sheet's did onto the guns (Tools/Blender/fp_arms_sheet.py:
	// closed till they touch); the thumb's first number swings it in across the palm.
	switch (Grip)
	{
	case EHandGrip::Trigger:
	{
		// Wrapped round a grip; the index finger's curl is the gun's (FGunHands::TriggerCurl), this is a stand-in.
		FHandFingers Hand = Fist(60.f, 80.f, 50.f, 3.f, 28.f, 15.f, 10.f);
		SetFinger(Hand, IndexFinger, 30.f, 50.f, 25.f);
		return Hand;
	}
	case EHandGrip::Forend:
		return Fist(40.f, 55.f, 35.f, 3.5f, 20.f, 10.f, 5.f);
	case EHandGrip::Wrap:
		return Fist(35.f, 45.f, 30.f, 2.f, 25.f, 15.f, 10.f);
	case EHandGrip::Pinch:
	{
		// Thumb and index meet round the shell or the handle; the others tuck in out of the way.
		FHandFingers Hand = Fist(65.f, 80.f, 50.f, 5.f, 35.f, 25.f, 15.f);
		SetFinger(Hand, IndexFinger, 35.f, 45.f, 25.f);
		return Hand;
	}
	case EHandGrip::Support:
		return Fist(45.f, 60.f, 40.f, 2.f, 18.f, 5.f, 5.f);
	default:
		return Fist(5.f, 5.f, 5.f, 0.f, 0.f, 5.f, 5.f);
	}
}

FHandFingers FHandFingers::Blend(const FHandFingers& From, const FHandFingers& To, float Alpha)
{
	FHandFingers Hand;
	for (int32 Finger = 0; Finger < Count; ++Finger)
	{
		for (int32 Joint = 0; Joint < Joints; ++Joint)
		{
			Hand.Curl[Finger][Joint] = FMath::Lerp(From.Curl[Finger][Joint], To.Curl[Finger][Joint], Alpha);
		}
	}
	return Hand;
}

void FFirstPersonHandsRig::BindHand(FArm& Arm, EFirstPersonHand Side, const FReferenceSkeleton& Skeleton, const USkeletalMesh& Mesh)
{
	const int32 SideIndex = FirstPersonHands::Index(Side);
	for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
	{
		for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
		{
			Arm.Fingers[Finger][Joint] = Skeleton.FindBoneIndex(FName(*FString::Printf(TEXT("%s_%02d_%s"), FingerNames[Finger], Joint + 1, HandSuffix[SideIndex])));
		}
	}
	auto At = [this](int32 Bone) { return RestSpace[Bone].GetLocation(); };
	const FVector Wrist = At(Arm.Hand);

	// The hand's frame at rest: wrist to the middle knuckle, and its palm, which faces the side the fingers curl to. The
	// line across the knuckles runs pinky to index; a right hand's palm is then knuckles-line cross across, a left hand's
	// the other way round (they're mirror images). On SK_FP_Arms this gives FPArms_rig.json's PalmNormal.
	const int32 Knuckle = Arm.Fingers[2][0] != INDEX_NONE ? Arm.Fingers[2][0] : Arm.Fingers[1][0];
	const FVector FingersDir = Knuckle != INDEX_NONE ? (At(Knuckle) - Wrist).GetSafeNormal(UE_SMALL_NUMBER, Arm.LowerDir) : Arm.LowerDir;
	const int32 IndexBase = Arm.Fingers[1][0];
	const int32 OuterBase = Arm.Fingers[4][0] != INDEX_NONE ? Arm.Fingers[4][0] : Arm.Fingers[3][0];
	FVector Across = IndexBase != INDEX_NONE && OuterBase != INDEX_NONE ? (At(IndexBase) - At(OuterBase)).GetSafeNormal(UE_SMALL_NUMBER, Arm.Hinge)
		: Arm.Hinge;
	FVector Palm = Side == EFirstPersonHand::Right ? FVector::CrossProduct(FingersDir, Across) : FVector::CrossProduct(Across, FingersDir);
	Palm = (Palm - FingersDir * FVector::DotProduct(FingersDir, Palm)).GetSafeNormal(UE_SMALL_NUMBER, Up);
	Arm.HandFrame = FirstPersonHands::Frame(FingersDir, Palm);
	// The knuckles' line square to the hand's facing, index-ward.
	Across = FVector::CrossProduct(Palm, FingersDir) * (Side == EFirstPersonHand::Right ? 1.0 : -1.0);

	// The grip point: the mesh's own socket when it has one on this hand, else out toward the knuckles and off the palm.
	const USkeletalMeshSocket* Socket = Mesh.FindSocket(GripSockets[SideIndex]);
	if (Socket && Skeleton.FindBoneIndex(Socket->BoneName) == Arm.Hand)
	{
		Arm.HoldLocal = Socket->RelativeLocation;
	}
	else
	{
		const float KnuckleDistance = Knuckle != INDEX_NONE ? static_cast<float>(FVector::Dist(At(Knuckle), Wrist)) : 9.f;
		const FVector Hold = Wrist + FingersDir * (KnuckleDistance * HoldAlongKnuckles) + Palm * FirstPersonHands::GripOffPalm - Across * HoldTowardPinky;
		Arm.HoldLocal = RestSpace[Arm.Hand].InverseTransformPosition(Hold);
	}

	// Each joint curls about the line square to its own bone and to the palm's facing: turning about it carries the finger
	// toward the palm, whichever way the bone's own axes run. How bent it rests is measured the same way, against the bone
	// before it (the finger's metacarpal, or the hand), so a curl asked for is from straight.
	for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
	{
		for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
		{
			const int32 Bone = Arm.Fingers[Finger][Joint];
			if (Bone == INDEX_NONE)
			{
				continue;
			}
			const int32 Next = Joint + 1 < FHandFingers::Joints ? Arm.Fingers[Finger][Joint + 1] : INDEX_NONE;
			const int32 Before = Joint > 0 ? Arm.Fingers[Finger][Joint - 1] : INDEX_NONE;
			FVector Along = At(Bone) - Wrist;
			if (Next != INDEX_NONE)
			{
				Along = At(Next) - At(Bone);
			}
			else if (Before != INDEX_NONE)
			{
				Along = At(Bone) - At(Before);
			}
			Along = Along.GetSafeNormal(UE_SMALL_NUMBER, FingersDir);
			const FVector Axis = FVector::CrossProduct(Along, Palm).GetSafeNormal(UE_SMALL_NUMBER, Across);
			Arm.CurlAxes[Finger][Joint] = RestSpace[Bone].GetRotation().UnrotateVector(Axis).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);

			// The rest bend: the knuckle against the metacarpal (or the hand's line), the middle joint against the first
			// bone; the end joint as a share of the middle one. The thumb's base swings from wherever it rests.
			float Rest = 0.f;
			if (Joint == 0 && Finger != FHandFingers::Thumb && Next != INDEX_NONE)
			{
				const int32 Parent = Parents[Bone];
				const FVector Previous = Parent != INDEX_NONE && Parent != Arm.Hand ? (At(Bone) - At(Parent)).GetSafeNormal(UE_SMALL_NUMBER, FingersDir) : FingersDir;
				Rest = AngleAbout(Previous, Along, Axis);
			}
			else if (Joint == 1 && Next != INDEX_NONE && Before != INDEX_NONE)
			{
				Rest = AngleAbout((At(Bone) - At(Before)).GetSafeNormal(UE_SMALL_NUMBER, Along), Along, Axis);
			}
			Arm.RestCurls[Finger][Joint] = FMath::Clamp(Rest, -30.f, 90.f);
		}
		Arm.RestCurls[Finger][2] = Arm.RestCurls[Finger][1] * EndJointRestShare;
	}
}

FTransform FFirstPersonHandsRig::CurledLocal(int32 Bone, const FHandsRigTargets& Targets) const
{
	const int32 Slot = FingerSlot[Bone];
	const int32 Side = Slot / 15;
	const int32 Finger = (Slot % 15) / 3;
	const int32 Joint = Slot % 3;
	const float Degrees = Targets.Hands[Side].Fingers.Curl[Finger][Joint] - Arms[Side].RestCurls[Finger][Joint];
	FTransform Local = RestLocal[Bone];
	if (!FMath::IsNearlyZero(Degrees))
	{
		// About the joint's own axis, so the bones beyond it follow it round.
		Local.SetRotation((Local.GetRotation() * FQuat(Arms[Side].CurlAxes[Finger][Joint], FMath::DegreesToRadians(Degrees))).GetNormalized());
	}
	return Local;
}
