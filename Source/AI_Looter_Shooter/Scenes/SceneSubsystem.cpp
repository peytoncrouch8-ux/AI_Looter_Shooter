// USceneSubsystem: playing scenes one at a time, skipping them, and whether they play at all.

#include "Scenes/SceneSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Dev/ViewTour.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SkiffRide.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CString.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	TAutoConsoleVariable<int32> CVarScenes(
		TEXT("Looter.Scenes"),
		1,
		TEXT("Scenes (the skiff ride, the cold open, the grave wake-up): 1 plays them except in tour and perf runs, 0 never (nothing plays and ")
		TEXT("the game goes straight on, a scene playing skips to its end), 2 always, even in a tour or perf run (to measure one)."));

	/** The mission event a scene's end sends: the missions hear Scene.<Name>. */
	void TellMissions(const UObject* WorldContext, FName Scene)
	{
		if (UMissionRunner* Runner = UMissionRunner::Get(WorldContext))
		{
			Runner->NotifyEvent(FMissionEvent::Named(FMissionEvent::SceneEvent(Scene)));
		}
	}
}

USceneSubsystem* USceneSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<USceneSubsystem>() : nullptr;
}

bool USceneSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void USceneSubsystem::Deinitialize()
{
	// The level is going (a trip, quitting, the end of Play-In-Editor). A scene still playing stops where it is, and a white
	// it was bringing up goes with it; a white held for the trip stays, for the next level to reveal.
	if (Current.IsValid())
	{
		UE_LOG(LogLooter, Log, TEXT("Scene: %s cut short by its level ending."), *Current->Name.ToString());
		Current.Reset();
		UTransitionScreenSubsystem* Screen = UTransitionScreenSubsystem::Get(this);
		if (Screen && !Screen->IsHeld())
		{
			Screen->Clear();
		}
	}
	// The level takes its actors, the player's keys and their HUD with it, and a level change releases the held saves.
	SceneActors.Reset();
	PendingReturn = nullptr;
	HeldAfterEnd = -1.f;
	bHoldingPlayer = false;
	Super::Deinitialize();
}

void USceneSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Current.IsValid())
	{
		if (AreScenesOn(GetWorld()))
		{
			// The scene's own frame first (the wake-up's camera), then its timeline; the frame may have asked for a skip.
			if (Current->OnTick)
			{
				Current->OnTick(DeltaTime);
			}
			if (Current.IsValid())
			{
				AdvanceScene(DeltaTime);
			}
		}
		else
		{
			// A tour started, or Looter.Scenes 0: the scene goes to its end at once, so whatever follows it still happens.
			UE_LOG(LogLooter, Log, TEXT("Scene: scenes are off now."));
			SkipScene();
		}
	}
	UpdateSkipKeys(DeltaTime);

	if (HeldAfterEnd >= 0.f && !Current.IsValid())
	{
		HeldAfterEnd += DeltaTime;
		if (HeldAfterEnd >= TravelGraceSeconds)
		{
			UE_LOG(LogLooter, Warning, TEXT("Scene: no trip came %.0f s after a scene that holds the player for one; they come back."),
				TravelGraceSeconds);
			ReturnFromScene();
		}
	}
}

bool USceneSubsystem::IsTickable() const
{
	return Current.IsValid() || bHoldingPlayer || HeldAfterEnd >= 0.f;
}

TStatId USceneSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USceneSubsystem, STATGROUP_Tickables);
}

// ---------------------------------------------------------------------------
// Playing
// ---------------------------------------------------------------------------

bool USceneSubsystem::PlaySkiffRide(AActor* Skiff, FOnSceneMoment OnWhiteout)
{
	if (!Skiff)
	{
		UE_LOG(LogLooter, Warning, TEXT("Scene: the skiff ride has no skiff, so it doesn't play."));
		return false;
	}
	return Play(SkiffRide::Make(*this, *Skiff, MoveTemp(OnWhiteout)));
}

bool USceneSubsystem::IsPlaying() const
{
	return Current.IsValid();
}

bool USceneSubsystem::Play(FScenePlay&& Scene)
{
	if (Current.IsValid())
	{
		UE_LOG(LogLooter, Warning, TEXT("Scene: %s can't play while %s does."), *Scene.Name.ToString(), *Current->Name.ToString());
		return false;
	}
	if (!AreScenesOn(GetWorld()))
	{
		// Nothing plays, but it counts as played: a mission waiting for it would wait forever otherwise.
		UE_LOG(LogLooter, Log, TEXT("Scene: %s doesn't play, scenes are off (a tour or perf run, -NoScenes, or Looter.Scenes 0)."),
			*Scene.Name.ToString());
		MarkPlayed(Scene.Name);
		return false;
	}

	Current = MakeUnique<FScenePlay>(MoveTemp(Scene));
	Current->Timeline.OnMoment = [this](FName Moment) { HandleMoment(Moment); };
	bSkipWanted = false;
	bSkipKeyDown = false;
	SkipKeyHeld = 0.f;
	EscapeWait = 0.f;
	PromptLeft = 0.f;
	// A player still held after a scene whose trip never came is this one's to hold now.
	PendingReturn = nullptr;
	HeldAfterEnd = -1.f;
	if (!bHoldingPlayer)
	{
		HoldPlayer(Current->bLookOnly);
	}
	else if (APlayerController* Controller = HeldController.Get())
	{
		BindSceneInput(*Controller, Current->bLookOnly);
	}
	UE_LOG(LogLooter, Log, TEXT("Scene: %s plays (%.1f s)%s."), *Current->Name.ToString(), Current->Timeline.GetDuration(),
		bHoldingPlayer ? TEXT("") : TEXT(" with no player to hold"));

	if (Current->OnStart)
	{
		Current->OnStart();
	}
	// Its first frame: the moves that begin at the start take their first place, and moments at 0 happen now.
	AdvanceScene(0.f);
	return true;
}

