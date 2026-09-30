#include "Creatures/SpiderAnimInstance.h"
#include "Creatures/SpiderCreature.h"
#include "Animation/AnimNodeBase.h"
#include "BoneContainer.h"
#include "BonePose.h"

FAnimInstanceProxy* USpiderAnimInstance::CreateAnimInstanceProxy()
{
	return new FSpiderAnimInstanceProxy(this);
}

void USpiderAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FSpiderAnimInstanceProxy*>(InProxy);
}

void FSpiderAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	// Still on the game thread, after the spider ticked: copy its pose for the evaluation (which may run on a worker).
	const ASpiderCreature* Spider = Cast<ASpiderCreature>(InAnimInstance->GetOwningActor());
	Pose = Spider ? Spider->GetBonePose() : TArray<FSpiderBonePose>();
}

bool FSpiderAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	Output.ResetToRefPose();
	if (Pose.IsEmpty())
	{
		return true;
	}

	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	TArray<FBoneTransform, TInlineAllocator<32>> Transforms;
	for (const FSpiderBonePose& Bone : Pose)
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
