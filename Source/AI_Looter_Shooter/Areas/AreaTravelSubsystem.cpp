#include "Areas/AreaTravelSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaDefinition.h"
#include "Areas/StationBoard.h"
#include "Core/LooterMenuGameMode.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Scenes/ColdOpenSubsystem.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "World/SkiffJetty.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{
	/** The autosave and save-soons wait while a plain trip fades out. */
	const FName FadeHold(TEXT("TripFade"));

	UPlayerProgressionSubsystem* FindProgression(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const ULocalPlayer* Player = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
		return Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	}
}

UAreaTravelSubsystem* UAreaTravelSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UAreaTravelSubsystem>() : nullptr;
}

bool UAreaTravelSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAreaTravelSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Read now: as the level's start puts the player on the trip's landing, the session forgets which it was.
	const USessionSubsystem* Sessions = USessionSubsystem::Get(&InWorld);
	ArrivalLanding = Sessions && !ALooterMenuGameMode::IsMenuWorld(&InWorld) ? Sessions->GetArrivalLanding() : NAME_None;
	InWorld.OnWorldBeginPlay.AddUObject(this, &UAreaTravelSubsystem::HandleLevelBegun);
}

void UAreaTravelSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeTimer);
	}
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Leaving
// ---------------------------------------------------------------------------

bool UAreaTravelSubsystem::Depart(const FStationBoardLine& Line, AActor* From)
{
	if (!Line.IsDestination())
	{
		return false;
	}
	UAreaDefinition* Area = Line.Area.Get();
	if (!Area && !Line.AreaId.IsNone())
	{
		Area = UAreaDefinition::FindByName(Line.AreaId.ToString());
	}
	if (!Area || !Area->HasMap())
	{
		UE_LOG(LogLooter, Warning, TEXT("Trip: %s's level isn't in the game yet, so nobody goes."), *Line.Name.ToString());
		return false;
	}
	if (bDeparting)
	{
		UE_LOG(LogLooter, Warning, TEXT("Trip: one is already leaving."));
		return false;
	}
	if (Line.bFirstCastOff)
	{
		// Off Skyreach for the first time: the skiff's ride when its jetty is here, else straight on behind the white.
		ASkiffJetty* Jetty = Cast<ASkiffJetty>(From);
		for (TActorIterator<ASkiffJetty> It(GetWorld()); !Jetty && It; ++It)
		{
			Jetty = *It;
		}
		return (Jetty && Jetty->CastOff()) || LeaveForFirstArrival();
	}
	return FadeTo(*Area, Line.Landing);
}

bool UAreaTravelSubsystem::FadeTo(UAreaDefinition& Area, FName Landing)
{
	UWorld* World = GetWorld();
	USessionSubsystem* Sessions = USessionSubsystem::Get(World);
	if (!World || !Sessions || bDeparting || Sessions->IsTravelling())
	{
		UE_LOG(LogLooter, Warning, TEXT("Trip to %s: %s, so nobody goes."), *Area.DisplayName.ToString(),
			bDeparting || (Sessions && Sessions->IsTravelling()) ? TEXT("a trip is already under way") : TEXT("there's no game to leave"));
		return false;
	}
	if (!Area.HasMap())
	{
		UE_LOG(LogLooter, Warning, TEXT("Trip: %s's level isn't in the game yet, so nobody goes."), *Area.DisplayName.ToString());
		return false;
	}
	bDeparting = true;
	FadeArea = &Area;
	FadeLanding = Landing;
	Sessions->HoldSaves(FadeHold);
	// The player stops where they stand while the screen goes black; the trip takes them from there.
	if (APlayerController* Controller = World->GetFirstPlayerController())
	{
		Controller->SetIgnoreMoveInput(true);
		if (Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->StartCameraFade(0.f, 1.f, FadeOutSeconds, FLinearColor::Black, /*bShouldFadeAudio*/ true,
				/*bHoldWhenFinished*/ true);
		}
	}
	World->GetTimerManager().SetTimer(FadeTimer, this, &UAreaTravelSubsystem::FinishFade, FadeOutSeconds, false);
	UE_LOG(LogLooter, Log, TEXT("Trip: fading out for %s (arriving at %s)"), *Area.DisplayName.ToString(),
		Landing.IsNone() ? TEXT("its start") : *Landing.ToString());
	return true;
}

void UAreaTravelSubsystem::FinishFade()
{
	UWorld* World = GetWorld();
	USessionSubsystem* Sessions = USessionSubsystem::Get(World);
	UAreaDefinition* Area = FadeArea.Get();
	FadeArea = nullptr;
	if (Area && Sessions && Sessions->TravelToArea(*Area, FadeLanding))
	{
		// The destination opens behind the black and fades in as it begins.
		return;
	}
	// Nobody goes (the session couldn't save, a trip got there first): the screen comes back where the player stood.
	bDeparting = false;
	if (Sessions)
	{
		Sessions->ReleaseSaves(FadeHold);
	}
	if (APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr)
	{
		Controller->SetIgnoreMoveInput(false);
		if (Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeInSeconds, FLinearColor::Black, true, false);
		}
	}
}

bool UAreaTravelSubsystem::LeaveForFirstArrival()
{
	if (bDeparting)
	{
		return false;
	}
	UTransitionScreenSubsystem* White = GetTransition();
	if (White)
	{
		White->HoldWhite();
	}
	if (CompleteFirstCastOff())
	{
		return true;
	}
	if (White)
	{
		White->Reveal();
	}
	return false;
}

