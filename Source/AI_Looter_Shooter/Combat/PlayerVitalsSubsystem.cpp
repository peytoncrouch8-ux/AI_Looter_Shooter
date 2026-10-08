#include "Combat/PlayerVitalsSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Missions/MissionRunner.h"
#include "World/RespawnMarker.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

TStatId UPlayerVitalsSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPlayerVitalsSubsystem, STATGROUP_Tickables);
}

bool UPlayerVitalsSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UPlayerVitalsSubsystem::Tick(float DeltaTime)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		// Only the player's own character (a spectator or camera pawn has no health).
		const ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		UHealthComponent* Health = Character ? Character->FindComponentByClass<UHealthComponent>() : nullptr;
		if (!Health || !PC->IsLocalController())
		{
			continue;
		}

		FPlayerVitals& Vitals = Players.FindOrAdd(PC);
		const float Current = Health->GetHealth();

		if (Vitals.bDying)
		{
			Vitals.DeathTime += DeltaTime;
			if (Vitals.DeathTime >= RespawnDelay)
			{
				Respawn(PC, Vitals);
			}
			continue;
		}

		// A hit shows on the HUD (the screen's red edges, the portrait's flash and flinch, the health bar's chip): the
		// whole screen no longer flashes red, which hid the fight for a moment and doubled the edges.
		Vitals.LastHealth = Current;

		if (Health->IsDead())
		{
			UE_LOG(LogLooter, Log, TEXT("Player died; respawning in %.1fs"), RespawnDelay);
			Vitals.bDying = true;
			Vitals.DeathTime = 0.f;
			Vitals.DeathLocation = Character->GetActorLocation();
			PC->SetIgnoreMoveInput(true);
			PC->SetIgnoreLookInput(true);
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(0.4f, 1.f, RespawnDelay * 0.6f, FLinearColor(0.12f, 0.f, 0.f), false, true);
			}
		}
	}

	for (auto It = Players.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void UPlayerVitalsSubsystem::Respawn(APlayerController* PC, FPlayerVitals& Vitals)
{
	ACharacter* Character = Cast<ACharacter>(PC->GetPawn());
	UHealthComponent* Health = Character ? Character->FindComponentByClass<UHealthComponent>() : nullptr;
	if (!Health)
	{
		return;
	}

	// The open grave nearest where they fell, standing on it and facing its way; with none open, the level's own start.
	// Never a trip's landing. The story's graves come from the session's campaign record.
	FVector Location = Character->GetActorLocation();
	FRotator Rotation = Character->GetActorRotation();
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const FRespawnWakeSpot Spot = ARespawnMarker::ChooseWakeSpot(GetWorld(), Vitals.DeathLocation, Runner ? &Runner->GetCampaign() : nullptr);
	if (Spot.IsSet())
	{
		Location = Spot.Location;
		Rotation = Spot.Facing;
		// A grave is a spot on the ground: the player stands on it, half their height higher. A player start already
		// marks where their middle goes.
		if (Spot.Grave)
		{
			Location.Z += Character->GetDefaultHalfHeight();
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Player wakes %s"), Spot.Grave ? *FString::Printf(TEXT("at the grave %s"), *Spot.Grave->GetMarkerId().ToString())
		: Spot.Start ? TEXT("at the level's start") : TEXT("where they fell (the level has no start of its own)"));

	Character->TeleportTo(Location, Rotation, false, true);
	PC->SetControlRotation(Rotation);
	Health->ResetHealth();
	PC->ResetIgnoreMoveInput();
	PC->ResetIgnoreLookInput();
	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.8f, FLinearColor(0.12f, 0.f, 0.f), false, false);
	}

	Vitals.bDying = false;
	Vitals.LastHealth = Health->GetHealth();
}
