#include "Player/Animation/BodyRetargetAnimInstance.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "Retargeter/IKRetargeter.h"

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
}

FAnimInstanceProxy* UBodyRetargetAnimInstance::CreateAnimInstanceProxy()
{
	return new FBodyRetargetAnimInstanceProxy(this);
}

void UBodyRetargetAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FBodyRetargetAnimInstanceProxy*>(InProxy);
}
