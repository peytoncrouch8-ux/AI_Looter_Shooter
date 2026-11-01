// APilotSortie: Main 6's escape from her side (step 7): she hangs under the dying barge's broken gangway; a player who
// misses the jump is caught under her by Hob and put on her deck; one who lands on it plays BargeFall from there, and once
// it's over she flies home.

#include "World/PilotSortie.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Scenes/BargeFall.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "World/BargeBoarding.h"
#include "World/LamplighterShip.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "PilotSortie"

namespace
{
	APawn* JumpPlayer(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}
}

void APilotSortie::AdvanceJump(float DeltaSeconds)
{
	ALamplighterShip* Boat = FindShip();
	UWorld* World = GetWorld();
	APawn* Player = JumpPlayer(World);
	if (!bAtJump || !Boat || !Player || bFallAsked)
	{
		return;
	}
	const ACharacter* Character = Cast<ACharacter>(Player);
	const bool bFalling = Character && Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsFalling();
	const FVector Local = Boat->GetActorTransform().InverseTransformPositionNoScale(Player->GetActorLocation());
	if (PilotLight::IsCaughtBelow(Local, bFalling, ALamplighterShip::DeckTop, ALamplighterShip::RailOut))
	{
		// Missed her: Hob has them by the collar and drops them on her deck, out of a short pale flash.
		++Catches;
		const APlayerController* Controller = World->GetFirstPlayerController();
		if (Controller && Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.6f, FLinearColor(0.75f, 0.88f, 1.f), false, false);
		}
		Boat->PutPlayerOnDeck();
		if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
		{
			Captions->Play({ FStoryLine::Make(LOCTEXT("Hob", "Hob"), LOCTEXT("HobCatch", "Got you, sunshine. Aim for the deck next time."), 3.f) },
				ECaptionPlay::Interrupt);
		}
		LooterSound::PlayAt(this, LooterSoundCue::Crow, Boat->DeckPoint(FVector(300.0, 0.0, 120.0)));
		UE_LOG(LogLooter, Log, TEXT("The Pilot Light: the player missed the jump; Hob caught them onto her deck (%d)."), Catches);
		LandedFor = 0.f;
		return;
	}
	if (Boat->IsPlayerAboard())
	{
		LandedFor += DeltaSeconds;
		if (LandedFor >= PilotLight::LandedSeconds)
		{
			PlayFall();
		}
	}
	else
	{
		LandedFor = 0.f;
	}
}

void APilotSortie::PlayFall()
{
	if (Phase != EPilotPhase::Falling)
	{
		// Falling first: the scene's end (or its being passed over) tells the missions, and the story mustn't find her mid-jump.
		EnterPhase(EPilotPhase::Falling, /*bFresh*/ false);
	}
	ALamplighterShip* Boat = FindShip();
	ABargeBoarding* Here = FindBarge();
	bFallAsked = true;
	if (!Here)
	{
		// No barge to fall (a level before W3's): straight home.
		UE_LOG(LogLooter, Warning, TEXT("The Pilot Light: no barge in the level: no BargeFall; she flies home."));
		EnterPhase(EPilotPhase::Home, /*bFresh*/ false);
		return;
	}
	// Played from her deck; when it can't play (scenes off, another scene) it leaves the barge fallen at once and tells
	// the missions it played (BargeFall::Play), so either way she flies home once it's over (AdvanceFalling).
	BargeFall::Play(*Here, Boat);
}

void APilotSortie::AdvanceFalling()
{
	if (!bFallAsked)
	{
		PlayFall();
		return;
	}
	const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	const bool bStillPlaying = Scenes && Scenes->IsPlaying() && Scenes->GetPlayingName() == BargeFall::SceneName();
	if (!bStillPlaying && (!Scenes || Scenes->HasPlayed(BargeFall::SceneName())))
	{
		// The barge is down in the canyon: home.
		EnterPhase(EPilotPhase::Home, /*bFresh*/ false);
	}
}

#undef LOCTEXT_NAMESPACE
