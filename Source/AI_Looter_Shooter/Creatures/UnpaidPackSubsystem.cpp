#include "Creatures/UnpaidPackSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/UnpaidCreature.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

// UUnpaidPackSubsystem: the pack's board, its shriek gate and the move tally (2026-10-29). The show (lights, the ground wave)
// is UnpaidPackSubsystemShow.cpp.

namespace
{
	/** A board entry nobody has renewed in this long (s) is dropped: a creature gone without a word. */
	constexpr float BoardMaxAge = 6.f;

	const TCHAR* TallyName(EUnpaidTally What)
	{
		switch (What)
		{
		case EUnpaidTally::Lunge: return TEXT("Lunge");
		case EUnpaidTally::Rake: return TEXT("Rake");
		case EUnpaidTally::Feint: return TEXT("Feint");
		case EUnpaidTally::PhaseStrike: return TEXT("PhaseStrike");
		case EUnpaidTally::LowSwipe: return TEXT("LowSwipe");
		case EUnpaidTally::HangBack: return TEXT("HangBack");
		case EUnpaidTally::Flank: return TEXT("Flank");
		case EUnpaidTally::Cinder: return TEXT("Cinder");
		case EUnpaidTally::Rally: return TEXT("Rally");
		case EUnpaidTally::Shriek: return TEXT("Shriek");
		case EUnpaidTally::ShriekInterrupted: return TEXT("ShriekInterrupted");
		default: return TEXT("?");
		}
	}

