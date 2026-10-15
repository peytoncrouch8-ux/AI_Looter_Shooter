#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_RetargetPoseFromMesh.h"
#include "Player/Animation/BodyHatTip.h"
#include "Player/Animation/BodySwing.h"
#include "BodyRetargetAnimInstance.generated.h"

/**
 * The engine's Retarget Pose From Mesh node as the whole graph of a native anim instance (no Anim Blueprint): it copies
 * the pose of the skeletal mesh its component is attached to (the hidden mannequin) through the component's retargeter.
 * A Blueprint's graph would call the node's game-thread step for it; a native root node's is called here. Then the
 * swing bones (the duster, cape, bandana, hair, lantern: FBodySwing) swing on top, and the hat tips back as a sight
 * comes up (FBodyHatTip).
 */
struct FBodyRetargetAnimInstanceProxy : public FAnimInstanceProxy
{
	FBodyRetargetAnimInstanceProxy() = default;
	explicit FBodyRetargetAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:
	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) override;
	virtual FAnimNode_Base* GetCustomRootNode() override { return &Retarget; }

private:
	/** Value-initialized: the node's own counters have no initializers (a Blueprint's nodes start zeroed). */
	FAnimNode_RetargetPoseFromMesh Retarget{};
	FBodySwing Swing;
	/** Time since the swing last ran (an update skipped adds its time to the next). */
	float SwingSeconds = 0.f;
	/** The hat tipped back as a sight comes up, by the owner's aim (read on the game thread). */
	FBodyHatTip HatTip;
	float Aim = 0.f;
	TWeakObjectPtr<const class UPlayerViewComponent> View;
	bool bLookedForView = false;
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
