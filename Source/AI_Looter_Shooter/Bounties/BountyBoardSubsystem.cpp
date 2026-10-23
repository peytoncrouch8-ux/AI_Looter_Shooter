// UBountyBoardSubsystem: its life in the level, the board, the record and its clock, posting and rotating (Refresh).
// Taking, giving up, paying and the cards are in BountyBoardSubsystemActions.cpp; the fight, the strongboxes and the
// tracker in BountyBoardSubsystemFight.cpp.

#include "Bounties/BountyBoardSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Audio/LooterSound.h"
#include "Bounties/BountyArena.h"
#include "Bounties/BountyBoard.h"
#include "Bounties/BountyFight.h"
#include "Bounties/BountyTuning.h"
#include "Core/LooterMenuGameMode.h"
#include "Creatures/EncounterSubsystem.h"
#include "Economy/WalletSubsystem.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLine.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "LooterBounty"

namespace
{
	const FBountyBoardRecord EmptyRecord;
}

UBountyBoardSubsystem* UBountyBoardSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject && GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UBountyBoardSubsystem>() : nullptr;
}

bool UBountyBoardSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests (they call Refresh themselves).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UBountyBoardSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Behind the main menu nobody plays: the board stays as the session left it.
	bMenuWorld = ALooterMenuGameMode::IsMenuWorld(&InWorld);
	if (bMenuWorld)
	{
		return;
	}
	if (UMissionRunner* Runner = InWorld.GetSubsystem<UMissionRunner>())
	{
		RunnerHandle = Runner->OnChanged.AddUObject(this, &UBountyBoardSubsystem::HandleRunnerChanged);
	}
	InWorld.GetTimerManager().SetTimer(Timer, this, &UBountyBoardSubsystem::HandleTimer, RefreshSeconds, /*bLoop*/ true, RefreshSeconds);
}

void UBountyBoardSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Timer);
		if (UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>())
		{
			Runner->OnChanged.Remove(RunnerHandle);
		}
	}
	Super::Deinitialize();
}

void UBountyBoardSubsystem::HandleTimer()
{
	Refresh();
}

void UBountyBoardSubsystem::HandleRunnerChanged()
{
	// An arena may have opened (its story's fight done) or a harder tier: an empty slot tries again at the next look.
	bStoryChanged = true;
}

// ---------------------------------------------------------------------------
// The board
// ---------------------------------------------------------------------------

void UBountyBoardSubsystem::RegisterBoard(ABountyBoard* InBoard)
{
	if (!InBoard || Board.IsValid())
	{
		if (InBoard && Board.Get() != InBoard)
		{
			UE_LOG(LogLooter, Warning, TEXT("Bounty board %s: the level has one already (%s); it shows nothing."), *InBoard->GetName(),
				*GetNameSafe(Board.Get()));
		}
		return;
	}
	Board = InBoard;
	LastBoardId = InBoard->BoardId;
	bStoryChanged = true;
	// Once every actor has begun (the arenas, the session's world put back), the first look.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() { Refresh(); }));
	}
}

void UBountyBoardSubsystem::UnregisterBoard(ABountyBoard* InBoard)
{
	if (Board.Get() == InBoard)
	{
		Board.Reset();
	}
}

bool UBountyBoardSubsystem::IsBoardOpen(const ABountyBoard& InBoard) const
{
	if (bMenuWorld || !FBountyTuning::Current().bEnabled)
	{
		return false;
	}
	const FCampaignRecord* Campaign = FindCampaign();
	const UWorld* World = GetWorld();
	const UMissionRunner* Runner = World ? World->GetSubsystem<UMissionRunner>() : nullptr;
	return InBoard.OpenWhen.IsEmpty() || (Campaign && InBoard.OpenWhen.IsMet(*Campaign, Runner));
}

EBountyTier UBountyBoardSubsystem::GetHighestTier() const
{
	const ABountyBoard* Here = Board.Get();
	const FCampaignRecord* Campaign = FindCampaign();
	const UWorld* World = GetWorld();
	const UMissionRunner* Runner = World ? World->GetSubsystem<UMissionRunner>() : nullptr;
	if (!Here || !Campaign)
	{
		return EBountyTier::Wanted;
	}
	if (Here->DeadlyWhen.IsMet(*Campaign, Runner))
	{
		return EBountyTier::Deadly;
	}
	return Here->DangerousWhen.IsMet(*Campaign, Runner) ? EBountyTier::Dangerous : EBountyTier::Wanted;
}

// ---------------------------------------------------------------------------
// The record, the clock, the player
// ---------------------------------------------------------------------------

FCampaignRecord* UBountyBoardSubsystem::FindCampaign() const
{
	// The runner's (a test's own, or the session's), else the session's (kept in memory without one).
	const UWorld* World = GetWorld();
	if (const UMissionRunner* Runner = World ? World->GetSubsystem<UMissionRunner>() : nullptr; Runner && Runner->IsActive())
	{
		return &Runner->GetCampaign();
	}
	USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	return Sessions ? Sessions->GetCampaign() : nullptr;
}

