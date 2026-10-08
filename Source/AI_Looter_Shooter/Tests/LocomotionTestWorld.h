#pragma once

// What the locomotion play tests build in their test levels: a floor or ceiling, the player standing on it, and the
// frames and keys they drive it with.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace LocomotionTestWorld
{
	inline constexpr float Frame = 1.f / 60.f;

	/** A floor or a ceiling: the engine's cube stretched to Size (cm), blocking everything. */
	inline AStaticMeshActor* SpawnBlock(UWorld* World, const FVector& Center, const FVector& Size)
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		// Sized before it has a mesh, so its collision is made at that size, straight into the level's queries.
		AStaticMeshActor* Block = Cube ? World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FQuat::Identity, Center, Size / 100.0)) : nullptr;
		if (Block)
		{
			UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
			Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Mesh->SetStaticMesh(Cube);
		}
		return Block;
	}

	/**
	 * The player (BP_LooterCharacter, at the player's size and speeds) standing on the floor at Feet, walking, facing +X;
	 * returns its locomotion component. No controller and no keys bound: the tests press them. The test level never
	 * ticks, so the movement only runs where a test runs it.
	 */
	inline UPlayerLocomotionComponent* SpawnPlayer(UWorld* World, const FVector& Feet)
	{
		UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Game/Player/BP_LooterCharacter.BP_LooterCharacter_C"));
		const ACharacter* Defaults = Class ? Class->GetDefaultObject<ACharacter>() : nullptr;
		ACharacter* Player = Defaults ? World->SpawnActor<ACharacter>(Class, Feet + FVector(0.0, 0.0, Defaults->GetDefaultHalfHeight() + 1.0), FRotator::ZeroRotator) : nullptr;
		if (!Player)
		{
			return nullptr;
		}
		// A movement component only takes the capsule it moves in a game level (UMovementComponent::OnRegister), and the
		// test level is an editor preview: without it, it can't crouch, walk or read the movement keys.
		UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
		Movement->SetUpdatedComponent(Player->GetCapsuleComponent());
		Player->DispatchBeginPlay();
		// Falling first, so walking starts afresh: it finds the floor, and crouching then keeps the feet on it.
		Movement->SetMovementMode(MOVE_Falling);
		Movement->SetMovementMode(MOVE_Walking);
		return Player->FindComponentByClass<UPlayerLocomotionComponent>();
	}

	inline ACharacter* BodyOf(const UPlayerLocomotionComponent* Locomotion)
	{
		return CastChecked<ACharacter>(Locomotion->GetOwner());
	}

	/** One frame of the locomotion component. */
	inline void Step(UPlayerLocomotionComponent* Locomotion)
	{
		static_cast<UActorComponent*>(Locomotion)->TickComponent(Frame, LEVELTICK_All, nullptr);
	}

	/**
	 * One frame as the game plays it: first what the movement does with the crouch wish before it moves (crouches, or
	 * stands if there's room: UCharacterMovementComponent::UpdateCharacterStateBeforeMovement), then the locomotion.
	 */
	inline void PlayFrame(UPlayerLocomotionComponent* Locomotion)
	{
		UCharacterMovementComponent* Movement = BodyOf(Locomotion)->GetCharacterMovement();
		if (Movement->bWantsToCrouch && !Movement->IsCrouching())
		{
			Movement->Crouch(false);
		}
		else if (!Movement->bWantsToCrouch && Movement->IsCrouching())
		{
			Movement->UnCrouch(false);
		}
		Step(Locomotion);
	}

	inline UCameraComponent* CameraOf(const UPlayerLocomotionComponent* Locomotion)
	{
		return UPlayerViewComponent::FindFirstPersonCamera(Locomotion->GetOwner());
	}

	/** The first-person camera's height above the feet (the capsule's bottom). */
	inline float EyeAboveFeet(const UPlayerLocomotionComponent* Locomotion)
	{
		const ACharacter* Body = BodyOf(Locomotion);
		const UCameraComponent* Camera = CameraOf(Locomotion);
		const double Feet = Body->GetActorLocation().Z - Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		return Camera ? static_cast<float>(Camera->GetComponentLocation().Z - Feet) : 0.f;
	}

	/**
	 * The first-person view, frame by frame: the camera's height in the world and its roll, the most either moved in one
	 * frame (a pop), and the most that move changed from one frame to the next (a jolt: a sudden start, stop or turn).
	 */
	struct FViewTrack
	{
		float MaxStep = 0.f;
		float MaxStepChange = 0.f;
		float MaxRollStep = 0.f;
		float MaxRollStepChange = 0.f;
		/** The frames the worst step and the worst change came on (counted from the first sample). */
		int32 WorstStepFrame = -1;
		int32 WorstChangeFrame = -1;
		int32 Frames = 0;

		void Sample(const UPlayerLocomotionComponent* Locomotion)
		{
			const UCameraComponent* Camera = CameraOf(Locomotion);
			if (!Camera)
			{
				return;
			}
			FTransform Offset;
			float FieldOfView = 0.f;
			Camera->GetAdditiveOffset(Offset, FieldOfView);
			const float Z = static_cast<float>(Camera->GetComponentLocation().Z);
			const float Roll = static_cast<float>(Offset.Rotator().Roll);
			if (Frames > 0)
			{
				const float Moved = Z - LastZ;
				const float Rolled = Roll - LastRoll;
				if (FMath::Abs(Moved) > MaxStep)
				{
					MaxStep = FMath::Abs(Moved);
					WorstStepFrame = Frames;
				}
				MaxRollStep = FMath::Max(MaxRollStep, FMath::Abs(Rolled));
				if (Frames > 1)
				{
					if (FMath::Abs(Moved - LastMoved) > MaxStepChange)
					{
						MaxStepChange = FMath::Abs(Moved - LastMoved);
						WorstChangeFrame = Frames;
					}
					MaxRollStepChange = FMath::Max(MaxRollStepChange, FMath::Abs(Rolled - LastRolled));
				}
				LastMoved = Moved;
				LastRolled = Rolled;
			}
			LastZ = Z;
			LastRoll = Roll;
			++Frames;
		}

	private:
		float LastZ = 0.f;
		float LastMoved = 0.f;
		float LastRoll = 0.f;
		float LastRolled = 0.f;
	};

	/** Pushing forward at Speed: the keys as the last move read them, and the velocity they made. */
	inline void MoveForward(UPlayerLocomotionComponent* Locomotion, float Speed)
	{
		ACharacter* Body = BodyOf(Locomotion);
		UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
		Body->AddMovementInput(Body->GetActorForwardVector(), 1.f, true);
		Movement->ConsumeInputVector();
		Movement->Velocity = Body->GetActorForwardVector() * Speed;
	}

	/** The sprint key held, running forward at sprint speed, and a frame for the stance to take it. */
	inline void StartSprint(UPlayerLocomotionComponent* Locomotion)
	{
		const UCharacterMovementComponent* Movement = BodyOf(Locomotion)->GetCharacterMovement();
		Locomotion->HandleSprintPressed();
		MoveForward(Locomotion, Movement->MaxWalkSpeed * Locomotion->SprintSpeedMultiplier);
		Step(Locomotion);
	}

	/** The crouch key held and the capsule crouched (what the next move would do). */
	inline void CrouchDown(UPlayerLocomotionComponent* Locomotion)
	{
		Locomotion->HandleCrouchPressed();
		Step(Locomotion);
		BodyOf(Locomotion)->GetCharacterMovement()->Crouch(false);
	}

	/** Sprinting, then crouch: a slide under way, in the crouched capsule. */
	inline void StartSlide(UPlayerLocomotionComponent* Locomotion)
	{
		StartSprint(Locomotion);
		Locomotion->HandleCrouchPressed();
		BodyOf(Locomotion)->GetCharacterMovement()->Crouch(false);
	}

	/**
	 * Frames until the slide is over (5 s at most); returns its length in seconds, and the least the slide let the player
	 * move at on the way (OutLowest) and on its last frame (OutLast).
	 */
	inline float RunSlideOut(UPlayerLocomotionComponent* Locomotion, float& OutLowest, float& OutLast)
	{
		const UCharacterMovementComponent* Movement = BodyOf(Locomotion)->GetCharacterMovement();
		float Time = 0.f;
		OutLowest = Movement->MaxWalkSpeedCrouched;
		OutLast = OutLowest;
		while (Locomotion->IsSliding() && Time < 5.f)
		{
			Step(Locomotion);
			Time += Frame;
			if (Locomotion->IsSliding())
			{
				OutLast = Movement->MaxWalkSpeedCrouched;
				OutLowest = FMath::Min(OutLowest, OutLast);
			}
		}
		return Time;
	}
}

#endif
