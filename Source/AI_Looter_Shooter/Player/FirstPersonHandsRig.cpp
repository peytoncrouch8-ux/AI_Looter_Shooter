#include "Player/FirstPersonHandsRig.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "TwoBoneIK.h"

// FFirstPersonHandsRig's binding and arms: the skeleton read once, then each frame the arms solved to the hands'
// placements and every bone written against its parent. The hands' frames, grip points and fingers are in
// FirstPersonHandsFingers.cpp.

namespace
{
	const TCHAR* const SideSuffix[2] = { TEXT("l"), TEXT("r") };

	int32 FindSideBone(const FReferenceSkeleton& Skeleton, const TCHAR* Name, int32 Side)
	{
		return Skeleton.FindBoneIndex(FName(*FString::Printf(TEXT("%s_%s"), Name, SideSuffix[Side])));
	}
}

bool FFirstPersonHandsRig::FindArm(FArm& Arm, EFirstPersonHand Side, const FReferenceSkeleton& Skeleton) const
{
	const int32 SideIndex = FirstPersonHands::Index(Side);
	Arm = FArm();
	Arm.Clavicle = FindSideBone(Skeleton, TEXT("clavicle"), SideIndex);
	Arm.UpperArm = FindSideBone(Skeleton, TEXT("upperarm"), SideIndex);
	Arm.LowerArm = FindSideBone(Skeleton, TEXT("lowerarm"), SideIndex);
	Arm.Hand = FindSideBone(Skeleton, TEXT("hand"), SideIndex);
	for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
	{
		for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
		{
			Arm.Fingers[Finger][Joint] = INDEX_NONE;
			Arm.CurlAxes[Finger][Joint] = FVector::RightVector;
			Arm.RestCurls[Finger][Joint] = 0.f;
		}
		Arm.SpreadAxes[Finger] = FVector::UpVector;
		Arm.FingerBase[Finger] = FTransform::Identity;
		Arm.TipLocal[Finger] = FVector::ZeroVector;
	}
	return Arm.UpperArm != INDEX_NONE && Arm.LowerArm != INDEX_NONE && Arm.Hand != INDEX_NONE;
}

void FFirstPersonHandsRig::MeasureArm(FArm& Arm) const
{
	const FVector Shoulder = RestSpace[Arm.UpperArm].GetLocation();
	const FVector Elbow = RestSpace[Arm.LowerArm].GetLocation();
	const FVector Wrist = RestSpace[Arm.Hand].GetLocation();
	Arm.UpperLength = static_cast<float>(FVector::Dist(Shoulder, Elbow));
	Arm.LowerLength = static_cast<float>(FVector::Dist(Elbow, Wrist));
	Arm.UpperDir = (Elbow - Shoulder).GetSafeNormal(UE_SMALL_NUMBER, -Up);
	Arm.LowerDir = (Wrist - Elbow).GetSafeNormal(UE_SMALL_NUMBER, Arm.UpperDir);

	// The elbow's hinge: the axis about which bending it swings the forearm the way an elbow bends. Hanging, that's
	// forward; reaching ahead (a rest pose made holding something), up. The rest pose's own bend says it best when there
	// is one that agrees; a hinge taken backward would bend every elbow inside out.
	FVector Anatomical = FVector::CrossProduct(Arm.UpperDir, Forward);
	if (Anatomical.Size() < 0.3)
	{
		Anatomical = FVector::CrossProduct(Arm.UpperDir, Up);
	}
	Anatomical = Anatomical.GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
	const FVector Bend = FVector::CrossProduct(Arm.UpperDir, Arm.LowerDir);
	const bool bRestBends = Bend.Size() > 0.087 && FVector::DotProduct(Bend, Anatomical) > 0.0;
	Arm.Hinge = bRestBends ? Bend.GetSafeNormal() : Anatomical;
}

