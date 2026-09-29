#include "Game/FallRecoverySubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

TStatId UFallRecoverySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFallRecoverySubsystem, STATGROUP_Tickables);
}

bool UFallRecoverySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UFallRecoverySubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
		if (!Movement)
		{
			continue;
		}

		FSafeSpot& Spot = SafeSpots.FindOrAdd(Character);

		// Sample while standing on walkable ground, a few times a second, so the spot is recent but not the
		// exact frame the player stepped off an edge.
		if (Movement->IsMovingOnGround() && Movement->CurrentFloor.IsWalkableFloor())
		{
			Spot.SampleTimer -= DeltaTime;
			if (Spot.SampleTimer <= 0.f || !Spot.bValid)
			{
				Spot.Location = Character->GetActorLocation();
				Spot.Rotation = PC->GetControlRotation();
				Spot.bValid = true;
				Spot.SampleTimer = 0.5f;
			}
			continue;
		}

		if (Spot.bValid && Character->GetActorLocation().Z < Spot.Location.Z - RecoverDropHeight)
		{
			Movement->StopMovementImmediately();
			Character->TeleportTo(Spot.Location + FVector(0.f, 0.f, 30.f), FRotator(0.f, Spot.Rotation.Yaw, 0.f), false, true);
			PC->SetControlRotation(FRotator(0.f, Spot.Rotation.Yaw, 0.f));
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.6f, FLinearColor(0.75f, 0.88f, 1.f), false, false);
			}
		}
	}

	// Forget pawns that no longer exist.
	for (auto It = SafeSpots.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}
