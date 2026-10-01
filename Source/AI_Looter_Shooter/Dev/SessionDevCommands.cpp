// Developer console commands for sessions (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Session/SessionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The sessions of the world the command was typed in, or of the running play session (from the editor). */
	USessionSubsystem* FindSessions(UWorld* World)
	{
		UWorld* GameWorld = World && World->IsGameWorld() ? World : nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (!GameWorld && Context.World() && Context.World()->IsGameWorld())
			{
				GameWorld = Context.World();
			}
		}
		const UGameInstance* GameInstance = GameWorld ? GameWorld->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<USessionSubsystem>() : nullptr;
	}

	void PlaySession(const TArray<FString>& Args, UWorld* World)
	{
		USessionSubsystem* Sessions = FindSessions(World);
		const int32 Number = Args.Num() == 1 && Args[0].IsNumeric() ? FCString::Atoi(*Args[0]) : 0;
		if (!Sessions || Number < 1 || Number > USessionSubsystem::MaxSessions)
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.Session.Play <1-%d>"), USessionSubsystem::MaxSessions);
			return;
		}
		Sessions->PlaySession(Number - 1);
	}

	void SaveSession(const TArray<FString>& Args, UWorld* World)
	{
		USessionSubsystem* Sessions = FindSessions(World);
		if (!Sessions || !Sessions->SaveNow())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Session.Save: no session is being played (start one from the main menu or Looter.Session.Play)."));
		}
	}

	void OpenMenu(const TArray<FString>& Args, UWorld* World)
	{
		if (USessionSubsystem* Sessions = FindSessions(World))
		{
			Sessions->OpenMainMenu();
		}
	}

	void ListSessions(const TArray<FString>& Args, UWorld* World)
	{
		const USessionSubsystem* Sessions = FindSessions(World);
		if (!Sessions)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Session.List: start the game first."));
			return;
		}
		for (int32 Index = 0; Index < USessionSubsystem::MaxSessions; ++Index)
		{
			const FSessionSummary Summary = Sessions->GetSummary(Index);
			if (!Summary.bExists)
			{
				UE_LOG(LogLooter, Log, TEXT("Session %d: empty"), Index + 1);
				continue;
			}
			UE_LOG(LogLooter, Log, TEXT("Session %d: level %d, %d guns, %s played, saved %s, in %s%s"), Index + 1, Summary.Level, Summary.Weapons,
				*USessionSubsystem::FormatPlayTime(Summary.PlayedSeconds), *USessionSubsystem::FormatSavedTime(Summary.Saved, FDateTime::Now()),
				*Summary.Place, Index == Sessions->GetActiveSession() ? TEXT(" (being played)") : TEXT(""));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs PlaySessionCommand(
		TEXT("Looter.Session.Play"),
		TEXT("Plays a session as the main menu would (a new game when it's empty): Looter.Session.Play <1-3>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PlaySession));

	FAutoConsoleCommandWithWorldAndArgs SaveSessionCommand(
		TEXT("Looter.Session.Save"),
		TEXT("Saves the session being played now."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SaveSession));

	FAutoConsoleCommandWithWorldAndArgs OpenMenuCommand(
		TEXT("Looter.Session.Menu"),
		TEXT("Opens the main menu (the session being played saves as its level ends)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&OpenMenu));

	FAutoConsoleCommandWithWorldAndArgs ListSessionsCommand(
		TEXT("Looter.Session.List"),
		TEXT("Lists the three sessions: level, guns, play time, when saved, where."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListSessions));
}

#endif
