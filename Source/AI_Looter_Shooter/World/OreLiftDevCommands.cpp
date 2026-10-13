// Developer console commands for the mountain's pieces (not in shipping builds): the ore lift (Looter.Lift.Status, .Open,
// .Call, .Ride), the watch fire (Looter.WatchFire.Light, .Douse), the far answering light (Looter.FarLight) and the
// interiors' visibility (Looter.Interior.Status).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/FarSignalLight.h"
#include "World/InteriorVisibility.h"
#include "World/OreLift.h"
#include "World/WatchFire.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* GameWorld(UWorld* World)
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

	const TCHAR* EndName(EOreLiftEnd End)
	{
		return End == EOreLiftEnd::Bottom ? TEXT("bottom") : TEXT("top");
	}

	const TCHAR* BellName(EOreLiftBell Bell)
	{
		switch (Bell)
		{
		case EOreLiftBell::Closed: return TEXT("closed");
		case EOreLiftBell::Ride: return TEXT("ride");
		case EOreLiftBell::Call: return TEXT("call");
		default: return TEXT("busy");
		}
	}

	/** Looter.Lift.Status: each lift, whether it runs (and why not), where its cage is, what each bell pull does. */
	void LiftStatus(const TArray<FString>& Args, UWorld* World)
	{
		int32 Found = 0;
		for (TActorIterator<AOreLift> It(GameWorld(World)); It; ++It)
		{
			++Found;
			UE_LOG(LogLooter, Display, TEXT("%s: %s; the cage %s the %s landing; bell pulls: bottom %s, top %s; %.0f m of cable%s"),
				*It->GetActorNameOrLabel(), It->IsOpen() ? TEXT("runs") : *FString::Printf(TEXT("closed (\"%s\")"), *It->GetClosedReason().ToString()),
				It->IsMoving() ? TEXT("left") : TEXT("rests at"), EndName(It->GetCageEnd()), BellName(It->GetBellState(EOreLiftEnd::Bottom)),
				BellName(It->GetBellState(EOreLiftEnd::Top)), It->GetCableLength() / 100.f, It->IsHoldingSaves() ? TEXT("; the autosave waits") : TEXT(""));
		}
		if (Found == 0)
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Lift.Status: no ore lift here."));
		}
	}

	/** Looter.Lift.Open [1|0]: runs every lift whatever the story (1, the default), or back to what the story says (0). */
	void LiftOpen(const TArray<FString>& Args, UWorld* World)
	{
		const bool bOpen = Args.IsEmpty() || FCString::Atoi(*Args[0]) != 0;
		for (TActorIterator<AOreLift> It(GameWorld(World)); It; ++It)
		{
			It->SetForcedOpen(bOpen);
			UE_LOG(LogLooter, Display, TEXT("%s: %s."), *It->GetActorNameOrLabel(), bOpen ? TEXT("runs, whatever the story") : TEXT("runs as the story says"));
		}
	}

	/** Looter.Lift.Call [top|bottom]: calls each lift's cage to that landing (the other one by default). */
	void LiftCall(const TArray<FString>& Args, UWorld* World)
	{
		for (TActorIterator<AOreLift> It(GameWorld(World)); It; ++It)
		{
			const EOreLiftEnd Other = It->GetCageEnd() == EOreLiftEnd::Bottom ? EOreLiftEnd::Top : EOreLiftEnd::Bottom;
			const EOreLiftEnd To = Args.IsEmpty() ? Other : Args[0].Equals(TEXT("top"), ESearchCase::IgnoreCase) ? EOreLiftEnd::Top : EOreLiftEnd::Bottom;
			UE_LOG(LogLooter, Display, TEXT("%s: %s"), *It->GetActorNameOrLabel(), It->CallTo(To) ? TEXT("called.") : TEXT("can't be called now (Looter.Lift.Status says why)."));
		}
	}

	/** Looter.Lift.Ride: rides the player from the landing the cage waits at (wherever they stand), as a held bell pull would. */
	void LiftRide(const TArray<FString>& Args, UWorld* World)
	{
		World = GameWorld(World);
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		for (TActorIterator<AOreLift> It(World); It; ++It)
		{
			UE_LOG(LogLooter, Display, TEXT("%s: %s"), *It->GetActorNameOrLabel(),
				It->Ride(It->GetCageEnd(), Pawn) ? TEXT("riding.") : TEXT("no ride now (Looter.Lift.Status says why; Looter.Lift.Open opens it)."));
			return;
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Lift.Ride: no ore lift here."));
	}

	/** Looter.WatchFire.Light / .Douse: lights every watch fire (setting its flag, as the player's light does) or puts it out. */
	void WatchFireSet(UWorld* World, bool bLit)
	{
		World = GameWorld(World);
		UMissionRunner* Runner = UMissionRunner::Get(World);
		for (TActorIterator<AWatchFire> It(World); It; ++It)
		{
			if (Runner && Runner->IsActive())
			{
				FCampaignRecord& Campaign = Runner->GetCampaign();
				if (bLit)
				{
					Campaign.SetFlag(It->LitFlag);
				}
				else
				{
					Campaign.ClearFlag(It->LitFlag);
				}
			}
			It->SetLit(bLit);
			UE_LOG(LogLooter, Display, TEXT("%s: %s."), *It->GetActorNameOrLabel(), bLit ? TEXT("lit") : TEXT("dark"));
		}
	}

	void WatchFireLight(const TArray<FString>& Args, UWorld* World)
	{
		WatchFireSet(World, true);
	}

	void WatchFireDouse(const TArray<FString>& Args, UWorld* World)
	{
		WatchFireSet(World, false);
	}

	/** Looter.FarLight [1|0]: shows every far light keying its message (1), or back to what the story says (0). */
	void FarLight(const TArray<FString>& Args, UWorld* World)
	{
		const bool bShow = Args.IsEmpty() || FCString::Atoi(*Args[0]) != 0;
		for (TActorIterator<AFarSignalLight> It(GameWorld(World)); It; ++It)
		{
			It->bAlwaysShown = bShow;
			It->SetShown(bShow);
			It->RefreshStory();
			UE_LOG(LogLooter, Display, TEXT("%s: %s."), *It->GetActorNameOrLabel(), It->IsShown() ? TEXT("keying") : TEXT("dark"));
		}
	}

	/** Looter.Interior.Status: each interior, where the view is, and how many pieces it hides. */
	void InteriorStatus(const TArray<FString>& Args, UWorld* World)
	{
		for (TActorIterator<AInteriorVisibility> It(GameWorld(World)); It; ++It)
		{
			const EInteriorView View = It->GetView();
			UE_LOG(LogLooter, Display, TEXT("%s: the view is %s; %d pieces hidden (inside tag %s, outside tag %s)."), *It->GetActorNameOrLabel(),
				View == EInteriorView::Inside ? TEXT("inside") : View == EInteriorView::Outside ? TEXT("outside") : TEXT("at the portal"),
				It->CountHidden(), *It->InsideTag.ToString(), *It->OutsideTag.ToString());
		}
	}

	FAutoConsoleCommandWithWorldAndArgs LiftStatusCommand(TEXT("Looter.Lift.Status"),
		TEXT("The ore lift: whether it runs (and why not), where its cage is, what each bell pull does."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LiftStatus));
	FAutoConsoleCommandWithWorldAndArgs LiftOpenCommand(TEXT("Looter.Lift.Open"),
		TEXT("Looter.Lift.Open [1|0]: runs the ore lift whatever the story (1), or as the story says (0)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LiftOpen));
	FAutoConsoleCommandWithWorldAndArgs LiftCallCommand(TEXT("Looter.Lift.Call"),
		TEXT("Looter.Lift.Call [top|bottom]: calls the ore lift's cage to that landing (the other one by default)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LiftCall));
	FAutoConsoleCommandWithWorldAndArgs LiftRideCommand(TEXT("Looter.Lift.Ride"),
		TEXT("Rides the player on the ore lift from the landing its cage waits at."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LiftRide));
	FAutoConsoleCommandWithWorldAndArgs WatchFireLightCommand(TEXT("Looter.WatchFire.Light"),
		TEXT("Lights every watch fire, as the player's hold does (its flag set)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WatchFireLight));
	FAutoConsoleCommandWithWorldAndArgs WatchFireDouseCommand(TEXT("Looter.WatchFire.Douse"),
		TEXT("Puts every watch fire out (its flag cleared)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WatchFireDouse));
	FAutoConsoleCommandWithWorldAndArgs FarLightCommand(TEXT("Looter.FarLight"),
		TEXT("Looter.FarLight [1|0]: every far answering light keys its message (1), or shows as the story says (0)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FarLight));
	FAutoConsoleCommandWithWorldAndArgs InteriorStatusCommand(TEXT("Looter.Interior.Status"),
		TEXT("Each interior's view (outside, at the portal, inside) and how many pieces it hides."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&InteriorStatus));
}

#endif
