// Developer console commands for Main 7, "Given to the Wind" (the Wardens story, lane MAIN67; not in shipping builds):
//   Looter.Coal.Free [n]            the n (1) nearest whole soul coal lumps cracked now: their souls join the trail
//   Looter.Lantern.Carry [0|1|-1]   Abel's lantern carried (1, the default), not (0), or as the story says (-1)
//   Looter.Lantern.Drain <share>    a line's worth taken off his light now (0-1)
//   Looter.Reliquary.Wake [quiet]   Abel's lantern set on the Reliquary now and Saint Ada wakes (quiet: her end state, no scene)

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Scenes/SaintAdaWakes.h"
#include "World/CarriedLantern.h"
#include "World/ReliquaryLantern.h"
#include "World/SoulCoal.h"
#include "Algo/Sort.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindMain7World(UWorld* World)
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

	FVector PlayerAt(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		return Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	}

	void CoalFree(const TArray<FString>& Args, UWorld* World)
	{
		World = FindMain7World(World);
		if (!World)
		{
			return;
		}
		const int32 Count = Args.Num() > 0 ? FMath::Max(1, FCString::Atoi(*Args[0])) : 1;
		TArray<ASoulCoalLump*> Whole;
		for (TActorIterator<ASoulCoalLump> It(World); It; ++It)
		{
			if (!It->IsCracked())
			{
				Whole.Add(*It);
			}
		}
		const FVector Here = PlayerAt(World);
		Algo::SortBy(Whole, [Here](const ASoulCoalLump* Lump) { return FVector::DistSquared(Lump->GetActorLocation(), Here); });
		int32 Freed = 0;
		for (ASoulCoalLump* Lump : Whole)
		{
			if (Freed >= Count)
			{
				break;
			}
			Freed += Lump->Crack(World->GetFirstPlayerController(), /*bForce*/ true) ? 1 : 0;
		}
		const UCarriedLanternSubsystem* Lantern = UCarriedLanternSubsystem::Get(World);
		UE_LOG(LogLooter, Display, TEXT("Looter.Coal.Free: %d cracked (%d whole left); %d souls follow."), Freed, Whole.Num() - Freed,
			Lantern ? Lantern->NumFollowers() : 0);
	}

	void LanternCarry(const TArray<FString>& Args, UWorld* World)
	{
		World = FindMain7World(World);
		if (UCarriedLanternSubsystem* Lantern = UCarriedLanternSubsystem::Get(World))
		{
			Lantern->ForceCarried(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1);
			UE_LOG(LogLooter, Display, TEXT("Looter.Lantern.Carry: %s, his light %.0f%%."), Lantern->IsCarried() ? TEXT("carried") : TEXT("not carried"),
				Lantern->GetLight() * 100.f);
		}
	}

	void LanternDrain(const TArray<FString>& Args, UWorld* World)
	{
		World = FindMain7World(World);
		if (UCarriedLanternSubsystem* Lantern = UCarriedLanternSubsystem::Get(World))
		{
			const float Share = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.25f;
			Lantern->Drain(Share, nullptr);
			UE_LOG(LogLooter, Display, TEXT("Looter.Lantern.Drain: his light %.0f%%%s."), Lantern->GetLight() * 100.f,
				Lantern->IsCarried() ? TEXT("") : TEXT(" (not carried: nothing drained; Looter.Lantern.Carry 1 first)"));
		}
	}

	void ReliquaryWake(const TArray<FString>& Args, UWorld* World)
	{
		World = FindMain7World(World);
		if (!World)
		{
			return;
		}
		AReliquaryLantern* Rest = nullptr;
		for (TActorIterator<AReliquaryLantern> It(World); It; ++It)
		{
			Rest = *It;
			break;
		}
		if (!Rest)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Reliquary.Wake: no Reliquary lantern in this level (Ransom's Rest: build_area_main7.py)."));
			return;
		}
		const bool bQuiet = Args.Num() > 0 && Args[0].Equals(TEXT("quiet"), ESearchCase::IgnoreCase);
		const bool bSet = Rest->SetLantern(World->GetFirstPlayerController(), /*bForce*/ true, /*bQuiet*/ bQuiet);
		if (bQuiet)
		{
			SaintAdaWakes::FinishAtOnce(*Rest);
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Reliquary.Wake: %s"), bSet ? (bQuiet ? TEXT("her end state") : TEXT("the scene plays"))
			: TEXT("already set (Saint Ada is awake)"));
	}

	FAutoConsoleCommandWithWorldAndArgs CoalFreeRegistration(TEXT("Looter.Coal.Free"),
		TEXT("Cracks the n nearest whole soul coal lumps now (Main 7): Looter.Coal.Free [n] (1); their souls join the trail."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CoalFree));

	FAutoConsoleCommandWithWorldAndArgs LanternCarryRegistration(TEXT("Looter.Lantern.Carry"),
		TEXT("Abel's lantern carried at Sully's hip: Looter.Lantern.Carry [1|0|-1] (1 carried, 0 not, -1 as the story says)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LanternCarry));

	FAutoConsoleCommandWithWorldAndArgs LanternDrainRegistration(TEXT("Looter.Lantern.Drain"),
		TEXT("Takes a share of Abel's carried light now: Looter.Lantern.Drain <share> (0-1, default 0.25)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LanternDrain));

	FAutoConsoleCommandWithWorldAndArgs ReliquaryWakeRegistration(TEXT("Looter.Reliquary.Wake"),
		TEXT("Sets Abel's lantern on Saint Ada's Reliquary now and plays SaintAdaWakes: Looter.Reliquary.Wake [quiet] (quiet: her end state only)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ReliquaryWake));
}

#endif
