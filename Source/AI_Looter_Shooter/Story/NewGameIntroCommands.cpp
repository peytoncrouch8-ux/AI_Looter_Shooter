// Developer console commands for the new game's intro (not in shipping builds). Its cvars (Looter.Intro,
// Looter.Intro.Letterbox) are NewGameIntro.cpp's.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Core/LooterMenuGameMode.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Story/NewGameIntro.h"
#include "Story/NewGameIntroSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void PlayCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* Played = FindGameWorld(World);
		if (!Played)
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Intro.Play: no game running."));
			return;
		}
		// In the menu: the planet part as a preview (the menu comes back). In a level: the Rest's part (west: Westward
		// first), then the player back where they stood; nothing is recorded either way.
		if (ALooterMenuGameMode::IsMenuWorld(Played))
		{
			UNewGameIntroSubsystem* Intro = UNewGameIntroSubsystem::Get(Played);
			const bool bPlays = Intro && Intro->PlayPlanet(*Played, INDEX_NONE, nullptr);
			UE_LOG(LogLooter, Display, TEXT("Looter.Intro.Play: %s"), bPlays ? TEXT("Teropa plays (a preview).")
				: TEXT("nothing plays (the menu isn't the planet scene, scenes or Looter.Intro are off, or it's playing)."));
			return;
		}
		const bool bWestward = Args.Num() > 0 && Args[0].Equals(TEXT("west"), ESearchCase::IgnoreCase);
		if (UTransitionScreenSubsystem* White = UTransitionScreenSubsystem::Get(Played))
		{
			// It opens on the white as a new game's does.
			White->HoldWhite();
		}
		const TWeakObjectPtr<UWorld> WeakWorld = Played;
		const bool bPlays = NewGameIntro::PlayRest(*Played, bWestward, [WeakWorld]()
		{
			if (USceneSubsystem* Scenes = USceneSubsystem::Get(WeakWorld.Get()))
			{
				// It ends as a new game's does: the title rising through the white.
				Scenes->ReturnFromScene(UTransitionScreenSubsystem::GameTitle());
			}
		});
		if (!bPlays)
		{
			if (UTransitionScreenSubsystem* White = UTransitionScreenSubsystem::Get(Played))
			{
				White->Reveal();
			}
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Intro.Play: %s"), bPlays ? (bWestward ? TEXT("Ransom's Rest plays, Westward first.")
			: TEXT("Ransom's Rest plays.")) : TEXT("nothing plays (a scene is playing, or scenes are off)."));
	}

	void SkipCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* Played = FindGameWorld(World);
		UNewGameIntroSubsystem* Intro = Played ? UNewGameIntroSubsystem::Get(Played) : nullptr;
		if (Intro && Intro->IsPlayingPlanet())
		{
			Intro->SkipPlanet();
			return;
		}
		// The Rest's part is a scene: the scene's own skip.
		if (USceneSubsystem* Scenes = Played ? USceneSubsystem::Get(Played) : nullptr; Scenes && Scenes->GetPlayingName() == NewGameIntro::SceneName())
		{
			Scenes->SkipScene();
			return;
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Intro.Skip: the intro isn't playing."));
	}

	FAutoConsoleCommandWithWorldAndArgs PlayRegistration(
		TEXT("Looter.Intro.Play"),
		TEXT("Plays the new game's intro now, recording nothing: in the menu Teropa from orbit (a preview); in a level Ransom's Rest ")
		TEXT("from the white (Looter.Intro.Play west: Westward first, as when the planet couldn't play)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PlayCommand));

	FAutoConsoleCommandWithWorldAndArgs SkipRegistration(
		TEXT("Looter.Intro.Skip"),
		TEXT("Skips the intro playing (in the menu the whole intro, as the skip keys do)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SkipCommand));
}

#endif