FName UBountyBoardSubsystem::GetBoardId() const
{
	const ABountyBoard* Here = Board.Get();
	return Here && !Here->BoardId.IsNone() ? Here->BoardId : LastBoardId;
}

FBountyBoardRecord& UBountyBoardSubsystem::MutableRecord()
{
	FCampaignRecord* Campaign = FindCampaign();
	FBountyBoardRecord& Record = Campaign ? Campaign->Bounties.FindOrAdd(GetBoardId()) : LocalRecord;
	EnsureSlots(Record);
	return Record;
}

const FBountyBoardRecord& UBountyBoardSubsystem::GetRecord() const
{
	const FCampaignRecord* Campaign = FindCampaign();
	if (!Campaign)
	{
		return LocalRecord;
	}
	const FBountyBoardRecord* Found = Campaign->Bounties.Find(GetBoardId());
	return Found ? *Found : EmptyRecord;
}

void UBountyBoardSubsystem::EnsureSlots(FBountyBoardRecord& Record)
{
	// A record from another build (or a fresh one) gets exactly the board's slots.
	Record.Slots.SetNum(BountyRules::SlotCount);
	Record.RefillAt.SetNumZeroed(BountyRules::SlotCount);
}

const FBountyPosting* UBountyBoardSubsystem::GetActive() const
{
	const FBountyBoardRecord& Record = GetRecord();
	const int32 Slot = Record.FindActiveSlot();
	return Slot != INDEX_NONE ? &Record.Slots[Slot] : nullptr;
}

double UBountyBoardSubsystem::GetNow() const
{
	if (TestClock.IsSet())
	{
		return TestClock.GetValue();
	}
	const UWorld* World = GetWorld();
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	const double Played = Sessions ? Sessions->GetPlayedSecondsNow(World) : (World ? World->GetTimeSeconds() : 0.0);
	return Played + ClockOffset;
}

void UBountyBoardSubsystem::AdvanceClock(double Seconds)
{
	if (TestClock.IsSet())
	{
		TestClock = TestClock.GetValue() + Seconds;
	}
	else
	{
		ClockOffset += Seconds;
	}
	Refresh();
}

APawn* UBountyBoardSubsystem::FindPlayer() const
{
	const UWorld* World = GetWorld();
	const UEncounterSubsystem* Encounters = World ? World->GetSubsystem<UEncounterSubsystem>() : nullptr;
	return Encounters ? Encounters->GetPlayer() : nullptr;
}

int32 UBountyBoardSubsystem::GetPlayerLevel() const
{
	const APawn* Player = FindPlayer();
	const APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	const ULocalPlayer* Local = PC ? PC->GetLocalPlayer() : nullptr;
	const UPlayerProgressionSubsystem* Progression = Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	return Progression ? Progression->GetLevel() : 1;
}

void UBountyBoardSubsystem::SaveSoon() const
{
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->SaveSoon();
	}
}

// ---------------------------------------------------------------------------
// Posting
// ---------------------------------------------------------------------------

ABountyArena* UBountyBoardSubsystem::FindArena(FName ArenaId) const
{
	UWorld* World = GetWorld();
	if (!World || ArenaId.IsNone())
	{
		return nullptr;
	}
	for (TActorIterator<ABountyArena> It(World); It; ++It)
	{
		if (It->GetArenaId() == ArenaId)
		{
			return *It;
		}
	}
	return nullptr;
}

void UBountyBoardSubsystem::LevelsAt(const ABountyArena& Arena, int32& OutLow, int32& OutHigh) const
{
	OutLow = 1;
	OutHigh = 1;
	const UWorld* World = GetWorld();
	const UAreaRulesSubsystem* Rules = World ? World->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	const UAreaDefinition* Area = Rules ? Rules->GetArea() : nullptr;
	if (Area && Area->HasLevelBand())
	{
		// Where its creatures will stand: its zone's levels, never the player's (the user's rule, 2026-10-12).
		Area->GetLevelsAt(Arena.GetActorLocation(), OutLow, OutHigh);
		OutLow = FMath::Max(OutLow, 1);
		OutHigh = FMath::Max(OutHigh, OutLow);
	}
}

TArray<FBountyArenaChoice> UBountyBoardSubsystem::GatherOpenArenas() const
{
	TArray<FBountyArenaChoice> Open;
	UWorld* World = GetWorld();
	const FCampaignRecord* Campaign = FindCampaign();
	const UMissionRunner* Runner = World ? World->GetSubsystem<UMissionRunner>() : nullptr;
	if (!World)
	{
		return Open;
	}
	for (TActorIterator<ABountyArena> It(World); It; ++It)
	{
		const ABountyArena& Arena = **It;
		const bool bOpen = Arena.OpenWhen.IsEmpty() || (Campaign && Arena.OpenWhen.IsMet(*Campaign, Runner));
		if (!bOpen || Arena.Kinds.IsEmpty())
		{
			continue;
		}
		FBountyArenaChoice& Choice = Open.AddDefaulted_GetRef();
		Choice.Id = Arena.GetArenaId();
		Choice.Kinds = Arena.Kinds;
		LevelsAt(Arena, Choice.LevelLow, Choice.LevelHigh);
	}
	// By id: the level's actor order isn't the draw's to lean on (the same seed must draw the same arena on every load).
	Open.Sort([](const FBountyArenaChoice& A, const FBountyArenaChoice& B) { return A.Id.LexicalLess(B.Id); });
	return Open;
}

