#include "Scenes/ColdOpenSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Scenes/ColdOpen.h"
#include "Scenes/ColdOpenSet.h"
#include "Scenes/GraveWake.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** A black the cold open ended in thins over this long when no wake-up takes it over (seconds). */
	constexpr float FadeBackSeconds = 0.8f;
}

UColdOpenSubsystem* UColdOpenSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UColdOpenSubsystem>() : nullptr;
}

bool UColdOpenSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UColdOpenSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// A story already past the cold open (a session continued): Main 1's first steps, should they be where a session
	// resumes it, pass as they begin. The missions aren't listening yet; they ask the scenes (USceneSubsystem::HasPlayed).
	const FCampaignRecord* Campaign = FindCampaign();
	if (Campaign && Campaign->bColdOpenSeen && AColdOpenSet::Find(&InWorld))
	{
		if (USceneSubsystem* Scenes = USceneSubsystem::Get(&InWorld))
		{
			Scenes->MarkPlayed(ColdOpen::SceneName(), /*bTellMissions*/ false);
			Scenes->MarkPlayed(GraveWake::SceneName(), /*bTellMissions*/ false);
		}
	}
}

bool UColdOpenSubsystem::IsDue(const FCampaignRecord& Campaign)
{
	return Campaign.bFirstCastOff && !Campaign.bColdOpenSeen;
}

FCampaignRecord* UColdOpenSubsystem::FindCampaign() const
{
	return UAreaTravelSubsystem::FindCampaign(GetWorld());
}

bool UColdOpenSubsystem::BeginIfDue()
{
	UWorld* World = GetWorld();
	if (!World || !AColdOpenSet::Find(World))
	{
		// Not the story's first area: nothing opens here.
		return false;
	}
	const FCampaignRecord* Campaign = FindCampaign();
	const USceneSubsystem* Scenes = USceneSubsystem::Get(World);
	if (!Campaign || Campaign->bColdOpenSeen || (Scenes && Scenes->IsPlaying()))
	{
		// Seen already, or playing now (it's the scene playing, or another is): nothing to begin.
		return false;
	}
	if (!IsDue(*Campaign))
	{
		// The story hasn't begun here (the level played straight from the editor): no first arrival to open on, and Main 1
		// goes on from the grave.
		UE_LOG(LogLooter, Log, TEXT("Cold open: the story hasn't begun here (no first cast-off), so it's passed."));
		PassColdOpen(true);
		return false;
	}
	if (!USceneSubsystem::AreScenesOn(World))
	{
		UE_LOG(LogLooter, Log, TEXT("Cold open: scenes are off (a tour or perf run, -NoScenes, or Looter.Scenes 0), so it's passed."));
		PassColdOpen(true);
		return false;
	}
	if (!PlayColdOpen(true))
	{
		PassColdOpen(true);
		return false;
	}
	return true;
}

bool UColdOpenSubsystem::PlayColdOpen(bool bRecord)
{
	UWorld* World = GetWorld();
	AColdOpenSet* Set = AColdOpenSet::Find(World);
	USceneSubsystem* Scenes = USceneSubsystem::Get(World);
	if (!Set || !Scenes || Scenes->IsPlaying() || !USceneSubsystem::AreScenesOn(World))
	{
		UE_LOG(LogLooter, Warning, TEXT("Cold open: it can't play (%s)."), !Set ? TEXT("the level has no cold open set: Tools/Unreal/build_area_story.py places it")
			: !Scenes ? TEXT("no scenes here") : Scenes->IsPlaying() ? TEXT("a scene is playing") : TEXT("scenes are off"));
		return false;
	}
	// It opens on the white: the one the trip held, or one brought up now (a console command, a continued session).
	UTransitionScreenSubsystem* White = UTransitionScreenSubsystem::Get(World);
	if (White && !White->IsHeld())
	{
		White->HoldWhite();
	}
	const TWeakObjectPtr<UColdOpenSubsystem> WeakThis = this;
	FScenePlay Scene = ColdOpen::Make(*Scenes, *Set, UTransitionScreenSubsystem::GameTitle(), [WeakThis, bRecord]()
	{
		UColdOpenSubsystem* Opening = WeakThis.Get();
		if (Opening && !Opening->PlayGraveWake(bRecord))
		{
			// No wake-up to follow (no grave here): the player comes back where they arrived.
			if (USceneSubsystem* Played = USceneSubsystem::Get(Opening))
			{
				Played->ReturnFromScene();
			}
			Opening->FinishOpening(bRecord);
		}
	});
	if (!Scenes->Play(MoveTemp(Scene)))
	{
		if (White)
		{
			White->Reveal(UTransitionScreenSubsystem::GameTitle());
		}
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("Cold open: it plays (%.0f s with the Point)%s."), ColdOpen::DurationFor(Set->SkiffSeconds),
		bRecord ? TEXT("") : TEXT("; it won't be recorded as seen"));
	return true;
}

