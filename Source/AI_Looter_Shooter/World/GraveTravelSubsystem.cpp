#include "World/GraveTravelSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "World/RespawnMarker.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The autosave and save-soons wait while the screen is black: the player is between two places. */
	const FName TravelHold(TEXT("GraveTravel"));
}

UGraveTravelSubsystem* UGraveTravelSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UGraveTravelSubsystem>() : nullptr;
}

bool UGraveTravelSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests (the rules, and the travel moved on by Advance).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UGraveTravelSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

void UGraveTravelSubsystem::Advance(float DeltaSeconds)
{
	if (!bTravelling)
	{
		return;
	}
	TravelClock += DeltaSeconds;
	if (TravelClock >= GraveTravelRules::FadeOutSeconds + GraveTravelRules::HoldSeconds)
	{
		Arrive();
	}
}

void UGraveTravelSubsystem::Deinitialize()
{
	if (bTravelling)
	{
		if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
		{
			Sessions->ReleaseSaves(TravelHold);
		}
		bTravelling = false;
	}
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// The rules, as the world stands
// ---------------------------------------------------------------------------

FGraveTravelSenses UGraveTravelSubsystem::Sense(const APawn* Player) const
{
	FGraveTravelSenses Senses;
	const UWorld* World = GetWorld();
	Senses.bHasPlayer = IsValid(Player);
	if (Senses.bHasPlayer)
	{
		const UHealthComponent* Health = Player->FindComponentByClass<UHealthComponent>();
		Senses.bDying = Health && Health->IsDead();
	}
	if (const USceneSubsystem* Scenes = USceneSubsystem::Get(this))
	{
		Senses.bInScene = Scenes->IsPlaying() || Scenes->IsHoldingPlayer();
	}
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	const UAreaTravelSubsystem* Trips = UAreaTravelSubsystem::Get(this);
	Senses.bTravelling = bTravelling || (Sessions && Sessions->IsTravelling()) || (Trips && Trips->IsDeparting());

	// A few dozen creatures at most, asked only while the map is open (a few times a second) or as a travel starts.
	if (World)
	{
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			const ACreatureBase* Creature = *It;
			if (!IsValid(Creature) || Creature->IsDead())
			{
				continue;
			}
			if (const UBossComponent* Boss = Creature->FindComponentByClass<UBossComponent>())
			{
				Senses.bBossFight |= Boss->IsFighting();
			}
			if (Senses.bHasPlayer && Creature->GetTarget() == Player)
			{
				++Senses.Hunters;
			}
		}
	}
	return Senses;
}

EGraveTravelBlock UGraveTravelSubsystem::CheckNow(const APawn* Player) const
{
	return GraveTravelRules::CheckNow(Sense(Player));
}

EGraveTravelBlock UGraveTravelSubsystem::CheckGrave(const APawn* Player, const ARespawnMarker& Grave) const
{
	const double Distance = Player ? FVector::Dist2D(Player->GetActorLocation(), Grave.GetActorLocation()) : TNumericLimits<double>::Max();
	return GraveTravelRules::CheckGrave(Sense(Player), IsOpen(Grave), Distance);
}

bool UGraveTravelSubsystem::IsOpen(const ARespawnMarker& Grave) const
{
	// The story's record says which graves are open (it's the session's while one is played); a world with no mission
	// runner has no story, so only the graves open from the start.
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner ? Grave.IsActive(Runner->GetCampaign()) : Grave.bStartActive;
}

// ---------------------------------------------------------------------------
// Travelling
// ---------------------------------------------------------------------------

