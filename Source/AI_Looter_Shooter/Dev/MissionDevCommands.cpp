// Developer console commands for missions (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
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

	/** The level's mission runner, when it plays missions (not the main menu); logged when there's none. */
	UMissionRunner* FindRunner(UWorld* World, const TCHAR* Command)
	{
		UWorld* GameWorld = FindGameWorld(World);
		UMissionRunner* Runner = GameWorld ? GameWorld->GetSubsystem<UMissionRunner>() : nullptr;
		if (!Runner || !Runner->IsActive())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no missions here (start the game first; the main menu plays none)."), Command);
			return nullptr;
		}
		return Runner;
	}

	/** The mission the words name: its id, its asset's name or its title. Logs the ids when nothing matches. */
	UMissionDefinition* FindMission(const UMissionRunner& Runner, const FString& Words, const TCHAR* Command)
	{
		for (UMissionDefinition* Mission : Runner.GetDefinitions())
		{
			if (Mission && Mission->IsNamed(Words))
			{
				return Mission;
			}
		}
		TArray<FString> Ids;
		for (const UMissionDefinition* Mission : Runner.GetDefinitions())
		{
			if (Mission)
			{
				Ids.Add(Mission->GetMissionId().ToString());
			}
		}
		UE_LOG(LogLooter, Warning, TEXT("%s: no mission is called '%s'. Missions: %s"), Command, *Words,
			Ids.IsEmpty() ? TEXT("none") : *FString::Join(Ids, TEXT(", ")));
		return nullptr;
	}

	FString StatusWord(EMissionStatus Status)
	{
		switch (Status)
		{
		case EMissionStatus::Active:    return TEXT("active");
		case EMissionStatus::Available: return TEXT("available");
		case EMissionStatus::Completed: return TEXT("completed");
		case EMissionStatus::Locked:    break;
		}
		return TEXT("locked");
	}

	/** Looter.Mission.Start <id> [step] */
	void StartMission(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Mission.Start");
		UMissionRunner* Runner = FindRunner(World, Command);
		if (!Runner)
		{
			return;
		}
		if (Args.IsEmpty() || (Args.Num() > 1 && !Args[1].IsNumeric()))
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: %s <mission id> [step, from 1]"), Command);
			return;
		}
		const UMissionDefinition* Mission = FindMission(*Runner, Args[0], Command);
		if (!Mission)
		{
			return;
		}
		// Testing: it starts whatever stands in its way, and says what that was.
		const FName MissionId = Mission->GetMissionId();
		if (!Runner->ArePrerequisitesMet(*Mission))
		{
			UE_LOG(LogLooter, Log, TEXT("%s: %s's prerequisites (%s) aren't finished; starting it anyway."), Command, *MissionId.ToString(),
				*FString::JoinBy(Mission->Prerequisites, TEXT(", "), [](const FName& Needed) { return Needed.ToString(); }));
		}
		if (!Mission->Area.IsNone() && Mission->Area != Runner->GetAreaHere())
		{
			UE_LOG(LogLooter, Log, TEXT("%s: %s belongs to %s, not this level; starting it here anyway."), Command, *MissionId.ToString(),
				*Mission->Area.ToString());
		}
		const int32 Step = Args.Num() > 1 ? FCString::Atoi(*Args[1]) - 1 : 0;
		if (Runner->StartMission(MissionId, Step, /*bForce*/ true))
		{
			UE_LOG(LogLooter, Log, TEXT("%s: %s is on step %d of %d."), Command, *MissionId.ToString(), Runner->GetStep(MissionId) + 1,
				Mission->Steps.Num());
		}
		else if (Runner->IsCompleted(MissionId))
		{
			UE_LOG(LogLooter, Log, TEXT("%s: %s finished at once (its steps were done already)."), Command, *MissionId.ToString());
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: %s couldn't start (it has no steps)."), Command, *MissionId.ToString());
		}
	}

	/**
	 * Looter.Mission.Complete <id> [all]: a running mission's current step (the last one finishes it), so a mission can
	 * be walked through step by step; "all" finishes the whole mission. A mission not running is recorded as finished.
	 * Rewards come with a finish, once per session.
	 */
	void CompleteMission(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Mission.Complete");
		UMissionRunner* Runner = FindRunner(World, Command);
		if (!Runner)
		{
			return;
		}
		if (Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: %s <mission id> [all]: finishes its current step, or with 'all' the whole mission"), Command);
			return;
		}
		const UMissionDefinition* Mission = FindMission(*Runner, Args[0], Command);
		if (!Mission)
		{
			return;
		}
		const FName MissionId = Mission->GetMissionId();
		const bool bAll = Args.Num() > 1 && Args[1].Equals(TEXT("all"), ESearchCase::IgnoreCase);
		if (Runner->IsRunning(MissionId) && !bAll)
		{
			const int32 Step = Runner->GetStep(MissionId);
			Runner->CompleteStep(MissionId);
			UE_LOG(LogLooter, Log, TEXT("%s: %s's step %d of %d done; %s"), Command, *MissionId.ToString(), Step + 1, Mission->Steps.Num(),
				Runner->IsRunning(MissionId) ? *FString::Printf(TEXT("now on step %d."), Runner->GetStep(MissionId) + 1) : TEXT("the mission is finished."));
			return;
		}
		Runner->CompleteMission(MissionId);
		UE_LOG(LogLooter, Log, TEXT("%s: %s finished."), Command, *MissionId.ToString());
	}

	/** Looter.Mission.List: every mission, where it stands, and the running ones' objectives. */
	void ListMissions(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Mission.List");
		UMissionRunner* Runner = FindRunner(World, Command);
		if (!Runner)
		{
			return;
		}
		const FName Tracked = Runner->GetTrackedMission();
		UE_LOG(LogLooter, Log, TEXT("%s: %d missions, here: %s"), Command, Runner->GetDefinitions().Num(),
			Runner->GetAreaHere().IsNone() ? TEXT("no area") : *Runner->GetAreaHere().ToString());
		for (const UMissionDefinition* Mission : Runner->GetDefinitions())
		{
			if (!Mission)
			{
				continue;
			}
			const FName MissionId = Mission->GetMissionId();
			const bool bRunning = Runner->IsRunning(MissionId);
			const FString Where = bRunning ? FString::Printf(TEXT(" step %d/%d"), Runner->GetStep(MissionId) + 1, Mission->Steps.Num()) : FString();
			UE_LOG(LogLooter, Log, TEXT("  %-12s %s \"%s\" (%s, %s)%s%s"), *StatusWord(Runner->GetStatus(*Mission)), *MissionId.ToString(),
				*Mission->Title.ToString(), *UMissionDefinition::KindName(Mission->Kind).ToString(),
				Mission->Area.IsNone() ? TEXT("anywhere") : *Mission->Area.ToString(), *Where, Tracked == MissionId ? TEXT("  <- tracked") : TEXT(""));
			for (const FMissionObjectiveView& View : Runner->GetObjectiveViews(MissionId))
			{
				UE_LOG(LogLooter, Log, TEXT("      [%s] %s%s"), View.bDone ? TEXT("x") : TEXT(" "), *View.Text,
					View.Progress.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" (%s)"), *View.Progress));
			}
		}
		const FCampaignRecord& Campaign = Runner->GetCampaign();
		UE_LOG(LogLooter, Log, TEXT("  Campaign: finished [%s]; main mission %s; areas open [%s]"),
			*FString::JoinBy(Campaign.CompletedMissions, TEXT(", "), [](const FName& Id) { return Id.ToString(); }),
			Campaign.ActiveMission.IsNone() ? TEXT("none") : *FString::Printf(TEXT("%s at step %d"), *Campaign.ActiveMission.ToString(), Campaign.ActiveMissionStep + 1),
			*FString::JoinBy(Campaign.OpenedAreas, TEXT(", "), [](const FName& Id) { return Id.ToString(); }));
	}

	/**
	 * Looter.Mission.Event <name> [tag] [hold]: sends an event, as the systems that come later will (Interact, Talk,
	 * Collect, Scene.<Name>, Board.<Vehicle>, or any name). With a tag it's about the nearest actor carrying it, or the tag
	 * alone when none does.
	 */
	void SendEvent(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Mission.Event");
		UMissionRunner* Runner = FindRunner(World, Command);
		if (!Runner)
		{
			return;
		}
		if (Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: %s <Interact|Talk|Collect|Scene.Name|Board.Vehicle|any name> [actor tag] [hold]"), Command);
			return;
		}
		FMissionEvent Event = FMissionEvent::Named(FName(*Args[0]));
		if (Args.Num() > 1 && !Args[1].Equals(TEXT("hold"), ESearchCase::IgnoreCase))
		{
			Event.Tag = FName(*Args[1]);
			FMissionActorFilter Tagged;
			Tagged.ActorTag = Event.Tag;
			UWorld* GameWorld = FindGameWorld(World);
			const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
			const TOptional<FVector> From = Pawn ? TOptional<FVector>(Pawn->GetActorLocation()) : TOptional<FVector>();
			Event.Actor = MissionTargets::FindNearest(GameWorld, Tagged, From, /*bLivingOnly*/ false);
		}
		Event.bHeld = Args.ContainsByPredicate([](const FString& Arg) { return Arg.Equals(TEXT("hold"), ESearchCase::IgnoreCase); });
		Runner->NotifyEvent(Event);
		UE_LOG(LogLooter, Log, TEXT("%s: %s sent%s%s."), Command, *Event.Name.ToString(),
			Event.Actor.IsValid() ? *FString::Printf(TEXT(" about %s"), *Event.Actor->GetName()) : Event.Tag.IsNone() ? TEXT("") : *FString::Printf(TEXT(" about the tag %s"), *Event.Tag.ToString()),
			Event.bHeld ? TEXT(" (held)") : TEXT(""));
	}

	FAutoConsoleCommandWithWorldAndArgs StartMissionCommand(
		TEXT("Looter.Mission.Start"),
		TEXT("Looter.Mission.Start <mission id> [step]: starts a mission here, at its first step or the one given (from 1), whatever its ")
		TEXT("prerequisites and area; a running one starts that step again."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StartMission));

	FAutoConsoleCommandWithWorldAndArgs CompleteMissionCommand(
		TEXT("Looter.Mission.Complete"),
		TEXT("Looter.Mission.Complete <mission id> [all]: finishes a running mission's current step (the last one finishes the mission), ")
		TEXT("or with 'all' the whole mission; one not running is recorded as finished. Rewards come once per session."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CompleteMission));

	FAutoConsoleCommandWithWorldAndArgs ListMissionsCommand(
		TEXT("Looter.Mission.List"),
		TEXT("Looter.Mission.List: every mission, where it stands (active, available, completed, locked), the running ones' ")
		TEXT("objectives, and the campaign record."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListMissions));

	FAutoConsoleCommandWithWorldAndArgs SendEventCommand(
		TEXT("Looter.Mission.Event"),
		TEXT("Looter.Mission.Event <name> [actor tag] [hold]: sends a mission event (Interact, Talk, Collect, Scene.<Name>, ")
		TEXT("Board.<Vehicle>, or any name), about the nearest actor with the tag, or the tag alone."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SendEvent));
}

#endif
