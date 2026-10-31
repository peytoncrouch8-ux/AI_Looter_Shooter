#include "Creatures/UnpaidPackSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/UnpaidCreature.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
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
		case EUnpaidTally::Rise: return TEXT("Rise");
		case EUnpaidTally::Opening: return TEXT("Opening");
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

	/** Ground under Spot (world-static geometry only), from a little over the player's feet: false with none near. */
	bool FindRiseGround(UWorld& World, const FVector& Spot, float FeetZ, FVector& OutGround)
	{
		FHitResult Hit;
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("UnpaidRiseDev"), nullptr);
		if (!World.LineTraceSingleByObjectType(Hit, FVector(Spot.X, Spot.Y, FeetZ + 400.f), FVector(Spot.X, Spot.Y, FeetZ - 800.f),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			return false;
		}
		OutGround = Hit.ImpactPoint;
		return true;
	}

	/**
	 * Looter.Unpaid.Rise [count] [around] [radius]: count plain Unpaid (default 6) rise out of the ground as an ambush's do, round
	 * the player (around 1, the default) or where they're put, 8 m in front (around 0), hunting them once up: the play-check of
	 * the rise, its stagger and its surround (2026-10-31).
	 */
	FAutoConsoleCommandWithWorldAndArgs RiseCommand(TEXT("Looter.Unpaid.Rise"),
		TEXT("Looter.Unpaid.Rise [count] [around] [radius]: count Unpaid (6) rise out of the ground as an ambush's dead do: round the player (around 1) or 8 m in front (0), radius cm (Looter.Unpaid.Rise.Radius)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* ConsoleWorld)
		{
			UWorld* World = PlayedWorld(ConsoleWorld);
			const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
			APawn* Pawn = Player ? Player->GetPawn() : nullptr;
			if (!World || !Pawn)
			{
				UE_LOG(LogLooter, Warning, TEXT("UNPAID RISE: no player (start the game first)."));
				return;
			}
			// The dead never rise inside a safe zone (Delia's salt line round the farm, a town made safe): round a player standing in
			// one every spot is refused, and they come up in the row where they were put.
			const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(World);
			if (Encounters && Encounters->IsInSafeZone(Pawn->GetActorLocation()))
			{
				UE_LOG(LogLooter, Warning, TEXT("UNPAID RISE: the player stands in a safe zone: no Unpaid rises round them there (they come up where they're put). Try it outside the salt line or a safe town."));
			}
			const int32 Count = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 1, 12) : 6;
			const bool bAround = Args.Num() > 1 ? FCString::Atoi(*Args[1]) != 0 : true;
			const float Radius = Args.Num() > 2 ? FMath::Max(FCString::Atof(*Args[2]), 0.f) : 0.f;
			const float FeetZ = static_cast<float>(Pawn->GetActorLocation().Z) - Pawn->GetSimpleCollisionHalfHeight();
			const FVector Ahead = FRotator(0.f, Player->GetControlRotation().Yaw, 0.f).Vector();
			const FVector Side = FVector::CrossProduct(FVector::UpVector, Ahead);
			FCreatureRuntimeSpawn Setup;
			Setup.bMayBeTouched = false;
			int32 Risen = 0;
			for (int32 Index = 0; Index < Count; ++Index)
			{
				// Spawned in a row 8 m in front (where they rise without "around"); round the player they move to their slots.
				const FVector Spot = Pawn->GetActorLocation() + Ahead * 800.f + Side * ((Index - (Count - 1) * 0.5f) * 220.f);
				FVector Ground;
				if (!FindRiseGround(*World, Spot, FeetZ, Ground))
				{
					continue;
				}
				const float Yaw = static_cast<float>((Pawn->GetActorLocation() - Ground).Rotation().Yaw);
				AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(ACreatureBase::SpawnAtRuntime(World, AUnpaidCreature::StaticClass(), Ground, Yaw, Setup));
				if (!Unpaid)
				{
					continue;
				}
				Unpaid->RiseAmbush(Pawn, bAround, Radius);
				Unpaid->AlertTo(Pawn);
				++Risen;
			}
			UE_LOG(LogLooter, Display, TEXT("UNPAID RISE: %d of %d rising %s."), Risen, Count, bAround ? TEXT("round the player") : TEXT("in front of the player"));
		}));

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

// ---------------------------------------------------------------------------
// Rises and openings
// ---------------------------------------------------------------------------

FUnpaidRiseBurst& UUnpaidPackSubsystem::BurstFor(const APawn* Target)
{
	if (!Target)
	{
		return Unaimed;
	}
	// Bursts of players gone are dropped as a new one is made (a handful of entries at most).
	if (!Bursts.Contains(TWeakObjectPtr<const APawn>(Target)))
	{
		for (auto It = Bursts.CreateIterator(); It; ++It)
		{
			if (!It->Key.IsValid())
			{
				It.RemoveCurrent();
			}
		}
	}
	return Bursts.FindOrAdd(TWeakObjectPtr<const APawn>(Target));
}

FUnpaidOpeningGate& UUnpaidPackSubsystem::OpeningFor(const APawn& Target)
{
	if (!Openings.Contains(TWeakObjectPtr<const APawn>(&Target)))
	{
		for (auto It = Openings.CreateIterator(); It; ++It)
		{
			if (!It->Key.IsValid())
			{
				It.RemoveCurrent();
			}
		}
	}
	return Openings.FindOrAdd(TWeakObjectPtr<const APawn>(&Target));
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
