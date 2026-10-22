// Developer console commands for the bounty board (UBountyBoardSubsystem; not in shipping builds): list it, post anew,
// take, give up, win, collect, post a chosen bounty, forget it, open its screen, move its clock on, draw the arenas.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Bounties/BountyArena.h"
#include "Bounties/BountyBoard.h"
#include "Bounties/BountyBoardSubsystem.h"
#include "Bounties/BountyChest.h"
#include "Bounties/BountyFight.h"
#include "Bounties/BountyRules.h"
#include "Creatures/CreatureBase.h"
#include "Components/LineBatchComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "UI/World/BountyBoardWidget.h"

namespace
{
	/** The batch the arenas' lines go in (cleared by the next draw). */
	constexpr uint32 ArenaLines = 0x0B0A7700;

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

	UBountyBoardSubsystem* FindBounties(UWorld* World, const TCHAR* Command)
	{
		UWorld* Game = FindGameWorld(World);
		UBountyBoardSubsystem* Bounties = Game ? Game->GetSubsystem<UBountyBoardSubsystem>() : nullptr;
		if (!Bounties || !Bounties->GetBoard())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: start the game in a level with a bounty board (Ransom's Rest)."), Command);
			return nullptr;
		}
		return Bounties;
	}

	APawn* FindPlayer(UWorld* World)
	{
		UWorld* Game = FindGameWorld(World);
		const APlayerController* Controller = Game ? Game->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	/** Looter.Bounty.List: the board's slots, its clock, the bounty under way, the strongboxes owed. */
	void List(const TArray<FString>& Args, UWorld* World)
	{
		UBountyBoardSubsystem* Bounties = FindBounties(World, TEXT("Looter.Bounty.List"));
		if (!Bounties)
		{
			return;
		}
		const FBountyBoardRecord& Record = Bounties->GetRecord();
		UE_LOG(LogLooter, Log, TEXT("Bounty board %s: %s; open %s, highest tier %s, rotation %d, %.1f min of play, %s; %d paid."),
			*Bounties->GetBoard()->BoardId.ToString(), Record.IsFresh() ? TEXT("fresh") : TEXT("posting"),
			Bounties->IsBoardOpen(*Bounties->GetBoard()) ? TEXT("yes") : TEXT("no"), *BountyRules::TierName(Bounties->GetHighestTier()),
			Record.Rotation, Bounties->GetNow() / 60.0, *Bounties->DescribeNextRotation().ToString(), Record.Paid);
		for (int32 Slot = 0; Slot < Record.Slots.Num(); ++Slot)
		{
			const FBountyPosting& Posting = Record.Slots[Slot];
			UE_LOG(LogLooter, Log, TEXT("  %d: %s%s"), Slot + 1, *BountyRules::Describe(Posting),
				!Posting.IsPosting() && Record.RefillAt.IsValidIndex(Slot) && Record.RefillAt[Slot] > 0.0
					? *FString::Printf(TEXT(" (fills at %.1f min)"), Record.RefillAt[Slot] / 60.0) : TEXT(""));
		}
		if (const ABountyFight* Fight = Bounties->GetFight())
		{
			UE_LOG(LogLooter, Log, TEXT("  fight: %s; mark %s"), *Fight->Describe(), Fight->GetMark() ? *Fight->GetMark()->GetName()
				: (Fight->IsMarkDown() ? TEXT("down") : TEXT("not out")));
		}
		for (const FBountyChestOwed& Owed : Record.OwedChests)
		{
			UE_LOG(LogLooter, Log, TEXT("  strongbox owed: bounty %d at %s (%s)"), Owed.Serial, *Owed.Arena.ToString(),
				Bounties->GetChest(Owed.Serial) ? TEXT("standing") : TEXT("not out"));
		}
	}

	void Post(const TArray<FString>& Args, UWorld* World)
	{
		if (UBountyBoardSubsystem* Bounties = FindBounties(World, TEXT("Looter.Bounty.Post")))
		{
			Bounties->RotateNow();
			List(Args, World);
		}
	}

	void Take(const TArray<FString>& Args, UWorld* World)
	{
		UBountyBoardSubsystem* Bounties = FindBounties(World, TEXT("Looter.Bounty.Take"));
		const int32 Slot = Args.Num() > 0 ? FCString::Atoi(*Args[0]) - 1 : 0;
		if (Bounties)
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Bounty.Take %d: %s."), Slot + 1, Bounties->Take(Slot) ? TEXT("taken")
				: TEXT("refused (not posted, the board shut, or one is under way)"));
		}
	}

	void Abandon(const TArray<FString>& Args, UWorld* World)
	{
		if (UBountyBoardSubsystem* Bounties = FindBounties(World, TEXT("Looter.Bounty.Abandon")))
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Bounty.Abandon: %s."), Bounties->GiveUp() ? TEXT("given up") : TEXT("none taken"));
		}
	}

	void Win(const TArray<FString>& Args, UWorld* World)
	{
		if (UBountyBoardSubsystem* Bounties = FindBounties(World, TEXT("Looter.Bounty.Win")))
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Bounty.Win: %s."), Bounties->WinNow() ? TEXT("the mark is down") : TEXT("none taken"));
		}
	}

	void Collect(const TArray<FString>& Args, UWorld* World)
	{
		if (UBountyBoardSubsystem* Bounties = FindBounties(World, TEXT("Looter.Bounty.Collect")))
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Bounty.Collect: %s."), Bounties->Collect(FindPlayer(World)) ? TEXT("paid") : TEXT("none won"));
		}
	}

	/** Looter.Bounty.Make <slot> <Kind> <Tier> <Arena> [twist...]: a chosen bounty in a slot (to try each arena and kind). */
	void Make(const TArray<FString>& Args, UWorld* World)
	{
		UBountyBoardSubsystem* Bounties = FindBounties(World, TEXT("Looter.Bounty.Make"));
		if (!Bounties)
		{
			return;
		}
		FBountyPosting Posting;
		EBountyTier Tier = EBountyTier::Wanted;
		if (Args.Num() < 4 || !BountyRules::FindKind(FName(*Args[1])) || !BountyRules::ParseTier(Args[2], Tier) || !Bounties->FindArena(FName(*Args[3])))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Bounty.Make <slot 1-3> <Unpaid|Spider|Spitter|DeadMiner> <Wanted|Dangerous|Deadly> <arena id> [twist...]"));
			return;
		}
		Posting.Kind = FName(*Args[1]);
		Posting.Tier = Tier;
		Posting.Arena = FName(*Args[3]);
		Posting.NameIndex = FMath::RandRange(0, FMath::Max(BountyRules::NumNames(Posting.Kind) - 1, 0));
		for (int32 Index = 4; Index < Args.Num(); ++Index)
		{
			EBountyTwist Twist;
			if (BountyRules::ParseTwist(Args[Index], Twist))
			{
				Posting.Twists.AddUnique(Twist);
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.Bounty.Make: no twist '%s' (Fleet, Warded, Mending, Rallying, Vengeful, GraveBurst, BigPosse, SwiftPosse)."), *Args[Index]);
			}
		}
		const int32 Slot = FCString::Atoi(*Args[0]) - 1;
		UE_LOG(LogLooter, Log, TEXT("Looter.Bounty.Make: %s."), Bounties->PostChosen(Slot, Posting) ? TEXT("posted")
			: TEXT("refused (no such slot, or it's taken or won)"));
	}

	void Forget(const TArray<FString>& Args, UWorld* World)
	{
		if (UBountyBoardSubsystem* Bounties = FindBounties(World, TEXT("Looter.Bounty.Forget")))
		{
			Bounties->Forget();
			UE_LOG(LogLooter, Log, TEXT("Looter.Bounty.Forget: the board's record is gone; it posts afresh."));
		}
	}

	void Board(const TArray<FString>& Args, UWorld* World)
	{
		UBountyBoardSubsystem* Bounties = FindBounties(World, TEXT("Looter.Bounty.Board"));
		UWorld* Game = FindGameWorld(World);
		APlayerController* Controller = Game ? Game->GetFirstPlayerController() : nullptr;
		if (Bounties && Controller)
		{
			UBountyBoardWidget::OpenFor(Controller, Bounties->GetBoard());
		}
	}

	void Clock(const TArray<FString>& Args, UWorld* World)
	{
		if (UBountyBoardSubsystem* Bounties = FindBounties(World, TEXT("Looter.Bounty.Clock")))
		{
			const double Minutes = Args.Num() > 0 ? FCString::Atod(*Args[0]) : 10.0;
			Bounties->AdvanceClock(Minutes * 60.0);
			UE_LOG(LogLooter, Log, TEXT("Looter.Bounty.Clock: the board's clock moved on %.1f min (now %.1f min)."), Minutes, Bounties->GetNow() / 60.0);
		}
	}

	/** Looter.Bounty.Arenas: draws every arena's ground and spots (Looter.Bounty.Arenas off clears them). */
	void Arenas(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* Game = FindGameWorld(World);
		ULineBatchComponent* Lines = Game ? Game->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent) : nullptr;
		if (!Lines)
		{
			return;
		}
		Lines->ClearBatch(ArenaLines);
		if (Args.Num() > 0 && Args[0].Equals(TEXT("off"), ESearchCase::IgnoreCase))
		{
			return;
		}
		int32 Count = 0;
		for (TActorIterator<ABountyArena> It(Game); It; ++It)
		{
			It->DrawArena(*Lines, ArenaLines);
			++Count;
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Bounty.Arenas: %d drawn."), Count);
	}

	FAutoConsoleCommandWithWorldAndArgs ListCommand(TEXT("Looter.Bounty.List"),
		TEXT("Looter.Bounty.List: the bounty board's postings, clock, the fight under way and the strongboxes owed."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&List));
	FAutoConsoleCommandWithWorldAndArgs PostCommand(TEXT("Looter.Bounty.Post"),
		TEXT("Looter.Bounty.Post: rotates the bounty board now (the postings not taken come down, new ones go up)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Post));
	FAutoConsoleCommandWithWorldAndArgs TakeCommand(TEXT("Looter.Bounty.Take"),
		TEXT("Looter.Bounty.Take <1-3>: takes the bounty in that slot."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Take));
	FAutoConsoleCommandWithWorldAndArgs AbandonCommand(TEXT("Looter.Bounty.Abandon"),
		TEXT("Looter.Bounty.Abandon: gives the taken bounty up."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Abandon));
	FAutoConsoleCommandWithWorldAndArgs WinCommand(TEXT("Looter.Bounty.Win"),
		TEXT("Looter.Bounty.Win: the taken bounty's mark dies (killed if it's out)."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Win));
	FAutoConsoleCommandWithWorldAndArgs CollectCommand(TEXT("Looter.Bounty.Collect"),
		TEXT("Looter.Bounty.Collect: pays the won bounty as the board does."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Collect));
	FAutoConsoleCommandWithWorldAndArgs MakeCommand(TEXT("Looter.Bounty.Make"),
		TEXT("Looter.Bounty.Make <slot> <Kind> <Tier> <Arena> [twist...]: posts a chosen bounty in a slot."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Make));
	FAutoConsoleCommandWithWorldAndArgs ForgetCommand(TEXT("Looter.Bounty.Forget"),
		TEXT("Looter.Bounty.Forget: forgets the bounty board's record (it posts afresh)."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Forget));
	FAutoConsoleCommandWithWorldAndArgs BoardCommand(TEXT("Looter.Bounty.Board"),
		TEXT("Looter.Bounty.Board: opens the bounty board's screen (stand by the board: it closes past 4.5 m)."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Board));
	FAutoConsoleCommandWithWorldAndArgs ClockCommand(TEXT("Looter.Bounty.Clock"),
		TEXT("Looter.Bounty.Clock [minutes]: moves the bounty board's clock on (10 by default)."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Clock));
	FAutoConsoleCommandWithWorldAndArgs ArenasCommand(TEXT("Looter.Bounty.Arenas"),
		TEXT("Looter.Bounty.Arenas [off]: draws every bounty arena's ground and spots."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Arenas));
}

#endif
