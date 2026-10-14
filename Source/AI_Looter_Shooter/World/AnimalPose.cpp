#include "World/AnimalPose.h"
#include "AnimationRuntime.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"

namespace
{
	/** A turn seen in a mirror across the animal's middle (its XZ plane): the same angle, the axis reflected. */
	FQuat Reflected(const FQuat& Turn)
	{
		return FQuat(-Turn.X, Turn.Y, -Turn.Z, Turn.W);
	}

	FVector Reflected(const FVector& Offset)
	{
		return FVector(Offset.X, -Offset.Y, Offset.Z);
	}

	/** A spring's longest step (s): stiff small parts stay steady at a few updates a second. */
	constexpr float SpringStep = 1.f / 90.f;
}

// ---------------------------------------------------------------------------
// The table
// ---------------------------------------------------------------------------

int32 FAnimalPoseTable::FindBone(FName Name) const
{
	for (int32 Bone = 0; Bones && Bone < NumBones; ++Bone)
	{
		if (Name == FName(Bones[Bone].Name))
		{
			return Bone;
		}
	}
	return INDEX_NONE;
}

int32 FAnimalPoseTable::FindPose(const TCHAR* Name) const
{
	for (int32 Pose = 0; PoseNames && Pose < NumPoses; ++Pose)
	{
		if (FCString::Stricmp(PoseNames[Pose], Name) == 0)
		{
			return Pose;
		}
	}
	return INDEX_NONE;
}

FName FAnimalPoseTable::BoneName(int32 Bone) const
{
	return IsValidBone(Bone) ? FName(Bones[Bone].Name) : NAME_None;
}

int32 FAnimalPoseTable::ParentOf(int32 Bone) const
{
	return IsValidBone(Bone) && Bones[Bone].Parent >= 0 ? Bones[Bone].Parent : INDEX_NONE;
}

int32 FAnimalPoseTable::MirrorOf(int32 Bone) const
{
	return IsValidBone(Bone) && IsValidBone(Bones[Bone].Mirror) ? Bones[Bone].Mirror : Bone;
}

FVector FAnimalPoseTable::RestHead(int32 Bone) const
{
	if (!IsValidBone(Bone))
	{
		return FVector::ZeroVector;
	}
	const float* Rest = Bones[Bone].Rest;
	return FVector(Rest[0], Rest[1], Rest[2]);
}

FQuat FAnimalPoseTable::Own(int32 Pose, int32 Bone) const
{
	if (!IsValidBone(Bone) || !Rows || Pose < 0 || Pose >= NumPoses)
	{
		return FQuat::Identity;
	}
	const float* Q = Rows[Pose * NumBones + Bone].Own;
	return FQuat(Q[0], Q[1], Q[2], Q[3]).GetNormalized();
}

FVector FAnimalPoseTable::Shift(int32 Pose, int32 Bone) const
{
	if (!IsValidBone(Bone) || !Rows || Pose < 0 || Pose >= NumPoses)
	{
		return FVector::ZeroVector;
	}
	const float* S = Rows[Pose * NumBones + Bone].Shift;
	return FVector(S[0], S[1], S[2]);
}

// ---------------------------------------------------------------------------
// A pose, a spring
// ---------------------------------------------------------------------------

void FAnimalPose::Reset(int32 NumBones)
{
	Own.Init(FQuat::Identity, NumBones);
	Shift.Init(FVector::ZeroVector, NumBones);
	Lead.Init(FQuat::Identity, NumBones);
}

