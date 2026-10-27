// UBountyBoardSubsystem: the taken bounty's fight (made, dropped, its mark down, its end), the mission tracker's entry, the
// banner when one is paid. Nothing drops at the fight: the reward is handed over at the board (Collect).

#include "Bounties/BountyBoardSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bounties/BountyArena.h"
#include "Bounties/BountyBoard.h"
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
	/** How long "Bounty paid" shows before the tracker's entry goes (s of the board's clock). */
	constexpr double PaidLineSeconds = 0.4;
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
	// Won, and the player gone far from what's left (its stragglers taken away): the fight goes; the pay waits at the board.
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
	// Its mark lost off the world rather than killed still counts: everything it brought is down. No chest here (the user's
	// playtest, 2026-10-27): the reward is handed over when the bounty is turned in at the board.
	MarkWon(InFight.GetSerial());
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
	UE_LOG(LogLooter, Log, TEXT("Bounty board: won %s; paid at the board."), *BountyRules::Describe(Posting));
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
	return true;
}

// ---------------------------------------------------------------------------
// The tracker and the banner
// ---------------------------------------------------------------------------

void UBountyBoardSubsystem::UpdateTracker()
{
	UWorld* World = GetWorld();
	UMissionSubsystem* Missions = World ? World->GetSubsystem<UMissionSubsystem>() : nullptr;
	const FBountyPosting* Active = GetActive();
	// The paid line stands out its moment (the tracker sees it before the entry goes).
	if (!Active && TrackerId != INDEX_NONE && PaidLineUntil > GetNow() && Missions && !bMenuWorld)
	{
		return;
	}
	if (!Missions || !Active || bMenuWorld)
	{
		PaidLineUntil = 0.0;
		ClearTracker();
		return;
	}
	// Another taken while the paid line still stood: its entry goes, the new one gets its own.
	if (PaidLineUntil > 0.0)
	{
		PaidLineUntil = 0.0;
		ClearTracker();
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

void UBountyBoardSubsystem::ShowPaidTracker()
{
	UWorld* World = GetWorld();
	UMissionSubsystem* Missions = World ? World->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (!Missions || TrackerId == INDEX_NONE)
	{
		ClearTracker();
		return;
	}
	FMissionTrackerParts Parts;
	Parts.StepCount = 2;
	Parts.Step = Parts.StepCount;
	Parts.bAnnouncedEnd = true;
	Parts.Line = LOCTEXT("TrackPaid", "Bounty paid").ToString();
	Missions->SetObjective(TrackerId, FText::FromString(Parts.Line), TOptional<FVector>(), Parts);
	// Long enough for the tracker's next tick to read it (a frame or two), short enough to be gone before it's noticed.
	PaidLineUntil = GetNow() + PaidLineSeconds;
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
	PaidLineUntil = 0.0;
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
	// The banner tells a bounty by its id ("Bounty_<serial>") and shows its purse (HudMissionCompleteWidgetBounty.cpp).
	Announced->Id = FName(*FString::Printf(TEXT("Bounty_%d"), Posting.Serial));
	Runner->OnMissionCompleted.Broadcast(*Announced, Given);
}

#undef LOCTEXT_NAMESPACE
