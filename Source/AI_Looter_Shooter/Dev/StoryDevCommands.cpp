// Developer console commands for captions, speaker points, story characters and the story's props (Main Street's shutters,
// the chapel's bell and Reliquary) (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Bestiary/BestiaryEntry.h"
#include "Bestiary/Ledger.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/GraveSightSubsystem.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryCharacter.h"
#include "Story/StoryLine.h"
#include "World/ChapelBell.h"
#include "World/ChapelReliquary.h"
#include "World/WindowShutter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
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

	APawn* FindPlayerPawn(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	/** The actor of type T nearest the player (any one without a player), or null when the level has none. */
	template <typename T>
	T* FindNearest(UWorld* World)
	{
		const APawn* Pawn = FindPlayerPawn(World);
		const FVector From = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
		T* Nearest = nullptr;
		for (TActorIterator<T> It(World); It; ++It)
		{
			if (!Nearest || FVector::DistSquared(It->GetActorLocation(), From) < FVector::DistSquared(Nearest->GetActorLocation(), From))
			{
				Nearest = *It;
			}
		}
		return Nearest;
	}

	/** Distance in front of the player's feet, turned to face them. */
	FTransform InFrontOfPlayer(const APawn& Pawn, double Distance)
	{
		const FVector Forward = FRotator(0.0, Pawn.GetViewRotation().Yaw, 0.0).Vector();
		const FVector Feet = Pawn.GetActorLocation() - FVector(0.0, 0.0, Pawn.GetSimpleCollisionHalfHeight());
		return FTransform((-Forward).Rotation(), Feet + Forward * Distance);
	}

	/**
	 * Grandma Delia at the screen door, from Main 1 (Docs/Areas/RansomsRest.md), as captions: the step's test lines. With
	 * no speaker, a speaker point names her.
	 */
	TArray<FStoryLine> DeliaTestLines(const FText& Speaker)
	{
		const TCHAR* const Words[] = {
			TEXT("I was to lay you on the boards tonight."),
			TEXT("Your Pa got up Wednesday night, came up through the dirt like it was fog."),
			TEXT("A keeper doesn't lie still while his saint is dark. He walks the boards at dusk."),
		};
		TArray<FStoryLine> Said;
		for (const TCHAR* Each : Words)
		{
			Said.Add(FStoryLine::Make(Speaker, FText::FromString(Each)));
		}
		return Said;
	}

	/**
	 * Looter.Story.Caption [speaker words...] | stop: Delia's test lines, or one line (the first word is the speaker, with
	 * underscores for spaces), cutting off what's being said; "stop" ends the captions.
	 */
	void ShowCaption(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.Caption");
		UCaptionSubsystem* Captions = UCaptionSubsystem::Get(FindGameWorld(World));
		if (!Captions)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no level is being played (start the game first)."), Command);
			return;
		}
		if (Args.Num() == 1 && Args[0].Equals(TEXT("stop"), ESearchCase::IgnoreCase))
		{
			Captions->Clear();
			UE_LOG(LogLooter, Log, TEXT("%s: captions stopped."), Command);
			return;
		}
		if (Args.Num() == 1)
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: %s [speaker words...] (Grandma_Delia I was to lay you...), or %s stop"), Command, Command);
			return;
		}
		TArray<FStoryLine> Said;
		if (Args.Num() >= 2)
		{
			TArray<FString> Words = Args;
			const FString Speaker = Words[0].Replace(TEXT("_"), TEXT(" "));
			Words.RemoveAt(0);
			Said.Add(FStoryLine::Make(FText::FromString(Speaker), FText::FromString(FString::Join(Words, TEXT(" ")))));
		}
		else
		{
			Said = DeliaTestLines(FText::FromString(TEXT("Grandma Delia")));
		}
		Captions->Play(Said, ECaptionPlay::Interrupt);
		UE_LOG(LogLooter, Log, TEXT("%s: %d line(s) playing."), Command, Said.Num());
	}

	/**
	 * Looter.Story.Door: Grandma Delia's talking screen door (a test), two meters in front of the player, facing them,
	 * tagged Speaker_Delia. Talking to it plays her lines and sends the Talk event; it never opens.
	 */
	void SpawnTalkingDoor(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.Door");
		UWorld* GameWorld = FindGameWorld(World);
		const APawn* Pawn = FindPlayerPawn(GameWorld);
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no player (start the game first)."), Command);
			return;
		}
		// Set up before its play begins, which enters it among the level's interactables.
		const FTransform Where = InFrontOfPlayer(*Pawn, 200.0);
		ASpeakerPoint* Door = GameWorld->SpawnActorDeferred<ASpeakerPoint>(ASpeakerPoint::StaticClass(), Where);
		if (!Door)
		{
			return;
		}
		// A 1 m by 2.1 m door leaf of its own (a level's door is the building's mesh): she won't open it to a corpse.
		Door->Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Door->Mesh->SetRelativeLocation(FVector(0.0, 0.0, 105.0));
		Door->Mesh->SetRelativeScale3D(FVector(0.06, 1.0, 2.1));
		// On the door's face, the player's side.
		Door->SpeakerPoint->SetRelativeLocation(FVector(5.0, 0.0, 140.0));
		Door->SpeakerPoint->SpeakerName = FText::FromString(TEXT("Grandma Delia"));
		Door->SpeakerPoint->Lines = DeliaTestLines(FText::GetEmpty());
		Door->Tags.Add(FName(TEXT("Speaker_Delia")));
		Door->FinishSpawning(Where);
		UE_LOG(LogLooter, Log, TEXT("%s: Delia's door (%s) in front of the player, tagged Speaker_Delia: talk to it with the Interact key; ")
			TEXT("a talk objective for Speaker_Delia completes from it."), Command, *Door->GetName());
	}

	/**
	 * Looter.Story.Character [mission id]: a placeholder story character (Mister Sexton, a test) three meters in front of
	 * the player, facing them, tagged Speaker_Sexton; with a mission id, it's there only once that mission is finished.
	 */
	void SpawnStoryCharacter(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.Character");
		UWorld* GameWorld = FindGameWorld(World);
		const APawn* Pawn = FindPlayerPawn(GameWorld);
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no player (start the game first)."), Command);
			return;
		}
		const FTransform Where = InFrontOfPlayer(*Pawn, 300.0);
		AStoryCharacter* Stranger = GameWorld->SpawnActorDeferred<AStoryCharacter>(AStoryCharacter::StaticClass(), Where);
		if (!Stranger)
		{
			return;
		}
		Stranger->SpeakerPoint->SpeakerName = FText::FromString(TEXT("Mister Sexton"));
		Stranger->SpeakerPoint->Lines = {
			FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("Ask a keeper. Their lanterns lean toward a saint's light."))),
			FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("So are you, friend."))),
		};
		Stranger->Tags.Add(FName(TEXT("Speaker_Sexton")));
		if (Args.Num() > 0)
		{
			Stranger->ShownWhen.AfterMissions.Add(FName(*Args[0]));
		}
		Stranger->FinishSpawning(Where);
		UE_LOG(LogLooter, Log, TEXT("%s: %s placed in front of the player, there %s; now %s."), Command, *Stranger->GetName(),
			*Stranger->ShownWhen.Describe(), Stranger->IsShown() ? TEXT("shown") : TEXT("hidden"));
	}

	/**
	 * Looter.Story.Shutters [open | slam]: Main Street's shutters (AWindowShutter, Main 3). With nothing, how many hang open
	 * and shut; "open" swings every one back open against the wall, watching for the player again; "slam" slams them all,
	 * each after its own moment, to see the slam without walking past.
	 */
	void Shutters(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.Shutters");
		UWorld* GameWorld = FindGameWorld(World);
		if (!GameWorld)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no level is being played (start the game first)."), Command);
			return;
		}
		const FString Mode = Args.Num() > 0 ? Args[0].ToLower() : FString();
		if (!Mode.IsEmpty() && Mode != TEXT("open") && Mode != TEXT("slam"))
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: %s [open | slam]"), Command);
			return;
		}
		int32 Total = 0;
		int32 Shut = 0;
		for (TActorIterator<AWindowShutter> It(GameWorld); It; ++It)
		{
			AWindowShutter* Shutter = *It;
			if (Mode == TEXT("open"))
			{
				Shutter->OpenNow();
			}
			else if (Mode == TEXT("slam"))
			{
				Shutter->Slam(FMath::FRandRange(0.f, Shutter->MaxDelay));
			}
			++Total;
			Shut += Shutter->IsShut() ? 1 : 0;
		}
		UE_LOG(LogLooter, Log, TEXT("%s: %d shutter(s), %d hanging open or slamming, %d shut%s."), Command, Total, Total - Shut, Shut,
			Mode.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" (%s)"), *Mode));
	}

	/**
	 * Looter.Story.Ledger: whether the bestiary is Sexton's Ledger yet (Main 2's last step or past it), and the pages written
	 * in it only: who's met, and which of the seven names' whereabouts are written in.
	 */
	void ShowLedger(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.Ledger");
		const UMissionRunner* Runner = UMissionRunner::Get(FindGameWorld(World));
		if (!Runner)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no level is being played (start the game first)."), Command);
			return;
		}
		const FCampaignRecord& Campaign = Runner->GetCampaign();
		const bool bOpen = Ledger::IsOpen(Campaign, Runner);
		UE_LOG(LogLooter, Log, TEXT("%s: the %s (handed over at %s's step %d)."), Command,
			bOpen ? TEXT("bestiary is Sexton's Ledger") : TEXT("bestiary isn't the Ledger yet"), *Ledger::Mission.ToString(), Ledger::HandedOverStep + 1);
		for (const UBestiaryEntry* Entry : UBestiaryEntry::LoadAll())
		{
			if (Entry->IsListed(false))
			{
				continue;
			}
			const bool bName = Entry->Page == EBestiaryPage::LedgerName;
			UE_LOG(LogLooter, Log, TEXT("  %s: %s%s"), *Entry->DisplayName.ToString(),
				bName ? TEXT("a name") : Entry->IsKnown(false, Campaign, Runner) ? TEXT("met") : TEXT("not met yet"),
				bName ? (Entry->IsFound(Campaign, Runner) ? *FString::Printf(TEXT(", found: %s"), *Entry->Habitat.ToString()) : TEXT(", whereabouts blank"))
					: TEXT(""));
		}
	}

	/**
	 * Looter.Story.GraveSight [force]: Grave Sight on the Reliquary nearest the player (Main 4), as a look at it does: the
	 * screen's flash, her ember lifting off the lid, and the missions told at its end. Before the story lets it be looked at,
	 * "force" plays it anyway. With no Reliquary in the level, the screen's flash alone.
	 */
	void GraveSight(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.GraveSight");
		UWorld* GameWorld = FindGameWorld(World);
		UGraveSightSubsystem* Sight = UGraveSightSubsystem::Get(GameWorld);
		if (!Sight)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no level is being played (start the game first)."), Command);
			return;
		}
		const bool bForce = Args.Num() > 0 && Args[0].Equals(TEXT("force"), ESearchCase::IgnoreCase);
		AChapelReliquary* Reliquary = FindNearest<AChapelReliquary>(GameWorld);
		if (!Reliquary)
		{
			Sight->Flash();
			UE_LOG(LogLooter, Log, TEXT("%s: no Reliquary in this level: the screen's flash alone."), Command);
			return;
		}
		if (Reliquary->Look(FindPlayerPawn(GameWorld), bForce))
		{
			UE_LOG(LogLooter, Log, TEXT("%s: Grave Sight on %s; the missions hear %s at its end."), Command, *Reliquary->GetActorNameOrLabel(),
				*AChapelReliquary::SightEvent.ToString());
			return;
		}
		UE_LOG(LogLooter, Warning, TEXT("%s: %s can't be looked at now (%s, or its flash still plays): '%s force' plays it anyway."), Command,
			*Reliquary->GetActorNameOrLabel(), *Reliquary->LookWhen.Describe(), Command);
	}

	/**
	 * Looter.Story.Bell: rings the chapel bell nearest the player as holding Interact on its rope does, and tells the
	 * missions so (Main 4's third step), to see the swing from the yard without going in.
	 */
	void RingBell(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Story.Bell");
		UWorld* GameWorld = FindGameWorld(World);
		AChapelBell* Bell = GameWorld ? FindNearest<AChapelBell>(GameWorld) : nullptr;
		if (!Bell)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no chapel bell in a level being played (start the game on Ransom's Rest)."), Command);
			return;
		}
		if (!Bell->Ring(FindPlayerPawn(GameWorld)))
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: %s can't be rung now (still swinging, or not yet: %s)."), Command, *Bell->GetActorNameOrLabel(),
				*Bell->RingWhen.Describe());
			return;
		}
		// The missions hear of it as they would from the player's held Interact.
		if (UMissionRunner* Runner = UMissionRunner::Get(GameWorld))
		{
			Runner->NotifyEvent(FMissionEvent::Interaction(Bell, /*bHeld*/ true));
		}
		UE_LOG(LogLooter, Log, TEXT("%s: %s rings."), Command, *Bell->GetActorNameOrLabel());
	}

	FAutoConsoleCommandWithWorldAndArgs GraveSightCommand(
		TEXT("Looter.Story.GraveSight"),
		TEXT("Looter.Story.GraveSight [force]: Grave Sight on the nearest Reliquary (the flash, her ember lifting off the lid, the ")
		TEXT("missions told at its end); 'force' plays it before the story lets it be looked at. With no Reliquary, the flash alone."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GraveSight));

	FAutoConsoleCommandWithWorldAndArgs RingBellCommand(
		TEXT("Looter.Story.Bell"),
		TEXT("Looter.Story.Bell: rings the nearest chapel bell as holding Interact on its rope does, and tells the missions."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RingBell));

	FAutoConsoleCommandWithWorldAndArgs ShuttersCommand(
		TEXT("Looter.Story.Shutters"),
		TEXT("Looter.Story.Shutters [open | slam]: how Main Street's shutters hang; 'open' swings them all back open, watching for the ")
		TEXT("player; 'slam' slams them all."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Shutters));

	FAutoConsoleCommandWithWorldAndArgs ShowLedgerCommand(
		TEXT("Looter.Story.Ledger"),
		TEXT("Looter.Story.Ledger: whether the bestiary is Sexton's Ledger yet, and the pages written in it only (met or not, and the ")
		TEXT("seven names' whereabouts)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowLedger));

	FAutoConsoleCommandWithWorldAndArgs ShowCaptionCommand(
		TEXT("Looter.Story.Caption"),
		TEXT("Looter.Story.Caption [speaker words...] | stop: plays Delia's test lines as captions, or one line (the first word is the ")
		TEXT("speaker, underscores for spaces); 'stop' ends the captions."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowCaption));

	FAutoConsoleCommandWithWorldAndArgs SpawnTalkingDoorCommand(
		TEXT("Looter.Story.Door"),
		TEXT("Looter.Story.Door: Grandma Delia's talking screen door (a test, not saved) two meters in front of the player, tagged ")
		TEXT("Speaker_Delia: it talks and never opens."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnTalkingDoor));

	FAutoConsoleCommandWithWorldAndArgs SpawnStoryCharacterCommand(
		TEXT("Looter.Story.Character"),
		TEXT("Looter.Story.Character [mission id]: a placeholder story character (a test, not saved) in front of the player, tagged ")
		TEXT("Speaker_Sexton; with a mission id it appears only once that mission is finished."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnStoryCharacter));
}

#endif
