// ALamplighterShip: how she begins from the campaign (a flight, or moored), the player put on her deck, the session's
// saves held while she flies, and the crew's lines on Main 1's steps.

#include "World/LamplighterShip.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The crew start talking this long into the flight: once the arrival's title has risen through the white. */
	constexpr float DeckLinesDelay = 4.5f;
}

bool ALamplighterShip::BeginLandfall()
{
	if (bBegun)
	{
		return Flight != EShipFlight::Moored;
	}
	bBegun = true;
	UWorld* World = GetWorld();
	FCampaignRecord* Campaign = UAreaTravelSubsystem::FindCampaign(World);
	ELandfallStart Start = Campaign ? LandfallRules::StartFor(*Campaign) : ELandfallStart::Flight;
	if (Start == ELandfallStart::Flight && (!HasCourse() || !USceneSubsystem::AreScenesOn(World)))
	{
		// A tour or perf run (scenes off), or no course placed yet: she lies moored and anchored, the story untouched.
		UE_LOG(LogLooter, Log, TEXT("Landfall: %s, so the Lamplighter lies moored and nothing flies."),
			HasCourse() ? TEXT("scenes are off") : TEXT("she has no course (build_area_landfall.py places it)"));
		bAnchored = true;
		EnterFlight(EShipFlight::Moored);
		return false;
	}
	if (Campaign)
	{
		LandfallRules::ApplyStart(*Campaign, Start);
	}
	switch (Start)
	{
	case ELandfallStart::Flight:
		Clock = 0.f;
		Distance = 0.f;
		Speed = FLamplighterCourse::CruiseSpeed;
		bDeckLinesSaid = false;
		DeckLinesWait = DeckLinesDelay;
		EnterFlight(EShipFlight::Sailing);
		PutPlayerOnDeck();
		HoldSaves(true);
		UE_LOG(LogLooter, Log, TEXT("Landfall: the Lamplighter sails in (%.0f m of course, grappled %.0f m along)."), Path.GetLength() / 100.f,
			GetGrappleStopDistance() / 100.f);
		return true;
	case ELandfallStart::MooredOldSave:
		UE_LOG(LogLooter, Log, TEXT("Landfall: a save from before the Lamplighter: she's moored and anchored, Main 1 goes on from the climb to the Point."));
		bAnchored = true;
		EnterFlight(EShipFlight::Moored);
		FindOrSpawnWinch();
		return false;
	case ELandfallStart::Moored:
	default:
		bAnchored = !Campaign || LandfallRules::IsAnchoredIn(*Campaign);
		EnterFlight(EShipFlight::Moored);
		// The winch shows her chain when she's anchored, and waits to be wound when she isn't.
		FindOrSpawnWinch();
		return false;
	}
}

bool ALamplighterShip::PutPlayerOnDeck()
{
	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn || !LandingSpot)
	{
		return false;
	}
	// Standing on the spot (a pawn's location is its middle), facing the prow and the Lantern ahead.
	const FTransform Spot = LandingSpot->GetComponentTransform();
	const FRotator Facing(0.0, Spot.Rotator().Yaw, 0.0);
	Pawn->TeleportTo(Spot.GetLocation() + FVector(0.0, 0.0, Pawn->GetDefaultHalfHeight() + 2.0), Facing, /*bIsATest*/ false, /*bNoCheck*/ true);
	Controller->SetControlRotation(Facing);
	// Her deck is their floor from the first frame, so they ride her before she's moved an inch (walking finds it next
	// frame anyway; this keeps the first frame from leaving them behind).
	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
		Character->SetBase(Deck.Get());
	}
	OffDeckFor = 0.f;
	return true;
}

void ALamplighterShip::HoldSaves(bool bHold)
{
	if (bHold == bSavesHeld)
	{
		return;
	}
	USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	if (!Sessions)
	{
		return;
	}
	// A save mid-air would wake the player in the sky: the autosaves wait until she's down (Save & Quit still saves, and
	// that session flies again from the start: LandfallRules::StartFor).
	if (bHold)
	{
		Sessions->HoldSaves(LandfallRules::SaveHold());
	}
	else
	{
		Sessions->ReleaseSaves(LandfallRules::SaveHold());
	}
	bSavesHeld = bHold;
}

void ALamplighterShip::BindMissions()
{
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &ALamplighterShip::HandleMissionsChanged);
	}
}

int32 ALamplighterShip::GetMainStep() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const FName Main = LandfallRules::MissionId();
	return Runner && Runner->IsRunning(Main) ? Runner->GetStep(Main) : INDEX_NONE;
}

void ALamplighterShip::HandleMissionsChanged()
{
	const int32 Step = GetMainStep();
	if (Step == LastMainStep)
	{
		return;
	}
	LastMainStep = Step;
	// Main 1 past its first step (the prow reached): the tender comes.
	if (Step > LandfallRules::StepProw && Flight == EShipFlight::Sailing)
	{
		CallTender();
	}
	// The climb's and the Point's lines (DA_Lines_LandfallClimb, DA_Lines_LandfallPoint) are Hob's arrivals on the
	// lookout rail as steps 5 and 6 begin (story_texts LANDFALL_PERCHES): one owner, so they're never heard twice.
	bPointLinesSaid = true;
}