bool UAreaTravelSubsystem::CompleteFirstCastOff()
{
	UWorld* World = GetWorld();
	USessionSubsystem* Sessions = USessionSubsystem::Get(World);
	FCampaignRecord* Campaign = FindCampaign(World);
	UAreaDefinition* First = UAreaDefinition::FindByName(StationBoard::FirstAreaId().ToString());
	if (!Sessions || !Campaign || !First || !First->HasMap() || bDeparting)
	{
		UE_LOG(LogLooter, Warning, TEXT("First cast-off: %s, so nobody goes."),
			!First ? TEXT("there's no area asset for the story's first area")
			: !First->HasMap() ? TEXT("the story's first level isn't in the game yet")
			: bDeparting ? TEXT("a trip is already leaving") : TEXT("there's no game to leave"));
		return false;
	}

	// Written before the trip, so the trip's own save carries it: the story has begun, its first area is open, and the
	// tutorial is behind the player (a practice visit never sends them through it again). Put back if nobody goes.
	const FCampaignRecord Before = *Campaign;
	UPlayerProgressionSubsystem* Progression = FindProgression(World);
	const bool bTutorialWasDone = Progression && Progression->IsTutorialDone();
	StationBoard::RecordFirstCastOff(*Campaign);
	if (Progression)
	{
		Progression->SetTutorialDone(true);
	}
	bDeparting = true;
	if (Sessions->TravelToArea(*First, StationBoard::FirstArrivalLanding()))
	{
		UE_LOG(LogLooter, Log, TEXT("First cast-off: the story begins on %s, at %s."), *First->DisplayName.ToString(),
			*StationBoard::FirstArrivalLanding().ToString());
		return true;
	}
	bDeparting = false;
	*Campaign = Before;
	if (Progression)
	{
		Progression->SetTutorialDone(bTutorialWasDone);
	}
	UE_LOG(LogLooter, Warning, TEXT("First cast-off: the trip couldn't go, so the story waits."));
	return false;
}

// ---------------------------------------------------------------------------
// Arriving
// ---------------------------------------------------------------------------

void UAreaTravelSubsystem::HandleLevelBegun()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// The story's first area opens on the cold open while it's due (the first arrival): it takes the white the trip held
	// and reveals it with the title on the gang's skiff, seven days ago.
	if (UColdOpenSubsystem* Opening = World->GetSubsystem<UColdOpenSubsystem>())
	{
		if (Opening->BeginIfDue())
		{
			return;
		}
	}
	// A white held through the load comes off now, the player standing on their landing: with the game's title on the
	// story's first arrival, plain on any other.
	UTransitionScreenSubsystem* White = GetTransition();
	if (White && White->IsHeld())
	{
		const FText Title = ArrivalTitle(ArrivalLanding);
		UE_LOG(LogLooter, Log, TEXT("Arrival: the white comes off%s"), Title.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(", %s rising through it"),
			*Title.ToString()));
		White->Reveal(Title);
		return;
	}
	// A plain trip went out through black: it comes back the same way.
	if (!ArrivalLanding.IsNone())
	{
		const APlayerController* Controller = World->GetFirstPlayerController();
		if (Controller && Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeInSeconds, FLinearColor::Black, true, false);
		}
	}
}

FText UAreaTravelSubsystem::ArrivalTitle(FName Landing)
{
	return !Landing.IsNone() && Landing == StationBoard::FirstArrivalLanding() ? UTransitionScreenSubsystem::GameTitle() : FText::GetEmpty();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

TArray<FStationBoardLine> UAreaTravelSubsystem::GatherBoardLines(const UWorld* World, TArray<UAreaDefinition*>& OutAreas)
{
	OutAreas = UAreaDefinition::LoadAll();
	const FCampaignRecord* Campaign = FindCampaign(World);
	if (!Campaign)
	{
		return TArray<FStationBoardLine>();
	}
	// The level's missions (the runner's, code-made ones too), else every mission asset.
	TArray<UMissionDefinition*> Missions;
	const UMissionRunner* Runner = World ? World->GetSubsystem<UMissionRunner>() : nullptr;
	if (Runner && Runner->IsActive())
	{
		for (UMissionDefinition* Mission : Runner->GetDefinitions())
		{
			Missions.Add(Mission);
		}
	}
	else
	{
		Missions = UMissionDefinition::LoadAll();
	}
	const FString Map = USessionSubsystem::MapOf(World);
	FName HereAreaId;
	for (const UAreaDefinition* Area : OutAreas)
	{
		if (Area && !Map.IsEmpty() && Area->GetMapPackage().Equals(Map, ESearchCase::IgnoreCase))
		{
			HereAreaId = Area->GetAreaId();
		}
	}
	return StationBoard::BuildLines(*Campaign, OutAreas, Missions, HereAreaId);
}

FCampaignRecord* UAreaTravelSubsystem::FindCampaign(const UWorld* World)
{
	// The runner's record is the session's (kept in memory without one); a test's own in the tests.
	const UMissionRunner* Runner = World ? World->GetSubsystem<UMissionRunner>() : nullptr;
	if (Runner)
	{
		return &Runner->GetCampaign();
	}
	USessionSubsystem* Sessions = USessionSubsystem::Get(World);
	return Sessions ? Sessions->GetCampaign() : nullptr;
}

UTransitionScreenSubsystem* UAreaTravelSubsystem::GetTransition() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UTransitionScreenSubsystem>() : nullptr;
}
