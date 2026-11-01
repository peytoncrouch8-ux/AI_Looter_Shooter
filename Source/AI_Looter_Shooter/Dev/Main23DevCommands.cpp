// Developer console commands for Mains 2 and 3 (the Wardens story; not in shipping builds):
//   Looter.HoseCart.Stop [burst]       the nearest pumping hose-cart stopped (its valve shut; "burst": its tank burst)
//   Looter.Loudspeaker.Silence         the nearest Company loudspeaker's horns shot out
//   Looter.Loudspeaker.Say             the nearest loudspeaker says its first cue now (whatever its story)
//   Looter.FlyBy.Ned                   Lucky Ned's skiff casts off from the barge now
//   Looter.Ferry.Climb                 the player down (or up) the Ferryman's ladder
//   Looter.Banner.Cut                  both of Main Street's banner ropes cut: it falls into the street

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Story/StoryLineSet.h"
#include "World/CompanyBanner.h"
#include "World/CompanyHoseCart.h"
#include "World/CompanyLoudspeaker.h"
#include "World/FerryLadder.h"
#include "World/GangSkiffFlyBy.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindMain23World(UWorld* World)
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

	APawn* FindMain23Player(UWorld* World)
	{
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	/** The actor of class T nearest the player (or any, without one), or null with a word in the log. */
	template <typename T>
	T* FindNearest(UWorld* World, const TCHAR* Command, const TCHAR* What)
	{
		const APawn* Player = FindMain23Player(World);
		T* Best = nullptr;
		double BestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<T> It(World); It; ++It)
		{
			const double Distance = Player ? FVector::DistSquared(Player->GetActorLocation(), It->GetActorLocation()) : 0.0;
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = *It;
			}
		}
		if (!Best)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no %s in this level (Ransom's Rest: build_area_main2.py / build_area_main3.py)."), Command, What);
		}
		return Best;
	}

	void HoseCartStop(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.HoseCart.Stop");
		World = FindMain23World(World);
		if (ACompanyHoseCart* Cart = World ? FindNearest<ACompanyHoseCart>(World, Command, TEXT("hose-cart")) : nullptr)
		{
			const bool bBurst = Args.Num() > 0 && Args[0].Equals(TEXT("burst"), ESearchCase::IgnoreCase);
			const bool bStopped = Cart->StopPumping(FindMain23Player(World) ? FindMain23Player(World)->GetController() : nullptr, bBurst);
			UE_LOG(LogLooter, Display, TEXT("%s: %s %s."), Command, *Cart->GetActorNameOrLabel(),
				bStopped ? (bBurst ? TEXT("burst") : TEXT("shut")) : TEXT("isn't pumping"));
		}
	}

	void LoudspeakerSilence(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Loudspeaker.Silence");
		World = FindMain23World(World);
		if (ACompanyLoudspeaker* Horn = World ? FindNearest<ACompanyLoudspeaker>(World, Command, TEXT("Company loudspeaker")) : nullptr)
		{
			UE_LOG(LogLooter, Display, TEXT("%s: %s %s."), Command, *Horn->GetActorNameOrLabel(),
				Horn->Silence(nullptr) ? TEXT("silenced") : TEXT("was silent already (or is a voice only)"));
		}
	}

	void LoudspeakerSay(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Loudspeaker.Say");
		World = FindMain23World(World);
		ACompanyLoudspeaker* Horn = World ? FindNearest<ACompanyLoudspeaker>(World, Command, TEXT("Company loudspeaker")) : nullptr;
		if (!Horn)
		{
			return;
		}
		for (const FLoudspeakerCue& Cue : Horn->Cues)
		{
			if (Cue.Lines && Horn->Broadcast(Cue.Lines->Lines))
			{
				UE_LOG(LogLooter, Display, TEXT("%s: %s says %s."), Command, *Horn->GetActorNameOrLabel(), *Cue.Lines->GetName());
				return;
			}
		}
		UE_LOG(LogLooter, Display, TEXT("%s: %s has nothing to say (no cues, or silenced)."), Command, *Horn->GetActorNameOrLabel());
	}

	void FlyByNed(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.FlyBy.Ned");
		World = FindMain23World(World);
		if (AGangSkiffFlyBy* Skiff = World ? FindNearest<AGangSkiffFlyBy>(World, Command, TEXT("gang skiff fly-by")) : nullptr)
		{
			UE_LOG(LogLooter, Display, TEXT("%s: %s."), Command, Skiff->Begin() ? TEXT("casting off") : TEXT("already flying, or no course"));
		}
	}

	void FerryClimb(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Ferry.Climb");
		World = FindMain23World(World);
		APawn* Player = FindMain23Player(World);
		AFerryLadder* Ladder = World ? FindNearest<AFerryLadder>(World, Command, TEXT("ferry ladder")) : nullptr;
		if (Ladder && Player)
		{
			const bool bDown = Ladder->IsAtTop(Player->GetActorLocation());
			Ladder->Climb(Player, bDown);
			UE_LOG(LogLooter, Display, TEXT("%s: climbing %s."), Command, bDown ? TEXT("down to the ferry") : TEXT("up to the deck"));
		}
	}

	void BannerCut(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Banner.Cut");
		World = FindMain23World(World);
		if (ACompanyBanner* Banner = World ? FindNearest<ACompanyBanner>(World, Command, TEXT("Company banner")) : nullptr)
		{
			Banner->CutRope(0, nullptr);
			Banner->CutRope(1, nullptr);
			UE_LOG(LogLooter, Display, TEXT("%s: %s, both ropes cut."), Command, *Banner->GetActorNameOrLabel());
		}
	}

	FAutoConsoleCommandWithWorldAndArgs HoseCartStopRegistration(TEXT("Looter.HoseCart.Stop"),
		TEXT("Stops the nearest pumping Company hose-cart: Looter.HoseCart.Stop [burst] (its valve shut, or its tank burst)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HoseCartStop));

	FAutoConsoleCommandWithWorldAndArgs LoudspeakerSilenceRegistration(TEXT("Looter.Loudspeaker.Silence"),
		TEXT("Shoots out the nearest Company loudspeaker's horns (Vane cut off if he's talking from it)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LoudspeakerSilence));

	FAutoConsoleCommandWithWorldAndArgs LoudspeakerSayRegistration(TEXT("Looter.Loudspeaker.Say"),
		TEXT("The nearest Company loudspeaker says its first cue now, whatever its story."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LoudspeakerSay));

	FAutoConsoleCommandWithWorldAndArgs FlyByNedRegistration(TEXT("Looter.FlyBy.Ned"),
		TEXT("Lucky Ned's skiff casts off from Vane's barge now and flies south over the Hogback (Main 3)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FlyByNed));

	FAutoConsoleCommandWithWorldAndArgs FerryClimbRegistration(TEXT("Looter.Ferry.Climb"),
		TEXT("The player down the Ferryman's ladder to his landing, or up it to Gravewind Point's deck (Main 2)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FerryClimb));

	FAutoConsoleCommandWithWorldAndArgs BannerCutRegistration(TEXT("Looter.Banner.Cut"),
		TEXT("Cuts both ropes of Main Street's Company banner: it falls into the street (Main 3's optional)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BannerCut));
}

#endif
