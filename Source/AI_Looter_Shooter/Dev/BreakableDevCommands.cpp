// Developer console commands for the breakable crates and barrels (ABreakableProp, the lootable world): break the nearest,
// mend them all, list them (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "World/BreakableDebris.h"
#include "World/BreakableProp.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
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

	APawn* FindPlayer(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	/** Every breakable in the level, nearest the player first. */
	TArray<ABreakableProp*> FindBreakables(UWorld* World)
	{
		TArray<ABreakableProp*> Found;
		for (TActorIterator<ABreakableProp> It(World); It; ++It)
		{
			Found.Add(*It);
		}
		const APawn* Player = FindPlayer(World);
		const FVector From = Player ? Player->GetActorLocation() : FVector::ZeroVector;
		Found.Sort([&From](const ABreakableProp& A, const ABreakableProp& B)
		{
			return FVector::DistSquared(A.GetActorLocation(), From) < FVector::DistSquared(B.GetActorLocation(), From);
		});
		return Found;
	}

	FString Describe(const ABreakableProp& Prop, const FVector& From)
	{
		const FVector At = Prop.GetActorLocation();
		const FBreakableKindInfo Info = Prop.GetKindInfo();
		return FString::Printf(TEXT("%s (%s, %s): %s; %.0f%% ammo, %.0f%% soul-mote; at (%.0f, %.0f, %.0f), %.0f m away"),
			*Prop.GetSaveKey().ToString(), *Prop.GetActorNameOrLabel(), *UEnum::GetDisplayValueAsText(Prop.Kind).ToString(),
			Prop.IsBroken() ? TEXT("broken") : TEXT("whole"), Info.AmmoChance * 100.f, Info.MoteChance * 100.f, At.X, At.Y, At.Z,
			FVector::Dist(At, From) / 100.0);
	}

	/** Looter.Breakable.Break [all]: breaks the nearest whole breakable as a blow from the player would; 'all' breaks every one. */
	void BreakProps(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Breakable.Break");
		UWorld* GameWorld = FindGameWorld(World);
		TArray<ABreakableProp*> Whole = GameWorld ? FindBreakables(GameWorld).FilterByPredicate([](const ABreakableProp* Prop)
		{
			return !Prop->IsBroken();
		}) : TArray<ABreakableProp*>();
		if (Whole.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no whole breakable in the level being played (Looter.Breakable.Mend mends them)."), Command);
			return;
		}
		const bool bAll = Args.ContainsByPredicate([](const FString& Arg) { return Arg.Equals(TEXT("all"), ESearchCase::IgnoreCase); });
		if (!bAll)
		{
			Whole.SetNum(1);
		}
		APawn* Player = FindPlayer(GameWorld);
		AController* By = Player ? Player->GetController() : nullptr;
		const FVector From = Player ? Player->GetActorLocation() : FVector::ZeroVector;
		for (ABreakableProp* Prop : Whole)
		{
			Prop->Break(By, Prop->GetActorLocation() - From);
			UE_LOG(LogLooter, Log, TEXT("%s: %s"), Command, *Describe(*Prop, From));
		}
	}

	/** Looter.Breakable.Mend: every breakable whole again (what they dropped stays where it lies). */
	void MendProps(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const TArray<ABreakableProp*> All = GameWorld ? FindBreakables(GameWorld) : TArray<ABreakableProp*>();
		for (ABreakableProp* Prop : All)
		{
			Prop->Mend();
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Breakable.Mend: %d breakable(s) whole again (a save keeps them so)."), All.Num());
	}

	/** Looter.Breakable.List: every breakable, nearest first, and the pieces in flight. */
	void ListProps(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const TArray<ABreakableProp*> All = GameWorld ? FindBreakables(GameWorld) : TArray<ABreakableProp*>();
		const APawn* Player = FindPlayer(GameWorld);
		const FVector From = Player ? Player->GetActorLocation() : FVector::ZeroVector;
		int32 Broken = 0;
		for (const ABreakableProp* Prop : All)
		{
			Broken += Prop->IsBroken() ? 1 : 0;
			UE_LOG(LogLooter, Log, TEXT("Looter.Breakable.List: %s"), *Describe(*Prop, From));
		}
		const UBreakableDebrisSubsystem* Debris = GameWorld ? UBreakableDebrisSubsystem::Get(GameWorld) : nullptr;
		UE_LOG(LogLooter, Log, TEXT("Looter.Breakable.List: %d breakable(s), %d broken; %d piece(s) out on %d pooled component(s)."),
			All.Num(), Broken, Debris ? Debris->NumPieces() : 0, Debris ? Debris->NumComponents() : 0);
	}

	FAutoConsoleCommandWithWorldAndArgs BreakCommand(
		TEXT("Looter.Breakable.Break"),
		TEXT("Looter.Breakable.Break [all]: breaks the nearest whole crate or barrel as a blow from the player would; 'all' breaks every one."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BreakProps));

	FAutoConsoleCommandWithWorldAndArgs MendCommand(
		TEXT("Looter.Breakable.Mend"),
		TEXT("Looter.Breakable.Mend: every crate and barrel whole again (what they dropped stays where it lies)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MendProps));

	FAutoConsoleCommandWithWorldAndArgs ListCommand(
		TEXT("Looter.Breakable.List"),
		TEXT("Looter.Breakable.List: every crate and barrel, nearest first (id, kind, broken or whole, its chances), and the pieces in flight."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListProps));
}

#endif
