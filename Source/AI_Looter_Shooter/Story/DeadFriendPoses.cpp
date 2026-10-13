#include "Story/DeadFriendPoses.h"
#include "AnimationRuntime.h"
#include "ReferenceSkeleton.h"

// ---------------------------------------------------------------------------
// A pose
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
	return Blended;
}

// ---------------------------------------------------------------------------
// Tables and the solvers
// ---------------------------------------------------------------------------

namespace
{
	/** A bone's whole turn: its parent's with its own on top, the layer's head turn and jaw added where they belong. */
	FQuat Turned(const FQuat& Above, FQuat Own, FName Name, const FDeadFriendLayer& Layer)
	{
		if (Name == DeadFriendPoses::Jaw && Layer.JawOpen != 0.f)
		{
			Own = (Own * FQuat(FVector::RightVector, FMath::DegreesToRadians(Layer.JawOpen))).GetNormalized();
		}
		const FQuat Whole = (Above * Own).GetNormalized();
		// Turned to the listener about the body's up at the head's joint; the jaw and the hat ride it as its children.
		if (Name == DeadFriendPoses::Head && Layer.HeadYaw != 0.f)
		{
			return (FQuat(FVector::UpVector, FMath::DegreesToRadians(Layer.HeadYaw)) * Whole).GetNormalized();
		}
		return Whole;
	}
}

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

	FDeadFriendPose FromTable(const FDeadFriendPoseTable& Table, int32 Pose)
	{
		FDeadFriendPose Made;
		if (!Table.Bones || !Table.Rows || Pose < 0 || Pose >= Table.NumPoses)
		{
			return Made;
		}
		for (int32 Bone = 0; Bone < Table.NumBones; ++Bone)
		{
			const FDeadFriendPoseBone& Row = Table.Rows[Pose * Table.NumBones + Bone];
			const FName Name(Table.Bones[Bone].Name);
			const FQuat Own = FQuat(Row.Own[0], Row.Own[1], Row.Own[2], Row.Own[3]).GetNormalized();
			if (!Own.Equals(FQuat::Identity, 1.e-5f))
			{
				Made.Turns.Add(Name, Own);
			}
			const FVector Shift(Row.Shift[0], Row.Shift[1], Row.Shift[2]);
			if (!Shift.IsNearlyZero(0.01))
			{
				Made.Shifts.Add(Name, Shift);
			}
		}
		return Made;
	}

	int32 FindTableBone(const FDeadFriendPoseTable& Table, FName Bone)
	{
		for (int32 Index = 0; Table.Bones && Index < Table.NumBones; ++Index)
		{
			if (Bone == FName(Table.Bones[Index].Name))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	FTableHeads SolveTable(const FDeadFriendPoseTable& Table, const FDeadFriendPose& Pose, const FDeadFriendLayer& Layer)
	{
		// Parents come before their children in the table, so one pass poses them all: a bone's joint rides where its
		// parent's turn carries it, then its own shift; its turn is its parent's with its own on top.
		FTableHeads Solved;
		Solved.Heads.SetNum(Table.NumBones);
		Solved.Turns.SetNum(Table.NumBones);
		for (int32 Bone = 0; Table.Bones && Bone < Table.NumBones; ++Bone)
		{
			const FDeadFriendPoseBoneInfo& Info = Table.Bones[Bone];
			const FName Name(Info.Name);
			const FVector Rest(Info.Rest[0], Info.Rest[1], Info.Rest[2]);
			const int32 Parent = Info.Parent;
			const bool bParent = Parent >= 0 && Parent < Bone;
			const FQuat Above = bParent ? Solved.Turns[Parent] : FQuat::Identity;
			const FVector ParentRest = bParent ? FVector(Table.Bones[Parent].Rest[0], Table.Bones[Parent].Rest[1], Table.Bones[Parent].Rest[2]) : FVector::ZeroVector;
			FVector BoneHead = bParent ? Solved.Heads[Parent] + Above.RotateVector(Rest - ParentRest) : Rest;
			if (const FVector* Shift = Pose.Shifts.Find(Name))
			{
				BoneHead += *Shift;
			}
			const FQuat* Own = Pose.Turns.Find(Name);
			Solved.Heads[Bone] = BoneHead;
			Solved.Turns[Bone] = Turned(Above, Own ? *Own : FQuat::Identity, Name, Layer);
		}
		return Solved;
	}

	FVector TableHead(const FDeadFriendPoseTable& Table, const FTableHeads& Solved, FName Bone)
	{
		const int32 Index = FindTableBone(Table, Bone);
		return Solved.Heads.IsValidIndex(Index) ? Solved.Heads[Index] : FVector::ZeroVector;
	}

	float TableYaw(const FDeadFriendPoseTable& Table, const FTableHeads& Solved, FName Bone)
	{
		const int32 Index = FindTableBone(Table, Bone);
		if (!Solved.Turns.IsValidIndex(Index))
		{
			return 0.f;
		}
		const FVector Looking = Solved.Turns[Index].RotateVector(FVector::ForwardVector);
		return FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Looking.Y, Looking.X)));
	}

	TArray<FTransform> Solve(const FReferenceSkeleton& Skeleton, const FDeadFriendPose& Pose, const FDeadFriendLayer& Layer)
	{
		const int32 Count = Skeleton.GetNum();
		TArray<FTransform> Rest;
		TArray<FTransform> Posed;
		TArray<FQuat> Whole;
		Rest.SetNum(Count);
		Posed.SetNum(Count);
		Whole.SetNum(Count);
		// A skeleton lists every bone after its parent: one pass poses them all.
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Rest[Index] = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index);
			const FName Name = Skeleton.GetBoneName(Index);
			const int32 Parent = Skeleton.GetParentIndex(Index);
			const FQuat Above = Parent != INDEX_NONE ? Whole[Parent] : FQuat::Identity;
			FVector Joint = Parent != INDEX_NONE
				? Posed[Parent].GetLocation() + Above.RotateVector(Rest[Index].GetLocation() - Rest[Parent].GetLocation())
				: Rest[Index].GetLocation();
			if (const FVector* Shift = Pose.Shifts.Find(Name))
			{
				Joint += *Shift;
			}
			const FQuat* Own = Pose.Turns.Find(Name);
			Whole[Index] = Turned(Above, Own ? *Own : FQuat::Identity, Name, Layer);
			Posed[Index] = FTransform(Whole[Index] * Rest[Index].GetRotation(), Joint, Rest[Index].GetScale3D());
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
