// UBountyBoardSubsystem: the taken bounty's fight (made, dropped, its mark down, its end), the strongboxes owed (dropped
// where the fight ends, stood at their arena on a later visit, forgotten once looted), the mission tracker's entry, the
// banner when one is paid.

#include "Bounties/BountyBoardSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bounties/BountyArena.h"
#include "Bounties/BountyBoard.h"
#include "Bounties/BountyChest.h"
#include "Bounties/BountyFight.h"
#include "Bounties/BountyTuning.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLine.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "LooterBounty"

namespace
{
	/** A fight round the player (the Webwood's) drops its strongbox this far in front of where they stand (cm). */
	constexpr double ChestAhead = 300.0;
}

// ---------------------------------------------------------------------------
// The fight
// ---------------------------------------------------------------------------

void UBountyBoardSubsystem::EnsureFight()
{
	const FBountyPosting* Active = GetActive();
	const bool bWanted = !bMenuWorld && Active && Active->State == EBountyState::Taken && FBountyTuning::Current().bEnabled;
	ABountyFight* Current = Fight.Get();
	// Not this bounty's any more (given up, paid, another taken), or the board switched off: it goes, with what it has out.
	if (Current && (!Active || Current->GetSerial() != Active->Serial || (Active->State == EBountyState::Taken && !bWanted)))
	{
		DropFight();
		Current = nullptr;
	}
	// Won, and the player gone far from what's left (its stragglers taken away): the fight goes, its strongbox stands.
	if (Current && Active && Active->State == EBountyState::Won && !Current->IsDone() && Current->GetState() != EEncounterState::Engaged)
	{
		DropFight();
		Current = nullptr;
	}
	if (bWanted && !Current)
	{
		ABountyArena* Arena = FindArena(Active->Arena);
		Current = Arena ? ABountyFight::SpawnFor(GetWorld(), *Arena, *Active) : nullptr;
		if (Current)
		{
			Current->OnMarkDown.AddUObject(this, &UBountyBoardSubsystem::HandleMarkDown);
			Current->OnDone.AddUObject(this, &UBountyBoardSubsystem::HandleFightDone);
			Fight = Current;
		}
	}
}

void UBountyBoardSubsystem::DropFight()
{
	if (ABountyFight* Current = Fight.Get())
	{
		Current->OnMarkDown.RemoveAll(this);
		Current->OnDone.RemoveAll(this);
		// Destroyed, it takes what it has out with it (AEncounterSpawner::EndPlay).
		Current->Destroy();
	}
	Fight.Reset();
}

void UBountyBoardSubsystem::HandleMarkDown(ABountyFight& InFight)
{
	const FBountyPosting Posting = InFight.GetPosting();
	MarkWon(Posting.Serial);
	// Felt at once, over the fight's own noise: the sting, and Hob after whatever's being said.
	LooterSound::Play2D(this, LooterSoundCue::Bounty::MarkDown);
	if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		Captions->Play({ FStoryLine::Make(BountyRules::HobName(), BountyRules::MarkDownLine(Posting)) }, ECaptionPlay::Queue);
	}
}

void UBountyBoardSubsystem::HandleFightDone(ABountyFight& InFight)
{
	const FBountyPosting Posting = InFight.GetPosting();
	// Its mark lost off the world rather than killed still counts: everything it brought is down.
	MarkWon(Posting.Serial);
	if (!IsChestOwed(Posting.Serial) || GetChest(Posting.Serial))
	{
		return;
	}
	const ABountyArena* Arena = FindArena(Posting.Arena);
	if (!Arena)
	{
		return;
	}
	// Where the fight ended: the arena's spot, or in front of the player for one that came round them.
	FVector Spot = Arena->GetRewardLocation();
	float Yaw = Arena->RewardYaw;
	if (Arena->bAroundPlayer)
	{
		if (const APawn* Player = FindPlayer())
		{
			FVector Ahead = Player->GetActorForwardVector().GetSafeNormal2D();
			Ahead = Ahead.IsNearlyZero() ? FVector::ForwardVector : Ahead;
			Spot = Player->GetActorLocation() + Ahead * ChestAhead;
			Yaw = static_cast<float>((-Ahead).Rotation().Yaw);
		}
	}
	for (const FBountyChestOwed& Owed : GetRecord().OwedChests)
	{
		if (Owed.Serial == Posting.Serial)
		{
			SpawnChest(Owed, Spot, Yaw, /*bDrop*/ true);
			break;
		}
	}
}