bool UColdOpenSubsystem::PlayGraveWake(bool bRecord)
{
	UWorld* World = GetWorld();
	USceneSubsystem* Scenes = USceneSubsystem::Get(World);
	AActor* Grave = GraveWake::FindGrave(World);
	if (!Scenes || !Grave || Scenes->IsPlaying() || !USceneSubsystem::AreScenesOn(World))
	{
		UE_LOG(LogLooter, Warning, TEXT("Grave wake-up: it can't play (%s)."), !Grave ? TEXT("there's no Ellis's grave (SM_Grave_Ellis) in the level")
			: !Scenes ? TEXT("no scenes here") : Scenes->IsPlaying() ? TEXT("a scene is playing") : TEXT("scenes are off"));
		return false;
	}
	TSharedPtr<GraveWake::FState> State;
	FScenePlay Scene = GraveWake::Make(*Scenes, GraveWake::SpotsFor(*Grave), State);
	const TWeakObjectPtr<UColdOpenSubsystem> WeakThis = this;
	Scene.AfterEnd = [WeakThis, bRecord, Inner = MoveTemp(Scene.AfterEnd)]()
	{
		if (Inner)
		{
			Inner();
		}
		if (UColdOpenSubsystem* Opening = WeakThis.Get())
		{
			Opening->FinishOpening(bRecord);
		}
	};
	if (!Scenes->Play(MoveTemp(Scene)))
	{
		return false;
	}
	WakeState = State;
	return true;
}

bool UColdOpenSubsystem::Claw()
{
	return WakeState.IsValid() && GraveWake::Press(*WakeState);
}

int32 UColdOpenSubsystem::GetClaws() const
{
	return WakeState.IsValid() ? GraveWake::GetPresses(*WakeState) : 0;
}

void UColdOpenSubsystem::FinishOpening(bool bRecord)
{
	WakeState.Reset();
	FCampaignRecord* Campaign = bRecord ? FindCampaign() : nullptr;
	if (Campaign && !Campaign->bColdOpenSeen)
	{
		// Kept with the session: the next save has it (the scene held the saves until now).
		Campaign->bColdOpenSeen = true;
		UE_LOG(LogLooter, Log, TEXT("Cold open: seen; it won't play again in this session."));
		if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
		{
			Sessions->SaveSoon();
		}
	}
	// Nothing left over the view: a black the cold open ended in (when no wake-up took it over) thins away.
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APlayerCameraManager* Camera = Controller ? Controller->PlayerCameraManager.Get() : nullptr;
	if (Camera && Camera->bEnableFading && Camera->FadeAmount > 0.f)
	{
		Camera->StartCameraFade(Camera->FadeAmount, 0.f, FadeBackSeconds, FLinearColor::Black, /*bShouldFadeAudio*/ false, /*bHoldWhenFinished*/ false);
	}
}

void UColdOpenSubsystem::PassColdOpen(bool bRecord)
{
	if (FCampaignRecord* Campaign = bRecord ? FindCampaign() : nullptr)
	{
		Campaign->bColdOpenSeen = true;
	}
	// Main 1's first two steps are these scenes: they count as played.
	if (USceneSubsystem* Scenes = USceneSubsystem::Get(this))
	{
		Scenes->MarkPlayed(ColdOpen::SceneName());
		Scenes->MarkPlayed(GraveWake::SceneName());
	}
}
