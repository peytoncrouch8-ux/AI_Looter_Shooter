// Developer console commands for the Sink in Main 5, "The Keeper's Lantern": the egg sacs and the lantern in the webbing
// (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "World/EggSac.h"
#include "World/KeepersLantern.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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

	bool HasWord(const TArray<FString>& Args, const TCHAR* Word)
	{
		return Args.ContainsByPredicate([Word](const FString& Arg) { return Arg.Equals(Word, ESearchCase::IgnoreCase); });
	}

	/**
	 * Looter.Story.EggSacs [force | reset]: every egg sac in the level shot down as the player's killing shot would: each
	 * falls, bursts on the ground, lets out its two spiders coming for the player, and tells the missions as it lands (Main
	 * 5's second step). Before or after their step, "force" shoots them anyway. "reset" hangs them all up again, intact (after
	 * a step started over from the console); their spiders stay out.
	 */
	void EggSacs(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.EggSacs");
		UWorld* GameWorld = FindGameWorld(World);
		if (!GameWorld)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no level is being played (start the game on Ransom's Rest)."), Command);
			return;
		}
		const bool bReset = HasWord(Args, TEXT("reset"));
		const bool bForce = HasWord(Args, TEXT("force"));
		// The player's shot: the spiders come for them.
		AController* By = GameWorld->GetFirstPlayerController();
		int32 Found = 0;
		int32 Done = 0;
		for (TActorIterator<AEggSac> It(GameWorld); It; ++It)
		{
			AEggSac* Sac = *It;
			++Found;
			if (bReset)
			{
				Sac->Hang();
				++Done;
				continue;
			}
			if (Sac->ShootDown(By, bForce))
			{
				++Done;
				continue;
			}
			UE_LOG(LogLooter, Log, TEXT("%s: %s %s."), Command, *Sac->GetActorNameOrLabel(), Sac->IsDown()
				? TEXT("is down already") : *FString::Printf(TEXT("can't be shot now (%s): 'force' shoots it anyway"), *Sac->ShootableWhen.Describe()));
		}
		if (Found == 0)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no egg sacs in this level (they're in the Sink on Ransom's Rest)."), Command);
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("%s: %d of %d egg sacs %s."), Command, Done, Found,
			bReset ? TEXT("hung up again") : *FString::Printf(TEXT("shot down; the missions hear %s as each lands"), *AEggSac::BurstEvent.ToString()));
	}

	/**
	 * Looter.Story.Lantern [force]: takes the Keeper's Lantern from the webbing as a tap of Interact does, and tells the
	 * missions so (Main 5's third step). Outside that step "force" takes it anyway. Says whether Ellis has it by the story.
	 */
	void TakeLantern(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.Lantern");
		UWorld* GameWorld = FindGameWorld(World);
		if (!GameWorld)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no level is being played (start the game on Ransom's Rest)."), Command);
			return;
		}
		AKeepersLantern* Lantern = nullptr;
		for (TActorIterator<AKeepersLantern> It(GameWorld); It && !Lantern; ++It)
		{
			Lantern = *It;
		}
		const APlayerController* Controller = GameWorld->GetFirstPlayerController();
		APawn* Player = Controller ? Controller->GetPawn() : nullptr;
		if (!Lantern)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no Keeper's Lantern in this level (it's in the Sink on Ransom's Rest). Ellis has it "
				"by the story: %s."), Command, AKeepersLantern::IsTakenIn(GameWorld) ? TEXT("yes") : TEXT("no"));
			return;
		}
		if (!Lantern->Take(Player, HasWord(Args, TEXT("force"))))
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: %s can't be taken now (%s): 'force' takes it anyway."), Command,
				Lantern->IsHanging() ? *Lantern->GetActorNameOrLabel() : TEXT("it's gone from the snare already, so it"),
				*Lantern->TakeWhen.Describe());
			return;
		}
		// The missions hear of it as they would from the player's tap.
		if (UMissionRunner* Runner = UMissionRunner::Get(GameWorld))
		{
			Runner->NotifyEvent(FMissionEvent::Interaction(Lantern, /*bHeld*/ false));
		}
		UE_LOG(LogLooter, Log, TEXT("%s: the Keeper's Lantern taken from %s. Ellis has it by the story: %s."), Command,
			*Lantern->GetActorNameOrLabel(), AKeepersLantern::IsTakenIn(GameWorld) ? TEXT("yes") : TEXT("not yet"));
	}

	FAutoConsoleCommandWithWorldAndArgs EggSacsCommand(
		TEXT("Looter.Story.EggSacs"),
		TEXT("Looter.Story.EggSacs [force | reset]: shoots down every egg sac in the Sink as a killing shot would (the fall, the ")
		TEXT("burst, two spiders each, the missions told as each lands); 'force' outside Main 5's step; 'reset' hangs them up again."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&EggSacs));

	FAutoConsoleCommandWithWorldAndArgs TakeLanternCommand(
		TEXT("Looter.Story.Lantern"),
		TEXT("Looter.Story.Lantern [force]: takes the Keeper's Lantern from the Sink's webbing as a tap of Interact does, and tells ")
		TEXT("the missions; 'force' outside Main 5's step."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TakeLantern));
}

#endif
