#include "Combat/PlayerVitalsSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/HealthComponent.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "World/RespawnMarker.h"
#include "World/RoostCarrySubsystem.h"
#include "World/RoostVeil.h"
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

const FRecoverySettings& UPlayerVitalsSubsystem::SettingsFor(const UObject* WorldContext)
{
	static const FRecoverySettings Defaults;
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UPlayerVitalsSubsystem* Vitals = World ? World->GetSubsystem<UPlayerVitalsSubsystem>() : nullptr;
	return Vitals ? Vitals->Recovery : Defaults;
}

bool UPlayerVitalsSubsystem::IsRegenerating(const APlayerController* PC) const
{
	const TWeakObjectPtr<APlayerController> Key(const_cast<APlayerController*>(PC));
	const FPlayerVitals* Vitals = PC ? Players.Find(Key) : nullptr;
	return Vitals && !Vitals->bDying && Vitals->Wounds.IsRegenerating();
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
			Vitals.Wounds.Reset();
			PC->SetIgnoreMoveInput(true);
			PC->SetIgnoreLookInput(true);
			// Where they'll wake decides how they go: on a map with roosts, Hob's crows gather round the dying and drag them
			// back (the screen goes slower to black under the crows, URoostCarrySubsystem); elsewhere, the plain fade.
			const UMissionRunner* Runner = UMissionRunner::Get(this);
			Vitals.bByCrows = ARespawnMarker::ChooseWakeSpot(GetWorld(), Runner ? &Runner->GetCampaign() : nullptr).bByCrows;
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(Vitals.bByCrows ? 0.25f : 0.4f, 1.f, RespawnDelay * (Vitals.bByCrows ? CrowsCover : 0.6f),
					FLinearColor(0.12f, 0.f, 0.f), false, true);
			}
			URoostCarrySubsystem* Carry = Vitals.bByCrows ? URoostCarrySubsystem::Get(this) : nullptr;
			if (Carry)
			{
				Carry->BeginDeath(PC, RespawnDelay * CrowsCover);
			}
		}
		else
		{
			// The wounds close on their own after a while without being hurt. Not in a scene (it holds the player, and
			// takes no damage): the wait is kept, the ramp starts over after it. Dead and dying never reach here.
			const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
			const bool bInScene = Scenes && (Scenes->IsPlaying() || Scenes->IsHoldingPlayer());
			Vitals.Wounds.Tick(Recovery, *Health, DeltaTime, bInScene);
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

	// The roost they last found on this map (the user's rule, 2026-10-14: wherever they fell, never the nearest), standing
	// on it and facing its way; with none found yet, the level's own start. Never a trip's landing. The story's finds come
	// from the session's campaign record.
	FVector Location = Character->GetActorLocation();
	FRotator Rotation = Character->GetActorRotation();
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const FRespawnWakeSpot Spot = ARespawnMarker::ChooseWakeSpot(GetWorld(), Runner ? &Runner->GetCampaign() : nullptr);
	if (Spot.IsSet())
	{
		Location = Spot.Location;
		Rotation = Spot.Facing;
		// A roost's spot is on the ground: the player stands on it, half their height higher. A player start already
		// marks where their middle goes.
		if (Spot.Grave)
		{
			Location.Z += Character->GetDefaultHalfHeight();
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Player wakes %s"), Spot.Grave ? *FString::Printf(TEXT("at the roost %s (the last found here)"), *Spot.Grave->GetMarkerId().ToString())
		: Spot.Start ? TEXT("at the level's start (no roost found here yet)") : TEXT("where they fell (the level has no start of its own)"));

	Character->TeleportTo(Location, Rotation, false, true);
	PC->SetControlRotation(Rotation);
	Health->ResetHealth();
	PC->ResetIgnoreMoveInput();
	PC->ResetIgnoreLookInput();
	if (PC->PlayerCameraManager)
	{
		// Carried by the crows, the world comes back as they burst off the screen; elsewhere, the plain fade.
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, Spot.bByCrows ? FRoostVeil::RevealSeconds : 0.8f, FLinearColor(0.12f, 0.f, 0.f), false, false);
	}
	URoostCarrySubsystem* Carry = URoostCarrySubsystem::Get(this);
	if (Carry)
	{
		Carry->Reveal();
	}
	if (Spot.Grave)
	{
		// The crows that dragged them burst off the roost round them, and Hob has his word.
		Spot.Grave->Welcome(ERoostArrival::Death, Character);
	}
	else if (Spot.bByCrows)
	{
		// No roost found yet: the crows drop Ellis at the level's start (the family plot) all the same, with the flock bursting
		// off them and Hob's word, as at a roost.
		LooterSound::PlayAt(this, LooterSoundCue::RoostMark::DeathWake, Location);
		if (Carry)
		{
			Carry->SayOnArrival(ERoostArrival::Death);
		}
	}

	Vitals.bDying = false;
	Vitals.bByCrows = false;
	Vitals.LastHealth = Health->GetHealth();
	// A new life waits the whole delay before its first healing.
	Vitals.Wounds.Reset();
}
