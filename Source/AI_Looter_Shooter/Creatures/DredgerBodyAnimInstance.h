#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Creatures/DredgerBody.h"
#include "DredgerBodyAnimInstance.generated.h"

class UAnimSequenceBase;

/**
 * A Dredger's animation on the worker thread, with no graph: the legs from the mannequin's walk and jog clips (two ways
 * round blended by the move's angle, the gaits by its speed, on one step cycle so they stay in step), the upper body
 * holding MF_Rifle_Idle_ADS with its spine kept upright in mesh space (the player's own armed hold), MM_Rifle_Reload over
 * it while reloading, then the player's stance layer (LooterStancePose::Apply: crouch behind cover, the aim's pitch, the left
 * hand on the foregrip, the kick) and the Dredger's code layer (DredgerBody::ApplyCodeLayer).
 */
struct FDredgerBodyProxy : public FAnimInstanceProxy
{
	FDredgerBodyProxy() = default;
	explicit FDredgerBodyProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	/** The legs: the gaits' and ways' clips, blended into Output (each sampled on the shared step cycle). */
	void EvaluateLegs(FPoseContext& Output) const;
	/** The spine and everything above it take the held rifle pose, the spine's base kept as that pose has it in mesh space. */
	void HoldUpperBody(FPoseContext& Output) const;
	/** The reload's clip over the upper body by its weight. */
	void OverlayReload(FPoseContext& Output) const;

	FDredgerBodyInput Input;
	const UAnimSequenceBase* Idle = nullptr;
	const UAnimSequenceBase* Walk[DredgerBody::Ways] = {};
	const UAnimSequenceBase* Jog[DredgerBody::Ways] = {};
	const UAnimSequenceBase* Upper = nullptr;
	const UAnimSequenceBase* Reload = nullptr;
	/** The step cycle (0-1), the idle's and the held pose's own clocks (s). */
	float Cycle = 0.f;
	float IdleTime = 0.f;
	float UpperTime = 0.f;
	/** The walk's and jog's clip speeds (Looter.Dredger.Body.*), read on the game thread each update. */
	float WalkClipSpeed = 165.f;
	float JogClipSpeed = 375.f;
};

/**
 * The anim instance a Dredger's body runs (ADredgerCreature sets it on its mesh): it loads the mannequin's clips once
 * (DredgerBody's paths; a missing clip is skipped, the stance and code layers still run) and hands the proxy the body's
 * input each update. The Tripo Dredger will be rigged to the mannequin's skeleton, so it plays the same.
 */
UCLASS(Transient, NotBlueprintable)
class AI_LOOTER_SHOOTER_API UDredgerBodyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	const UAnimSequenceBase* GetIdleClip() const { return IdleClip; }
	const UAnimSequenceBase* GetWalkClip(int32 Way) const { return WalkClips.IsValidIndex(Way) ? WalkClips[Way].Get() : nullptr; }
	const UAnimSequenceBase* GetJogClip(int32 Way) const { return JogClips.IsValidIndex(Way) ? JogClips[Way].Get() : nullptr; }
	const UAnimSequenceBase* GetUpperClip() const { return UpperClip; }
	const UAnimSequenceBase* GetReloadClip() const { return ReloadClip; }

protected:
	virtual void NativeInitializeAnimation() override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
	/** Held here so they stay loaded (the proxy reads them on the worker thread). */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> IdleClip;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequenceBase>> WalkClips;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequenceBase>> JogClips;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> UpperClip;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> ReloadClip;
};
