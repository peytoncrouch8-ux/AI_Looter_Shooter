// Developer console commands for missions offered by their givers (not in shipping builds): the offer card on any mission,
// and what is offered where.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "UI/HUD/LooterHUD.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindPlayedWorld(UWorld* World)
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

	UMissionRunner* FindActiveRunner(UWorld* World, const TCHAR* Command)
	{
		UWorld* Played = FindPlayedWorld(World);
		UMissionRunner* Runner = Played ? Played->GetSubsystem<UMissionRunner>() : nullptr;
		if (!Runner || !Runner->IsActive())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no missions here (start the game first; the main menu plays none)."), Command);
			return nullptr;
		}
		return Runner;
	}

	/**
	 * Looter.Mission.Offer <id>: the offer card for a mission as its giver would open it (any mission: one not offered yet,
	 * or not picked up from a giver at all, shows how its card would read). Its giver is the nearest actor with its offer's
	 * tag, when there is one. Accept takes it as the card always does: only when it can be taken.
	 */
	void OfferMission(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Mission.Offer");
		UMissionRunner* Runner = FindActiveRunner(World, Command);
		if (!Runner)
		{
			return;
		}
		if (Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: %s <mission id>"), Command);
			return;
		}
		const UMissionDefinition* Mission = nullptr;
		for (const UMissionDefinition* Each : Runner->GetDefinitions())
		{
			if (Each && Each->IsNamed(Args[0]))
			{
				Mission = Each;
				break;
			}
		}
		if (!Mission)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no mission is called '%s' (Looter.Mission.List names them)."), Command, *Args[0]);
			return;
		}
		UWorld* Played = FindPlayedWorld(World);
		APlayerController* Controller = Played ? Played->GetFirstPlayerController() : nullptr;
		ALooterHUD* HUD = ALooterHUD::FindFor(Controller);
		AActor* Giver = Runner->FindOfferGiverActor(*Mission);
		if (!HUD || !HUD->OpenMissionOffer(Mission, Giver))
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: the card can't open now (close the menu that's up; a finished mission isn't offered)."), Command);
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("%s: the card for %s is open%s; %s."), Command, *Mission->GetMissionId().ToString(),
			Giver ? *FString::Printf(TEXT(" (from %s)"), *Giver->GetActorNameOrLabel()) : TEXT(""),
			Runner->IsAvailable(Mission->GetMissionId()) ? TEXT("it's offered here now") : TEXT("it isn't offered here now, so Accept may refuse"));
	}

	/** Looter.Mission.Offers: every mission picked up from a giver, where it stands, and who offers it here. */
	void ListOffers(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Mission.Offers");
		UMissionRunner* Runner = FindActiveRunner(World, Command);
		if (!Runner)
		{
			return;
		}
		int32 Offered = 0;
		for (const UMissionDefinition* Mission : Runner->GetDefinitions())
		{
			if (!Mission || !Mission->IsOffered())
			{
				continue;
			}
			++Offered;
			const FName MissionId = Mission->GetMissionId();
			const AActor* Giver = Runner->FindOfferGiverActor(*Mission);
			const TCHAR* Where = Runner->IsCompleted(MissionId) ? TEXT("finished")
				: Runner->IsAccepted(MissionId) ? TEXT("taken")
				: Runner->IsAvailable(MissionId) ? TEXT("offered")
				: TEXT("not offered yet");
			UE_LOG(LogLooter, Log, TEXT("  %-16s %s \"%s\" from %s (%s)%s"), Where, *MissionId.ToString(), *Mission->Title.ToString(),
				*Mission->GetOfferGiverName().ToString(), *Mission->Offer.GiverTag.ToString(),
				Giver ? *FString::Printf(TEXT(" at %s"), *Giver->GetActorNameOrLabel()) : TEXT(", not in this level"));
		}
		UE_LOG(LogLooter, Log, TEXT("%s: %d missions are picked up from givers."), Command, Offered);
	}

	FAutoConsoleCommandWithWorldAndArgs OfferMissionCommand(
		TEXT("Looter.Mission.Offer"),
		TEXT("Looter.Mission.Offer <mission id>: opens the offer card for a mission as its giver would (Accept takes it when it can ")
		TEXT("be taken)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&OfferMission));

	FAutoConsoleCommandWithWorldAndArgs ListOffersCommand(
		TEXT("Looter.Mission.Offers"),
		TEXT("Looter.Mission.Offers: every mission picked up from a giver, whether it's offered, taken or finished, and who offers it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListOffers));
}

#endif
