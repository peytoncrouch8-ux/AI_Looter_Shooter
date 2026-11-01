// Developer console commands for Main 6's barge (the Wardens story, lane MAIN67's W3; not in shipping builds):
//   Looter.Barge.Show [step] [aboard]  the barge as if Main 6 were on step (3: the deck fight), "aboard": the player on her deck
//   Looter.Barge.Story                 back to the story's word (shown during Main 6 only)
//   Looter.Barge.Valve [n]             shuts the next n (1) draft valves now
//   Looter.Barge.Jam                   jams the hose drum now (sets Pilot.DrumJammed)
//   Looter.Barge.Cage                  bursts the soul cage now (the CageBurst scene), whatever the valves and the lantern
//   Looter.Barge.Die                   her pilot light out: listing, drifting, burning, the stack's fall
//   Looter.Barge.Fall                  the BargeFall scene now (from the Lamplighter's deck if the player is on it)
//   Looter.Barge.Grates [run|stop]     the grate ring's state (and starts or stops it)

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Scenes/BargeFall.h"
#include "World/BargeBoarding.h"
#include "World/BargeFurnace.h"
#include "World/BargeHoseDrum.h"
#include "World/BargeRules.h"
#include "World/FurnaceDraftValve.h"
#include "World/LamplighterShip.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	UWorld* FindBargeWorld(UWorld* World)
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

	ABargeBoarding* FindBarge(UWorld* World, const TCHAR* Command)
	{
		ABargeBoarding* Barge = ABargeBoarding::Find(FindBargeWorld(World));
		if (!Barge)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no barge in this level (Ransom's Rest: build_area_main6.py)."), Command);
		}
		return Barge;
	}

	void BargeShow(const TArray<FString>& Args, UWorld* World)
	{
		ABargeBoarding* Barge = FindBarge(World, TEXT("Looter.Barge.Show"));
		if (!Barge)
		{
			return;
		}
		const int32 Step = Args.Num() > 0 && Args[0].IsNumeric() ? FCString::Atoi(*Args[0]) : BargeRules::DeckStep;
		Barge->ForceStep(Step);
		if (Args.ContainsByPredicate([](const FString& Each) { return Each.Equals(TEXT("aboard"), ESearchCase::IgnoreCase); }))
		{
			if (APawn* Player = Barge->GetWorld()->GetFirstPlayerController() ? Barge->GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr)
			{
				const FTransform Start = Barge->GetPlayerStart();
				Player->TeleportTo(Start.GetLocation(), Start.Rotator());
			}
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Barge.Show: as if Main 6 were on step %d (%s)."), Step, *UEnum::GetValueAsString(Barge->GetState()));
	}

	void BargeStory(const TArray<FString>& Args, UWorld* World)
	{
		if (ABargeBoarding* Barge = FindBarge(World, TEXT("Looter.Barge.Story")))
		{
			Barge->ForceStep(INDEX_NONE);
			UE_LOG(LogLooter, Display, TEXT("Looter.Barge.Story: back to the story (%s)."), *UEnum::GetValueAsString(Barge->GetState()));
		}
	}

	void BargeValve(const TArray<FString>& Args, UWorld* World)
	{
		ABargeBoarding* Barge = FindBarge(World, TEXT("Looter.Barge.Valve"));
		if (!Barge)
		{
			return;
		}
		int32 Left = Args.Num() > 0 && Args[0].IsNumeric() ? FMath::Max(FCString::Atoi(*Args[0]), 1) : 1;
		for (AFurnaceDraftValve* Valve : Barge->GetValves())
		{
			if (Left > 0 && Valve && Valve->Shut(nullptr, /*bForce*/ true))
			{
				--Left;
			}
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Barge.Valve: %d of %d shut."), Barge->CountValvesShut(), BargeRules::NumValves);
	}

	void BargeJam(const TArray<FString>& Args, UWorld* World)
	{
		ABargeBoarding* Barge = FindBarge(World, TEXT("Looter.Barge.Jam"));
		ABargeHoseDrum* Drum = Barge ? Barge->GetHoseDrum() : nullptr;
		if (Drum)
		{
			const bool bJammed = Drum->Jam(nullptr, /*bForce*/ true);
			UE_LOG(LogLooter, Display, TEXT("Looter.Barge.Jam: %s"), bJammed ? TEXT("the hose drum is jammed") : TEXT("already jammed"));
		}
	}

	void BargeCage(const TArray<FString>& Args, UWorld* World)
	{
		ABargeBoarding* Barge = FindBarge(World, TEXT("Looter.Barge.Cage"));
		ABargeFurnace* Furnace = Barge ? Barge->GetFurnace() : nullptr;
		if (Furnace)
		{
			const bool bBurst = Furnace->BurstCage(nullptr, /*bForce*/ true);
			UE_LOG(LogLooter, Display, TEXT("Looter.Barge.Cage: %s"), bBurst ? TEXT("the soul cage bursts") : TEXT("already burst"));
		}
	}

	void BargeDie(const TArray<FString>& Args, UWorld* World)
	{
		if (ABargeBoarding* Barge = FindBarge(World, TEXT("Looter.Barge.Die")))
		{
			Barge->NoteCageBurst();
			Barge->BeginDying();
			UE_LOG(LogLooter, Display, TEXT("Looter.Barge.Die: %s"), *UEnum::GetValueAsString(Barge->GetState()));
		}
	}

	void BargeFallNow(const TArray<FString>& Args, UWorld* World)
	{
		if (ABargeBoarding* Barge = FindBarge(World, TEXT("Looter.Barge.Fall")))
		{
			const bool bPlays = BargeFall::Play(*Barge, ALamplighterShip::Find(Barge->GetWorld()));
			UE_LOG(LogLooter, Display, TEXT("Looter.Barge.Fall: %s"), bPlays ? TEXT("the scene plays") : TEXT("she's down at once (scenes off or busy)"));
		}
	}

	void BargeGrates(const TArray<FString>& Args, UWorld* World)
	{
		ABargeBoarding* Barge = FindBarge(World, TEXT("Looter.Barge.Grates"));
		ABargeFurnace* Furnace = Barge ? Barge->GetFurnace() : nullptr;
		if (!Furnace)
		{
			return;
		}
		if (Args.Num() > 0)
		{
			Furnace->SetRingRunning(!Args[0].Equals(TEXT("stop"), ESearchCase::IgnoreCase));
		}
		FString Ring;
		for (int32 Wedge = 0; Wedge < BargeRules::Grates; ++Wedge)
		{
			Ring += (Furnace->GetHotMask() & (1u << Wedge)) ? (Furnace->IsRingBurning() ? TEXT("#") : TEXT("o")) : TEXT(".");
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Barge.Grates: %s, %d valves shut, clock %.1f s, [%s]"), Furnace->IsRingRunning() ? TEXT("running") : TEXT("still"),
			Furnace->GetValvesShut(), Furnace->GetRingClock(), *Ring);
	}

	FAutoConsoleCommandWithWorldAndArgs BargeShowRegistration(TEXT("Looter.Barge.Show"),
		TEXT("Plays Main 6's barge as if the mission were on a step: Looter.Barge.Show [step (3)] [aboard] (aboard: the player on her deck)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BargeShow));

	FAutoConsoleCommandWithWorldAndArgs BargeStoryRegistration(TEXT("Looter.Barge.Story"),
		TEXT("Puts Main 6's barge back to the story's word."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BargeStory));

	FAutoConsoleCommandWithWorldAndArgs BargeValveRegistration(TEXT("Looter.Barge.Valve"),
		TEXT("Shuts the next n (1) of the furnace's draft valves now: Looter.Barge.Valve [n]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BargeValve));

	FAutoConsoleCommandWithWorldAndArgs BargeJamRegistration(TEXT("Looter.Barge.Jam"),
		TEXT("Jams the barge's hose drum now (Pilot.DrumJammed)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BargeJam));

	FAutoConsoleCommandWithWorldAndArgs BargeCageRegistration(TEXT("Looter.Barge.Cage"),
		TEXT("Bursts the furnace's soul cage now (the CageBurst scene), whatever the valves and the lantern."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BargeCage));

	FAutoConsoleCommandWithWorldAndArgs BargeDieRegistration(TEXT("Looter.Barge.Die"),
		TEXT("Puts the barge's pilot light out: she lists, drifts west and burns, and her third stack falls."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BargeDie));

	FAutoConsoleCommandWithWorldAndArgs BargeFallRegistration(TEXT("Looter.Barge.Fall"),
		TEXT("Plays the BargeFall scene now: the barge over the Rest and down into Gravewind Canyon."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BargeFallNow));

	FAutoConsoleCommandWithWorldAndArgs BargeGratesRegistration(TEXT("Looter.Barge.Grates"),
		TEXT("The furnace's grate ring: Looter.Barge.Grates [run|stop] (. cold, o glowing, # burning)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BargeGrates));
}

#endif