bool FFirstPersonHandsRig::Bind(const USkeletalMesh* Mesh, bool bArmsOnly)
{
	*this = FFirstPersonHandsRig();
	if (!Mesh)
	{
		return false;
	}
	const FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
	const int32 Count = Skeleton.GetNum();
	if (Count <= 0)
	{
		return false;
	}
	RestLocal = Skeleton.GetRefBonePose();
	Parents.SetNum(Count);
	RestSpace.SetNum(Count);
	for (int32 Bone = 0; Bone < Count; ++Bone)
	{
		// A skeleton lists every bone after its parent.
		Parents[Bone] = Skeleton.GetParentIndex(Bone);
		RestSpace[Bone] = Parents[Bone] != INDEX_NONE ? RestLocal[Bone] * RestSpace[Parents[Bone]] : RestLocal[Bone];
	}

	FArm& Left = Arms[FirstPersonHands::Index(EFirstPersonHand::Left)];
	FArm& Right = Arms[FirstPersonHands::Index(EFirstPersonHand::Right)];
	if (!FindArm(Left, EFirstPersonHand::Left, Skeleton) || !FindArm(Right, EFirstPersonHand::Right, Skeleton))
	{
		*this = FFirstPersonHandsRig();
		return false;
	}

	// The body's facing, whichever way the mesh was made: the mannequin faces its +Y, the first-person arms their +X.
	const FVector LeftShoulder = RestSpace[Left.UpperArm].GetLocation();
	const FVector RightShoulder = RestSpace[Right.UpperArm].GetLocation();
	FVector Across = LeftShoulder - RightShoulder;
	Across.Z = 0.0;
	const FVector Leftward = Across.GetSafeNormal(UE_SMALL_NUMBER, FVector(0.0, -1.0, 0.0));
	Up = FVector::UpVector;
	Forward = FVector::CrossProduct(Up, Leftward).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	ShoulderMiddle = (LeftShoulder + RightShoulder) * 0.5;

	for (int32 Side = 0; Side < 2; ++Side)
	{
		MeasureArm(Arms[Side]);
		BindHand(Arms[Side], Side == 0 ? EFirstPersonHand::Left : EFirstPersonHand::Right, Skeleton, *Mesh);
		if (Arms[Side].UpperLength < 1.f || Arms[Side].LowerLength < 1.f)
		{
			*this = FFirstPersonHandsRig();
			return false;
		}
	}

	// The stand-in keeps its arms (each clavicle and everything under it) and nothing else.
	Kept.Init(true, Count);
	if (bArmsOnly)
	{
		for (int32 Bone = 0; Bone < Count; ++Bone)
		{
			const bool bArmRoot = Bone == Left.Clavicle || Bone == Right.Clavicle
				|| (Left.Clavicle == INDEX_NONE && Bone == Left.UpperArm) || (Right.Clavicle == INDEX_NONE && Bone == Right.UpperArm);
			// Parents come first, so a parent's verdict is in by now.
			Kept[Bone] = bArmRoot || (Parents[Bone] != INDEX_NONE && Kept[Parents[Bone]]);
		}
	}

	FingerSlot.Init(-1, Count);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
		{
			for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
			{
				const int32 Bone = Arms[Side].Fingers[Finger][Joint];
				if (Bone != INDEX_NONE)
				{
					FingerSlot[Bone] = static_cast<int16>(Side * 15 + Finger * 3 + Joint);
				}
			}
		}
	}
	bBound = true;
	return true;
}