	/** The moves whose blows land (their landed count is shown). */
	bool HasBlow(EUnpaidTally What)
	{
		return What == EUnpaidTally::Lunge || What == EUnpaidTally::Rake || What == EUnpaidTally::PhaseStrike || What == EUnpaidTally::LowSwipe;
	}

#if !UE_BUILD_SHIPPING
	/** Looter.Unpaid.Moves [reset]: the move mix since the last reset (the brief's play-check: counts per move over a fight). */
	FAutoConsoleCommandWithWorldAndArgs MovesCommand(TEXT("Looter.Unpaid.Moves"),
		TEXT("Prints the Unpaid's move mix since its last reset (each move, how many landed, flanks, cinders, rallies, shrieks, interrupts); 'reset' starts it again."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(World);
			if (!Packs)
			{
				UE_LOG(LogLooter, Warning, TEXT("UNPAID MOVES: no Unpaid pack subsystem in this world."));
				return;
			}
			if (Args.Num() > 0 && Args[0].Equals(TEXT("reset"), ESearchCase::IgnoreCase))
			{
				Packs->ResetTally();
				UE_LOG(LogLooter, Display, TEXT("UNPAID MOVES: reset."));
				return;
			}
			UE_LOG(LogLooter, Display, TEXT("UNPAID MOVES: %s"), *Packs->Report());
		}));

	/** The world being played (the console's may be the editor's): the game's, or PIE's. */
	UWorld* PlayedWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if ((Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE) && Context.World())
				{
					return Context.World();
				}
			}
		}
		return nullptr;
	}

	/** The nearest Unpaid with their kit (up to Wanted) shriek at the player now; how many began. */
	int32 ShriekNearest(UWorld* World, int32 Wanted)
	{
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		if (!Pawn)
		{
			return 0;
		}
		TArray<AUnpaidCreature*> Near;
		for (TActorIterator<AUnpaidCreature> It(World); It; ++It)
		{
			if (!It->IsDead() && It->HasFightKit())
			{
				Near.Add(*It);
			}
		}
		const FVector Where = Pawn->GetActorLocation();
		Near.Sort([&Where](const AUnpaidCreature& A, const AUnpaidCreature& B)
		{
			return FVector::DistSquared(A.GetActorLocation(), Where) < FVector::DistSquared(B.GetActorLocation(), Where);
		});
		int32 Started = 0;
		for (AUnpaidCreature* Unpaid : Near)
		{
			if (Started >= Wanted)
			{
				break;
			}
			if (Unpaid->GetTarget() != Pawn || Unpaid->GetCreatureState() != ECreatureState::Chase)
			{
				Unpaid->DevPutInState(ECreatureState::Chase, Pawn);
			}
			Started += Unpaid->StartKitAction(EUnpaidAction::Shriek) ? 1 : 0;
		}
		UE_LOG(LogLooter, Display, TEXT("UNPAID SHRIEK: %d of %d shrieking (%d Unpaid with their kit in the level)."), Started, Wanted, Near.Num());
		return Started;
	}

	/** Looter.Unpaid.Shriek.Now's repeat (one at a time, in the world it was set in). */
	FTimerHandle ShriekRepeat;
	TWeakObjectPtr<UWorld> ShriekRepeatWorld;

	/**
	 * Looter.Unpaid.Shriek.Now [count] [every]: the nearest Unpaid with their kit (up to count, 1 by default) shriek at the player
	 * now, as a missed lunge's would; with every (s), again and again (count 0 stops it): for grabs and the perf run (perf.ps1
	 * -Exec "...,Looter.Unpaid.Shriek.PackGap 0,Looter.Unpaid.Shriek.Cooldown 0,Looter.Unpaid.Shriek.Now 3 2.5").
	 */
	FAutoConsoleCommandWithWorldAndArgs ShriekNowCommand(TEXT("Looter.Unpaid.Shriek.Now"),
		TEXT("Looter.Unpaid.Shriek.Now [count] [every]: the nearest Unpaid (count, default 1) shriek at the player now, every seconds if given (count 0 stops it). Several of one pack need Looter.Unpaid.Shriek.PackGap 0."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* ConsoleWorld)
		{
			UWorld* World = PlayedWorld(ConsoleWorld);
			if (UWorld* Was = ShriekRepeatWorld.Get())
			{
				Was->GetTimerManager().ClearTimer(ShriekRepeat);
			}
			ShriekRepeatWorld.Reset();
			const int32 Wanted = Args.Num() > 0 ? FMath::Max(FCString::Atoi(*Args[0]), 0) : 1;
			const float Every = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f;
			if (!World || Wanted <= 0)
			{
				UE_LOG(LogLooter, Display, TEXT("UNPAID SHRIEK: %s"), World ? TEXT("repeat stopped.") : TEXT("no game (start the game first)."));
				return;
			}
			ShriekNearest(World, Wanted);
			if (Every > 0.f)
			{
				// Again every Every seconds (from the command line the horde may not be there yet: it shrieks once it is).
				const TWeakObjectPtr<UWorld> WeakWorld(World);
				World->GetTimerManager().SetTimer(ShriekRepeat, FTimerDelegate::CreateLambda([WeakWorld, Wanted]()
				{
					if (UWorld* Again = WeakWorld.Get())
					{
						ShriekNearest(Again, Wanted);
					}
				}), FMath::Max(Every, 0.5f), true);
				ShriekRepeatWorld = World;
			}
		}));
#endif
}

UUnpaidPackSubsystem* UUnpaidPackSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UUnpaidPackSubsystem>() : nullptr;
}

bool UUnpaidPackSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

double UUnpaidPackSubsystem::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

// ---------------------------------------------------------------------------
// The board
// ---------------------------------------------------------------------------

void UUnpaidPackSubsystem::Commit(const APawn& Target, uint32 Id, EUnpaidMove Move)
{
	// One entry per creature across every player: committing at one lets go of any other.
	for (TPair<TWeakObjectPtr<const APawn>, FTargetBoard>& Each : Boards)
	{
		if (Each.Key.Get() != &Target)
		{
			Each.Value.Board.Release(Id);
		}
	}
	FTargetBoard& Board = Boards.FindOrAdd(TWeakObjectPtr<const APawn>(&Target));
	Board.Board.Prune(Now(), BoardMaxAge);
	Board.Board.Commit(Id, Move, Now());
}

