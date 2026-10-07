// Developer console commands for Whitlock Fields in Side 2, "Unfinished Business": Amos's hay bales, his old hired hands'
// fight and Amos on his fence (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Story/AmosWhitlock.h"
#include "Story/SpeakerPointComponent.h"
#include "World/HayBale.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The hired hands' encounter (Tools/Unreal/build_area_whitlock.py). */
	const FName HandsId(TEXT("WhitlockHands"));

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

	APawn* FindPlayer(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	/**
	 * Looter.Story.Bales [force | reset]: loads every hay bale in Whitlock's field into the stack by his barn as a held
	 * Interact does (the remarks, the missions told for each). Before Amos has asked (Side 2's second step) "force" loads
	 * them anyway. "reset" puts them all back out in the field.
	 */
	void LoadBales(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.Bales");
		UWorld* GameWorld = FindGameWorld(World);
		if (!GameWorld)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no level is being played (start the game on Ransom's Rest)."), Command);
			return;
		}
		const bool bReset = HasWord(Args, TEXT("reset"));
		const bool bForce = HasWord(Args, TEXT("force"));
		APawn* Player = FindPlayer(GameWorld);
		UMissionRunner* Runner = UMissionRunner::Get(GameWorld);
		int32 Found = 0;
		int32 Done = 0;
		for (TActorIterator<AHayBale> It(GameWorld); It; ++It)
		{
			AHayBale* Bale = *It;
			++Found;
			if (bReset)
			{
				Bale->PutBack();
				++Done;
				continue;
			}
			if (Bale->Load(Player, bForce))
			{
				++Done;
				// The missions hear of it as they would from the player's hold.
				if (Runner)
				{
					Runner->NotifyEvent(FMissionEvent::Interaction(Bale, /*bHeld*/ true));
				}
				continue;
			}
			UE_LOG(LogLooter, Log, TEXT("%s: %s %s."), Command, *Bale->GetActorNameOrLabel(), Bale->IsLoaded()
				? TEXT("is loaded already") : *FString::Printf(TEXT("can't be loaded now (%s): 'force' loads it anyway"), *Bale->LoadWhen.Describe()));
		}
		if (Found == 0)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no hay bales in this level (they're in Whitlock Fields on Ransom's Rest)."), Command);
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("%s: %d of %d bales %s; %d in the stack."), Command, Done, Found,
			bReset ? TEXT("put back in the field") : TEXT("loaded"), AHayBale::CountLoaded(GameWorld));
	}

	/**
	 * Looter.Story.Hands [force]: Amos's old hired hands come now, in his barn yard (their encounter's wave, whether the
	 * player is near or not). Outside Side 2's third step "force" brings them anyway (and starts a cleared fight over).
	 */
	void BringHands(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.Hands");
		UWorld* GameWorld = FindGameWorld(World);
		UEncounterSubsystem* Encounters = GameWorld ? UEncounterSubsystem::Get(GameWorld) : nullptr;
		AEncounterSpawner* Spawner = Encounters ? Encounters->FindSpawner(HandsId) : nullptr;
		if (!Spawner)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no %s encounter in this level (it's in Whitlock's barn yard on Ransom's Rest)."), Command,
				*HandsId.ToString());
			return;
		}
		if (!Spawner->TriggerWave(HasWord(Args, TEXT("force"))))
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: the hands won't come now (%s): 'force' brings them anyway."), Command, *Spawner->Describe());
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("%s: the hired hands are up in the barn yard: %s."), Command, *Spawner->Describe());
	}

	/**
	 * Looter.Story.Amos [sit [now] | lean | talk]: Amos on his fence. "sit" settles him onto the rail as Side 2's end does
	 * ("now": at once), "lean" puts him back leaning on it, "talk" talks to him as a tap of Interact does (the missions told).
	 * With nothing, says where he is and what he'd say.
	 */
	void PoseAmos(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.Amos");
		UWorld* GameWorld = FindGameWorld(World);
		if (!GameWorld)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no level is being played (start the game on Ransom's Rest)."), Command);
			return;
		}
		AAmosWhitlock* Amos = nullptr;
		for (TActorIterator<AAmosWhitlock> It(GameWorld); It && !Amos; ++It)
		{
			Amos = *It;
		}
		if (!Amos)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no Amos in this level (he's at his fence in Whitlock Fields on Ransom's Rest)."), Command);
			return;
		}
		if (HasWord(Args, TEXT("sit")))
		{
			Amos->SitNow(HasWord(Args, TEXT("now")));
		}
		else if (HasWord(Args, TEXT("lean")))
		{
			Amos->LeanNow();
		}
		else if (HasWord(Args, TEXT("talk")))
		{
			if (!Amos->SpeakerPoint->Talk(FindPlayer(GameWorld)))
			{
				UE_LOG(LogLooter, Warning, TEXT("%s: he can't be talked to now (%s, %s)."), Command, Amos->IsShown() ? TEXT("shown")
					: *FString::Printf(TEXT("hidden: %s"), *Amos->ShownWhen.Describe()), Amos->SpeakerPoint->IsTalking() ? TEXT("still talking") : TEXT("quiet"));
			}
		}
		int32 Topic = INDEX_NONE;
		const TArray<FStoryLine> Next = Amos->SpeakerPoint->GetLinesNow(&Topic);
		UE_LOG(LogLooter, Log, TEXT("%s: %s, %s (%s); he'd say %d line(s)%s."), Command, *Amos->GetActorNameOrLabel(),
			Amos->IsShown() ? TEXT("shown") : TEXT("hidden"),
			Amos->IsSettling() ? TEXT("settling onto the rail") : Amos->IsSitting() ? TEXT("sitting on his fence") : TEXT("leaning on his fence"),
			Next.Num(), Topic == INDEX_NONE ? TEXT("") : *FString::Printf(TEXT(", topic %d"), Topic + 1));
	}

	FAutoConsoleCommandWithWorldAndArgs BalesCommand(
		TEXT("Looter.Story.Bales"),
		TEXT("Looter.Story.Bales [force | reset]: loads every hay bale in Whitlock's field into the stack by his barn as a held ")
		TEXT("Interact does (the missions told); 'force' before Amos has asked; 'reset' puts them back in the field."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LoadBales));

	FAutoConsoleCommandWithWorldAndArgs HandsCommand(
		TEXT("Looter.Story.Hands"),
		TEXT("Looter.Story.Hands [force]: Amos's old hired hands come now in his barn yard (Side 2's fight); 'force' outside its step."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BringHands));

	FAutoConsoleCommandWithWorldAndArgs AmosCommand(
		TEXT("Looter.Story.Amos"),
		TEXT("Looter.Story.Amos [sit [now] | lean | talk]: Amos settles onto his fence (now: at once), leans on it again, or is ")
		TEXT("talked to as a tap of Interact does; with nothing, says where he is and what he'd say."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PoseAmos));
}

#endif
