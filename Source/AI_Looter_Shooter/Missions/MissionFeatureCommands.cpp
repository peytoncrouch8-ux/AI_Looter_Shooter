// Developer console commands for the mission features (not in shipping builds): mission items, a turn-in's choice, the
// story's flags and the optional objectives done.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "UI/HUD/LooterHUD.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
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

	const UMissionDefinition* FindNamed(const UMissionRunner& Runner, const FString& Words)
	{
		for (const UMissionDefinition* Mission : Runner.GetDefinitions())
		{
			if (Mission && Mission->IsNamed(Words))
			{
				return Mission;
			}
		}
		return nullptr;
	}

	/** Looter.Mission.Items: every mission's items, carried, dropped or still to find. */
	void ListItems(const TArray<FString>& Args, UWorld* World)
	{
		UMissionRunner* Runner = FindActiveRunner(World, TEXT("Looter.Mission.Items"));
		if (!Runner)
		{
			return;
		}
		const FCampaignRecord& Campaign = Runner->GetCampaign();
		int32 Count = 0;
		for (const UMissionDefinition* Mission : Runner->GetDefinitions())
		{
			for (const FMissionItemInfo& Item : Mission ? Mission->Items : TArray<FMissionItemInfo>())
			{
				++Count;
				UE_LOG(LogLooter, Log, TEXT("  %-10s %s \"%s\" (%s)%s"), Campaign.IsCarrying(Item.Id) ? TEXT("carried")
					: Campaign.DroppedItems.Contains(Item.Id) ? TEXT("dropped") : TEXT("to find"), *Item.Id.ToString(), *Item.Name.ToString(),
					*Mission->GetMissionId().ToString(), Item.DroppedBy.IsSet() ? *FString::Printf(TEXT(", carried by %s"), *Item.DroppedBy.Describe()) : TEXT(""));
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Mission.Items: %d mission items."), Count);
	}

	/** Looter.Mission.GiveItem <item id>: into the pocket, as a pickup puts it. */
	void GiveItem(const TArray<FString>& Args, UWorld* World)
	{
		UMissionRunner* Runner = FindActiveRunner(World, TEXT("Looter.Mission.GiveItem"));
		if (!Runner || Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: Looter.Mission.GiveItem <item id> (Looter.Mission.Items lists them)"));
			return;
		}
		const bool bGiven = Runner->GiveItem(FName(*Args[0]));
		UE_LOG(LogLooter, Log, TEXT("Looter.Mission.GiveItem: %s %s."), *Args[0], bGiven ? TEXT("carried")
			: TEXT("not given (no mission has it, its mission is finished, or it's carried already)"));
	}

	/** Looter.Mission.Choose <mission> <1|2>: turns a mission waiting here in with a choice; with no number, opens the card. */
	void Choose(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Mission.Choose");
		UMissionRunner* Runner = FindActiveRunner(World, Command);
		const UMissionDefinition* Mission = Runner && !Args.IsEmpty() ? FindNamed(*Runner, Args[0]) : nullptr;
		if (!Mission)
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: %s <mission id> [1|2]: the mission must wait for its turn-in here."), Command);
			return;
		}
		if (Args.Num() < 2)
		{
			// The card as its giver opens it, at the giver when they're here.
			UWorld* Played = FindPlayedWorld(World);
			ALooterHUD* HUD = ALooterHUD::FindFor(Played ? Played->GetFirstPlayerController() : nullptr);
			const bool bOpened = HUD && HUD->OpenTurnInChoice(Mission, nullptr);
			UE_LOG(LogLooter, Log, TEXT("%s: %s."), Command, bOpened ? TEXT("the choice card is open") : TEXT("the card can't open now"));
			return;
		}
		const int32 Choice = FCString::Atoi(*Args[1]) - 1;
		const bool bDone = Runner->TurnInWithChoice(Mission->GetMissionId(), Choice);
		UE_LOG(LogLooter, Log, TEXT("%s: %s %s."), Command, *Mission->GetMissionId().ToString(), bDone ? TEXT("turned in")
			: TEXT("not turned in (not waiting here, or no such choice)"));
	}

	/** Looter.Story.Flag [flag] [clear]: lists the story's flags, sets one, or clears it. */
	void Flag(const TArray<FString>& Args, UWorld* World)
	{
		UMissionRunner* Runner = FindActiveRunner(World, TEXT("Looter.Story.Flag"));
		if (!Runner)
		{
			return;
		}
		FCampaignRecord& Campaign = Runner->GetCampaign();
		if (Args.IsEmpty())
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Story.Flag: %s"), Campaign.StoryFlags.IsEmpty() ? TEXT("none set")
				: *FString::JoinBy(Campaign.StoryFlags, TEXT(", "), [](const FName& Each) { return Each.ToString(); }));
			return;
		}
		const FName Named(*Args[0]);
		const bool bClear = Args.Num() > 1 && Args[1].Equals(TEXT("clear"), ESearchCase::IgnoreCase);
		const bool bChanged = bClear ? Campaign.ClearFlag(Named) : Runner->SetFlag(Named);
		UE_LOG(LogLooter, Log, TEXT("Looter.Story.Flag: %s %s%s."), *Named.ToString(), bClear ? TEXT("cleared") : TEXT("set"),
			bChanged ? TEXT("") : TEXT(" already"));
	}

	/** Looter.Mission.Optional: the optional objectives done for missions not finished yet. */
	void ListOptional(const TArray<FString>& Args, UWorld* World)
	{
		UMissionRunner* Runner = FindActiveRunner(World, TEXT("Looter.Mission.Optional"));
		if (!Runner)
		{
			return;
		}
		const FCampaignRecord& Campaign = Runner->GetCampaign();
		UE_LOG(LogLooter, Log, TEXT("Looter.Mission.Optional: %s"), Campaign.OptionalDone.IsEmpty() ? TEXT("none done")
			: *FString::JoinBy(Campaign.OptionalDone, TEXT(", "), [](const FName& Each) { return Each.ToString(); }));
	}

	FAutoConsoleCommandWithWorldAndArgs ListItemsCommand(TEXT("Looter.Mission.Items"),
		TEXT("Looter.Mission.Items: every mission item, carried, dropped by its carrier or still to find."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListItems));

	FAutoConsoleCommandWithWorldAndArgs GiveItemCommand(TEXT("Looter.Mission.GiveItem"),
		TEXT("Looter.Mission.GiveItem <item id>: puts a mission item in the pocket, as picking it up does."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GiveItem));

	FAutoConsoleCommandWithWorldAndArgs ChooseCommand(TEXT("Looter.Mission.Choose"),
		TEXT("Looter.Mission.Choose <mission id> [1|2]: turns in a mission waiting here with that choice; with no number, opens its choice card."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Choose));

	FAutoConsoleCommandWithWorldAndArgs FlagCommand(TEXT("Looter.Story.Flag"),
		TEXT("Looter.Story.Flag [flag] [clear]: lists the story's flags (BackPay.Kept, PorchPlate.Taken), sets one, or clears it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Flag));

	FAutoConsoleCommandWithWorldAndArgs OptionalCommand(TEXT("Looter.Mission.Optional"),
		TEXT("Looter.Mission.Optional: the optional objectives done, as mission.step.objective, for missions not finished yet."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListOptional));
}

#endif