void FAnimalSpring::Update(float DeltaSeconds, const FVector2D& Push, float Frequency, float Damping, float Limit)
{
	const float Omega = UE_TWO_PI * FMath::Max(Frequency, 0.1f);
	float Left = FMath::Clamp(DeltaSeconds, 0.f, 0.25f);
	while (Left > 0.f)
	{
		const float Step = FMath::Min(Left, SpringStep);
		Left -= Step;
		const FVector2D Pull = Push - Angle * (Omega * Omega) - Speed * (2.f * Damping * Omega);
		Speed += Pull * Step;
		Angle += Speed * Step;
		for (int32 Axis = 0; Axis < 2; ++Axis)
		{
			if (FMath::Abs(Angle[Axis]) > Limit)
			{
				Angle[Axis] = FMath::Sign(Angle[Axis]) * Limit;
				Speed[Axis] = 0.f;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Building, blending, solving
// ---------------------------------------------------------------------------

namespace AnimalPose
{
	FAnimalPose FromTable(const FAnimalPoseTable& Table, int32 Pose)
	{
		FAnimalPose Made;
		if (!Table.Bones || Pose < 0 || Pose >= Table.NumPoses)
		{
			return Made;
		}
		Made.Reset(Table.NumBones);
		for (int32 Bone = 0; Bone < Table.NumBones; ++Bone)
		{
			Made.Own[Bone] = Table.Own(Pose, Bone);
			Made.Shift[Bone] = Table.Shift(Pose, Bone);
		}
		return Made;
	}

	FAnimalPose Mirrored(const FAnimalPoseTable& Table, const FAnimalPose& Pose)
	{
		// The mirror is a reflection, so it keeps every bone's rule (Turned = Lead * Above * Own): each bone takes its
		// other side's turns and shift, reflected.
		FAnimalPose Made;
		Made.Reset(Pose.Num());
		for (int32 Bone = 0; Bone < Pose.Num(); ++Bone)
		{
			const int32 Other = FMath::Clamp(Table.MirrorOf(Bone), 0, Pose.Num() - 1);
			Made.Own[Bone] = Reflected(Pose.Own[Other]);
			Made.Shift[Bone] = Reflected(Pose.Shift[Other]);
			Made.Lead[Bone] = Reflected(Pose.Lead[Other]);
		}
		return Made;
	}

	void Blend(const FAnimalPose& A, const FAnimalPose& B, float Alpha, FAnimalPose& Out)
	{
		const float Share = FMath::Clamp(Alpha, 0.f, 1.f);
		if (A.Num() != B.Num())
		{
			Out = Share < 0.5f ? A : B;
			return;
		}
		if (&Out != &A && &Out != &B)
		{
			Out.Reset(A.Num());
		}
		for (int32 Bone = 0; Bone < A.Num(); ++Bone)
		{
			Out.Own[Bone] = FQuat::Slerp(A.Own[Bone], B.Own[Bone], Share).GetNormalized();
			Out.Shift[Bone] = FMath::Lerp(A.Shift[Bone], B.Shift[Bone], static_cast<double>(Share));
			Out.Lead[Bone] = FQuat::Slerp(A.Lead[Bone], B.Lead[Bone], Share).GetNormalized();
		}
	}

	void BlendBones(FAnimalPose& InOut, const FAnimalPose& B, float Alpha, TConstArrayView<int32> Bones)
	{
		const float Share = FMath::Clamp(Alpha, 0.f, 1.f);
		for (const int32 Bone : Bones)
		{
			if (InOut.Own.IsValidIndex(Bone) && B.Own.IsValidIndex(Bone))
			{
				InOut.Own[Bone] = FQuat::Slerp(InOut.Own[Bone], B.Own[Bone], Share).GetNormalized();
				InOut.Shift[Bone] = FMath::Lerp(InOut.Shift[Bone], B.Shift[Bone], static_cast<double>(Share));
				InOut.Lead[Bone] = FQuat::Slerp(InOut.Lead[Bone], B.Lead[Bone], Share).GetNormalized();
			}
		}
	}

	float StrideBlend(const FAnimalPose& First, const FAnimalPose& Second, float Phase, FAnimalPose& Out)
	{
		const float Wrapped = Phase - FMath::FloorToFloat(Phase);
		const bool bSecondHalf = Wrapped >= 0.5f;
		const float Step = bSecondHalf ? (Wrapped - 0.5f) * 2.f : Wrapped * 2.f;
		Blend(bSecondHalf ? Second : First, bSecondHalf ? First : Second, Ease(Step), Out);
		return Step;
	}

	void Bend(FAnimalPose& Pose, int32 Bone, float Pitch, float Yaw, float Roll)
	{
		if (Pose.Own.IsValidIndex(Bone))
		{
			Pose.Own[Bone] = (Pose.Own[Bone] * FRotator(Pitch, Yaw, Roll).Quaternion()).GetNormalized();
		}
	}

	void Look(FAnimalPose& Pose, int32 Bone, float Yaw, float Pitch)
	{
		if (Pose.Lead.IsValidIndex(Bone))
		{
			Pose.Lead[Bone] = (FRotator(Pitch, Yaw, 0.f).Quaternion() * Pose.Lead[Bone]).GetNormalized();
		}
	}

	void Nudge(FAnimalPose& Pose, int32 Bone, const FVector& Offset)
	{
		if (Pose.Shift.IsValidIndex(Bone))
		{
			Pose.Shift[Bone] += Offset;
		}
	}

	void Solve(const FAnimalPoseTable& Table, const FAnimalPose& Pose, FAnimalPoseSolve& Out)
	{
		// Parents come before their children in the table, so one pass poses them all: a bone's joint rides where its
		// parent's turn carries it, then its own shift; its turn is its lead, its parent's and its own.
		const int32 Count = FMath::Min(Table.NumBones, Pose.Num());
		Out.Heads.SetNum(Count);
		Out.Turns.SetNum(Count);
		for (int32 Bone = 0; Bone < Count; ++Bone)
		{
			const int32 Parent = Table.ParentOf(Bone);
			const bool bParent = Parent != INDEX_NONE && Parent < Bone;
			const FQuat Above = bParent ? Out.Turns[Parent] : FQuat::Identity;
			const FVector Carried = bParent ? Out.Heads[Parent] + Above.RotateVector(Table.RestHead(Bone) - Table.RestHead(Parent))
				: Table.RestHead(Bone);
			Out.Heads[Bone] = Carried + Pose.Shift[Bone];
			Out.Turns[Bone] = (Pose.Lead[Bone] * Above * Pose.Own[Bone]).GetNormalized();
		}
	}

	float YawOf(const FAnimalPoseSolve& Solved, int32 Bone)
	{
		if (!Solved.Turns.IsValidIndex(Bone))
		{
			return 0.f;
		}
		const FVector Looking = Solved.Turns[Bone].RotateVector(FVector::ForwardVector);
		return FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Looking.Y, Looking.X)));
	}

	float Degrees(const FQuat& Turn)
	{
		return FMath::RadiansToDegrees(2.f * FMath::Acos(FMath::Min(1.f, static_cast<float>(FMath::Abs(Turn.GetNormalized().W)))));
	}

	float Ease(float Share)
	{
		const float T = FMath::Clamp(Share, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}
}

// ---------------------------------------------------------------------------
// The rig
// ---------------------------------------------------------------------------

bool FAnimalRig::Bind(const USkeletalMesh* Mesh, const FAnimalPoseTable& InTable)
{
	Table = nullptr;
	TableBone.Reset();
	Parent.Reset();
	RestPose.Reset();
	SkeletonBone.Init(INDEX_NONE, InTable.NumBones);
	if (!Mesh || !InTable.Bones)
	{
		return false;
	}
	const FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
	const int32 Count = Skeleton.GetNum();
	TableBone.SetNum(Count);
	Parent.SetNum(Count);
	RestPose.SetNum(Count);
	int32 Found = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		TableBone[Index] = InTable.FindBone(Skeleton.GetBoneName(Index));
		Parent[Index] = Skeleton.GetParentIndex(Index);
		RestPose[Index] = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index);
		if (TableBone[Index] != INDEX_NONE)
		{
			SkeletonBone[TableBone[Index]] = Index;
			++Found;
		}
	}
	if (Found == 0)
	{
		TableBone.Reset();
		return false;
	}
	Table = &InTable;
	Posed = RestPose;
	Turned.Init(FQuat::Identity, Count);
	return true;
}

