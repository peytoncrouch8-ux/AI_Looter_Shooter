#include "Player/Animation/LooterCharacterAnimInstance.h"
#include "Player/Animation/LooterClimbPose.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

// ULooterCharacterAnimInstance's mantle and vault: on the game thread it turns the locomotion component's move (its plan and
// its eased pose alpha) into the numbers the stance layer poses the body with. Only the third-person body shows the pose;
// the first-person view's own climb (the gun lowered and turned in) is the locomotion component's.

void ULooterCharacterAnimInstance::HandleTraversalStarted(ETraversalKind Kind, float TopOverFeet)
{
	ClimbTopOverFeet = TopOverFeet;
}

void ULooterCharacterAnimInstance::RefreshClimbInput()
{
	StanceInput.Climb = FLooterClimbInput();
	const UPlayerLocomotionComponent* Loco = Locomotion.Get();
	const float Alpha = Loco ? Loco->GetTraversalAlpha() : 0.f;
	if (Alpha <= UE_KINDA_SMALL_NUMBER)
	{
		// Between moves: the next one's start is heard as it begins (or estimated, if this instance came in mid-move).
		if (!Loco || !Loco->IsTraversing())
		{
			ClimbTopOverFeet = LooterClimbPose::UnknownTop;
		}
		return;
	}

	const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	// The capsule in world cm, as the plan's own numbers are (the player's radius at its 0.85 size).
	const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 30.f;
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	StanceInput.Climb = LooterClimbPose::Describe(Loco->GetTraversal(), Alpha, ClimbTopOverFeet, Radius, BodyScale,
		Mesh ? Mesh->GetComponentQuat() : FQuat::Identity);
	StanceInput.Climb.TorsoLean = StanceInput.Climb.bVault ? VaultTorsoLean : MantleTorsoLean;
}
