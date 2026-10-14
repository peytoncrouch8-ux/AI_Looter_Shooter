#include "Player/FirstPersonHandsRig.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "ReferenceSkeleton.h"

// The hands themselves: each grip's finger curls (code, no animation assets), and FFirstPersonHandsRig's hands: their
// frames and grip points read off the skeleton, and the finger joints curled toward the palm.

namespace
{
	const TCHAR* const FingerNames[FHandFingers::Count] = { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") };
	const TCHAR* const HandSuffix[2] = { TEXT("l"), TEXT("r") };
	/** The mannequin's grip points (the middle of what a hand holds), which the third-person guns sit in too. */
	const TCHAR* const GripSockets[2] = { TEXT("HandGrip_L"), TEXT("HandGrip_R") };

	/** A hand without a grip socket: its grip point this share of the way out to the knuckles, and this far off the palm (cm). */
	constexpr float HoldAlongKnuckles = 0.7f;
	constexpr float HoldOffPalm = 2.2f;

	void SetFinger(FHandFingers& Hand, int32 Finger, float Knuckle, float Middle, float Tip)
	{
		Hand.Curl[Finger][0] = Knuckle;
		Hand.Curl[Finger][1] = Middle;
		Hand.Curl[Finger][2] = Tip;
	}

	/** Every finger but the thumb alike, each a little more bent than the one before it toward the pinky (as a real fist). */
	FHandFingers Fist(float Knuckle, float Middle, float Tip, float Spread, float ThumbKnuckle, float ThumbMiddle, float ThumbTip)
	{
		FHandFingers Hand;
		SetFinger(Hand, FHandFingers::Thumb, ThumbKnuckle, ThumbMiddle, ThumbTip);
		for (int32 Finger = 1; Finger < FHandFingers::Count; ++Finger)
		{
			const float More = Spread * static_cast<float>(Finger - 1);
			SetFinger(Hand, Finger, Knuckle + More, Middle + More, Tip);
		}
		return Hand;
	}
}

FHandFingers FHandFingers::Of(EHandGrip Grip)
{
	switch (Grip)
	{
	case EHandGrip::Trigger:
	{
		// Wrapped hard round a grip; the index finger's curl is the gun's (FGunHands::TriggerCurl), this is a stand-in.
		FHandFingers Hand = Fist(55.f, 75.f, 45.f, 2.5f, 20.f, 25.f, 20.f);
		SetFinger(Hand, IndexFinger, 30.f, 50.f, 25.f);
		return Hand;
	}
	case EHandGrip::Forend:
		return Fist(35.f, 45.f, 30.f, 3.5f, 10.f, 15.f, 10.f);
	case EHandGrip::Wrap:
		return Fist(30.f, 40.f, 30.f, 2.f, 15.f, 20.f, 15.f);
	case EHandGrip::Pinch:
	{
		// Thumb and index meet round the shell or the handle; the others tuck in out of the way.
		FHandFingers Hand = Fist(60.f, 75.f, 45.f, 5.f, 25.f, 25.f, 15.f);
		SetFinger(Hand, IndexFinger, 35.f, 45.f, 25.f);
		return Hand;
	}
	case EHandGrip::Support:
		return Fist(40.f, 55.f, 40.f, 2.f, 5.f, 10.f, 5.f);
	default:
		return Fist(5.f, 5.f, 5.f, 0.f, 5.f, 5.f, 5.f);
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
	// the other way round (they're mirror images).
	const int32 Knuckle = Arm.Fingers[2][0] != INDEX_NONE ? Arm.Fingers[2][0] : Arm.Fingers[1][0];
	const FVector FingersDir = Knuckle != INDEX_NONE ? (At(Knuckle) - Wrist).GetSafeNormal(UE_SMALL_NUMBER, Arm.LowerDir) : Arm.LowerDir;
	const int32 IndexBase = Arm.Fingers[1][0];
	const int32 OuterBase = Arm.Fingers[4][0] != INDEX_NONE ? Arm.Fingers[4][0] : Arm.Fingers[3][0];
	const FVector Across = IndexBase != INDEX_NONE && OuterBase != INDEX_NONE ? (At(IndexBase) - At(OuterBase)).GetSafeNormal(UE_SMALL_NUMBER, Arm.Hinge)
		: Arm.Hinge;
	FVector Palm = Side == EFirstPersonHand::Right ? FVector::CrossProduct(FingersDir, Across) : FVector::CrossProduct(Across, FingersDir);
	Palm = (Palm - FingersDir * FVector::DotProduct(FingersDir, Palm)).GetSafeNormal(UE_SMALL_NUMBER, Up);
	Arm.HandFrame = FirstPersonHands::Frame(FingersDir, Palm);

	// The grip point: the mesh's own socket when it has one on this hand, else out toward the knuckles and off the palm.
	const USkeletalMeshSocket* Socket = Mesh.FindSocket(GripSockets[SideIndex]);
	if (Socket && Skeleton.FindBoneIndex(Socket->BoneName) == Arm.Hand)
	{
		Arm.HoldLocal = Socket->RelativeLocation;
	}
	else
	{
		const float KnuckleDistance = Knuckle != INDEX_NONE ? static_cast<float>(FVector::Dist(At(Knuckle), Wrist)) : 9.f;
		const FVector Hold = Wrist + FingersDir * (KnuckleDistance * HoldAlongKnuckles) + Palm * HoldOffPalm;
		Arm.HoldLocal = RestSpace[Arm.Hand].InverseTransformPosition(Hold);
	}

	// Each joint curls about the line square to its own bone and to the palm's facing: turning about it carries the finger
	// toward the palm, whichever way the bone's own axes run.
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
			const FVector Axis = FVector::CrossProduct(Along.GetSafeNormal(UE_SMALL_NUMBER, FingersDir), Palm).GetSafeNormal(UE_SMALL_NUMBER, Across);
			Arm.CurlAxes[Finger][Joint] = RestSpace[Bone].GetRotation().UnrotateVector(Axis).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
		}
	}
}

FTransform FFirstPersonHandsRig::CurledLocal(int32 Bone, const FHandsRigTargets& Targets) const
{
	const int32 Slot = FingerSlot[Bone];
	const int32 Side = Slot / 15;
	const int32 Finger = (Slot % 15) / 3;
	const int32 Joint = Slot % 3;
	const float Degrees = Targets.Hands[Side].Fingers.Curl[Finger][Joint];
	FTransform Local = RestLocal[Bone];
	if (!FMath::IsNearlyZero(Degrees))
	{
		// About the joint's own axis, so the bones beyond it follow it round.
		Local.SetRotation((Local.GetRotation() * FQuat(Arms[Side].CurlAxes[Finger][Joint], FMath::DegreesToRadians(Degrees))).GetNormalized());
	}
	return Local;
}
