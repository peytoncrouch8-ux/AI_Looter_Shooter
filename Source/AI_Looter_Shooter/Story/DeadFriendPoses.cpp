#include "Story/DeadFriendPoses.h"
#include "AnimationRuntime.h"
#include "ReferenceSkeleton.h"

namespace
{
	FVector Ease(const FVector& From, const FVector& To, float Alpha)
	{
		const FVector Eased = FMath::Lerp(From, To, static_cast<double>(Alpha)).GetSafeNormal();
		return Eased.IsNearlyZero() ? To : Eased;
	}
}

// ---------------------------------------------------------------------------
// Building a pose
// ---------------------------------------------------------------------------

FDeadFriendPose& FDeadFriendPose::Bend(FName Bone, float Forward, float TurnRight, float LeanRight)
{
	FQuat& Own = Turns.FindOrAdd(Bone, FQuat::Identity);
	Own = (Own * DeadFriendPoses::Turn(Forward, TurnRight, LeanRight)).GetNormalized();
	return *this;
}

FDeadFriendPose& FDeadFriendPose::Move(FName Bone, const FVector& Shift)
{
	Shifts.FindOrAdd(Bone, FVector::ZeroVector) += Shift;
	return *this;
}

FDeadFriendPose& FDeadFriendPose::Arm(int32 Side, const FVector& Upper, const FVector& Fore, float Wrist)
{
	FDeadFriendArm& Laid = Arms[FMath::Clamp(Side, 0, 1)];
	Laid.bSet = true;
	Laid.Upper = Upper.GetSafeNormal();
	Laid.Fore = Fore.GetSafeNormal();
	Laid.Wrist = Wrist;
	return *this;
}

FDeadFriendPose FDeadFriendPose::Blend(const FDeadFriendPose& A, const FDeadFriendPose& B, float Alpha)
{
	const float Share = FMath::Clamp(Alpha, 0.f, 1.f);
	FDeadFriendPose Blended;
	TSet<FName> Bones;
	A.Turns.GetKeys(Bones);
	for (const TPair<FName, FQuat>& Each : B.Turns)
	{
		Bones.Add(Each.Key);
	}
	for (const FName& Bone : Bones)
	{
		const FQuat* From = A.Turns.Find(Bone);
		const FQuat* To = B.Turns.Find(Bone);
		Blended.Turns.Add(Bone, FQuat::Slerp(From ? *From : FQuat::Identity, To ? *To : FQuat::Identity, Share).GetNormalized());
	}
	TSet<FName> Moved;
	A.Shifts.GetKeys(Moved);
	for (const TPair<FName, FVector>& Each : B.Shifts)
	{
		Moved.Add(Each.Key);
	}
	for (const FName& Bone : Moved)
	{
		const FVector* From = A.Shifts.Find(Bone);
		const FVector* To = B.Shifts.Find(Bone);
		Blended.Shifts.Add(Bone, FMath::Lerp(From ? *From : FVector::ZeroVector, To ? *To : FVector::ZeroVector, static_cast<double>(Share)));
	}
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FDeadFriendArm& From = A.Arms[Side];
		const FDeadFriendArm& To = B.Arms[Side];
		FDeadFriendArm& Out = Blended.Arms[Side];
		if (!From.bSet && !To.bSet)
		{
			continue;
		}
		// An arm laid in one pose only eases from (or to) hanging at rest.
		const FDeadFriendArm Rest;
		const FDeadFriendArm& F = From.bSet ? From : Rest;
		const FDeadFriendArm& T = To.bSet ? To : Rest;
		Out.bSet = true;
		Out.Upper = Ease(F.Upper, T.Upper, Share);
		Out.Fore = Ease(F.Fore, T.Fore, Share);
		Out.Wrist = FMath::Lerp(F.Wrist, T.Wrist, Share);
	}
	return Blended;
}

// ---------------------------------------------------------------------------
// The solver
// ---------------------------------------------------------------------------

