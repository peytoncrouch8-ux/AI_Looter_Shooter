#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Player/Animation/LooterCharacterAnimInstance.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Tests/LocomotionTestWorld.h"

// What the player's animation play tests share (PlayerAnimPlayTests.cpp, PlayerAnimClimbPlayTests.cpp): the body's real
// Anim Blueprint updated and evaluated frame by frame on the real character in a test level, next to the locomotion
// component the tests already drive (Tests/LocomotionTestWorld.h).

namespace PlayerAnimPlayKit
{
	inline USkeletalMeshComponent* BodyMesh(const UPlayerLocomotionComponent* Locomotion)
	{
		return LocomotionTestWorld::BodyOf(Locomotion)->GetMesh();
	}

	/** The body's Anim Blueprint when it is one of ours (the stance layer's); null for a test level that makes none. */
	inline ULooterCharacterAnimInstance* AnimOf(const UPlayerLocomotionComponent* Locomotion)
	{
		const USkeletalMeshComponent* Mesh = BodyMesh(Locomotion);
		return Mesh ? Cast<ULooterCharacterAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
	}

	/**
	 * One frame of the body's animation as the game plays it: the Anim Blueprint updated (the event graph, the blend space,
	 * the stance layer's input gathered) and its pose evaluated, here and now rather than on the tick the test level never has.
	 */
	inline void PlayAnimation(const UPlayerLocomotionComponent* Locomotion)
	{
		if (USkeletalMeshComponent* Mesh = BodyMesh(Locomotion))
		{
			Mesh->TickAnimation(LocomotionTestWorld::Frame, false);
			Mesh->RefreshBoneTransforms(nullptr);
		}
	}

	/** A bone in the body's component space (the mannequin's own size) and in the world, as last evaluated. */
	inline FVector InMesh(const UPlayerLocomotionComponent* Locomotion, const TCHAR* Bone)
	{
		return BodyMesh(Locomotion)->GetSocketTransform(Bone, RTS_Component).GetLocation();
	}

	inline FVector InWorld(const UPlayerLocomotionComponent* Locomotion, const TCHAR* Bone)
	{
		return BodyMesh(Locomotion)->GetSocketLocation(Bone);
	}

	/** The capsule's radius at the player's size. */
	inline float RadiusOf(const UPlayerLocomotionComponent* Locomotion)
	{
		return LocomotionTestWorld::BodyOf(Locomotion)->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}

	/** Every bone of the evaluated pose is a finite transform. */
	inline bool PoseIsFinite(const UPlayerLocomotionComponent* Locomotion)
	{
		for (const FTransform& Bone : BodyMesh(Locomotion)->GetComponentSpaceTransforms())
		{
			if (Bone.ContainsNaN())
			{
				return false;
			}
		}
		return true;
	}
}

#endif
