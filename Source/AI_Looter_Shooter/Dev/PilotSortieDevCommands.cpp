// Developer console commands for the Lamplighter's part of Main 6, "The Pilot Light" (not in shipping builds):
//   Looter.Pilot.State <Moored|Flying|Tenders|Alongside|Grappled|Jump|Home|Story>
//                                     jumps her there now (Story: back to following Main 6's step); a jump stops her
//                                     following the story until Story
//   Looter.Pilot.TendersDown          both tenders and every boarder gone now: Pilot.TendersDown
//   Looter.Pilot.Bag <0|1>            tears tender 0's (port) or 1's (the boarder's) gas-bag now
//   Looter.Pilot.Launch               fires both launchers at the barge now (she must be alongside for them to haul her in)

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "World/PilotSortie.h"
#include "World/PilotSortieRules.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindPilotWorld(UWorld* World)
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

	APilotSortie* FindPilotSortie(UWorld* World, const TCHAR* Command)
	{
		World = FindPilotWorld(World);
		APilotSortie* Sortie = World ? APilotSortie::Find(World) : nullptr;
		if (!Sortie)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no sortie in this level (Ransom's Rest: build_area_main6_sortie.py places it)."), Command);
		}
		return Sortie;
	}

	void PilotState(const TArray<FString>& Args, UWorld* World)
	{
		APilotSortie* Sortie = FindPilotSortie(World, TEXT("Looter.Pilot.State"));
		if (!Sortie)
		{
			return;
		}
		const FString Words = Args.Num() > 0 ? Args[0] : FString();
		if (Words.Equals(TEXT("Story"), ESearchCase::IgnoreCase))
		{
			Sortie->bFollowStory = true;
			Sortie->RefreshStory();
			UE_LOG(LogLooter, Display, TEXT("Looter.Pilot.State: following Main 6 again (%s)."), PilotLight::PhaseName(Sortie->GetPhase()));
			return;
		}
		EPilotPhase Phase;
		if (!PilotLight::ParsePhase(Words, Phase))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Pilot.State <Moored|Flying|Tenders|Alongside|Grappled|Jump|Home|Story> (now %s)."),
				PilotLight::PhaseName(Sortie->GetPhase()));
			return;
		}
		// "Tenders": on her course out, with them called at once.
		Sortie->bFollowStory = false;
		Sortie->SetPhase(Phase == EPilotPhase::Cruise && Words.Equals(TEXT("Flying"), ESearchCase::IgnoreCase) ? EPilotPhase::LiftOff : Phase);
		UE_LOG(LogLooter, Display, TEXT("Looter.Pilot.State: %s (the story's word ignored until Looter.Pilot.State Story)."),
			PilotLight::PhaseName(Sortie->GetPhase()));
	}

	void PilotTendersDown(const TArray<FString>& Args, UWorld* World)
	{
		if (APilotSortie* Sortie = FindPilotSortie(World, TEXT("Looter.Pilot.TendersDown")))
		{
			Sortie->TendersDownNow();
			UE_LOG(LogLooter, Display, TEXT("Looter.Pilot.TendersDown: %s"), Sortie->AreTendersDown() ? TEXT("both down, her deck clear") : TEXT("not yet"));
		}
	}

	void PilotBag(const TArray<FString>& Args, UWorld* World)
	{
		if (APilotSortie* Sortie = FindPilotSortie(World, TEXT("Looter.Pilot.Bag")))
		{
			const int32 Index = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
			UE_LOG(LogLooter, Display, TEXT("Looter.Pilot.Bag %d: %s"), Index, Sortie->BurstTenderBag(Index) ? TEXT("torn") : TEXT("no tender there, or down already"));
		}
	}

	void PilotLaunch(const TArray<FString>& Args, UWorld* World)
	{
		if (APilotSortie* Sortie = FindPilotSortie(World, TEXT("Looter.Pilot.Launch")))
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Pilot.Launch: %d fired"), Sortie->FireLaunchers());
		}
	}

	FAutoConsoleCommandWithWorldAndArgs PilotStateRegistration(TEXT("Looter.Pilot.State"),
		TEXT("Puts the Lamplighter's Main 6 sortie in a state now: Looter.Pilot.State <Moored|Flying|Tenders|Alongside|Grappled|Jump|Home|Story>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PilotState));

	FAutoConsoleCommandWithWorldAndArgs PilotTendersDownRegistration(TEXT("Looter.Pilot.TendersDown"),
		TEXT("Both Company tenders and every boarder gone now (Main 6's Pilot.TendersDown)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PilotTendersDown));

	FAutoConsoleCommandWithWorldAndArgs PilotBagRegistration(TEXT("Looter.Pilot.Bag"),
		TEXT("Tears a Company tender's gas-bag now: Looter.Pilot.Bag <0|1> (0 the port tender, 1 the boarder)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PilotBag));

	FAutoConsoleCommandWithWorldAndArgs PilotLaunchRegistration(TEXT("Looter.Pilot.Launch"),
		TEXT("Fires both of the Lamplighter's grapple launchers at the barge now."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PilotLaunch));
}

#endif
