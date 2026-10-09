// Developer console commands for the ambient fauna (not in shipping builds): Looter.Fauna.Stats, .Scare, .Tumble and
// .DustDevil. The on/off toggle, Looter.Fauna 0/1, is a console variable in FaunaSubsystem.cpp.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "World/FaunaActor.h"
#include "World/FaunaDustDevils.h"
#include "World/FaunaFlock.h"
#include "World/FaunaSubsystem.h"
#include "World/FaunaSwarm.h"
#include "World/FaunaTumbleweeds.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FaunaWorld(UWorld* World)
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
		return World;
	}

	/** The local player's pawn's place, if there is one. */
	bool PlayerLocation(const UWorld* World, FVector& OutLocation)
	{
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		if (Pawn)
		{
			OutLocation = Pawn->GetActorLocation();
		}
		return Pawn != nullptr;
	}

	/** Looter.Fauna.Stats: every fauna actor, what it has out, and what the updates cost on the game thread. */
	void StatsCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = FaunaWorld(World);
		const UFaunaSubsystem* Fauna = UFaunaSubsystem::Get(World);
		if (!Fauna)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Fauna.Stats: no fauna here."));
			return;
		}
		int32 Shown = 0;
		int32 Birds = 0;
		int32 Insects = 0;
		for (const TWeakObjectPtr<AFaunaActor>& Weak : Fauna->GetActors())
		{
			const AFaunaActor* Actor = Weak.Get();
			if (!Actor)
			{
				continue;
			}
			Shown += Actor->IsFaunaShown() ? 1 : 0;
			FString What;
			if (const AFaunaFlock* Flock = Cast<AFaunaFlock>(Actor))
			{
				Birds += Flock->GetBirds().Num();
				What = FString::Printf(TEXT("%d birds, %d perched%s"), Flock->GetBirds().Num(), Flock->CountPerched(),
					Flock->IsFlushed() ? TEXT(", up") : TEXT(""));
			}
			else if (const AFaunaSwarm* Swarm = Cast<AFaunaSwarm>(Actor))
			{
				Insects += Swarm->GetActiveCount();
				What = FString::Printf(TEXT("%d of %d insects out"), Swarm->GetActiveCount(), Swarm->MaxActive);
			}
			UE_LOG(LogLooter, Display, TEXT("Looter.Fauna.Stats: %s %s%s"), *Actor->GetActorNameOrLabel(),
				Actor->IsFaunaShown() ? TEXT("shown") : TEXT("hidden"), What.IsEmpty() ? TEXT("") : *(TEXT(": ") + What));
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Fauna.Stats: %s; %d actors (%d shown), %d birds, %d insects out; %d updated last frame, %.3f ms a frame on average."),
			UFaunaSubsystem::IsEnabled() ? TEXT("on") : TEXT("off (Looter.Fauna 0)"), Fauna->GetActors().Num(), Shown, Birds, Insects,
			Fauna->GetUpdatedLastFrame(), Fauna->GetAverageMilliseconds());
	}

	/** Looter.Fauna.Scare: a gunshot's noise at the player, startling every flock that hears it. */
	void ScareCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = FaunaWorld(World);
		UFaunaSubsystem* Fauna = UFaunaSubsystem::Get(World);
		FVector Where;
		if (!Fauna || !PlayerLocation(World, Where))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Fauna.Scare: needs a player in a level with fauna."));
			return;
		}
		Fauna->ReportNoise(Where, UFaunaSubsystem::GunshotRadius, EFaunaNoise::Gunshot);
		UE_LOG(LogLooter, Display, TEXT("Looter.Fauna.Scare: a shot's noise at the player."));
	}

	/** Looter.Fauna.Tumble: a tumbleweed off the lane whose start is nearest the player. */
	void TumbleCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = FaunaWorld(World);
		const UFaunaSubsystem* Fauna = UFaunaSubsystem::Get(World);
		FVector Where;
		if (!Fauna || !PlayerLocation(World, Where))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Fauna.Tumble: needs a player in a level with fauna."));
			return;
		}
		for (const TWeakObjectPtr<AFaunaActor>& Weak : Fauna->GetActors())
		{
			AFaunaTumbleweeds* Tumbleweeds = Cast<AFaunaTumbleweeds>(Weak.Get());
			if (!Tumbleweeds)
			{
				continue;
			}
			int32 Nearest = INDEX_NONE;
			double Best = TNumericLimits<double>::Max();
			for (int32 Lane = 0; Lane < Tumbleweeds->Lanes.Num(); ++Lane)
			{
				const double Distance = FVector::Dist2D(Tumbleweeds->Lanes[Lane].Start, Where);
				if (Distance < Best)
				{
					Best = Distance;
					Nearest = Lane;
				}
			}
			const bool bLaunched = Tumbleweeds->Launch(Nearest);
			UE_LOG(LogLooter, Display, TEXT("Looter.Fauna.Tumble: %s (lane %d, %.0f m off)."), bLaunched ? TEXT("rolling") : TEXT("none free"),
				Nearest, Best / 100.0);
			return;
		}
		UE_LOG(LogLooter, Warning, TEXT("Looter.Fauna.Tumble: no tumbleweeds in this level."));
	}

	/** Looter.Fauna.DustDevil: a dust devil at the spot nearest the player. */
	void DustDevilCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = FaunaWorld(World);
		const UFaunaSubsystem* Fauna = UFaunaSubsystem::Get(World);
		FVector Where;
		if (!Fauna || !PlayerLocation(World, Where))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Fauna.DustDevil: needs a player in a level with fauna."));
			return;
		}
		for (const TWeakObjectPtr<AFaunaActor>& Weak : Fauna->GetActors())
		{
			AFaunaDustDevils* Devils = Cast<AFaunaDustDevils>(Weak.Get());
			if (!Devils)
			{
				continue;
			}
			int32 Nearest = INDEX_NONE;
			double Best = TNumericLimits<double>::Max();
			for (int32 Spot = 0; Spot < Devils->Spots.Num(); ++Spot)
			{
				const double Distance = FVector::Dist2D(Devils->Spots[Spot].Center, Where);
				if (Distance < Best)
				{
					Best = Distance;
					Nearest = Spot;
				}
			}
			const bool bRaised = Devils->Raise(Nearest);
			UE_LOG(LogLooter, Display, TEXT("Looter.Fauna.DustDevil: %s (spot %d, %.0f m off)."), bRaised ? TEXT("rising") : TEXT("one is up already"),
				Nearest, Best / 100.0);
			return;
		}
		UE_LOG(LogLooter, Warning, TEXT("Looter.Fauna.DustDevil: no dust devils in this level."));
	}

	FAutoConsoleCommandWithWorldAndArgs FaunaStatsCommand(TEXT("Looter.Fauna.Stats"),
		TEXT("The ambient fauna: each actor, what it has out, and what its updates cost a frame."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StatsCommand));
	FAutoConsoleCommandWithWorldAndArgs FaunaScareCommand(TEXT("Looter.Fauna.Scare"),
		TEXT("A gunshot's noise at the player: every flock that hears it takes off."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ScareCommand));
	FAutoConsoleCommandWithWorldAndArgs FaunaTumbleCommand(TEXT("Looter.Fauna.Tumble"),
		TEXT("Sends a tumbleweed off the lane nearest the player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TumbleCommand));
	FAutoConsoleCommandWithWorldAndArgs FaunaDustDevilCommand(TEXT("Looter.Fauna.DustDevil"),
		TEXT("Raises a dust devil at the spot nearest the player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DustDevilCommand));
}

#endif
