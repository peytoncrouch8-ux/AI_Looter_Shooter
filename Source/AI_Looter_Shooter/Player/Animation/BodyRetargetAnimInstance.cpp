#include "Player/Animation/BodyRetargetAnimInstance.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/IConsoleManager.h"
#include "Retargeter/IKRetargeter.h"

namespace
{
	TAutoConsoleVariable<int32> CVarBodySwing(TEXT("Looter.Body.Swing"), 1,
		TEXT("Ellis's duster, cape, bandana, hair and lantern swing (1) or hold their animated pose (0)."));
}

void FBodyRetargetAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	// Set up before the base class takes the root node and initializes it.
	const UBodyFigureComponent* Figure = InAnimInstance ? Cast<UBodyFigureComponent>(InAnimInstance->GetSkelMeshComponent()) : nullptr;
	Retarget.IKRetargeterAsset = Figure ? Figure->GetRetargeter() : nullptr;
	Retarget.RetargetFrom = ERetargetSourceMode::ParentSkeletalMeshComponent;
	FAnimInstanceProxy::Initialize(InAnimInstance);
}

void FBodyRetargetAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	// The source's pose is read on the game thread, before this instance updates on a worker.
	Retarget.PreUpdate(InAnimInstance);
	SwingSeconds += DeltaSeconds;
	if (!bLookedForView)
	{
		bLookedForView = true;
		const AActor* Owner = InAnimInstance ? InAnimInstance->GetOwningActor() : nullptr;
		View = Owner ? Owner->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	}
	Aim = View.IsValid() ? View->GetAimAlpha() : 0.f;
}

bool FBodyRetargetAnimInstanceProxy::Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode)
{
	EvaluateAnimationNode_WithRoot(Output, InRootNode);

	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	const USkeletalMesh* Asset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
	if (InRootNode == GetRootNode() && Asset)
	{
		const FReferenceSkeleton& RefSkeleton = Asset->GetRefSkeleton();
		if (!Swing.IsBoundTo(RefSkeleton))
		{
			Swing.Bind(RefSkeleton);
		}
		if (!HatTip.IsBoundTo(RefSkeleton))
		{
			HatTip.Bind(RefSkeleton);
		}
		if (!ForearmTwist.IsBoundTo(RefSkeleton))
		{
			ForearmTwist.Bind(RefSkeleton);
		}
		const bool bTwist = ForearmTwist.NumTwistBones() > 0;
		const bool bSwing = Swing.NumChains() > 0 && CVarBodySwing.GetValueOnAnyThread() != 0;
		const bool bTip = HatTip.HasHat() && FBodyHatTip::DegreesFor(Aim) > UE_KINDA_SMALL_NUMBER;
		if (bTwist || bSwing || bTip)
		{
			FCSPose<FCompactPose> Pose;
			Pose.InitPose(Output.Pose);
			// The forearms first: the swing keeps the coat off them as they are.
			if (bTwist)
			{
				ForearmTwist.Apply(Pose);
			}
			if (bSwing)
			{
				Swing.Apply(Pose, GetComponentTransform(), SwingSeconds);
			}
			if (bTip)
			{
				HatTip.Apply(Pose, Aim);
			}
			FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Pose), Output.Pose);
		}
	}
	SwingSeconds = 0.f;
	return true;
}

FAnimInstanceProxy* UBodyRetargetAnimInstance::CreateAnimInstanceProxy()
{
	return new FBodyRetargetAnimInstanceProxy(this);
}

void UBodyRetargetAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FBodyRetargetAnimInstanceProxy*>(InProxy);
}
