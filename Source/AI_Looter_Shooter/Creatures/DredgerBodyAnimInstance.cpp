#include "Creatures/DredgerBodyAnimInstance.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/DredgerCreature.h"
#include "Creatures/DredgerTuning.h"
#include "Player/Animation/LooterStancePose.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/BoneReference.h"
#include "BoneContainer.h"
#include "BonePose.h"

// UDredgerBodyAnimInstance: the clips loaded once, and its proxy's evaluation on the worker thread (DredgerBodyAnimInstance.h).

namespace
{
	/** A clip's weight under this is left out (it would cost a sample for nothing). */
	constexpr float SkipWeight = 0.02f;

	const UAnimSequenceBase* LoadClip(const TCHAR* Path)
	{
		const UAnimSequenceBase* Clip = LoadObject<UAnimSequenceBase>(nullptr, Path);
		UE_CLOG(!Clip, LogLooter, Warning, TEXT("Dredger body: no clip at %s (that layer is skipped)."), Path);
		return Clip;
	}

	float LengthOf(const UAnimSequenceBase* Clip)
	{
		return Clip ? FMath::Max(static_cast<float>(Clip->GetPlayLength()), 0.05f) : 1.f;
	}

	FCompactPoseBoneIndex FindBodyBone(const FBoneContainer& Bones, const TCHAR* Name)
	{
		FBoneReference Reference(Name);
		Reference.Initialize(Bones);
		return Reference.GetCompactPoseIndex(Bones);
	}

	/** Clip at Time into Pose (on Output's bones); its curves and attributes are thrown away (the body reads none). */
	void SampleClip(const UAnimSequenceBase& Clip, double Time, const FPoseContext& Output, FCompactPose& Pose)
	{
		Pose.SetBoneContainer(&Output.Pose.GetBoneContainer());
		FBlendedCurve Curve;
		Curve.InitFrom(Output.Curve);
		UE::Anim::FStackAttributeContainer Attributes;
		FAnimationPoseData Data(Pose, Curve, Attributes);
		Clip.GetAnimationPose(Data, FAnimExtractContext(Time));
	}

	/** One clip's part in a blend: the clip, where in it, how much. */
	struct FClipShare
	{
		const UAnimSequenceBase* Clip = nullptr;
		double Time = 0.0;
		float Weight = 0.f;
	};
}

// ---------------------------------------------------------------------------
// The instance (game thread)
// ---------------------------------------------------------------------------

void UDredgerBodyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	IdleClip = const_cast<UAnimSequenceBase*>(LoadClip(DredgerBody::IdleClip));
	UpperClip = const_cast<UAnimSequenceBase*>(LoadClip(DredgerBody::UpperBodyClip));
	ReloadClip = const_cast<UAnimSequenceBase*>(LoadClip(DredgerBody::ReloadClip));
	WalkClips.Reset();
	JogClips.Reset();
	for (int32 Way = 0; Way < DredgerBody::Ways; ++Way)
	{
		WalkClips.Add(const_cast<UAnimSequenceBase*>(LoadClip(*DredgerBody::WalkClip(Way))));
		JogClips.Add(const_cast<UAnimSequenceBase*>(LoadClip(*DredgerBody::JogClip(Way))));
	}
}

FAnimInstanceProxy* UDredgerBodyAnimInstance::CreateAnimInstanceProxy()
{
	return new FDredgerBodyProxy(this);
}

void UDredgerBodyAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FDredgerBodyProxy*>(InProxy);
}

// ---------------------------------------------------------------------------
// The proxy
// ---------------------------------------------------------------------------

void FDredgerBodyProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	// Still on the game thread, after the Dredger ticked: its body's input and the clips, copied for the evaluation.
	const UDredgerBodyAnimInstance* Instance = CastChecked<UDredgerBodyAnimInstance>(InAnimInstance);
	const ADredgerCreature* Dredger = Cast<ADredgerCreature>(InAnimInstance->GetOwningActor());
	Input = Dredger ? Dredger->GetBodyInput() : FDredgerBodyInput();
	Idle = Instance->GetIdleClip();
	Upper = Instance->GetUpperClip();
	Reload = Instance->GetReloadClip();
	for (int32 Way = 0; Way < DredgerBody::Ways; ++Way)
	{
		Walk[Way] = Instance->GetWalkClip(Way);
		Jog[Way] = Instance->GetJogClip(Way);
	}

	// One step cycle for every clip, turning at the speed moved: the feet keep their ground whatever the blend.
	// The cvars are read here, on the game thread; the evaluation keeps these copies.
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	WalkClipSpeed = Tuning.WalkClipSpeed;
	JogClipSpeed = Tuning.JogClipSpeed;
	float IdleShare = 1.f;
	float WalkShare = 0.f;
	float JogShare = 0.f;
	DredgerBody::GaitShares(Input.Speed, WalkClipSpeed, JogClipSpeed, IdleShare, WalkShare, JogShare);
	const float JogOfMoving = WalkShare + JogShare > UE_KINDA_SMALL_NUMBER ? JogShare / (WalkShare + JogShare) : 0.f;
	const float Rate = DredgerBody::CycleRate(Input.Speed, LengthOf(Walk[0]), LengthOf(Jog[0]), JogOfMoving, WalkClipSpeed, JogClipSpeed);
	Cycle = FMath::Fmod(Cycle + DeltaSeconds * Rate, 1.f);
	IdleTime = FMath::Fmod(IdleTime + DeltaSeconds, LengthOf(Idle));
	UpperTime = FMath::Fmod(UpperTime + DeltaSeconds, LengthOf(Upper));
}

