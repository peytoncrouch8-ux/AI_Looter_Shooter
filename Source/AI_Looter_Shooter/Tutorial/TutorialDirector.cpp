#include "Tutorial/TutorialDirector.h"
#include "AI_Looter_Shooter.h"
#include "Core/LooterMenuGameMode.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionText.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "World/NoticeBoard.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** Checking a few times a second is plenty for the first goal's step (the mission runner looks as often). */
	constexpr float CheckInterval = 0.2f;

	UPlayerProgressionSubsystem* GetProgression(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
		return Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	}
}

ATutorialDirector::ATutorialDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = CheckInterval;

	// Two steps, no keys on the tracker: the contextual hints teach those as they're needed. The full sentences are the
	// Missions page's; the short lines the tracker's, under the mission's name.
	Steps = {
		{ TEXT("Find a gun in town. The gun rack in the square has a rifle on it."), ETutorialGoal::HoldWeapon, 1.f,
			TEXT("Find a gun in town") },
		{ TEXT("Read the notice board in the town square. The town posts its odd jobs there."), ETutorialGoal::ReadBoard, 1.f,
			TEXT("Read the notice board") },
	};
	MissionTitle = TEXT("Welcome to Skyreach");
}

ETutorialStart ATutorialDirector::DecideStart(bool bTutorialDone, bool bFirstGoalDone, bool bMainPostingDone)
{
	// Web Hollow turned in counts even when the flag didn't make it into the save (it's set a moment after).
	if (bTutorialDone || bMainPostingDone)
	{
		return ETutorialStart::Done;
	}
	return bFirstGoalDone ? ETutorialStart::Postings : ETutorialStart::Teach;
}

void ATutorialDirector::BeginPlay()
{
	Super::BeginPlay();
	// Behind the main menu there's no one to teach.
	if (ALooterMenuGameMode::IsMenuWorld(GetWorld()))
	{
		SetActorTickEnabled(false);
		return;
	}
	UMissionRunner* Runner = GetRunner();
	if (Runner)
	{
		FinishedHandle = Runner->OnMissionFinished.AddUObject(this, &ATutorialDirector::HandleMissionFinished);
	}
	UPlayerProgressionSubsystem* Progression = GetProgression(GetWorld());
	const bool bDone = Progression && Progression->IsTutorialDone();
	const bool bFirstGoalDone = Runner && Runner->IsCompleted(MissionId);
	const bool bMainDone = Runner && Runner->IsCompleted(MainPostingId);
	switch (DecideStart(bDone, bFirstGoalDone, bMainDone))
	{
	case ETutorialStart::Done:
		if (!bDone)
		{
			MarkTutorialDone();
		}
		// The campaign knows the first goal is behind them too (a skipped island, or a session from before the board), so
		// the board's postings, which wait on it, go up for practice.
		if (Runner && GetMission())
		{
			Runner->RecordCompleted(MissionId);
		}
		SetActorTickEnabled(false);
		return;
	case ETutorialStart::Postings:
		SetActorTickEnabled(false);
		return;
	case ETutorialStart::Teach:
		break;
	}
	StartStep(0);
}

void ATutorialDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = GetRunner())
	{
		Runner->OnMissionFinished.Remove(FinishedHandle);
	}
	FinishedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void ATutorialDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UMissionRunner* Runner = GetRunner();
	const UMissionDefinition* Mission = GetMission();
	if (!Runner || !Mission || Current == INDEX_NONE)
	{
		return;
	}
	// The runner moves the first goal on (both steps at once for a player who already carries a gun); the director keeps
	// its step for the save.
	const int32 RunnerStep = Runner->GetStep(MissionId);
	if (RunnerStep != INDEX_NONE && RunnerStep != Current)
	{
		Current = RunnerStep;
		UE_LOG(LogLooter, Log, TEXT("Tutorial: step %d/%d"), Current + 1, Mission->Steps.Num());
	}
	// A level built before the notice board came has none to read: the first goal doesn't wait for it, so the postings
	// (and with them Web Hollow and the skiff) can't be stranded. Looked at once per step.
	if (Current != BoardCheckedStep)
	{
		BoardCheckedStep = Current;
		const UMissionEventObjective* Read = Cast<UMissionEventObjective>(Mission->GetObjective(Current, 0));
		if (Read && Read->Event == ANoticeBoard::ReadEvent && !TActorIterator<ANoticeBoard>(GetWorld()))
		{
			UE_LOG(LogLooter, Warning, TEXT("Tutorial: this level has no notice board to read (Tools/Unreal/build_area_board.py places it), ")
				TEXT("so the first goal passes it and the postings go up."));
			Runner->CompleteStep(MissionId);
		}
	}
}

void ATutorialDirector::Restart()
{
	if (UPlayerProgressionSubsystem* Progression = GetProgression(GetWorld()))
	{
		Progression->SetTutorialDone(false);
	}
	// A closing line still up from another mission goes, so the tracker shows the first step at once.
	if (ALooterHUD* HUD = GetHUD())
	{
		HUD->ClearMissionClosingLine();
	}
	SetActorTickEnabled(true);
	StartStep(0);
	// The tracker shows the tracked mission: started again beside the postings, the first goal takes it back.
	UMissionRunner* Runner = GetRunner();
	if (Runner && Runner->IsRunning(MissionId))
	{
		Runner->TrackMission(MissionId);
	}
}