FName USceneSubsystem::GetPlayingName() const
{
	return Current.IsValid() ? Current->Name : FName();
}

float USceneSubsystem::GetSceneTime() const
{
	return Current.IsValid() ? Current->Timeline.GetTime() : 0.f;
}

bool USceneSubsystem::IsSkipping() const
{
	return Current.IsValid() && Current->Timeline.IsSkipping();
}

bool USceneSubsystem::HasPlayed(FName Scene) const
{
	return !Scene.IsNone() && PlayedScenes.Contains(Scene);
}

bool USceneSubsystem::HasPlayed(const UObject* WorldContextObject, FName Scene)
{
	const USceneSubsystem* Scenes = Get(WorldContextObject);
	return Scenes && Scenes->HasPlayed(Scene);
}

void USceneSubsystem::MarkPlayed(FName Scene, bool bTellMissions)
{
	if (Scene.IsNone())
	{
		return;
	}
	PlayedScenes.Add(Scene);
	if (bTellMissions)
	{
		TellMissions(this, Scene);
	}
}

void USceneSubsystem::AdvanceScene(float DeltaSeconds)
{
	if (!Current.IsValid())
	{
		return;
	}
	bAdvancing = true;
	Current->Timeline.Advance(DeltaSeconds);
	bAdvancing = false;
	if (bSkipWanted)
	{
		bSkipWanted = false;
		SkipScene();
	}
	else if (Current.IsValid() && Current->Timeline.IsFinished())
	{
		FinishScene(false);
	}
}

void USceneSubsystem::SkipScene()
{
	if (!Current.IsValid())
	{
		return;
	}
	if (bAdvancing)
	{
		// Asked from inside one of the scene's own moments: it skips once that moment is over.
		bSkipWanted = true;
		return;
	}
	UE_LOG(LogLooter, Log, TEXT("Scene: %s skipped at %.1f s."), *Current->Name.ToString(), Current->Timeline.GetTime());
	bAdvancing = true;
	Current->Timeline.SkipToEnd();
	bAdvancing = false;
	FinishScene(true);
}

void USceneSubsystem::FinishScene(bool bSkipped)
{
	// Taken out first: from here on nothing is playing, so what runs below may start the next scene.
	const TUniquePtr<FScenePlay> Ended = MoveTemp(Current);
	bSkipWanted = false;
	bSkipKeyDown = false;
	EscapeWait = 0.f;
	PromptLeft = 0.f;
	DestroySceneActors();
	if (!Ended.IsValid())
	{
		return;
	}
	if (!bSkipped)
	{
		UE_LOG(LogLooter, Log, TEXT("Scene: %s played (%.1f s)."), *Ended->Name.ToString(), Ended->Timeline.GetTime());
	}

	// Before anything travels, so a trip's save already has what the missions made of it.
	MarkPlayed(Ended->Name);

	if (Ended->bHoldAfterEnd)
	{
		// Travel follows: the player stays held under the white, and comes back here only if no trip does.
		PendingReturn = MoveTemp(Ended->OnReturn);
		HeldAfterEnd = 0.f;
	}
	else
	{
		ReleasePlayer(nullptr, nullptr);
	}

	if (Ended->AfterEnd)
	{
		Ended->AfterEnd();
	}
}

void USceneSubsystem::HandleMoment(FName Moment)
{
	UE_LOG(LogLooter, Log, TEXT("Scene: %s, %s at %.2f s."), *GetPlayingName().ToString(), *Moment.ToString(), GetSceneTime());
	OnSceneEvent.Broadcast(Moment);
}

void USceneSubsystem::AddSceneActor(AActor* Actor)
{
	if (Actor)
	{
		SceneActors.AddUnique(Actor);
	}
}

void USceneSubsystem::DestroySceneActors()
{
	for (AActor* Actor : SceneActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SceneActors.Reset();
}

// ---------------------------------------------------------------------------
// Whether scenes play
// ---------------------------------------------------------------------------

bool USceneSubsystem::AreScenesOn(const UWorld* World)
{
	const int32 Setting = CVarScenes.GetValueOnGameThread();
	if (Setting <= 0)
	{
		return false;
	}
	if (Setting >= 2)
	{
		return true;
	}
	if (CommandLineTurnsScenesOff(FCommandLine::Get()))
	{
		return false;
	}
	const UViewTourSubsystem* Tour = World ? World->GetSubsystem<UViewTourSubsystem>() : nullptr;
	return !(Tour && Tour->IsRunning());
}

bool USceneSubsystem::CommandLineTurnsScenesOff(const TCHAR* CommandLine)
{
	if (!CommandLine)
	{
		return false;
	}
	// The numbers of a tour or a perf run are the level's, so nothing plays over them, even before the tour has started
	// (its -ExecCmds run on the first frame, after the level has begun).
	return FParse::Param(CommandLine, TEXT("NoScenes"))
		|| FParse::Param(CommandLine, TEXT("ExitAfterCsvProfiling"))
		|| FCString::Stristr(CommandLine, TEXT("Looter.Tour")) != nullptr;
}
