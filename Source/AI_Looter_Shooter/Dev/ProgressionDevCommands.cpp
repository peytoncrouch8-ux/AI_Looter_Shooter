// Developer console commands for levels and experience (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Progression/PlayerProgressionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
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

	void ForgetBestiary(const TArray<FString>& Args, UWorld* World)
	{
		UPlayerProgressionSubsystem* Progression = FindProgression(World);
		if (!Progression)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.ForgetBestiary: no player (start the game first)."));
			return;
		}
		Progression->ForgetBestiary();
		UE_LOG(LogLooter, Log, TEXT("Looter.ForgetBestiary: nothing met or defeated; every bestiary page reads ??? again."));
	}

	void PrintXPTable(const TArray<FString>& Args, UWorld* World)
	{
		// Kills of a Basic creature at the player's own level, made one at a time with the leftover experience carried
		// into the next level, as play adds it.
		const FXPCurve Curve = UPlayerProgressionSubsystem::GetCurve();
		const FLevelRules Rules = UPlayerProgressionSubsystem::GetLevelRules();
		const int32 BaseXP = GetDefault<ACreatureBase>()->XPReward;
		UE_LOG(LogLooter, Log, TEXT("Looter.XP.Table: a Basic kill at the player's level gives %d x %.2f^(level - 1) XP; level L takes %lld x %.2f^(L - 1)."),
			BaseXP, Rules.KillXPGrowth, Curve.BaseXP, Curve.Growth);
		UE_LOG(LogLooter, Log, TEXT("  level   XP to next   XP a kill   kills   kills so far   XP so far"));
		int32 Level = 1;
		int64 XP = 0;
		int64 KillsSoFar = 0;
		int64 XPSoFar = 0;
		while (!Curve.IsMaxLevel(Level))
		{
			const int32 From = Level;
			const int64 Needed = Curve.XPToNextLevel(From);
			const int64 PerKill = Rules.KillXP(BaseXP, From, From);
			if (PerKill <= 0)
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.XP.Table: a kill gives no experience (the creatures' XPReward is 0)."));
				return;
			}
			int64 Kills = 0;
			while (Level == From)
			{
				Curve.ApplyXP(Level, XP, PerKill);
				++Kills;
			}
			KillsSoFar += Kills;
			XPSoFar += Needed;
			UE_LOG(LogLooter, Log, TEXT("  %2d->%-2d %12lld %11lld %7lld %14lld %11lld"), From, From + 1, Needed, PerKill, Kills, KillsSoFar, XPSoFar);
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.XP.Table: level %d takes %lld XP in all: %lld kills at the player's level (a kill %.0f%% less for each level below, at least %.0f%%)."),
			Curve.MaxLevel, XPSoFar, KillsSoFar, Rules.KillXPFalloff * 100.0, Rules.KillXPFloor * 100.0);
	}

	FAutoConsoleCommandWithWorldAndArgs XPTableCommand(
		TEXT("Looter.XP.Table"),
		TEXT("Prints the kills each level takes from 1 to the maximum (Basic creatures at the player's level) and the totals."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PrintXPTable));

	FAutoConsoleCommandWithWorldAndArgs ForgetBestiaryCommand(
		TEXT("Looter.ForgetBestiary"),
		TEXT("Forgets every kind of creature or character met and defeated, so every bestiary page reads ??? again (level and experience stay)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ForgetBestiary));

	FAutoConsoleCommandWithWorldAndArgs GiveXPCommand(
		TEXT("Looter.GiveXP"),
		TEXT("Gives the player experience (leveling up as it covers): Looter.GiveXP <amount>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GiveXP));

	FAutoConsoleCommandWithWorldAndArgs SetLevelCommand(
		TEXT("Looter.SetLevel"),
		TEXT("Puts the player at the start of a level (1 to the maximum): Looter.SetLevel <level>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetLevel));

	FAutoConsoleCommandWithWorldAndArgs ResetProgressCommand(
		TEXT("Looter.ResetProgress"),
		TEXT("Starts the player's progress over, as a new game: level 1, no experience."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ResetProgress));
}

#endif
