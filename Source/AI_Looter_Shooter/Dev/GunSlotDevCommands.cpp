// Developer console commands for the equip slots opening with progress (GunSlotRules; not in shipping builds):
// Looter.GunSlots shows where the player's slots stand and which missions the fourth waits for; Looter.GunSlots.FinishStory
// finishes those missions (their rewards with them, as Looter.Mission.Complete gives), opening the fourth with its banner.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Inventory/GunSlotRules.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindSlotsWorld(UWorld* World)
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

	UWeaponManagerComponent* FindPlayerWeapons(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		return Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	}

	const TCHAR* LockName(EGunSlotLock Lock)
	{
		switch (Lock)
		{
		case EGunSlotLock::Level: return TEXT("shut: the level");
		case EGunSlotLock::Story: return TEXT("shut: the story");
		default: return TEXT("open");
		}
	}

	/** Looter.GunSlots: the player's slots, open or shut and why, and the story missions done and to do. */
	void ShowSlots(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindSlotsWorld(World);
		const UWeaponManagerComponent* Weapons = FindPlayerWeapons(GameWorld);
		if (!Weapons)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.GunSlots: no player with guns (start the game first)."));
			return;
		}
		const TOptional<FGunSlotProgress> Progress = Weapons->GetSlotProgress();
		UE_LOG(LogLooter, Log, TEXT("Looter.GunSlots: %d of %d open (%d guns equipped)%s."), Weapons->GetUnlockedSlots(), Weapons->MaxWeapons,
			Weapons->GetWeapons().Num(), Progress.IsSet()
				? *FString::Printf(TEXT("; level %d, %s's story %s"), Progress->Level, *GunSlotRules::StoryArea().ToString(),
					Progress->bStoryDone ? TEXT("done") : TEXT("not done"))
				: TEXT("; not the player's, every slot open"));
		for (int32 SlotIndex = 0; SlotIndex < Weapons->MaxWeapons; ++SlotIndex)
		{
			UE_LOG(LogLooter, Log, TEXT("  slot %d: %s%s"), SlotIndex + 1, LockName(Weapons->GetSlotLock(SlotIndex)),
				Weapons->GetWeapons().IsValidIndex(SlotIndex) ? TEXT(", a gun in it") : TEXT(""));
		}
		const UMissionRunner* Runner = GameWorld ? GameWorld->GetSubsystem<UMissionRunner>() : nullptr;
		if (Runner)
		{
			for (const FName Mission : GunSlotRules::StoryMissions(Runner->GetDefinitions(), GunSlotRules::StoryArea()))
			{
				UE_LOG(LogLooter, Log, TEXT("  story: %s %s"), *Mission.ToString(), Runner->GetCampaign().HasCompleted(Mission) ? TEXT("done") : TEXT("to do"));
			}
		}
	}

	/** Looter.GunSlots.FinishStory: finishes every main mission of the story area still to do (rewards and all). */
	void FinishStory(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindSlotsWorld(World);
		UMissionRunner* Runner = GameWorld ? GameWorld->GetSubsystem<UMissionRunner>() : nullptr;
		if (!Runner || !Runner->IsActive())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.GunSlots.FinishStory: no missions here (start the game first; the main menu plays none)."));
			return;
		}
		int32 Finished = 0;
		for (const FName Mission : GunSlotRules::StoryMissions(Runner->GetDefinitions(), GunSlotRules::StoryArea()))
		{
			if (!Runner->IsCompleted(Mission) && Runner->CompleteMission(Mission))
			{
				++Finished;
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.GunSlots.FinishStory: %d of %s's main missions finished now."), Finished,
			*GunSlotRules::StoryArea().ToString());
	}

	FAutoConsoleCommandWithWorldAndArgs ShowSlotsCommand(
		TEXT("Looter.GunSlots"),
		TEXT("Looter.GunSlots: the player's equip slots, open or shut and why (2 from the start, the third at level 3, the fourth after Ransom's Rest's main missions), and those missions."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowSlots));

	FAutoConsoleCommandWithWorldAndArgs FinishStoryCommand(
		TEXT("Looter.GunSlots.FinishStory"),
		TEXT("Looter.GunSlots.FinishStory: finishes every Ransom's Rest main mission still to do, with their rewards, which opens the fourth slot (its banner too)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FinishStory));
}

#endif
