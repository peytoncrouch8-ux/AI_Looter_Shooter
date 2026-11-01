#include "Story/WardenCrewAnimInstance.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const UWardenCrewAnimInstance::IdlePath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle");

UWardenCrewAnimInstance::UWardenCrewAnimInstance()
{
	// The idle is the mannequin's own (cooked with the player's Anim Blueprints, which play it too).
	if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(IdlePath))))
	{
		static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> IdleFinder(IdlePath);
		Idle = IdleFinder.Object;
	}
}

FAnimInstanceProxy* UWardenCrewAnimInstance::CreateAnimInstanceProxy()
{
	return new FWardenCrewAnimInstanceProxy(this);
}

void UWardenCrewAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FWardenCrewAnimInstanceProxy*>(InProxy);
}

void FWardenCrewAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	// Still on the game thread: the actor has set this frame's pose; copied for the worker that evaluates it.
	const UWardenCrewAnimInstance* Instance = CastChecked<UWardenCrewAnimInstance>(InAnimInstance);
	Input = Instance->Input;
	Idle = Instance->Idle;
}

bool FWardenCrewAnimInstanceProxy::Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode)
{
	// It has no graph: anything but its own (empty) root is evaluated as the engine would.
	if (InRootNode != GetRootNode())
	{
		EvaluateAnimationNode_WithRoot(Output, InRootNode);
		return true;
	}
	Output.ResetToRefPose();
	if (const UAnimSequenceBase* Clip = Idle)
	{
		// Looped on the person's own clock (each starts somewhere of its own in it).
		const float Length = FMath::Max(static_cast<float>(Clip->GetPlayLength()), UE_KINDA_SMALL_NUMBER);
		const double Time = FMath::Fmod(FMath::Max(Input.IdleTime, 0.f), Length);
		FAnimationPoseData Data(Output.Pose, Output.Curve, Output.CustomAttributes);
		Clip->GetAnimationPose(Data, FAnimExtractContext(Time));
	}
	WardenCrewPose::Apply(Output.Pose, Input, GetComponentTransform());
	return true;
}
