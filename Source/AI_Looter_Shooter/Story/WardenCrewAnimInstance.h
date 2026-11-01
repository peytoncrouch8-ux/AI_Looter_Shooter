#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Story/WardenCrewPose.h"
#include "WardenCrewAnimInstance.generated.h"

class UAnimSequenceBase;

/**
 * The whole graph of a story person's mannequin, in code (no Anim Blueprint): the mannequin's unarmed idle (MM_Idle) looped
 * on the person's own clock, then the layer (WardenCrewPose::Apply) from what the person's actor asked for this frame.
 */
struct FWardenCrewAnimInstanceProxy : public FAnimInstanceProxy
{
	FWardenCrewAnimInstanceProxy() = default;
	explicit FWardenCrewAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) override;

private:
	FWardenPoseInput Input;
	/** The instance's idle (it keeps it referenced). */
	const UAnimSequenceBase* Idle = nullptr;
};

/**
 * The anim instance of a story person's hidden mannequin (AWardenCrewMember, ADeliaRansom): the hero body worn over it as a
 * stand-in (UBodyFigureComponent) copies its pose, and so would a model rigged to the mannequin's skeleton, worn as the
 * mannequin itself (the user's Tripo people). Its actor hands it the frame's pose (SetPoseInput) as it ticks.
 */
UCLASS(Transient, NotBlueprintable)
class AI_LOOTER_SHOOTER_API UWardenCrewAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UWardenCrewAnimInstance();

	/** This frame's pose (the actor's tick, before the mesh animates). */
	void SetPoseInput(const FWardenPoseInput& InInput) { Input = InInput; }
	const FWardenPoseInput& GetPoseInput() const { return Input; }

	/** The idle played under the layer (MM_Idle; null leaves the reference pose under it, as in a checkout without the mannequin). */
	const UAnimSequenceBase* GetIdle() const { return Idle; }

	/** The mannequin's unarmed idle, which every story person breathes with. */
	static const TCHAR* const IdlePath;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
	friend struct FWardenCrewAnimInstanceProxy;

	FWardenPoseInput Input;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> Idle;
};
