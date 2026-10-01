#include "Creatures/CreaturePoseAnimInstance.h"
#include "Creatures/CreatureBase.h"
#include "Animation/AnimNodeBase.h"
#include "BoneContainer.h"
#include "BonePose.h"

FAnimInstanceProxy* UCreaturePoseAnimInstance::CreateAnimInstanceProxy()
{
	return new FCreaturePoseAnimInstanceProxy(this);
}

void UCreaturePoseAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FCreaturePoseAnimInstanceProxy*>(InProxy);
}

void FCreaturePoseAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	// Still on the game thread, after the creature ticked: copy its pose for the evaluation (which may run on a worker).
	const ACreatureBase* Creature = Cast<ACreatureBase>(InAnimInstance->GetOwningActor());
	Pose = Creature ? Creature->GetBonePose() : TArray<FCreatureBonePose>();
}

bool FCreaturePoseAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	Output.ResetToRefPose();
	if (Pose.IsEmpty())
	{
		return true;
	}

	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	TArray<FBoneTransform, TInlineAllocator<32>> Transforms;
	for (const FCreatureBonePose& Bone : Pose)
	{
		FBoneReference Reference(Bone.Bone);
		Reference.Initialize(Bones);
		const FCompactPoseBoneIndex Index = Reference.GetCompactPoseIndex(Bones);
		if (Index.IsValid())
		{
			Transforms.Emplace(Index, Bone.Transform);
		}
	}
	// Parents first, so each bone is placed relative to where its parent went.
	Transforms.Sort(FCompareBoneTransformIndex());

	FCSPose<FCompactPose> ComponentPose;
	ComponentPose.InitPose(Output.Pose);
	ComponentPose.LocalBlendCSBoneTransforms(MakeArrayView(Transforms), 1.f);
	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(ComponentPose), Output.Pose);
	return true;
}
