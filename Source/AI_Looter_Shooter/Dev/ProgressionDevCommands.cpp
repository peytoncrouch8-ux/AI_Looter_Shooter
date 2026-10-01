// Developer console commands for levels and experience (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Progression/PlayerProgressionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The player's progression in the world the command was typed in, or in the running play session (from the editor). */
	UPlayerProgressionSubsystem* FindProgression(UWorld* World)
	{
		UWorld* GameWorld = World && World->IsGameWorld() ? World : nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (!GameWorld && Context.World() && Context.World()->IsGameWorld())
			{
				GameWorld = Context.World();
			}
		}
		const ULocalPlayer* Player = GameWorld ? GameWorld->GetFirstLocalPlayerFromController() : nullptr;
		return Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	}

	void LogProgress(const TCHAR* Command, const UPlayerProgressionSubsystem& Progression)
	{
		if (Progression.IsMaxLevel())
		{
			UE_LOG(LogLooter, Log, TEXT("%s: level %d (the maximum)"), Command, Progression.GetLevel());
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("%s: level %d, %lld / %lld XP"), Command, Progression.GetLevel(), Progression.GetXP(),
			Progression.GetXPToNextLevel());
	}

	void GiveXP(const TArray<FString>& Args, UWorld* World)
	{
		UPlayerProgressionSubsystem* Progression = FindProgression(World);
		const int64 Amount = Args.Num() > 0 ? FCString::Atoi64(*Args[0]) : 0;
		if (!Progression || Amount <= 0)
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.GiveXP <amount>"));
			return;
		}
		Progression->AddXP(Amount, EXPSource::Debug);
		LogProgress(TEXT("Looter.GiveXP"), *Progression);
	}

	void SetLevel(const TArray<FString>& Args, UWorld* World)
	{
		UPlayerProgressionSubsystem* Progression = FindProgression(World);
		// A typo must not read as 0 and quietly send the player back to level 1.
		if (!Progression || Args.Num() != 1 || !Args[0].IsNumeric())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.SetLevel <level>"));
			return;
		}
		Progression->SetLevel(FCString::Atoi(*Args[0]));
		LogProgress(TEXT("Looter.SetLevel"), *Progression);
	}

	void ResetProgress(const TArray<FString>& Args, UWorld* World)
	{
		UPlayerProgressionSubsystem* Progression = FindProgression(World);
		if (!Progression)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.ResetProgress: no player (start the game first)."));
			return;
		}
		Progression->ResetProgress();
		LogProgress(TEXT("Looter.ResetProgress"), *Progression);
	}

	FAutoConsoleCommandWithWorldAndArgs GiveXPCommand(
		TEXT("Looter.GiveXP"),
		TEXT("Gives the player experience (leveling up as it covers): Looter.GiveXP <amount>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GiveXP));

	FAutoConsoleCommandWithWorldAndArgs SetLevelCommand(
		TEXT("Looter.SetLevel"),
		TEXT("Puts the player at the start of a level (1 to the maximum), and saves it: Looter.SetLevel <level>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetLevel));

	FAutoConsoleCommandWithWorldAndArgs ResetProgressCommand(
		TEXT("Looter.ResetProgress"),
		TEXT("Starts the player's progress over, as a new game: level 1, no experience."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ResetProgress));
}

#endif
