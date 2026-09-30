#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "SpiderAnimInstance.generated.h"

/** Where the spider's code puts one bone this frame, in the mesh's component space. */
struct FSpiderBonePose
{
	FName Bone;
	FTransform Transform;
};

/** Poses the rig from the owner's bone list; every other bone keeps its rest pose relative to its parent. */
struct FSpiderAnimInstanceProxy : public FAnimInstanceProxy
{
	FSpiderAnimInstanceProxy() = default;
	explicit FSpiderAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	TArray<FSpiderBonePose> Pose;
};

/**
 * The spider's animation has no graph: ASpiderCreature works out its gait, IK and body motion on the game thread
 * (ASpiderCreature::GetBonePose) and this instance applies that pose to SK_Spider.
 */
UCLASS(Transient, NotBlueprintable)
class AI_LOOTER_SHOOTER_API USpiderAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
};
