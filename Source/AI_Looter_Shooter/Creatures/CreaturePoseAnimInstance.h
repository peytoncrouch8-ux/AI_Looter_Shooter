#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "CreaturePoseAnimInstance.generated.h"

/** Where a creature's code puts one bone this frame, in the mesh's component space. */
struct FCreatureBonePose
{
	FName Bone;
	FTransform Transform;
};

/** Poses the rig from the owner's bone list; every other bone keeps its rest pose relative to its parent. */
struct FCreaturePoseAnimInstanceProxy : public FAnimInstanceProxy
{
	FCreaturePoseAnimInstanceProxy() = default;
	explicit FCreaturePoseAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	TArray<FCreatureBonePose> Pose;
};

/**
 * Animation with no graph, for creatures whose motion is all code: the creature works out its pose on the game thread
 * (ACreatureBase::GetBonePose: the spider's gait and IK, the slime's squash) and this instance applies it to the rig.
 */
UCLASS(Transient, NotBlueprintable)
class AI_LOOTER_SHOOTER_API UCreaturePoseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
};