void UUnpaidPackSubsystem::Release(uint32 Id)
{
	for (TPair<TWeakObjectPtr<const APawn>, FTargetBoard>& Each : Boards)
	{
		Each.Value.Board.Release(Id);
	}
}

const FUnpaidMoveBoard& UUnpaidPackSubsystem::BoardFor(const APawn& Target) const
{
	static const FUnpaidMoveBoard Empty;
	const FTargetBoard* Board = Boards.Find(TWeakObjectPtr<const APawn>(&Target));
	return Board ? Board->Board : Empty;
}

float UUnpaidPackSubsystem::SincePackShriek(const APawn& Target, FName Pack) const
{
	const FTargetBoard* Board = Boards.Find(TWeakObjectPtr<const APawn>(&Target));
	const double* Last = Board ? Board->PackShriek.Find(Pack) : nullptr;
	return Last ? static_cast<float>(Now() - *Last) : 1000.f;
}

void UUnpaidPackSubsystem::NotePackShriek(const APawn& Target, FName Pack)
{
	Boards.FindOrAdd(TWeakObjectPtr<const APawn>(&Target)).PackShriek.Add(Pack, Now());
}

// ---------------------------------------------------------------------------
// The tally
// ---------------------------------------------------------------------------

void UUnpaidPackSubsystem::Count(EUnpaidTally What)
{
	if (What < EUnpaidTally::Count)
	{
		++Tally[static_cast<int32>(What)];
	}
}

void UUnpaidPackSubsystem::CountLanded(EUnpaidTally What)
{
	if (What < EUnpaidTally::Count)
	{
		++Landed[static_cast<int32>(What)];
	}
}

void UUnpaidPackSubsystem::Note(const UObject* WorldContext, EUnpaidTally What, bool bLanded)
{
	if (UUnpaidPackSubsystem* Packs = Get(WorldContext))
	{
		if (bLanded)
		{
			Packs->CountLanded(What);
		}
		else
		{
			Packs->Count(What);
		}
	}
}

FString UUnpaidPackSubsystem::Report() const
{
	int32 Attacks = 0;
	for (int32 Index = 0; Index <= static_cast<int32>(EUnpaidTally::LowSwipe); ++Index)
	{
		Attacks += Tally[Index];
	}
	FString Line = FString::Printf(TEXT("over %.0f s, %d attacks:"), Now() - TallySince, Attacks);
	for (int32 Index = 0; Index < static_cast<int32>(EUnpaidTally::Count); ++Index)
	{
		const EUnpaidTally What = static_cast<EUnpaidTally>(Index);
		Line += HasBlow(What)
			? FString::Printf(TEXT(" %s %d (%d%%, landed %d);"), TallyName(What), Tally[Index],
				Attacks > 0 ? FMath::RoundToInt32(100.f * Tally[Index] / Attacks) : 0, Landed[Index])
			: FString::Printf(TEXT(" %s %d;"), TallyName(What), Tally[Index]);
	}
	return Line;
}

void UUnpaidPackSubsystem::ResetTally()
{
	for (int32 Index = 0; Index < static_cast<int32>(EUnpaidTally::Count); ++Index)
	{
		Tally[Index] = 0;
		Landed[Index] = 0;
	}
	TallySince = Now();
}

// ---------------------------------------------------------------------------
// The tick (only while the show runs)
// ---------------------------------------------------------------------------

void UUnpaidPackSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AdvanceWaves(DeltaTime);
	AdvanceLights(DeltaTime);
	// Boards of players gone are dropped now and then (cheap: a handful of entries).
	for (auto It = Boards.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

bool UUnpaidPackSubsystem::IsTickable() const
{
	if (!Waves.IsEmpty())
	{
		return true;
	}
	for (const FLight& Each : Lights)
	{
		if (Each.bTaken || Each.Candelas > 0.f)
		{
			return true;
		}
	}
	return false;
}

TStatId UUnpaidPackSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UUnpaidPackSubsystem, STATGROUP_Tickables);
}