void UBountyBoardSubsystem::MarkWon(int32 Serial)
{
	FBountyBoardRecord& Record = MutableRecord();
	const int32 Slot = Record.FindSerial(Serial);
	if (Slot == INDEX_NONE || Record.Slots[Slot].State != EBountyState::Taken)
	{
		return;
	}
	FBountyPosting& Posting = Record.Slots[Slot];
	Posting.State = EBountyState::Won;
	if (!IsChestOwed(Serial))
	{
		FBountyChestOwed& Owed = Record.OwedChests.AddDefaulted_GetRef();
		Owed.Serial = Serial;
		Owed.Arena = Posting.Arena;
		Owed.Tier = Posting.Tier;
	}
	UE_LOG(LogLooter, Log, TEXT("Bounty board: won %s; its strongbox owed."), *BountyRules::Describe(Posting));
	Changed(true);
}

bool UBountyBoardSubsystem::WinNow()
{
	const FBountyPosting* Active = GetActive();
	if (!Active || Active->State != EBountyState::Taken)
	{
		return false;
	}
	// Its mark out: killed as the player's kill (its death plays as any). Not out yet: won as if it had died.
	ABountyFight* Current = Fight.Get();
	ACreatureBase* Mark = Current ? Current->GetMark() : nullptr;
	if (Mark && !Mark->IsDead())
	{
		APawn* Player = FindPlayer();
		const UHealthComponent* Health = Mark->FindComponentByClass<UHealthComponent>();
		UGameplayStatics::ApplyDamage(Mark, (Health ? Health->GetMaxHealth() : 1000.f) * 10.f, Player ? Player->GetController() : nullptr,
			Player, UDamageType::StaticClass());
		return true;
	}
	MarkWon(Active->Serial);
	DropFight();
	EnsureChests();
	return true;
}

// ---------------------------------------------------------------------------
// The strongboxes
// ---------------------------------------------------------------------------

bool UBountyBoardSubsystem::IsChestOwed(int32 Serial) const
{
	for (const FBountyChestOwed& Owed : GetRecord().OwedChests)
	{
		if (Owed.Serial == Serial)
		{
			return true;
		}
	}
	return false;
}

ABountyChest* UBountyBoardSubsystem::GetChest(int32 Serial) const
{
	const TWeakObjectPtr<ABountyChest>* Found = Chests.Find(Serial);
	return Found ? Found->Get() : nullptr;
}

void UBountyBoardSubsystem::EnsureChests()
{
	if (bMenuWorld)
	{
		return;
	}
	// Copied: spawning one may change the record (never the list itself, but stay safe).
	const TArray<FBountyChestOwed> Owed = GetRecord().OwedChests;
	for (const FBountyChestOwed& Each : Owed)
	{
		if (ABountyChest* Standing = GetChest(Each.Serial))
		{
			// Looted between the board's looks (its own tick tells it too).
			Standing->CheckLooted();
			continue;
		}
		// Tried this visit and couldn't be made (kept as null): not again twice a second.
		if (Chests.Contains(Each.Serial))
		{
			continue;
		}
		// Its fight still on: the box drops where it ends (HandleFightDone).
		const ABountyFight* Current = Fight.Get();
		if (Current && Current->GetSerial() == Each.Serial && !Current->IsDone())
		{
			continue;
		}
		const ABountyArena* Arena = FindArena(Each.Arena);
		if (!Arena)
		{
			continue;
		}
		// Owed from before (a later visit, or the player left the fight): standing at the arena's spot.
		if (!SpawnChest(Each, Arena->GetRewardLocation(), Arena->RewardYaw, /*bDrop*/ false))
		{
			UE_LOG(LogLooter, Warning, TEXT("Bounty %d: its strongbox couldn't be made at %s."), Each.Serial, *Each.Arena.ToString());
		}
	}
}

ABountyChest* UBountyBoardSubsystem::SpawnChest(const FBountyChestOwed& Owed, const FVector& Spot, float Yaw, bool bDrop)
{
	ABountyChest* Chest = ABountyChest::SpawnFor(GetWorld(), GetBoardId(), Owed, Spot, Yaw, bDrop);
	// Kept even when it failed (null), so a box that can't be made isn't tried again twice a second.
	Chests.Add(Owed.Serial, Chest);
	if (Chest)
	{
		Chest->OnLooted.AddUObject(this, &UBountyBoardSubsystem::HandleChestLooted);
	}
	return Chest;
}

