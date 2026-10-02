#include "World/FallRecoverySubsystem.h"
#include "AI_Looter_Shooter.h"
#include "World/PlayableArea.h"
#include "World/PlayableBoundary.h"
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

	// The level's playable area, if it has one: safe spots only inside it, and its open edges bring a fall back early.
	const APlayableArea* PlayableArea = APlayableArea::Find(World);
	const FPlayableBoundary* Boundary = PlayableArea ? &PlayableArea->GetBoundary() : nullptr;
	FFallRecoveryTracker::FRules Rules;
	Rules.LongDrop = RecoverDropHeight;
	Rules.OpenEdgeDrop = OpenEdgeDropHeight;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
		if (!Movement)
		{
			continue;
		}

		FFallRecoveryTracker& Tracker = Trackers.FindOrAdd(Character);
		const bool bOnWalkableGround = Movement->IsMovingOnGround() && Movement->CurrentFloor.IsWalkableFloor();
		const TOptional<EFallRecoveryReason> Reason = Tracker.Update(DeltaTime, Character->GetActorLocation(), PC->GetControlRotation(),
			bOnWalkableGround, Boundary, Rules);
		if (Reason.IsSet())
		{
			Recover(*PC, *Character, Tracker, Reason.GetValue());
		}
	}

	// Forget pawns that no longer exist.
	for (auto It = Trackers.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void UFallRecoverySubsystem::Recover(APlayerController& PC, ACharacter& Character, FFallRecoveryTracker& Tracker, EFallRecoveryReason Reason)
{
	FFallRecoveryEvent Event;
	Event.Pawn = &Character;
	Event.Reason = Reason;
	Event.From = Character.GetActorLocation();
	Event.To = Tracker.GetSafeLocation();
	Event.Edge = Tracker.GetExitEdge();

	const FRotator Facing(0.0, Tracker.GetSafeRotation().Yaw, 0.0);
	if (UCharacterMovementComponent* Movement = Character.GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	Character.TeleportTo(Event.To + FVector(0.0, 0.0, 30.0), Facing, false, true);
	PC.SetControlRotation(Facing);
	if (PC.PlayerCameraManager)
	{
		PC.PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.6f, FLinearColor(0.75f, 0.88f, 1.f), false, false);
	}
	Tracker.MarkRecovered();

	UE_LOG(LogLooter, Log, TEXT("Fall recovery: %s brought back %s (%.0f m below the safe spot, edge %d)."), *Character.GetName(),
		Reason == EFallRecoveryReason::OffOpenEdge ? TEXT("off an open edge") : TEXT("after a long fall"), (Event.To.Z - Event.From.Z) / 100.0, Event.Edge);
	OnRecovered.Broadcast(Event);
}
