#include "Player/Animation/BodyHatTip.h"
#include "AnimationRuntime.h"
#include "ReferenceSkeleton.h"

float FBodyHatTip::DegreesFor(float Aim)
{
	return MaxDegrees * FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Aim, 0.f, 1.f));
}

bool FBodyHatTip::IsBoundTo(const FReferenceSkeleton& Skeleton) const
{
	return BoundBoneCount == Skeleton.GetNum() && BoundBoneCount > 0;
}

void FBodyHatTip::Bind(const FReferenceSkeleton& Skeleton)
{
	BoundBoneCount = Skeleton.GetNum();
	bReferencesValid = false;
	const int32 HatIndex = Skeleton.FindBoneIndex(TEXT("hat"));
	const int32 HeadIndex = Skeleton.FindBoneIndex(TEXT("head"));
	const int32 Foot = Skeleton.FindBoneIndex(TEXT("foot_l"));
	const int32 Ball = Skeleton.FindBoneIndex(TEXT("ball_l"));
	bHasHat = HatIndex != INDEX_NONE && HeadIndex != INDEX_NONE;
	if (!bHasHat)
	{
		return;
	}
	Hat = FBoneReference(Skeleton.GetBoneName(HatIndex));
	Head = FBoneReference(Skeleton.GetBoneName(HeadIndex));

	// Which way he faces at rest (his toes' way: the mesh may face +X or +Y), level; across it, the head's turning axis.
	TArray<FTransform> Rest;
	FAnimationRuntime::FillUpComponentSpaceTransforms(Skeleton, Skeleton.GetRefBonePose(), Rest);
	FVector Forward = FVector::YAxisVector;
	if (Foot != INDEX_NONE && Ball != INDEX_NONE)
	{
		Forward = (Rest[Ball].GetLocation() - Rest[Foot].GetLocation()).GetSafeNormal2D(UE_SMALL_NUMBER, FVector::YAxisVector);
	}
	const FVector Across = FVector::CrossProduct(Forward, FVector::UpVector).GetSafeNormal();
	ForwardInHead = Rest[HeadIndex].InverseTransformVectorNoScale(Forward);
	AcrossInHead = Rest[HeadIndex].InverseTransformVectorNoScale(Across);
}

void FBodyHatTip::Apply(FCSPose<FCompactPose>& Pose, float Aim)
{
	const float Degrees = DegreesFor(Aim);
	if (!bHasHat || Degrees <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FBoneContainer& Bones = Pose.GetPose().GetBoneContainer();
	if (!bReferencesValid || BoundSerial != Bones.GetSerialNumber())
	{
		Hat.Initialize(Bones);
		Head.Initialize(Bones);
		BoundSerial = Bones.GetSerialNumber();
		bReferencesValid = true;
	}
	const FCompactPoseBoneIndex HatIndex = Hat.GetCompactPoseIndex(Bones);
	const FCompactPoseBoneIndex HeadIndex = Head.GetCompactPoseIndex(Bones);
	if (!HatIndex.IsValid() || !HeadIndex.IsValid())
	{
		return;
	}
	// The head's axes as it is now (it tilts onto the stock as he sights); about its pivot at the back of the band.
	const FTransform& HeadNow = Pose.GetComponentSpaceTransform(HeadIndex);
	const FVector Forward = HeadNow.TransformVectorNoScale(ForwardInHead);
	const FVector Across = HeadNow.TransformVectorNoScale(AcrossInHead).GetSafeNormal();
	FQuat Tip(Across, FMath::DegreesToRadians(Degrees));
	if (Tip.RotateVector(Forward).Z < Forward.Z)
	{
		// Whichever way the axis points, the brim's front goes up.
		Tip = FQuat(Across, -FMath::DegreesToRadians(Degrees));
	}
	FTransform HatNow = Pose.GetComponentSpaceTransform(HatIndex);
	HatNow.SetRotation((Tip * HatNow.GetRotation()).GetNormalized());
	const FBoneTransform Single[] = { FBoneTransform(HatIndex, HatNow) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Single), 1.f);
}
