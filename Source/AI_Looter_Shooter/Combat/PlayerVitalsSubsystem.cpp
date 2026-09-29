#include "Combat/PlayerVitalsSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"

namespace
{
	const FLinearColor HurtColor(0.55f, 0.04f, 0.02f);
}

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
		// Only the player's own character (Build Mode possesses a camera pawn without health).
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

		// Flash red in proportion to the hit.
		if (Vitals.LastHealth >= 0.f && Current < Vitals.LastHealth - 0.5f && PC->PlayerCameraManager)
		{
			const float Strength = FMath::Clamp((Vitals.LastHealth - Current) / Health->GetMaxHealth() * 3.f, 0.2f, 0.55f);
			PC->PlayerCameraManager->StartCameraFade(Strength, 0.f, 0.35f, HurtColor, false, false);
		}
		Vitals.LastHealth = Current;

		if (Health->IsDead())
		{
			UE_LOG(LogLooter, Log, TEXT("Player died; respawning in %.1fs"), RespawnDelay);
			Vitals.bDying = true;
			Vitals.DeathTime = 0.f;
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

	FVector Location = Character->GetActorLocation();
	FRotator Rotation = Character->GetActorRotation();
	for (TActorIterator<APlayerStart> Start(GetWorld()); Start; ++Start)
	{
		Location = Start->GetActorLocation();
		Rotation = FRotator(0.f, Start->GetActorRotation().Yaw, 0.f);
		break;
	}

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
