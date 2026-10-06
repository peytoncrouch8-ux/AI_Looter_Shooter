// ASkiffJetty: the first cast-off (the ride, its ropes slipping, the white, the trip) and what's put back if nobody goes.

#include "World/SkiffJetty.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The ride's moment the ropes slip (USceneSubsystem::OnSceneEvent). */
	const FName CastOffEvent(TEXT("CastOff"));

	UTransitionScreenSubsystem* FindWhite(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UTransitionScreenSubsystem>() : nullptr;
	}
}

bool ASkiffJetty::CastOff()
{
	UWorld* World = GetWorld();
	if (bCastingOff || !Skiff || !World)
	{
		return false;
	}
	bCastingOff = true;
	UE_LOG(LogLooter, Log, TEXT("%s: casting off."), *GetActorNameOrLabel());

	// The station board closes before the ride takes hold (closing gives game input back, which the ride then holds),
	// whatever cast off: the board's own Cast off, or Looter.Station.CastOff with the board still open.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALooterHUD* HUD = It->Get() ? Cast<ALooterHUD>(It->Get()->GetHUD()) : nullptr)
		{
			HUD->CloseStationBoard();
		}
	}

	// "Board the skiff" is done as the player goes aboard.
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(FMissionEvent::BoardEvent(Vehicle), this));
	}

	// The ride moves the skiff on its own (holding the player and the autosaves while it plays); its lines follow it until
	// they slip.
	Skiff->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	USceneSubsystem* Scenes = World->GetSubsystem<USceneSubsystem>();
	if (Scenes)
	{
		SceneEventHandle = Scenes->OnSceneEvent.AddUObject(this, &ASkiffJetty::HandleSceneEvent);
		if (Scenes->PlaySkiffRide(Skiff.Get(), FOnSceneMoment::CreateUObject(this, &ASkiffJetty::HandleWhiteout)))
		{
			bRiding = true;
			RefreshTick();
			return true;
		}
		StopListeningToScene();
	}

	// No ride (scenes off in tour and perf runs, or one playing already): straight on, behind the white.
	UE_LOG(LogLooter, Log, TEXT("%s: no ride plays, so the trip goes at once."), *GetActorNameOrLabel());
	if (UTransitionScreenSubsystem* White = FindWhite(World))
	{
		White->HoldWhite();
	}
	SlipLines();
	HandleWhiteout();
	return true;
}

void ASkiffJetty::HandleSceneEvent(FName Event)
{
	if (Event == CastOffEvent)
	{
		// The ropes slip as the skiff leaves, and the plank comes in behind the player.
		SlipLines();
		SetGangplankDown(false, /*bInstant*/ false);
	}
}

void ASkiffJetty::HandleWhiteout()
{
	StopListeningToScene();
	UAreaTravelSubsystem* Travel = UAreaTravelSubsystem::Get(this);
	if (Travel && Travel->CompleteFirstCastOff())
	{
		// The level goes behind the white, and the skiff with it, wherever the ride left it.
		return;
	}

	// Nobody goes: the skiff is moored again at once. After a ride, the scene gives the player back where it took them
	// from when no trip comes, and reveals the white with them; without one they never moved, and the white comes off now.
	UE_LOG(LogLooter, Warning, TEXT("%s: the trip couldn't go; the skiff is back at its mooring."), *GetActorNameOrLabel());
	const bool bRode = bRiding;
	RestoreMooring();
	UTransitionScreenSubsystem* White = FindWhite(GetWorld());
	if (!bRode && White && White->IsHeld())
	{
		White->Reveal();
	}
}

void ASkiffJetty::RestoreMooring()
{
	bCastingOff = false;
	bRiding = false;
	if (Skiff && Jetty)
	{
		Skiff->SetActorTransform(ComputeMooring() * Jetty->GetComponentTransform());
		Skiff->AttachToComponent(Jetty, FAttachmentTransformRules::KeepWorldTransform);
	}
	bLinesSlipped = false;
	SlipTime = 0.f;
	UpdateLines(0.f);
	// It was down to board, so it's down again.
	SetGangplankDown(true, /*bInstant*/ true);
}

void ASkiffJetty::StopListeningToScene()
{
	if (!SceneEventHandle.IsValid())
	{
		return;
	}
	if (USceneSubsystem* Scenes = GetWorld() ? GetWorld()->GetSubsystem<USceneSubsystem>() : nullptr)
	{
		Scenes->OnSceneEvent.Remove(SceneEventHandle);
	}
	SceneEventHandle.Reset();
}