void FFirstPersonHandsRig::SolveArm(const FArm& Arm, const FHandPlacement& Target, const FVector& DefaultForearm,
	TArray<int32, TInlineAllocator<8>>& OutBones, TArray<FTransform, TInlineAllocator<8>>& OutSpaces) const
{
	using FirstPersonHands::Frame;
	const FTransform& HandRest = RestSpace[Arm.Hand];
	const FTransform& UpperRest = RestSpace[Arm.UpperArm];
	const FTransform& LowerRest = RestSpace[Arm.LowerArm];

	// The hand: its rest frame turned onto the placement's, and the wrist wherever that puts the grip point on its spot.
	const FQuat HandTurn = (Target.Turn * Arm.HandFrame.Inverse() * HandRest.GetRotation()).GetNormalized();
	const FVector Wrist = Target.Hold - HandTurn.RotateVector(Arm.HoldLocal * HandRest.GetScale3D());

	// The elbow goes where the forearm leaves the wrist the way the placement says (back, out and down: the wrist stays
	// nearly straight, the arm comes up from under the view). A view-model gun is held further out than a real arm reaches
	// from a real shoulder (the long guns' grip sits half a metre ahead of the eye, a pump nearly a metre): when that elbow
	// is out of the upper arm's reach the shoulder comes to it, far under the view's bottom edge. Only the forearm and
	// hand are ever seen, and the arm never stretches (a stretched sleeve shows).
	const FVector ForearmWay = (Target.Forearm.IsNearlyZero() ? DefaultForearm : Target.Forearm).GetSafeNormal(UE_SMALL_NUMBER, -Target.GetFingersDir());
	const FVector WantedElbow = Wrist + ForearmWay * Arm.LowerLength;
	const FVector RestShoulder = UpperRest.GetLocation();
	const FVector FromElbow = RestShoulder - WantedElbow;
	const double UpperReach = FromElbow.Size();
	FVector Shift = FVector::ZeroVector;
	if (UpperReach > Arm.UpperLength)
	{
		Shift = (WantedElbow + FromElbow / UpperReach * Arm.UpperLength - RestShoulder).GetClampedToMaxSize(MaxShoulderShift);
	}
	const FVector Shoulder = RestShoulder + Shift;
	FVector Elbow;
	FVector End;
	AnimationCore::SolveTwoBoneIK(Shoulder, LowerRest.GetLocation() + Shift, HandRest.GetLocation() + Shift, WantedElbow, Wrist, Elbow, End,
		Arm.UpperLength, Arm.LowerLength, /*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
	const FVector& ElbowHint = WantedElbow;

	// Each bone turned so it points where the solve puts it and its hinge lies across the arm's new bend: the elbow then
	// only ever bends about its own hinge, the way the skin was made to, whatever the reach.
	const FVector UpperDir = (Elbow - Shoulder).GetSafeNormal(UE_SMALL_NUMBER, Arm.UpperDir);
	const FVector LowerDir = (End - Elbow).GetSafeNormal(UE_SMALL_NUMBER, Arm.LowerDir);
	FVector Hinge = FVector::CrossProduct(UpperDir, LowerDir);
	if (Hinge.SizeSquared() < 1.e-4)
	{
		Hinge = FVector::CrossProduct(UpperDir, ElbowHint - Shoulder);
	}
	Hinge = Hinge.GetSafeNormal(UE_SMALL_NUMBER, Arm.Hinge);
	const FQuat UpperTurn = Frame(UpperDir, Hinge) * Frame(Arm.UpperDir, Arm.Hinge).Inverse();
	const FQuat LowerTurn = Frame(LowerDir, Hinge) * Frame(Arm.LowerDir, Arm.Hinge).Inverse();

	// The forearm rolls with the hand (a real forearm turns the wrist): it takes most of the hand's twist about it, so the
	// wrist isn't wrung like a cloth.
	const FQuat Carried = LowerTurn * HandRest.GetRotation();
	const FQuat Twist = (HandTurn * Carried.Inverse()).GetNormalized();
	const double TwistAngle = Twist.GetTwistAngle(LowerDir);
	const FQuat LowerRotation = (FQuat(LowerDir, TwistAngle * ForearmTwistShare) * LowerTurn * LowerRest.GetRotation()).GetNormalized();

	if (Arm.Clavicle != INDEX_NONE)
	{
		const FTransform& ClavicleRest = RestSpace[Arm.Clavicle];
		OutBones.Add(Arm.Clavicle);
		OutSpaces.Add(FTransform(ClavicleRest.GetRotation(), ClavicleRest.GetLocation() + Shift, ClavicleRest.GetScale3D()));
	}
	OutBones.Add(Arm.UpperArm);
	OutSpaces.Add(FTransform((UpperTurn * UpperRest.GetRotation()).GetNormalized(), Shoulder, UpperRest.GetScale3D()));
	OutBones.Add(Arm.LowerArm);
	OutSpaces.Add(FTransform(LowerRotation, Elbow, LowerRest.GetScale3D()));
	OutBones.Add(Arm.Hand);
	OutSpaces.Add(FTransform(HandTurn, End, HandRest.GetScale3D()));
}

void FFirstPersonHandsRig::Pose(const FHandsRigTargets& Targets, TArray<FTransform>& OutLocal, TArray<FTransform>& OutSpace) const
{
	const int32 Count = RestLocal.Num();
	if (!bBound)
	{
		OutLocal = RestLocal;
		OutSpace = RestSpace;
		return;
	}
	OutLocal.SetNum(Count);
	OutSpace.SetNum(Count);

	// Each palm seated on what it holds first (the arm then reaches the seated spot), then its fingers closed on it from
	// where the solved hand is (FirstPersonHandsContact.cpp); a placement without a contact keeps its curls.
	TArray<int32, TInlineAllocator<8>> PlacedBones;
	TArray<FTransform, TInlineAllocator<8>> PlacedSpaces;
	FHandFingers Fingers[2];
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FHandPlacement Target = Seated(Targets.Hands[Side]);
		SolveArm(Arms[Side], Target, Targets.DefaultForearm[Side], PlacedBones, PlacedSpaces);
		Fingers[Side] = Target.Fingers;
		const int32 HandAt = PlacedBones.Find(Arms[Side].Hand);
		if (Target.Contact.Share > 0.f && Target.Contact.Handle.bValid && HandAt != INDEX_NONE)
		{
			Fingers[Side] = ContactFingers(Side == 0 ? EFirstPersonHand::Left : EFirstPersonHand::Right, PlacedSpaces[HandAt], Target);
		}
	}

	// Parents first: the arms' solved bones as they are, every other bone at rest under its parent (the fingers curled),
	// or squeezed into the chest when it isn't an arm's; each then written against its parent.
	const FTransform Collapsed(FQuat::Identity, ShoulderMiddle, FVector(CollapsedScale));
	for (int32 Bone = 0; Bone < Count; ++Bone)
	{
		const int32 Parent = Parents[Bone];
		const int32 Placed = PlacedBones.Find(Bone);
		FTransform Space;
		if (Placed != INDEX_NONE)
		{
			Space = PlacedSpaces[Placed];
		}
		else if (!Kept[Bone])
		{
			Space = Collapsed;
		}
		else
		{
			const FTransform Local = FingerSlot[Bone] >= 0 ? CurledLocal(Bone, Fingers) : RestLocal[Bone];
			Space = Parent != INDEX_NONE ? Local * OutSpace[Parent] : Local;
		}
		OutSpace[Bone] = Space;
		OutLocal[Bone] = Parent != INDEX_NONE ? Space.GetRelativeTransform(OutSpace[Parent]) : Space;
	}
}

int32 FFirstPersonHandsRig::GetFingerBone(EFirstPersonHand Hand, int32 Finger, int32 Joint) const
{
	if (!bBound || Finger < 0 || Finger >= FHandFingers::Count || Joint < 0 || Joint >= FHandFingers::Joints)
	{
		return INDEX_NONE;
	}
	return Arms[FirstPersonHands::Index(Hand)].Fingers[Finger][Joint];
}

FVector FFirstPersonHandsRig::HoldPointIn(EFirstPersonHand Hand, const TArray<FTransform>& Space) const
{
	const FArm& Arm = Arms[FirstPersonHands::Index(Hand)];
	return bBound && Space.IsValidIndex(Arm.Hand) ? Space[Arm.Hand].TransformPosition(Arm.HoldLocal) : FVector::ZeroVector;
}

FVector FFirstPersonHandsRig::FingersDirIn(EFirstPersonHand Hand, const TArray<FTransform>& Space) const
{
	const FArm& Arm = Arms[FirstPersonHands::Index(Hand)];
	if (!bBound || !Space.IsValidIndex(Arm.Hand))
	{
		return FVector::ZeroVector;
	}
	// The rest frame carried by however the hand has turned since rest.
	const FQuat Turned = Space[Arm.Hand].GetRotation() * RestSpace[Arm.Hand].GetRotation().Inverse();
	return (Turned * Arm.HandFrame).GetAxisX();
}

FVector FFirstPersonHandsRig::PalmDirIn(EFirstPersonHand Hand, const TArray<FTransform>& Space) const
{
	const FArm& Arm = Arms[FirstPersonHands::Index(Hand)];
	if (!bBound || !Space.IsValidIndex(Arm.Hand))
	{
		return FVector::ZeroVector;
	}
	const FQuat Turned = Space[Arm.Hand].GetRotation() * RestSpace[Arm.Hand].GetRotation().Inverse();
	return (Turned * Arm.HandFrame).GetAxisY();
}
