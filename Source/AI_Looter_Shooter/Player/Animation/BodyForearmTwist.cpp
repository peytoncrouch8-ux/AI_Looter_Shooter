#include "Player/Animation/BodyForearmTwist.h"
#include "AnimationRuntime.h"
#include "ReferenceSkeleton.h"

float FBodyForearmTwist::TwistAbout(const FQuat& Rotation, const FVector& Axis)
{
	FQuat Swing;
	FQuat Twist;
	Rotation.ToSwingTwist(Axis, Swing, Twist);
	// The twist's angle, signed by which way round the axis it turns, the short way (a quaternion and its negative are
	// the same turn).
	float Angle = Twist.GetAngle();
	if (FVector::DotProduct(FVector(Twist.X, Twist.Y, Twist.Z), Axis) < 0.0)
	{
		Angle = -Angle;
	}
	return FMath::UnwindRadians(Angle);
}

bool FBodyForearmTwist::IsBoundTo(const FReferenceSkeleton& Skeleton) const
{
	return BoundBoneCount == Skeleton.GetNum() && Skeleton.GetNum() > 0 && BoundFirstBone == Skeleton.GetBoneName(0);
}

int32 FBodyForearmTwist::NumTwistBones() const
{
	int32 Count = 0;
	for (const FSide& Side : Sides)
	{
		Count += Side.Twists.Num();
	}
	return Count;
}

void FBodyForearmTwist::Bind(const FReferenceSkeleton& Skeleton)
{
	Sides.Reset();
	bReferencesValid = false;
	BoundBoneCount = Skeleton.GetNum();
	BoundFirstBone = BoundBoneCount > 0 ? Skeleton.GetBoneName(0) : NAME_None;

	TArray<FTransform> Rest;
	FAnimationRuntime::FillUpComponentSpaceTransforms(Skeleton, Skeleton.GetRefBonePose(), Rest);
	for (const TCHAR* Suffix : { TEXT("_l"), TEXT("_r") })
	{
		const int32 Forearm = Skeleton.FindBoneIndex(FName(*(FString(TEXT("lowerarm")) + Suffix)));
		const int32 Hand = Skeleton.FindBoneIndex(FName(*(FString(TEXT("hand")) + Suffix)));
		if (Forearm == INDEX_NONE || Hand == INDEX_NONE)
		{
			continue;
		}
		FSide Side;
		Side.Forearm = FBoneReference(Skeleton.GetBoneName(Forearm));
		Side.Hand = FBoneReference(Skeleton.GetBoneName(Hand));
		Side.RestHand = Rest[Hand].GetRelativeTransform(Rest[Forearm]).GetRotation();
		const FVector Elbow = Rest[Forearm].GetLocation();
		const FVector Wrist = Rest[Hand].GetLocation();
		const double Length = FVector::Dist(Elbow, Wrist);
		Side.Axis = Rest[Forearm].InverseTransformVectorNoScale((Wrist - Elbow) / FMath::Max(Length, UE_KINDA_SMALL_NUMBER));
		for (int32 Bone = 0; Bone < Skeleton.GetNum(); ++Bone)
		{
			const FString Name = Skeleton.GetBoneName(Bone).ToString();
			if (Skeleton.GetParentIndex(Bone) != Forearm || !Name.StartsWith(TEXT("lowerarm_twist")) || !Name.EndsWith(Suffix))
			{
				continue;
			}
			FTwistBone& Twist = Side.Twists.AddDefaulted_GetRef();
			Twist.Bone = FBoneReference(Skeleton.GetBoneName(Bone));
			Twist.InForearm = Rest[Bone].GetRelativeTransform(Rest[Forearm]);
			const double Along = FVector::DotProduct(Rest[Bone].GetLocation() - Elbow, (Wrist - Elbow)) / FMath::Max(Length * Length, UE_KINDA_SMALL_NUMBER);
			Twist.Share = FMath::Clamp(static_cast<float>(Along), 0.f, 1.f);
		}
		if (!Side.Twists.IsEmpty())
		{
			Sides.Add(MoveTemp(Side));
		}
	}
}

void FBodyForearmTwist::InitBoneReferences(const FBoneContainer& Bones)
{
	for (FSide& Side : Sides)
	{
		Side.Forearm.Initialize(Bones);
		Side.Hand.Initialize(Bones);
		for (FTwistBone& Twist : Side.Twists)
		{
			Twist.Bone.Initialize(Bones);
		}
	}
	BoundSerial = Bones.GetSerialNumber();
	bReferencesValid = true;
}

void FBodyForearmTwist::Apply(FCSPose<FCompactPose>& Pose)
{
	if (Sides.IsEmpty())
	{
		return;
	}
	const FBoneContainer& Bones = Pose.GetPose().GetBoneContainer();
	if (!bReferencesValid || BoundSerial != Bones.GetSerialNumber())
	{
		InitBoneReferences(Bones);
	}
	TArray<FBoneTransform, TInlineAllocator<4>> Moved;
	for (const FSide& Side : Sides)
	{
		const FCompactPoseBoneIndex Forearm = Side.Forearm.GetCompactPoseIndex(Bones);
		const FCompactPoseBoneIndex Hand = Side.Hand.GetCompactPoseIndex(Bones);
		if (!Forearm.IsValid() || !Hand.IsValid())
		{
			continue;
		}
		// The hand's turn from rest, in the forearm's space, and how much of it rolls about the forearm.
		const FTransform& ForearmNow = Pose.GetComponentSpaceTransform(Forearm);
		const FQuat HandInForearm = Pose.GetComponentSpaceTransform(Hand).GetRelativeTransform(ForearmNow).GetRotation();
		const float Twist = TwistAbout(HandInForearm * Side.RestHand.Inverse(), Side.Axis);
		for (const FTwistBone& Each : Side.Twists)
		{
			const FCompactPoseBoneIndex Index = Each.Bone.GetCompactPoseIndex(Bones);
			if (!Index.IsValid())
			{
				continue;
			}
			FTransform InForearm = Each.InForearm;
			InForearm.SetRotation(FQuat(Side.Axis, Twist * Each.Share) * InForearm.GetRotation());
			Moved.Add(FBoneTransform(Index, InForearm * ForearmNow));
		}
	}
	if (!Moved.IsEmpty())
	{
		Moved.Sort(FCompareBoneTransformIndex());
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Moved), 1.f);
	}
}
