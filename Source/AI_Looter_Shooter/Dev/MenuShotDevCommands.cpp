// Developer console command that photographs the inventory through its states (not in shipping builds).
//
// Looter.MenuShots [quit] gives the player a realistic loadout, opens the inventory, drives it with key presses and takes a
// screenshot WITH the UI at each step, into Saved/Screenshots/MenuShots/<NN>_<state>.png, so a redesign can be checked
// before and after while nobody is at the keyboard (Tools/menushots.ps1 runs it standalone at 1920x1080). MenuShotScene.cpp
// makes each state; this file runs the steps on a clock and takes the pictures, as Looter.HudShots does for the HUD.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Dev/MenuShotScene.h"
#include "AI_Looter_Shooter.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
	// Real seconds: the run's own clock, like Looter.HudShots'.

	/** The most it waits for the player and the HUD to exist (a level loading, shaders compiling) before giving up. */
	constexpr float MenuPlayerWaitSeconds = 90.f;
	/** After the scene is set up: the level's first frames, streaming, eye adaptation and the guns' meshes loading. */
	constexpr float MenuWarmupSeconds = 6.f;
	/** A step whose moment (Ready) hasn't come this long after its time is photographed anyway, with a warning. */
	constexpr float MenuReadyWaitSeconds = 3.f;

	FString MenuShotDirectory()
	{
		return FPaths::ProjectSavedDir() / TEXT("Screenshots/MenuShots");
	}

	/** The run in progress: a state machine ticked every frame by the core ticker (no UObject needed). */
	class FMenuShotRun
	{
	public:
		explicit FMenuShotRun(bool bInQuit) : bQuit(bInQuit) {}

		/** One frame of the run; false once it's over. */
		bool Tick(float DeltaTime)
		{
			Clock += DeltaTime;
			if (Phase == EPhase::Waiting)
			{
				return TickWaiting();
			}
			UWorld* World = WorldPtr.Get();
			if (!World)
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.MenuShots: the world went away; stopped."));
				return Finish(false);
			}
			if (Phase == EPhase::Warmup)
			{
				return Clock >= MenuWarmupSeconds ? BeginStep(*World, 0) : true;
			}
			return TickStep(*World);
		}

	private:
		enum class EPhase : uint8
		{
			Waiting,
			Warmup,
			Step,
		};

		bool TickWaiting()
		{
			// The level may still be loading when the command runs (-ExecCmds): look again each frame.
			UWorld* World = MenuShotScene::FindGameWorld();
			if (World && MenuShotScene::IsPlayerReady(*World))
			{
				UE_LOG(LogLooter, Display, TEXT("Looter.MenuShots: %s, %d states, pictures in %s."), *World->GetMapName(),
					MenuShotScene::Steps().Num(), *MenuShotDirectory());
				WorldPtr = World;
				MenuShotScene::Prepare(*World);
				Phase = EPhase::Warmup;
				Clock = 0.f;
				return true;
			}
			if (Clock > MenuPlayerWaitSeconds)
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.MenuShots: no player with a HUD after %.0f s (start a level, not the menu); stopped."), MenuPlayerWaitSeconds);
				return Finish(false);
			}
			return true;
		}

		bool BeginStep(UWorld& World, int32 Index)
		{
			const TConstArrayView<MenuShotScene::FStep> Steps = MenuShotScene::Steps();
			if (!Steps.IsValidIndex(Index))
			{
				return Finish(true);
			}
			StepIndex = Index;
			Phase = EPhase::Step;
			Clock = 0.f;
			ShotAt = 0.f;
			bShotRequested = false;
			UE_LOG(LogLooter, Display, TEXT("Looter.MenuShots: %02d %s"), Index + 1, Steps[Index].Name);
			if (Steps[Index].Begin)
			{
				Steps[Index].Begin(World);
			}
			return true;
		}

		bool TickStep(UWorld& World)
		{
			const MenuShotScene::FStep& Step = MenuShotScene::Steps()[StepIndex];
			if (!bShotRequested)
			{
				if (Clock < Step.Seconds)
				{
					return true;
				}
				const bool bReady = !Step.Ready || Step.Ready(World);
				if (!bReady && Clock < Step.Seconds + MenuReadyWaitSeconds)
				{
					return true;
				}
				if (!bReady)
				{
					UE_LOG(LogLooter, Warning, TEXT("Looter.MenuShots: %02d %s: its moment didn't come in %.0f s; photographed anyway."),
						StepIndex + 1, Step.Name, MenuReadyWaitSeconds);
				}
				// The screenshot is taken at the end of this frame, with the UI.
				const FString File = MenuShotDirectory() / FString::Printf(TEXT("%02d_%s.png"), StepIndex + 1, Step.Name);
				FScreenshotRequest::RequestScreenshot(File, /*bInShowUI*/ true, /*bAddFilenameSuffix*/ false);
				bShotRequested = true;
				ShotAt = Clock;
				return true;
			}
			if (Clock >= ShotAt + Step.GapSeconds)
			{
				if (Step.After)
				{
					Step.After(World);
				}
				return BeginStep(World, StepIndex + 1);
			}
			return true;
		}

		/** Ends the run (false, for the ticker); with quit, the game goes too. */
		bool Finish(bool bCompleted)
		{
			if (bCompleted)
			{
				UE_LOG(LogLooter, Display, TEXT("Looter.MenuShots: done, %d pictures in %s."), MenuShotScene::Steps().Num(), *MenuShotDirectory());
			}
			if (bQuit)
			{
				FPlatformMisc::RequestExit(false, TEXT("Looter.MenuShots"));
			}
			return false;
		}

		TWeakObjectPtr<UWorld> WorldPtr;
		EPhase Phase = EPhase::Waiting;
		int32 StepIndex = 0;
		/** Seconds in the phase (in a step: since its Begin), and when in the step the picture was requested. */
		float Clock = 0.f;
		float ShotAt = 0.f;
		bool bShotRequested = false;
		bool bQuit = false;
	};

	TSharedPtr<FMenuShotRun> ActiveMenuShotRun;

	/** Looter.MenuShots [quit] */
	void MenuShotsCommand(const TArray<FString>& Args, UWorld* /*World*/)
	{
		if (ActiveMenuShotRun.IsValid())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.MenuShots: already running."));
			return;
		}
		const bool bQuit = Args.ContainsByPredicate([](const FString& Arg) { return Arg.Equals(TEXT("quit"), ESearchCase::IgnoreCase); });
		IFileManager::Get().MakeDirectory(*MenuShotDirectory(), /*Tree*/ true);

		ActiveMenuShotRun = MakeShared<FMenuShotRun>(bQuit);
		const TSharedPtr<FMenuShotRun> Run = ActiveMenuShotRun;
		FTSTicker::GetCoreTicker().AddTicker(TEXT("Looter.MenuShots"), 0.f, [Run](float DeltaTime)
			{
				const bool bGoOn = Run->Tick(DeltaTime);
				if (!bGoOn)
				{
					ActiveMenuShotRun.Reset();
				}
				return bGoOn;
			});
	}

	FAutoConsoleCommandWithWorldAndArgs MenuShotsCommandRegistration(
		TEXT("Looter.MenuShots"),
		TEXT("Looter.MenuShots [quit]: gives the player a realistic loadout and photographs the inventory through its states (the loadout, ")
		TEXT("a backpack gun and its comparison, an upgrade, Inspect, an equip, a swap target, a sort, the ledger, the missions) into ")
		TEXT("Saved/Screenshots/MenuShots/<NN>_<state>.png, each with the UI; with quit, the game quits after the last one. ")
		TEXT("Tools/menushots.ps1 runs it standalone at 1920x1080."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MenuShotsCommand));
}

#endif