bool UGraveTravelSubsystem::TravelTo(APawn* Player, const ARespawnMarker& Grave, EGraveTravelBlock* OutBlock)
{
	const EGraveTravelBlock Block = CheckGrave(Player, Grave);
	if (OutBlock)
	{
		*OutBlock = Block;
	}
	UWorld* World = GetWorld();
	if (Block != EGraveTravelBlock::None || !World || !Player)
	{
		UE_LOG(LogLooter, Log, TEXT("Fast travel to %s: not now (%s)"), *Grave.GetMarkerId().ToString(), *GraveTravelRules::Reason(Block).ToString());
		return false;
	}

	bTravelling = true;
	TravelClock = 0.f;
	Traveller = Player;
	Destination = &Grave;
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->HoldSaves(TravelHold);
	}
	// The player stops where they stand while the screen goes black. The world's sound carries on under the whoosh (a
	// camera fade's audio fade would take the whoosh with it).
	if (APlayerController* Controller = Cast<APlayerController>(Player->GetController()))
	{
		Controller->SetIgnoreMoveInput(true);
		Controller->SetIgnoreLookInput(true);
		if (Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->StartCameraFade(0.f, 1.f, GraveTravelRules::FadeOutSeconds, FLinearColor::Black,
				/*bShouldFadeAudio*/ false, /*bHoldWhenFinished*/ true);
		}
	}
	// The move comes once the black has held (Advance, on the subsystem's tick).
	LooterSound::Play2D(this, LooterSoundCue::TravelWhoosh);
	UE_LOG(LogLooter, Log, TEXT("Fast travel: to the grave %s (%.0f m away)"), *Grave.GetMarkerId().ToString(),
		FVector::Dist2D(Player->GetActorLocation(), Grave.GetActorLocation()) / 100.0);
	return true;
}

void UGraveTravelSubsystem::Arrive()
{
	APawn* Player = Traveller.Get();
	const ARespawnMarker* Grave = Destination.Get();
	const UHealthComponent* Health = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
	if (Health && Health->IsDead())
	{
		// Killed in the black (a fall): the death's own fade and wake-up take over, keys and all.
		UE_LOG(LogLooter, Warning, TEXT("Fast travel: the player died in the black; the wake-up takes over."));
		EndTravel(nullptr);
		return;
	}
	// The grave or the player may have gone in the black (a level going): nobody moves, the screen comes back.
	if (Player && Grave)
	{
		MoveToGrave(*Player, *Grave);
		UE_LOG(LogLooter, Log, TEXT("Fast travel: arrived at the grave %s"), *Grave->GetMarkerId().ToString());
		EndTravel(Player);
		OnTravelled.Broadcast(Player, *Grave);
		return;
	}
	UE_LOG(LogLooter, Warning, TEXT("Fast travel: the player or the grave went away in the black; nobody moved."));
	EndTravel(Player);
}

void UGraveTravelSubsystem::EndTravel(APawn* Player)
{
	bTravelling = false;
	Traveller.Reset();
	Destination.Reset();
	// Null: someone else has the screen and the keys now (a death's fade).
	if (APlayerController* Controller = Player ? Cast<APlayerController>(Player->GetController()) : nullptr)
	{
		Controller->ResetIgnoreMoveInput();
		Controller->ResetIgnoreLookInput();
		if (Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, GraveTravelRules::FadeInSeconds, FLinearColor::Black, false, false);
		}
	}
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->ReleaseSaves(TravelHold);
	}
}

void UGraveTravelSubsystem::MoveToGrave(APawn& Player, const ARespawnMarker& Grave)
{
	// As a death's wake-up: the grave's spot is on the ground, so the player's middle goes half their height above it.
	const FRespawnWakeSpot Spot = Grave.GetWakeSpot();
	const FVector Location = GraveTravelRules::ArrivalLocation(Spot.Location, Player.GetDefaultHalfHeight());
	Player.TeleportTo(Location, Spot.Facing, /*bIsATest*/ false, /*bNoCheck*/ true);
	if (const ACharacter* Character = Cast<ACharacter>(&Player))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
	}
	if (AController* Controller = Player.GetController())
	{
		Controller->SetControlRotation(Spot.Facing);
	}
}
