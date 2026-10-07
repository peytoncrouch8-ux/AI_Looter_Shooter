// Developer console commands for Legendary monsters' lairs and their return after 20 minutes of play (not in shipping
// builds). The Gravemother's fight itself: Looter.SpawnCreature Gravemother 1 chase, or her den's lair with
// Looter.Legendary.Return (before Main 5 too).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Session/SessionSubsystem.h"
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

	/** The level's Legendary lairs (spawners with a LegendaryId), or just the one named Id; logged when there are none. */
	TArray<AEncounterSpawner*> FindLairs(UWorld* GameWorld, const TCHAR* Command, FName Id = NAME_None)
	{
		TArray<AEncounterSpawner*> Lairs;
		const UEncounterSubsystem* Encounters = GameWorld ? GameWorld->GetSubsystem<UEncounterSubsystem>() : nullptr;
		if (!Encounters)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no game running."), Command);
			return Lairs;
		}
		for (AEncounterSpawner* Spawner : Encounters->GetSpawners())
		{
			if (!Spawner->LegendaryId.IsNone() && (Id.IsNone() || Spawner->LegendaryId == Id))
			{
				Lairs.Add(Spawner);
			}
		}
		if (Lairs.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no Legendary lair%s in %s (an encounter spawner with a LegendaryId; build_area_den.py places the ")
				TEXT("Gravemother's)."), Command, Id.IsNone() ? TEXT("") : *FString::Printf(TEXT(" for %s"), *Id.ToString()), *GameWorld->GetName());
		}
		return Lairs;
	}

	/** Looter.Legendary.List: every lair here, its encounter, and when its monster was last beaten and comes back. */
	void ListLegendary(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Legendary.List");
		UWorld* GameWorld = FindGameWorld(World);
		const USessionSubsystem* Sessions = GameWorld ? USessionSubsystem::Get(GameWorld) : nullptr;
		for (const AEncounterSpawner* Lair : FindLairs(GameWorld, Command))
		{
			const FName Id = Lair->LegendaryId;
			const double PlayedNow = Sessions ? Sessions->GetPlayedSecondsNow(GameWorld) : 0.0;
			const double DefeatedAt = Sessions ? Sessions->GetLegendaryDefeatedAt(GameWorld, Id) : -1.0;
			FString Return = TEXT("never beaten in this session: here on every arrival");
			if (DefeatedAt >= 0.0)
			{
				const double Left = USessionSubsystem::LegendaryReturnTime - (PlayedNow - DefeatedAt);
				Return = USessionSubsystem::IsLegendaryReturnDue(DefeatedAt, PlayedNow)
					? FString::Printf(TEXT("beaten %.1f min of play ago: back on the next arrival"), (PlayedNow - DefeatedAt) / 60.0)
					: FString::Printf(TEXT("beaten %.1f min of play ago: back on an arrival %.1f min of play from now"),
						(PlayedNow - DefeatedAt) / 60.0, Left / 60.0);
			}
			UE_LOG(LogLooter, Display, TEXT("  %-14s lair %s: %s; %s."), *Id.ToString(), *Lair->GetSpawnerId().ToString(), *Lair->Describe(),
				*Return);
		}
		UE_LOG(LogLooter, Display, TEXT("%s: %.1f min of play in this session%s."), Command,
			Sessions ? Sessions->GetPlayedSecondsNow(GameWorld) / 60.0 : 0.0,
			Sessions && Sessions->IsPlayingSession() ? TEXT("") : TEXT(" (no session: nothing is kept past this level)"));
	}

	/** Looter.Legendary.Forget [id | all]: the session forgets when they were beaten here, so they're back on the next arrival. */
	void ForgetLegendary(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Legendary.Forget");
		UWorld* GameWorld = FindGameWorld(World);
		USessionSubsystem* Sessions = GameWorld ? USessionSubsystem::Get(GameWorld) : nullptr;
		if (!Sessions)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no game running."), Command);
			return;
		}
		const bool bAll = Args.IsEmpty() || Args[0].Equals(TEXT("all"), ESearchCase::IgnoreCase);
		const int32 Forgotten = Sessions->ForgetLegendaryDefeats(GameWorld, bAll ? NAME_None : FName(*Args[0]));
		UE_LOG(LogLooter, Display, TEXT("%s: %d defeat%s forgotten on this map; back on the next arrival (Looter.Legendary.Return brings ")
			TEXT("one back now)."), Command, Forgotten, Forgotten == 1 ? TEXT("") : TEXT("s"));
	}

	/**
	 * Looter.Legendary.Return [id]: as an arrival 20 minutes of play on would: its defeat forgotten and its lair's monster
	 * brought now, whatever the story says (before Main 5 too), until the level loads again.
	 */
	void ReturnLegendary(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Legendary.Return");
		UWorld* GameWorld = FindGameWorld(World);
		USessionSubsystem* Sessions = GameWorld ? USessionSubsystem::Get(GameWorld) : nullptr;
		for (AEncounterSpawner* Lair : FindLairs(GameWorld, Command, Args.IsEmpty() ? NAME_None : FName(*Args[0])))
		{
			if (Sessions)
			{
				Sessions->ForgetLegendaryDefeats(GameWorld, Lair->LegendaryId);
			}
			const bool bCame = Lair->TriggerWave(/*bForce*/ true);
			UE_LOG(LogLooter, Display, TEXT("%s: %s %s (%s)."), Command, *Lair->LegendaryId.ToString(),
				bCame ? TEXT("is back") : TEXT("couldn't come back"), *Lair->Describe());
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ListCommand(
		TEXT("Looter.Legendary.List"),
		TEXT("Looter.Legendary.List: this level's Legendary monsters' lairs (the Gravemother's den), each one's encounter, and when ")
		TEXT("it was last beaten and comes back (on an arrival 20 minutes of play after its death)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListLegendary));

	FAutoConsoleCommandWithWorldAndArgs ForgetCommand(
		TEXT("Looter.Legendary.Forget"),
		TEXT("Looter.Legendary.Forget [id | all]: the session forgets when a Legendary monster (Gravemother) was last beaten on this ")
		TEXT("map, so it's back on the next arrival (Save & Quit, then Continue)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ForgetLegendary));

	FAutoConsoleCommandWithWorldAndArgs ReturnCommand(
		TEXT("Looter.Legendary.Return"),
		TEXT("Looter.Legendary.Return [id]: brings a Legendary monster back to its lair now, as an arrival 20 minutes of play on ")
		TEXT("would, whatever the story says (the Gravemother before Main 5 too), until the level loads again."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ReturnLegendary));
}

#endif