namespace DeadFriendPoses
{
	FName ArmBone(int32 Side, int32 Part)
	{
		static const FName Names[2][3] = {
			{ TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l") },
			{ TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r") },
		};
		return Names[FMath::Clamp(Side, 0, 1)][FMath::Clamp(Part, 0, 2)];
	}

	FQuat Turn(float Forward, float TurnRight, float LeanRight)
	{
		const FQuat Yaw(FVector::UpVector, FMath::DegreesToRadians(TurnRight));
		const FQuat Pitch(FVector::RightVector, FMath::DegreesToRadians(Forward));
		// Tipping its top to its right is a turn the other way about forward (Y toward Z is its left side rising).
		const FQuat Roll(FVector::ForwardVector, FMath::DegreesToRadians(-LeanRight));
		return (Yaw * Pitch * Roll).GetNormalized();
	}

	TArray<FTransform> Solve(const FReferenceSkeleton& Skeleton, const FDeadFriendPose& Pose, const FDeadFriendLayer& Layer)
	{
		const int32 Count = Skeleton.GetNum();
		TArray<FTransform> Rest;
		TArray<FTransform> Posed;
		TArray<FQuat> Turned;
		Rest.SetNum(Count);
		Posed.SetNum(Count);
		Turned.SetNum(Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Rest[Index] = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index);
		}

		// The arms' own turns from their directions: the upper arm taken from its rest to Upper, the forearm's turn
		// undoing the upper arm's first so it points along Fore, the hand bent at the wrist.
		TMap<int32, FQuat> ArmOwn;
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FDeadFriendArm& Laid = Pose.Arms[Side];
			const int32 Upper = Skeleton.FindBoneIndex(ArmBone(Side, 0));
			const int32 Lower = Skeleton.FindBoneIndex(ArmBone(Side, 1));
			const int32 Hand = Skeleton.FindBoneIndex(ArmBone(Side, 2));
			if (!Laid.bSet || Upper == INDEX_NONE || Lower == INDEX_NONE || Hand == INDEX_NONE)
			{
				continue;
			}
			auto RestWay = [&Rest](int32 From, int32 To)
			{
				const FVector Way = (Rest[To].GetLocation() - Rest[From].GetLocation()).GetSafeNormal();
				return Way.IsNearlyZero() ? FVector::DownVector : Way;
			};
			const FQuat UpperTurn = FQuat::FindBetweenNormals(RestWay(Upper, Lower), Laid.Upper);
			const FQuat ForeTurn = FQuat::FindBetweenNormals(RestWay(Lower, Hand), Laid.Fore);
			ArmOwn.Add(Upper, UpperTurn);
			ArmOwn.Add(Lower, (UpperTurn.Inverse() * ForeTurn).GetNormalized());
			ArmOwn.Add(Hand, FQuat(FVector::RightVector, FMath::DegreesToRadians(Laid.Wrist)));
		}

		// A skeleton lists every bone after its parent: one pass poses them all.
		const FQuat HeadTurn(FVector::UpVector, FMath::DegreesToRadians(Layer.HeadYaw));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FName Name = Skeleton.GetBoneName(Index);
			const int32 Parent = Skeleton.GetParentIndex(Index);
			const FQuat Above = Parent != INDEX_NONE ? Turned[Parent] : FQuat::Identity;
			FVector Joint = Parent != INDEX_NONE
				? Posed[Parent].GetLocation() + Above.RotateVector(Rest[Index].GetLocation() - Rest[Parent].GetLocation())
				: Rest[Index].GetLocation();
			if (const FVector* Shift = Pose.Shifts.Find(Name))
			{
				Joint += *Shift;
			}
			FQuat Own = FQuat::Identity;
			if (const FQuat* Laid = ArmOwn.Find(Index))
			{
				Own = *Laid;
			}
			else if (const FQuat* Bent = Pose.Turns.Find(Name))
			{
				Own = *Bent;
			}
			if (Name == Jaw && Layer.JawOpen != 0.f)
			{
				Own = (Own * FQuat(FVector::RightVector, FMath::DegreesToRadians(Layer.JawOpen))).GetNormalized();
			}
			FQuat Whole = (Above * Own).GetNormalized();
			// Turned to the listener about the body's up at the head's joint; the jaw and the hat ride it as its children.
			if (Name == Head && Layer.HeadYaw != 0.f)
			{
				Whole = (HeadTurn * Whole).GetNormalized();
			}
			Turned[Index] = Whole;
			Posed[Index] = FTransform(Whole * Rest[Index].GetRotation(), Joint, Rest[Index].GetScale3D());
		}
		return Posed;
	}

	FVector BoneLocation(const FReferenceSkeleton& Skeleton, const TArray<FTransform>& Solved, FName Bone)
	{
		const int32 Index = Skeleton.FindBoneIndex(Bone);
		return Solved.IsValidIndex(Index) ? Solved[Index].GetLocation() : FVector::ZeroVector;
	}

	float BoneYaw(const FReferenceSkeleton& Skeleton, const TArray<FTransform>& Solved, FName Bone)
	{
		const int32 Index = Skeleton.FindBoneIndex(Bone);
		if (!Solved.IsValidIndex(Index))
		{
			return 0.f;
		}
		// Its turn from rest, applied to the body's front.
		const FQuat FromRest = Solved[Index].GetRotation() * FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index).GetRotation().Inverse();
		const FVector Looking = FromRest.RotateVector(FVector::ForwardVector);
		return FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Looking.Y, Looking.X)));
	}
}