void FAnimalRig::Apply(UPoseableMeshComponent& Component, const FAnimalPose& Pose)
{
	const int32 Count = TableBone.Num();
	if (!IsBound() || Component.BoneSpaceTransforms.Num() != Count)
	{
		return;
	}
	// A skeleton lists every bone after its parent: one pass poses them all in component space, as the table's rule
	// says, then each is written against its parent (the poseable mesh keeps its bones that way).
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const int32 Up = Parent[Index];
		const FQuat Above = Up != INDEX_NONE ? Turned[Up] : FQuat::Identity;
		FVector Joint = Up != INDEX_NONE
			? Posed[Up].GetLocation() + Above.RotateVector(RestPose[Index].GetLocation() - RestPose[Up].GetLocation())
			: RestPose[Index].GetLocation();
		const int32 Bone = TableBone[Index];
		if (Bone != INDEX_NONE && Pose.Own.IsValidIndex(Bone))
		{
			Joint += Pose.Shift[Bone];
			Turned[Index] = (Pose.Lead[Bone] * Above * Pose.Own[Bone]).GetNormalized();
		}
		else
		{
			Turned[Index] = Above;
		}
		Posed[Index] = FTransform(Turned[Index] * RestPose[Index].GetRotation(), Joint, RestPose[Index].GetScale3D());
		Component.BoneSpaceTransforms[Index] = Up != INDEX_NONE ? Posed[Index].GetRelativeTransform(Posed[Up]) : Posed[Index];
	}
	Component.MarkRefreshTransformDirty();
	Component.RefreshBoneTransforms();
}

FVector FAnimalRig::JointOf(int32 TableBoneIndex) const
{
	const int32 Index = SkeletonBone.IsValidIndex(TableBoneIndex) ? SkeletonBone[TableBoneIndex] : INDEX_NONE;
	return Posed.IsValidIndex(Index) ? Posed[Index].GetLocation() : FVector::ZeroVector;
}
