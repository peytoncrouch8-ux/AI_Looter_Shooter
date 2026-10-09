#include "Tutorial/TutorialDirector.h"
#include "AI_Looter_Shooter.h"
#include "Core/LooterMenuGameMode.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionText.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** Checking a few times a second is plenty for a tutorial (the mission runner looks as often). */
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

	// The full sentences are the objectives' words (the Missions page); the tracker shows the short line, with the key in
	// its hint. The mission's name is the tracker's title, so the first line doesn't repeat it.
	Steps = {
		{ TEXT("Welcome to Skyreach. Move with {Move} and look around with the mouse."), ETutorialGoal::Move, 600.f,
			TEXT("Move and look around"), TEXT("Move"), TEXT("Move") },
		{ TEXT("Hold {Sprint} to run. Follow the road into town."), ETutorialGoal::ReachRack, 900.f,
			TEXT("Follow the road into town"), TEXT("Sprint"), TEXT("Hold to run") },
		{ TEXT("Grab the rifle on the gun rack: look at it and press {Interact}."), ETutorialGoal::HoldWeapon, 1.f,
			TEXT("Grab the rifle from the gun rack"), TEXT("Interact"), TEXT("Take it") },
		{ TEXT("Shoot the target dummies in the meadow under the windmill. {Reload} reloads."), ETutorialGoal::HitDummies, 5.f,
			TEXT("Shoot the target dummies"), TEXT("Reload"), TEXT("Reload") },
		{ TEXT("Spiders nest in the woods past the pond. Hunt down two of them."), ETutorialGoal::KillCreatures, 2.f,
			TEXT("Hunt spiders past the pond") },
		{ TEXT("Press {Inventory} to see your loadout and your weapons' stats."), ETutorialGoal::OpenInventory, 1.f,
			TEXT("Check your loadout"), TEXT("Inventory"), TEXT("Inventory") },
	};
	DoneText = TEXT("You're ready. Explore the island, and climb to the lookout on the plateau for the view.");
	MissionTitle = TEXT("Welcome to Skyreach");
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
	const UPlayerProgressionSubsystem* Progression = GetProgression(GetWorld());
	if (Progression && Progression->IsTutorialDone())
	{
		// Done in an earlier game: the campaign knows its mission is finished too (a session from before missions were
		// data), so missions that follow it can start.
		if (Runner && GetMission())
		{
			Runner->RecordCompleted(MissionId);
		}
		SetActorTickEnabled(false);
		return;
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
	const UMissionRunner* Runner = GetRunner();
	const UMissionDefinition* Mission = GetMission();
	if (!Runner || !Mission || Current == INDEX_NONE)
	{
		return;
	}
	// The runner moves the mission on, several steps at once when they're done already (the player carries a gun); the
	// director keeps its step for the save. The HUD's mission tracker shows it from the runner's display by itself.
	const int32 RunnerStep = Runner->GetStep(MissionId);
	if (RunnerStep != INDEX_NONE && RunnerStep != Current)
	{
		Current = RunnerStep;
		UE_LOG(LogLooter, Log, TEXT("Tutorial step %d/%d"), Current + 1, Mission->Steps.Num());
	}
}

void ATutorialDirector::Restart()
{
	if (UPlayerProgressionSubsystem* Progression = GetProgression(GetWorld()))
	{
		Progression->SetTutorialDone(false);
	}
	// A closing line still up from the last run goes, so the tracker shows the first step at once.
	if (ALooterHUD* HUD = GetHUD())
	{
		HUD->ClearMissionClosingLine();
	}
	SetActorTickEnabled(true);
	StartStep(0);
	// The tracker shows the tracked mission: started again beside another one (the skiff), the tutorial takes it back.
	UMissionRunner* Runner = GetRunner();
	if (Runner && Runner->IsRunning(MissionId))
	{
		Runner->TrackMission(MissionId);
	}
}

void ATutorialDirector::Skip()
{
	// Skipped: no closing line; the tracker ticks the mission off as it ends and moves on to what's tracked next.
	if (ALooterHUD* HUD = GetHUD())
	{
		HUD->ClearMissionClosingLine();
	}
	Finish(/*bShowDone*/ false);
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
		UE_LOG(LogLooter, Warning, TEXT("Tutorial: this level plays no missions, so the tutorial can't run."));
		SetActorTickEnabled(false);
		return;
	}
	if (!Mission->Steps.IsValidIndex(Index))
	{
		Finish(/*bShowDone*/ true);
		return;
	}
	Current = Index;
	UE_LOG(LogLooter, Log, TEXT("Tutorial step %d/%d"), Index + 1, Mission->Steps.Num());
	// The runner checks the step's objectives from here on, and passes at once the ones already done (it may even finish
	// the tutorial right here, which ends it through HandleMissionFinished).
	if (Runner->IsRunning(MissionId))
	{
		Runner->SetStep(MissionId, Index);
	}
	else
	{
		Runner->StartMission(MissionId, Index, /*bForce*/ true);
	}
}

void ATutorialDirector::Finish(bool bShowDone)
{
	Current = INDEX_NONE;
	SetActorTickEnabled(false);
	// Skipped: the mission ends here too, finished in the campaign. Played to the end, it has ended already.
	if (UMissionRunner* Runner = GetRunner())
	{
		if (Runner->IsRunning(MissionId))
		{
			Runner->CompleteMission(MissionId);
		}
	}
	if (UPlayerProgressionSubsystem* Progression = GetProgression(GetWorld()))
	{
		Progression->SetTutorialDone(true);
	}
	// The closing line is the tracker's last, ticked line under the tutorial's name, with every step done.
	const UMissionDefinition* Mission = bShowDone ? GetMission() : nullptr;
	ALooterHUD* HUD = bShowDone ? GetHUD() : nullptr;
	if (Mission && HUD)
	{
		HUD->ShowMissionClosingLine(Mission->Title, Mission->Steps.Num(), ResolveKeys(DoneText), DoneSeconds);
	}
	UE_LOG(LogLooter, Log, TEXT("Tutorial %s"), bShowDone ? TEXT("complete") : TEXT("skipped"));
}

void ATutorialDirector::HandleMissionFinished(const UMissionDefinition& Mission, bool bRewarded)
{
	// The runner finished the tutorial's mission: its last objective was done (or the console finished it).
	if (Current != INDEX_NONE && Mission.GetMissionId() == MissionId)
	{
		Finish(/*bShowDone*/ true);
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
		TEXT("Restarts or skips the level's tutorial: Looter.Tutorial restart|skip"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TutorialCommand));
}
#endif