bool UBountyBoardSubsystem::FillSlot(FBountyBoardRecord& Record, int32 Slot, double Now)
{
	FBountyDrawInput In;
	In.Arenas = GatherOpenArenas();
	for (int32 Other = 0; Other < Record.Slots.Num(); ++Other)
	{
		const FBountyPosting& Each = Record.Slots[Other];
		if (Other != Slot && Each.IsPosting())
		{
			// Once for each posting: the draw spreads over the arenas and doubles up only when it has to.
			In.BusyArenas.Add(Each.Arena);
			In.BusyNames.AddUnique(BountyRules::NameKey(Each.Kind, Each.NameIndex));
		}
	}
	In.HighestTier = GetHighestTier();
	In.Serial = FMath::Max(Record.NextSerial, 1);
	In.Seed = BountyRules::SeedFor(GetBoardId(), In.Serial);
	In.Now = Now;
	FBountyPosting Drawn = BountyRules::Draw(In);
	if (!Drawn.IsPosting())
	{
		// No arena free: it fills when the story opens one.
		Record.RefillAt[Slot] = 0.0;
		return false;
	}
	Record.NextSerial = In.Serial + 1;
	Record.Slots[Slot] = MoveTemp(Drawn);
	Record.RefillAt[Slot] = 0.0;
	UE_LOG(LogLooter, Log, TEXT("Bounty board %s: posted %s in slot %d."), *GetBoardId().ToString(), *BountyRules::Describe(Record.Slots[Slot]),
		Slot + 1);
	return true;
}

void UBountyBoardSubsystem::Rotate(FBountyBoardRecord& Record, double Now)
{
	// The postings not taken come down; the taken one (or one won and not paid yet) stays.
	++Record.Rotation;
	Record.RotatedAt = Now;
	for (int32 Slot = 0; Slot < Record.Slots.Num(); ++Slot)
	{
		if (Record.Slots[Slot].State == EBountyState::Posted)
		{
			Record.Slots[Slot] = FBountyPosting();
			Record.RefillAt[Slot] = 0.0;
		}
	}
	// Fill them all again from the same arenas, distinct from the one kept.
	for (int32 Slot = 0; Slot < Record.Slots.Num(); ++Slot)
	{
		if (!Record.Slots[Slot].IsPosting())
		{
			FillSlot(Record, Slot, Now);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Bounty board %s: rotation %d at %.1f min of play."), *GetBoardId().ToString(), Record.Rotation, Now / 60.0);
}

void UBountyBoardSubsystem::Refresh()
{
	if (bMenuWorld)
	{
		return;
	}
	ABountyBoard* Here = Board.Get();
	const bool bOpen = Here && IsBoardOpen(*Here);
	bool bDirty = false;
	bool bPosted = false;
	if (bOpen)
	{
		FBountyBoardRecord& Record = MutableRecord();
		const double Now = GetNow();
		const FBountyTuning Tuning = FBountyTuning::Current();
		const int32 SerialBefore = Record.NextSerial;
		if (BountyRules::IsRotationDue(Record.RotatedAt, Now, Tuning.RotateMinutes))
		{
			Rotate(Record, Now);
			bDirty = true;
		}
		for (int32 Slot = 0; Slot < Record.Slots.Num(); ++Slot)
		{
			FBountyPosting& Posting = Record.Slots[Slot];
			// A posting whose arena isn't in this level any more (a later build moved it) comes down; a taken one too,
			// with nowhere to fight it.
			if (Posting.IsPosting() && Posting.State != EBountyState::Won && !FindArena(Posting.Arena))
			{
				UE_LOG(LogLooter, Warning, TEXT("Bounty board: %s has no arena %s here: taken down."), *BountyRules::Describe(Posting),
					*Posting.Arena.ToString());
				Posting = FBountyPosting();
				Record.RefillAt[Slot] = 0.0;
				bDirty = true;
				continue;
			}
			// An empty slot fills once its wait is over, or (waiting on an arena) once the story moved on.
			const bool bWaitOver = Record.RefillAt[Slot] > 0.0 && Now >= Record.RefillAt[Slot];
			if (!Posting.IsPosting() && (bWaitOver || (Record.RefillAt[Slot] <= 0.0 && bStoryChanged)))
			{
				bDirty |= FillSlot(Record, Slot, Now) || bWaitOver;
			}
		}
		bStoryChanged = false;
		bPosted = Record.NextSerial != SerialBefore;
	}
	EnsureFight();
	EnsureChests();
	if (Here)
	{
		Here->ShowRecord(GetRecord(), bOpen);
	}
	UpdateTracker();
	if (bDirty)
	{
		Changed(true);
	}
	if (bPosted)
	{
		AnnouncePosted();
	}
}

#undef LOCTEXT_NAMESPACE
