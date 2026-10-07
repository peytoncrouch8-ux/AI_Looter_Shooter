#pragma once

// What the locomotion play tests build in their test levels: a floor or ceiling, the player standing on it, and the
// frames and keys they drive it with.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerLocomotionComponent.h"
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
