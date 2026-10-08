// Developer console command that photographs the gameplay HUD through its states (not in shipping builds).
//
// Looter.HudShots [quit] plays a timed sequence in the running game and takes a screenshot WITH the UI at each step, into
// Saved/Screenshots/HudShots/<NN>_<state>.png, so a new HUD can be checked against its mockup state by state
// (Tools/hudshots.ps1 runs it standalone at 1920x1080). HudShotScene.cpp makes each state through the game's own paths, so
// the HUD reacts as it does in play; this file runs the steps on a clock and takes the pictures.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Dev/HudShotScene.h"
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
	// Real seconds: the run's own clock, like Looter.Tour's.

	/** The most it waits for the player and the HUD to exist (a level loading, shaders compiling) before giving up. */
	constexpr float PlayerWaitSeconds = 90.f;
	/** After the scene is set up: the level's first frames, streaming, eye adaptation, and the HUD's clusters fading back to idle. */
	constexpr float WarmupSeconds = 6.f;
	/** A step whose moment (Ready) hasn't come this long after its time is photographed anyway, with a warning. */
	constexpr float ReadyWaitSeconds = 2.f;

	FString HudShotDirectory()
	{
		return FPaths::ProjectSavedDir() / TEXT("Screenshots/HudShots");
	}

	/** The run in progress: a state machine ticked every frame by the core ticker (no UObject needed). */
	class FHudShotRun
	{
	public:
		explicit FHudShotRun(bool bInQuit) : bQuit(bInQuit) {}

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
				UE_LOG(LogLooter, Warning, TEXT("Looter.HudShots: the world went away; stopped."));
				return Finish(false);
			}
			if (Phase == EPhase::Warmup)
			{
				return Clock >= WarmupSeconds ? BeginStep(*World, 0) : true;
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
			UWorld* World = HudShotScene::FindGameWorld();
			if (World && HudShotScene::IsPlayerReady(*World))
			{
				UE_LOG(LogLooter, Display, TEXT("Looter.HudShots: %s, %d states, pictures in %s."), *World->GetMapName(),
					HudShotScene::Steps().Num(), *HudShotDirectory());
				WorldPtr = World;
				HudShotScene::Prepare(*World);
				Phase = EPhase::Warmup;
				Clock = 0.f;
				return true;
			}
			if (Clock > PlayerWaitSeconds)
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.HudShots: no player with a HUD after %.0f s (start a level, not the menu); stopped."), PlayerWaitSeconds);
				return Finish(false);
			}
			return true;
		}

		bool BeginStep(UWorld& World, int32 Index)
		{
			const TConstArrayView<HudShotScene::FStep> Steps = HudShotScene::Steps();
			if (!Steps.IsValidIndex(Index))
			{
				return Finish(true);
			}
			StepIndex = Index;
			Phase = EPhase::Step;
			Clock = 0.f;
			ShotAt = 0.f;
			bShotRequested = false;
			UE_LOG(LogLooter, Display, TEXT("Looter.HudShots: %02d %s"), Index + 1, Steps[Index].Name);
			if (Steps[Index].Begin)
			{
				Steps[Index].Begin(World);
			}
			return true;
		}

		bool TickStep(UWorld& World)
		{
			const HudShotScene::FStep& Step = HudShotScene::Steps()[StepIndex];
			if (!bShotRequested)
			{
				if (Clock < Step.Seconds)
				{
					return true;
				}
				const bool bReady = !Step.Ready || Step.Ready(World);
				if (!bReady && Clock < Step.Seconds + ReadyWaitSeconds)
				{
					return true;
				}
				if (!bReady)
				{
					UE_LOG(LogLooter, Warning, TEXT("Looter.HudShots: %02d %s: its moment didn't come in %.0f s; photographed anyway."),
						StepIndex + 1, Step.Name, ReadyWaitSeconds);
				}
				// The screenshot is taken at the end of this frame, with the HUD ticked by it.
				const FString File = HudShotDirectory() / FString::Printf(TEXT("%02d_%s.png"), StepIndex + 1, Step.Name);
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
				UE_LOG(LogLooter, Display, TEXT("Looter.HudShots: done, %d pictures in %s."), HudShotScene::Steps().Num(), *HudShotDirectory());
			}
			if (bQuit)
			{
				FPlatformMisc::RequestExit(false, TEXT("Looter.HudShots"));
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

	TSharedPtr<FHudShotRun> ActiveHudShotRun;

	/** Looter.HudShots [quit] */
	void HudShotsCommand(const TArray<FString>& Args, UWorld* /*World*/)
	{
		if (ActiveHudShotRun.IsValid())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.HudShots: already running."));
			return;
		}
		const bool bQuit = Args.ContainsByPredicate([](const FString& Arg) { return Arg.Equals(TEXT("quit"), ESearchCase::IgnoreCase); });
		IFileManager::Get().MakeDirectory(*HudShotDirectory(), /*Tree*/ true);

		ActiveHudShotRun = MakeShared<FHudShotRun>(bQuit);
		const TSharedPtr<FHudShotRun> Run = ActiveHudShotRun;
		FTSTicker::GetCoreTicker().AddTicker(TEXT("Looter.HudShots"), 0.f, [Run](float DeltaTime)
			{
				const bool bGoOn = Run->Tick(DeltaTime);
				if (!bGoOn)
				{
					ActiveHudShotRun.Reset();
				}
				return bGoOn;
			});
	}

	FAutoConsoleCommandWithWorldAndArgs HudShotsCommandRegistration(
		TEXT("Looter.HudShots"),
		TEXT("Looter.HudShots [quit]: photographs the gameplay HUD through its states (calm, hit, low health, heal, experience, level-up, ")
		TEXT("reload, empty magazine, two guns, boss bar, objective done) into Saved/Screenshots/HudShots/<NN>_<state>.png, each with the UI; ")
		TEXT("with quit, the game quits after the last one. Tools/hudshots.ps1 runs it standalone at 1920x1080."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HudShotsCommand));
}

#endif
