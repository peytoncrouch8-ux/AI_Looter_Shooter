// Developer console command that photographs the menus through their states (not in shipping builds).
//
// Looter.UIShots [quit] looks at the world it runs in and photographs its half, each picture WITH the UI, into
// Saved/Screenshots/UIShots/<NN>_<state>.png: on the menu map the main menu (01-04: arrived, Settings lit, the settings
// opened from it, the session picker); on a level the pause menu and the level-up popup (05-11: Graphics, a row focused,
// Audio, Interface, Controls, the popup counting, then holding). Tools/uishots.ps1 runs the game twice standalone at
// 1920x1080, once for each half. UIShotScene.cpp makes each state; this file runs the steps on a clock and takes the
// pictures, as Looter.MenuShots does for the inventory.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Dev/UIShotScene.h"
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
	using UIShotScene::EHalf;

	// Real seconds: the run's own clock (the core ticker's), which goes on while the pause menu stops the game.

	/** The most it waits for the main menu, or the player and the HUD, to exist (a level loading, shaders compiling). */
	constexpr float UIShotWaitSeconds = 90.f;
	/** A step whose moment (Ready) hasn't come this long after its time is photographed anyway, with a warning. */
	constexpr float UIShotReadyWaitSeconds = 3.f;

	FString UIShotDirectory()
	{
		return FPaths::ProjectSavedDir() / TEXT("Screenshots/UIShots");
	}

	/** The run in progress: a state machine ticked every frame by the core ticker (no UObject needed). */
	class FUIShotRun
	{
	public:
		explicit FUIShotRun(bool bInQuit) : bQuit(bInQuit) {}

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
				UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: the world went away; stopped."));
				return Finish(false);
			}
			if (Phase == EPhase::Warmup)
			{
				return Clock >= UIShotScene::WarmupSeconds(Half) ? BeginStep(*World, 0) : true;
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

		/** The file of step Index of this half. */
		FString StepFile(int32 Index) const
		{
			return UIShotDirectory() / FString::Printf(TEXT("%02d_%s.png"), UIShotScene::FirstNumber(Half) + Index,
				UIShotScene::Steps(Half)[Index].Name);
		}

		bool TickWaiting()
		{
			// The map may still be loading when the command runs (-ExecCmds): look again each frame.
			UWorld* World = UIShotScene::FindGameWorld();
			const EHalf Found = World ? UIShotScene::FindHalf(*World) : EHalf::None;
			if (World && Found != EHalf::None)
			{
				Half = Found;
				const int32 First = UIShotScene::FirstNumber(Half);
				UE_LOG(LogLooter, Display, TEXT("Looter.UIShots: %s, the %s half, %d states (%02d-%02d), pictures in %s."), *World->GetMapName(),
					UIShotScene::HalfName(Half), UIShotScene::Steps(Half).Num(), First, First + UIShotScene::Steps(Half).Num() - 1,
					*UIShotDirectory());
				WorldPtr = World;
				UIShotScene::Prepare(*World, Half);
				Phase = EPhase::Warmup;
				Clock = 0.f;
				return true;
			}
			if (Clock > UIShotWaitSeconds)
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: no main menu and no player with a HUD after %.0f s; stopped."), UIShotWaitSeconds);
				return Finish(false);
			}
			return true;
		}

		bool BeginStep(UWorld& World, int32 Index)
		{
			const TConstArrayView<UIShotScene::FStep> Steps = UIShotScene::Steps(Half);
			if (!Steps.IsValidIndex(Index))
			{
				return Finish(true);
			}
			StepIndex = Index;
			Phase = EPhase::Step;
			Clock = 0.f;
			ShotAt = 0.f;
			bShotRequested = false;
			UE_LOG(LogLooter, Display, TEXT("Looter.UIShots: %02d %s"), UIShotScene::FirstNumber(Half) + Index, Steps[Index].Name);
			if (Steps[Index].Begin)
			{
				Steps[Index].Begin(World);
			}
			return true;
		}

		bool TickStep(UWorld& World)
		{
			const UIShotScene::FStep& Step = UIShotScene::Steps(Half)[StepIndex];
			if (!bShotRequested)
			{
				if (Clock < Step.Seconds)
				{
					return true;
				}
				const bool bReady = !Step.Ready || Step.Ready(World);
				if (!bReady && Clock < Step.Seconds + UIShotReadyWaitSeconds)
				{
					return true;
				}
				if (!bReady)
				{
					UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: %02d %s: its moment didn't come in %.0f s; photographed anyway."),
						UIShotScene::FirstNumber(Half) + StepIndex, Step.Name, UIShotReadyWaitSeconds);
				}
				// The screenshot is taken at the end of this frame, with the UI.
				FScreenshotRequest::RequestScreenshot(StepFile(StepIndex), /*bInShowUI*/ true, /*bAddFilenameSuffix*/ false);
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
				UE_LOG(LogLooter, Display, TEXT("Looter.UIShots: done, the %s half, %d pictures in %s."), UIShotScene::HalfName(Half),
					UIShotScene::Steps(Half).Num(), *UIShotDirectory());
			}
			if (bQuit)
			{
				FPlatformMisc::RequestExit(false, TEXT("Looter.UIShots"));
			}
			return false;
		}

		TWeakObjectPtr<UWorld> WorldPtr;
		EPhase Phase = EPhase::Waiting;
		EHalf Half = EHalf::None;
		int32 StepIndex = 0;
		/** Seconds in the phase (in a step: since its Begin), and when in the step the picture was requested. */
		float Clock = 0.f;
		float ShotAt = 0.f;
		bool bShotRequested = false;
		bool bQuit = false;
	};

	TSharedPtr<FUIShotRun> ActiveUIShotRun;

	/** Looter.UIShots [quit] */
	void UIShotsCommand(const TArray<FString>& Args, UWorld* /*World*/)
	{
		if (ActiveUIShotRun.IsValid())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.UIShots: already running."));
			return;
		}
		const bool bQuit = Args.ContainsByPredicate([](const FString& Arg) { return Arg.Equals(TEXT("quit"), ESearchCase::IgnoreCase); });
		IFileManager::Get().MakeDirectory(*UIShotDirectory(), /*Tree*/ true);

		ActiveUIShotRun = MakeShared<FUIShotRun>(bQuit);
		const TSharedPtr<FUIShotRun> Run = ActiveUIShotRun;
		FTSTicker::GetCoreTicker().AddTicker(TEXT("Looter.UIShots"), 0.f, [Run](float DeltaTime)
			{
				const bool bGoOn = Run->Tick(DeltaTime);
				if (!bGoOn)
				{
					ActiveUIShotRun.Reset();
				}
				return bGoOn;
			});
	}

	FAutoConsoleCommandWithWorldAndArgs UIShotsCommandRegistration(
		TEXT("Looter.UIShots"),
		TEXT("Looter.UIShots [quit]: photographs the menus through their states, the half the world is for: on the main menu's map ")
		TEXT("01-04 (the menu arrived, Settings lit, the settings opened from it, the session picker); on a level 05-11 (the pause ")
		TEXT("menu's Graphics, a row focused, Audio, Interface, Controls, then the level-up popup counting and holding), into ")
		TEXT("Saved/Screenshots/UIShots/<NN>_<state>.png, each with the UI; with quit, the game quits after the last one. ")
		TEXT("Tools/uishots.ps1 runs both halves standalone at 1920x1080."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&UIShotsCommand));
}

#endif