bool FDredgerBodyProxy::Evaluate(FPoseContext& Output)
{
	Output.ResetToRefPose();
	EvaluateLegs(Output);
	if (Upper)
	{
		HoldUpperBody(Output);
	}
	if (Reload && Input.Stance.ReloadWeight > UE_KINDA_SMALL_NUMBER)
	{
		OverlayReload(Output);
	}
	// The player's stance layer, as a Dredger fills it (crouch, the aim, the hand on the gun, the kick); then the Dredger's own.
	if (Input.Stance.NeedsLayer())
	{
		LooterStancePose::Apply(Output.Pose, Input.Stance);
	}
	DredgerBody::ApplyCodeLayer(Output.Pose, Input);
	return true;
}

void FDredgerBodyProxy::EvaluateLegs(FPoseContext& Output) const
{
	float IdleShare = 1.f;
	float WalkShare = 0.f;
	float JogShare = 0.f;
	DredgerBody::GaitShares(Input.Speed, WalkClipSpeed, JogClipSpeed, IdleShare, WalkShare, JogShare);
	int32 WayA = 0;
	int32 WayB = 1;
	float BShare = 0.f;
	DredgerBody::WayBlend(Input.MoveAngle, WayA, WayB, BShare);
	if (Input.bCheap)
	{
		// Far off: the nearer way and the stronger gait only.
		WayA = BShare > 0.5f ? WayB : WayA;
		BShare = 0.f;
		const bool bJog = JogShare > WalkShare;
		const float Moving = WalkShare + JogShare;
		WalkShare = bJog ? 0.f : Moving;
		JogShare = bJog ? Moving : 0.f;
	}

	TArray<FClipShare, TInlineAllocator<5>> Shares;
	auto Add = [&Shares](const UAnimSequenceBase* Clip, double Time, float Weight)
	{
		if (Clip && Weight >= SkipWeight)
		{
			Shares.Add(FClipShare{ Clip, Time, Weight });
		}
	};
	// Each gait's two ways round, on the shared step cycle (each clip at the same share of its own length).
	auto AddGait = [this, &Add, WayA, WayB, BShare](const UAnimSequenceBase* const* Clips, float GaitShare)
	{
		const UAnimSequenceBase* A = Clips[WayA];
		const UAnimSequenceBase* B = Clips[WayB];
		Add(A, static_cast<double>(Cycle * LengthOf(A)), GaitShare * (1.f - BShare));
		Add(B, static_cast<double>(Cycle * LengthOf(B)), GaitShare * BShare);
	};
	Add(Idle, IdleTime, IdleShare);
	AddGait(Walk, WalkShare);
	AddGait(Jog, JogShare);
	if (Shares.IsEmpty())
	{
		return;
	}

	// A running blend: each clip in by its share of the weight so far, so the shares come out as asked.
	float Total = 0.f;
	FCompactPose Sample;
	for (int32 Index = 0; Index < Shares.Num(); ++Index)
	{
		const FClipShare& Share = Shares[Index];
		SampleClip(*Share.Clip, Share.Time, Output, Sample);
		if (Index == 0)
		{
			Output.Pose.CopyBonesFrom(Sample);
			Total = Share.Weight;
			continue;
		}
		Total += Share.Weight;
		const float Alpha = Share.Weight / FMath::Max(Total, UE_KINDA_SMALL_NUMBER);
		for (const FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
		{
			Output.Pose[Bone].BlendWith(Sample[Bone], Alpha);
		}
	}
	// The body is moved by its capsule, never by a clip: the root stays where its rest pose has it.
	const FCompactPoseBoneIndex Root(0);
	Output.Pose[Root] = Output.Pose.GetRefPose(Root);
}

void FDredgerBodyProxy::HoldUpperBody(FPoseContext& Output) const
{
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const FCompactPoseBoneIndex UpperRoot = FindBodyBone(Bones, TEXT("spine_01"));
	if (!UpperRoot.IsValid())
	{
		return;
	}
	FCompactPose Held;
	SampleClip(*Upper, static_cast<double>(UpperTime), Output, Held);

	// Where the held pose points the spine's base against the whole mesh (not the hips, which sway every step).
	FCSPose<FCompactPose> HeldInMesh;
	HeldInMesh.InitPose(Held);
	const FQuat SpineInMesh = HeldInMesh.GetComponentSpaceTransform(UpperRoot).GetRotation();

	for (const FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
	{
		if (Bone == UpperRoot || Bones.BoneIsChildOf(Bone, UpperRoot))
		{
			Output.Pose[Bone] = Held[Bone];
		}
	}
	FCSPose<FCompactPose> Pose;
	Pose.InitPose(Output.Pose);
	FTransform Spine = Pose.GetComponentSpaceTransform(UpperRoot);
	Spine.SetRotation(SpineInMesh);
	const FBoneTransform Single[] = { FBoneTransform(UpperRoot, Spine) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Single), 1.f);
	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Pose), Output.Pose);
}

void FDredgerBodyProxy::OverlayReload(FPoseContext& Output) const
{
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const FCompactPoseBoneIndex UpperRoot = FindBodyBone(Bones, TEXT("spine_01"));
	if (!UpperRoot.IsValid())
	{
		return;
	}
	FCompactPose ReloadPose;
	SampleClip(*Reload, static_cast<double>(FMath::Clamp(Input.ReloadShare, 0.f, 1.f) * LengthOf(Reload)), Output, ReloadPose);
	const float Weight = FMath::Clamp(Input.Stance.ReloadWeight, 0.f, 1.f);
	for (const FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
	{
		if (Bone == UpperRoot || Bones.BoneIsChildOf(Bone, UpperRoot))
		{
			Output.Pose[Bone].BlendWith(ReloadPose[Bone], Weight);
		}
	}
}
