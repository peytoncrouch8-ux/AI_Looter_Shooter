// Console commands for listening to the ambience and the score in play (not in shipping builds):
//   Looter.Music                                     what the music is doing
//   Looter.Music.Force <Calm|Combat|Keeper|Gravemother|Off>   holds a mood (Off gives it back to the world)
//   Looter.Music.Sting <Elite|Phase|Victory>         plays a stinger
//   Looter.Ambience                                  the area, light, bed and sweeteners
//   Looter.Ambience.Sweetener                        plays the next sweetener now

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Audio/AmbienceSubsystem.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/MusicDirectorSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world a command is for: the one it was typed in, or the running play-in-editor session. */
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

	UMusicDirectorSubsystem* FindMusic(UWorld* World)
	{
		UMusicDirectorSubsystem* Music = UMusicDirectorSubsystem::Get(FindGameWorld(World));
		if (!Music)
		{
			UE_LOG(LogLooter, Display, TEXT("No music director: play the game (or a session in the editor) first."));
		}
		return Music;
	}

	void ShowMusic(const TArray<FString>& Args, UWorld* World)
	{
		if (UMusicDirectorSubsystem* Music = FindMusic(World))
		{
			UE_LOG(LogLooter, Display, TEXT("%s"), *Music->Describe());
		}
	}

	void ForceMusic(const TArray<FString>& Args, UWorld* World)
	{
		UMusicDirectorSubsystem* Music = FindMusic(World);
		if (!Music)
		{
			return;
		}
		const FString What = Args.IsEmpty() ? FString(TEXT("Off")) : Args[0];
		if (What.Equals(TEXT("Calm"), ESearchCase::IgnoreCase))
		{
			Music->Force(EMusicMood::Calm);
		}
		else if (What.Equals(TEXT("Combat"), ESearchCase::IgnoreCase))
		{
			Music->Force(EMusicMood::Combat);
		}
		else if (What.Equals(TEXT("Keeper"), ESearchCase::IgnoreCase) || What.Equals(TEXT("Abel"), ESearchCase::IgnoreCase))
		{
			Music->Force(EMusicMood::Boss, EBossTheme::Keeper);
		}
		else if (What.Equals(TEXT("Gravemother"), ESearchCase::IgnoreCase))
		{
			Music->Force(EMusicMood::Boss, EBossTheme::Gravemother);
		}
		else
		{
			Music->Release();
		}
		UE_LOG(LogLooter, Display, TEXT("Music: %s."), *What);
	}

	void PlayMusicSting(const TArray<FString>& Args, UWorld* World)
	{
		UMusicDirectorSubsystem* Music = FindMusic(World);
		if (!Music)
		{
			return;
		}
		const FString What = Args.IsEmpty() ? FString(TEXT("Elite")) : Args[0];
		if (What.Equals(TEXT("Phase"), ESearchCase::IgnoreCase))
		{
			Music->PlaySting(LooterSoundCue::StingPhase);
		}
		else if (What.Equals(TEXT("Victory"), ESearchCase::IgnoreCase))
		{
			Music->PlaySting(LooterSoundCue::StingVictory);
		}
		else
		{
			Music->PlaySting(LooterSoundCue::StingElite);
		}
	}

	void ShowAmbience(const TArray<FString>& Args, UWorld* World)
	{
		if (UAmbienceSubsystem* Ambience = UAmbienceSubsystem::Get(FindGameWorld(World)))
		{
			UE_LOG(LogLooter, Display, TEXT("%s"), *Ambience->Describe());
		}
	}

	void PlaySweetener(const TArray<FString>& Args, UWorld* World)
	{
		if (UAmbienceSubsystem* Ambience = UAmbienceSubsystem::Get(FindGameWorld(World)))
		{
			Ambience->PlaySweetenerNow();
			UE_LOG(LogLooter, Display, TEXT("%s"), *Ambience->Describe());
		}
	}

	FAutoConsoleCommandWithWorldAndArgs MusicCommand(TEXT("Looter.Music"), TEXT("What the music is doing."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowMusic));
	FAutoConsoleCommandWithWorldAndArgs MusicForceCommand(TEXT("Looter.Music.Force"),
		TEXT("Looter.Music.Force <Calm|Combat|Keeper|Gravemother|Off>: holds the music in a mood (Off gives it back to the world)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ForceMusic));
	FAutoConsoleCommandWithWorldAndArgs MusicStingCommand(TEXT("Looter.Music.Sting"),
		TEXT("Looter.Music.Sting <Elite|Phase|Victory>: plays a stinger."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PlayMusicSting));
	FAutoConsoleCommandWithWorldAndArgs AmbienceCommand(TEXT("Looter.Ambience"), TEXT("The area's bed, light and sweeteners."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowAmbience));
	FAutoConsoleCommandWithWorldAndArgs SweetenerCommand(TEXT("Looter.Ambience.Sweetener"), TEXT("Plays the next sweetener now."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PlaySweetener));
}

#endif
