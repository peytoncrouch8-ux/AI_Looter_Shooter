#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_RetargetPoseFromMesh.h"
#include "BodyRetargetAnimInstance.generated.h"

/**
 * The engine's Retarget Pose From Mesh node as the whole graph of a native anim instance (no Anim Blueprint): it copies
 * the pose of the skeletal mesh its component is attached to (the hidden mannequin) through the component's retargeter.
 * A Blueprint's graph would call the node's game-thread step for it; a native root node's is called here.
 */
struct FBodyRetargetAnimInstanceProxy : public FAnimInstanceProxy
{
	FBodyRetargetAnimInstanceProxy() = default;
	explicit FBodyRetargetAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:
	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual FAnimNode_Base* GetCustomRootNode() override { return &Retarget; }

private:
	/** Value-initialized: the node's own counters have no initializers (a Blueprint's nodes start zeroed). */
	FAnimNode_RetargetPoseFromMesh Retarget{};
};

/**
 * The anim instance of a body that copies another's pose (UBodyFigureComponent: Ellis on the mannequin). The retargeter
 * comes from its component; the pose from the skeletal mesh the component is attached to.
 */
UCLASS(Transient, NotBlueprintable)
class AI_LOOTER_SHOOTER_API UBodyRetargetAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
};