void UBountyBoardSubsystem::HandleChestLooted(ABountyChest& Chest)
{
	FBountyBoardRecord& Record = MutableRecord();
	const int32 Removed = Record.OwedChests.RemoveAll([&Chest](const FBountyChestOwed& Each) { return Each.Serial == Chest.GetSerial(); });
	if (Removed > 0)
	{
		UE_LOG(LogLooter, Log, TEXT("Bounty %d: its strongbox is looted."), Chest.GetSerial());
		Changed(true);
	}
}

// ---------------------------------------------------------------------------
// The tracker and the banner
// ---------------------------------------------------------------------------

void UBountyBoardSubsystem::UpdateTracker()
{
	UWorld* World = GetWorld();
	UMissionSubsystem* Missions = World ? World->GetSubsystem<UMissionSubsystem>() : nullptr;
	const FBountyPosting* Active = GetActive();
	if (!Missions || !Active || bMenuWorld)
	{
		ClearTracker();
		return;
	}
	if (TrackerId == INDEX_NONE)
	{
		TrackerId = Missions->AddMission(BountyRules::TitleFor(*Active));
	}
	const ABountyArena* Arena = FindArena(Active->Arena);
	const ABountyFight* Current = Fight.Get();
	const FText Name = BountyRules::MarkName(Active->Kind, Active->NameIndex);
	FMissionTrackerParts Parts;
	// Two steps (find it, put it down), then the turn-in at the board; the banner announces the end.
	Parts.StepCount = 2;
	Parts.bAnnouncedEnd = true;
	TOptional<FVector> Waypoint;
	FText Line;
	if (Active->State == EBountyState::Won)
	{
		Line = LOCTEXT("TrackCollect", "Collect the bounty at the board");
		Parts.Step = Parts.StepCount;
		Parts.bTurnIn = true;
		if (const ABountyBoard* Here = Board.Get())
		{
			Waypoint = Here->GetInteractionLocation().Get(Here->GetActorLocation());
		}
		if (IsChestOwed(Active->Serial))
		{
			FMissionTrackerOptional& Box = Parts.Optionals.AddDefaulted_GetRef();
			Box.Line = LOCTEXT("TrackChest", "Open the bounty's strongbox").ToString();
		}
	}
	else if (Current && Current->HasSprung())
	{
		Line = FText::Format(LOCTEXT("TrackKill", "Put {0} down"), Name);
		Parts.Step = 1;
		const ACreatureBase* Mark = Current->GetMark();
		Waypoint = Mark ? Mark->GetActorLocation() : Current->GetActorLocation();
	}
	else
	{
		Line = FText::Format(LOCTEXT("TrackHunt", "Hunt {0} at {1}"), Name, Arena ? Arena->GetDisplayName() : FText::FromName(Active->Arena));
		Parts.Step = 0;
		if (Arena)
		{
			Waypoint = Arena->GetActorLocation();
		}
	}
	Parts.Line = Line.ToString();
	Missions->SetObjective(TrackerId, Line, Waypoint, Parts);
}

void UBountyBoardSubsystem::ClearTracker()
{
	if (TrackerId == INDEX_NONE)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (UMissionSubsystem* Missions = World->GetSubsystem<UMissionSubsystem>())
		{
			Missions->RemoveMission(TrackerId);
		}
	}
	TrackerId = INDEX_NONE;
}

void UBountyBoardSubsystem::Announce(const FBountyPosting& Posting, const FMissionRewardsGiven& Given)
{
	// The mission-complete banner hears the runner's completions; a bounty is announced as a side job's end under its own
	// title (only the banner and the notice boards' screens listen, and they read the title and what it gave).
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner)
	{
		return;
	}
	UMissionDefinition* Announced = NewObject<UMissionDefinition>(this, NAME_None, RF_Transient);
	Announced->Title = BountyRules::TitleFor(Posting);
	Announced->Kind = EMissionKind::Side;
	Runner->OnMissionCompleted.Broadcast(*Announced, Given);
}

#undef LOCTEXT_NAMESPACE