void ATutorialDirector::Skip()
{
	FinishFirstGoal(/*bComplete*/ true);
	MarkTutorialDone();
	UE_LOG(LogLooter, Log, TEXT("Tutorial skipped: the skiff is offered."));
}

void ATutorialDirector::ResumeAtStep(int32 Index)
{
	const UMissionDefinition* Mission = GetMission();
	if (Current != INDEX_NONE && Mission && Mission->Steps.IsValidIndex(Index))
	{
		StartStep(Index);
	}
}

void ATutorialDirector::StartStep(int32 Index)
{
	UMissionRunner* Runner = GetRunner();
	const UMissionDefinition* Mission = GetMission();
	if (!Runner || !Runner->IsActive() || !Mission)
	{
		UE_LOG(LogLooter, Warning, TEXT("Tutorial: this level plays no missions, so the first goal can't run."));
		SetActorTickEnabled(false);
		return;
	}
	if (!Mission->Steps.IsValidIndex(Index))
	{
		FinishFirstGoal(/*bComplete*/ true);
		return;
	}
	Current = Index;
	BoardCheckedStep = INDEX_NONE;
	UE_LOG(LogLooter, Log, TEXT("Tutorial: step %d/%d"), Index + 1, Mission->Steps.Num());
	// The runner checks the step's objectives from here on, and passes at once the ones already done (a gun carried).
	if (Runner->IsRunning(MissionId))
	{
		Runner->SetStep(MissionId, Index);
	}
	else
	{
		Runner->StartMission(MissionId, Index, /*bForce*/ true);
	}
}

void ATutorialDirector::FinishFirstGoal(bool bComplete)
{
	Current = INDEX_NONE;
	SetActorTickEnabled(false);
	// Skipped: the mission ends here too, finished in the campaign, and the postings that wait on it go up. Played to the
	// end, it has ended already.
	UMissionRunner* Runner = GetRunner();
	if (bComplete && Runner && Runner->IsRunning(MissionId))
	{
		Runner->CompleteMission(MissionId);
	}
}

void ATutorialDirector::MarkTutorialDone()
{
	if (UPlayerProgressionSubsystem* Progression = GetProgression(GetWorld()))
	{
		Progression->SetTutorialDone(true);
	}
}

void ATutorialDirector::HandleMissionFinished(const UMissionDefinition& Mission, bool bRewarded)
{
	const FName Finished = Mission.GetMissionId();
	if (Finished == MissionId && Current != INDEX_NONE)
	{
		// The board was read with a gun in hand: the postings are up (the board tracks Web Hollow).
		FinishFirstGoal(/*bComplete*/ false);
		UE_LOG(LogLooter, Log, TEXT("Tutorial: the first goal is done, and the notice board's postings are up."));
	}
	else if (Finished == MainPostingId)
	{
		// Web Hollow turned in at the board: the tutorial is done, and the skiff's jetty offers the way off.
		MarkTutorialDone();
		UE_LOG(LogLooter, Log, TEXT("Tutorial: %s turned in, so the tutorial is done and the skiff is offered."), *Finished.ToString());
	}
}

UMissionRunner* ATutorialDirector::GetRunner() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UMissionRunner>() : nullptr;
}

ALooterHUD* ATutorialDirector::GetHUD() const
{
	const UWorld* World = GetWorld();
	return World ? ALooterHUD::FindFor(World->GetFirstPlayerController()) : nullptr;
}

const UMissionDefinition* ATutorialDirector::GetMission()
{
	UMissionRunner* Runner = GetRunner();
	if (!Runner)
	{
		return nullptr;
	}
	if (const UMissionDefinition* Mission = Runner->FindDefinition(MissionId))
	{
		return Mission;
	}
	// No DA_Mission_Tutorial: the built-in steps become the mission it would hold.
	UMissionDefinition* BuiltIn = MakeBuiltInMission(Runner);
	Runner->RegisterDefinition(BuiltIn);
	UE_LOG(LogLooter, Log, TEXT("Tutorial: no mission asset has the id %s, so its built-in steps are the mission."), *MissionId.ToString());
	return Runner->FindDefinition(MissionId);
}

FString ATutorialDirector::ResolveKeys(const FString& Text) const
{
	return MissionText::ResolveKeys(GetWorld(), Text);
}

#if !UE_BUILD_SHIPPING
namespace
{
	/** Looter.Tutorial restart|skip */
	void TutorialCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = World && World->IsGameWorld() ? World : nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			GameWorld = GameWorld ? GameWorld : (Context.World() && Context.World()->IsGameWorld() ? Context.World() : nullptr);
		}
		ATutorialDirector* Director = nullptr;
		for (TActorIterator<ATutorialDirector> It(GameWorld); GameWorld && It; ++It)
		{
			Director = *It;
		}
		if (!Director || Args.Num() != 1)
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game with a tutorial): Looter.Tutorial restart|skip"));
			return;
		}
		if (Args[0].Equals(TEXT("restart"), ESearchCase::IgnoreCase))
		{
			Director->Restart();
		}
		else if (Args[0].Equals(TEXT("skip"), ESearchCase::IgnoreCase))
		{
			Director->Skip();
		}
	}

	FAutoConsoleCommandWithWorldAndArgs TutorialCommandRegistration(
		TEXT("Looter.Tutorial"),
		TEXT("Restarts the tutorial's first goal, or skips it and counts the tutorial done: Looter.Tutorial restart|skip"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TutorialCommand));
}
#endif
