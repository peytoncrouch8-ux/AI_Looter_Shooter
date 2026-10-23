// UBountyBoardSubsystem: what the screen and the console do to the board (rotating now, a chosen posting, forgetting,
// reading, taking, giving up, paying), its change notice and posting chime, and the cards.

#include "Bounties/BountyBoardSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bounties/BountyArena.h"
#include "Bounties/BountyBoard.h"
#include "Bounties/BountyChest.h"
#include "Bounties/BountyFight.h"
#include "Bounties/BountyTuning.h"
#include "Economy/WalletSubsystem.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionSubsystem.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "LooterBounty"

// ---------------------------------------------------------------------------
// The console's and the screen's doings
// ---------------------------------------------------------------------------

void UBountyBoardSubsystem::RotateNow()
{
	FBountyBoardRecord& Record = MutableRecord();
	Rotate(Record, GetNow());
	Changed(true);
	Refresh();
}

bool UBountyBoardSubsystem::PostChosen(int32 Slot, FBountyPosting Posting)
{
	FBountyBoardRecord& Record = MutableRecord();
	if (!Record.Slots.IsValidIndex(Slot) || Record.Slots[Slot].State != EBountyState::Posted || !BountyRules::FindKind(Posting.Kind))
	{
		return false;
	}
	Posting.Serial = FMath::Max(Record.NextSerial, 1);
	Posting.State = EBountyState::Posted;
	Posting.PostedAt = GetNow();
	if (const ABountyArena* Arena = FindArena(Posting.Arena))
	{
		LevelsAt(*Arena, Posting.LevelLow, Posting.LevelHigh);
	}
	if (Posting.MarkTraits.IsEmpty())
	{
		if (const FBountyKindInfo* Kind = BountyRules::FindKind(Posting.Kind))
		{
			Posting.MarkTraits = BountyRules::MarkTraitsFor(*Kind, Posting.Tier, Posting.Twists, BountyRules::SeedFor(GetBoardId(), Posting.Serial));
		}
	}
	Record.NextSerial = Posting.Serial + 1;
	if (Record.RotatedAt < 0.0)
	{
		Record.RotatedAt = GetNow();
	}
	Record.Slots[Slot] = Posting;
	Record.RefillAt[Slot] = 0.0;
	UE_LOG(LogLooter, Log, TEXT("Bounty board: %s put in slot %d."), *BountyRules::Describe(Posting), Slot + 1);
	Changed(true);
	return true;
}

void UBountyBoardSubsystem::Forget()
{
	DropFight();
	for (const TPair<int32, TWeakObjectPtr<ABountyChest>>& Each : Chests)
	{
		if (ABountyChest* Chest = Each.Value.Get())
		{
			Chest->Destroy();
		}
	}
	Chests.Reset();
	if (FCampaignRecord* Campaign = FindCampaign())
	{
		Campaign->Bounties.Remove(GetBoardId());
	}
	LocalRecord = FBountyBoardRecord();
	bStoryChanged = true;
	ClearTracker();
	Changed(true);
}

void UBountyBoardSubsystem::Changed(bool bSave)
{
	if (bSave)
	{
		SaveSoon();
	}
	if (ABountyBoard* Here = Board.Get())
	{
		Here->ShowRecord(GetRecord(), IsBoardOpen(*Here));
	}
	UpdateTracker();
	OnChanged.Broadcast();
}

void UBountyBoardSubsystem::AnnouncePosted()
{
	const ABountyBoard* Here = Board.Get();
	const APawn* Player = FindPlayer();
	const UWorld* World = GetWorld();
	if (!Here || !Player || !World || !World->IsGameWorld())
	{
		return;
	}
	if (FVector::Dist(Player->GetActorLocation(), Here->GetActorLocation()) > FBountyTuning::Current().PostedChimeRange)
	{
		return;
	}
	LooterSound::Play2D(this, LooterSoundCue::Bounty::BoardPosted);
	if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		Captions->Play({ FStoryLine::Make(FText::GetEmpty(), LOCTEXT("Posted", "A new bounty is up on the board.")) }, ECaptionPlay::Queue);
	}
}

void UBountyBoardSubsystem::MarkRead()
{
	FBountyBoardRecord& Record = MutableRecord();
	int32 Highest = Record.SeenUpTo;
	for (const FBountyPosting& Posting : Record.Slots)
	{
		Highest = FMath::Max(Highest, Posting.Serial);
	}
	if (Highest != Record.SeenUpTo)
	{
		Record.SeenUpTo = Highest;
		Changed(true);
	}
}

// ---------------------------------------------------------------------------
// Taking, giving up, paying
// ---------------------------------------------------------------------------

bool UBountyBoardSubsystem::Take(int32 Slot)
{
	const ABountyBoard* Here = Board.Get();
	FBountyBoardRecord& Record = MutableRecord();
	if (!Here || !IsBoardOpen(*Here) || !Record.Slots.IsValidIndex(Slot) || !Record.Slots[Slot].IsPosting()
		|| Record.Slots[Slot].State != EBountyState::Posted || Record.FindActiveSlot() != INDEX_NONE)
	{
		return false;
	}
	FBountyPosting& Posting = Record.Slots[Slot];
	Posting.State = EBountyState::Taken;
	Record.SeenUpTo = FMath::Max(Record.SeenUpTo, Posting.Serial);
	UE_LOG(LogLooter, Log, TEXT("Bounty board: took %s."), *BountyRules::Describe(Posting));
	LooterSound::Play2D(this, LooterSoundCue::Bounty::Take);
	EnsureFight();
	// Tracked at once: the minimap's arrow points the way.
	UpdateTracker();
	if (UWorld* World = GetWorld())
	{
		if (UMissionSubsystem* Missions = World->GetSubsystem<UMissionSubsystem>(); Missions && TrackerId != INDEX_NONE)
		{
			Missions->TrackMission(TrackerId);
		}
	}
	Changed(true);
	return true;
}

bool UBountyBoardSubsystem::GiveUp()
{
	FBountyBoardRecord& Record = MutableRecord();
	const int32 Slot = Record.FindActiveSlot();
	if (Slot == INDEX_NONE || Record.Slots[Slot].State != EBountyState::Taken)
	{
		return false;
	}
	Record.Slots[Slot].State = EBountyState::Posted;
	UE_LOG(LogLooter, Log, TEXT("Bounty board: gave up %s."), *BountyRules::Describe(Record.Slots[Slot]));
	DropFight();
	ClearTracker();
	LooterSound::Play2D(this, LooterSoundCue::Bounty::Abandon);
	Changed(true);
	return true;
}

int32 UBountyBoardSubsystem::GetPurse(const FBountyPosting& Posting) const
{
	return BountyRules::PurseFor(Posting.Tier, Posting.LevelHigh, FBountyTuning::Current());
}

int64 UBountyBoardSubsystem::GetExperience(const FBountyPosting& Posting) const
{
	return BountyRules::ExperienceFor(Posting.Tier, Posting.LevelHigh, GetPlayerLevel(), UPlayerProgressionSubsystem::GetCurve(),
		UPlayerProgressionSubsystem::GetLevelRules(), FBountyTuning::Current());
}

bool UBountyBoardSubsystem::Collect(APawn* Player, FMissionRewardsGiven* OutGiven)
{
	FBountyBoardRecord& Record = MutableRecord();
	const int32 Slot = Record.FindActiveSlot();
	if (Slot == INDEX_NONE || Record.Slots[Slot].State != EBountyState::Won)
	{
		return false;
	}
	const FBountyPosting Paid = Record.Slots[Slot];
	FMissionRewardsGiven Given;

	// The experience at the player's level now (the falloff past the arena's), into their progress.
	const APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	const ULocalPlayer* Local = PC ? PC->GetLocalPlayer() : nullptr;
	UPlayerProgressionSubsystem* Progression = Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	const int32 PlayerLevel = Progression ? Progression->GetLevel() : GetPlayerLevel();
	Given.Experience = BountyRules::ExperienceFor(Paid.Tier, Paid.LevelHigh, PlayerLevel, UPlayerProgressionSubsystem::GetCurve(),
		UPlayerProgressionSubsystem::GetLevelRules(), FBountyTuning::Current());
	if (Progression && Given.Experience > 0)
	{
		Progression->AddXP(Given.Experience, EXPSource::Mission);
	}
	// The purse from the town's, into the wallet as a reward.
	const int32 Purse = GetPurse(Paid);
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(this);
	Given.Silver = Wallet ? Wallet->Add(Purse, ESilverChange::Reward) : 0;

	// The slot fills again later; the strongbox (if it's still shut) stays owed.
	Record.Slots[Slot] = FBountyPosting();
	Record.RefillAt[Slot] = GetNow() + FBountyTuning::Current().RefillMinutes * 60.0;
	++Record.Paid;
	UE_LOG(LogLooter, Log, TEXT("Bounty board: paid %s: +%lld XP, +%d silver (bounty %d paid here)."), *BountyRules::Describe(Paid),
		Given.Experience, Given.Silver, Record.Paid);
	DropFight();
	ShowPaidTracker();
	LooterSound::Play2D(this, LooterSoundCue::Bounty::Paid);
	Announce(Paid, Given);
	if (OutGiven)
	{
		*OutGiven = Given;
	}
	Changed(true);
	return true;
}

// ---------------------------------------------------------------------------
// The cards
// ---------------------------------------------------------------------------

TArray<FBountyCardView> UBountyBoardSubsystem::MakeCards() const
{
	TArray<FBountyCardView> Cards;
	const FBountyBoardRecord& Record = GetRecord();
	const bool bAnyActive = Record.FindActiveSlot() != INDEX_NONE;
	const FBountyTuning Tuning = FBountyTuning::Current();
	const double Now = GetNow();
	for (int32 Slot = 0; Slot < BountyRules::SlotCount; ++Slot)
	{
		const FBountyPosting* Posting = Record.Slots.IsValidIndex(Slot) ? &Record.Slots[Slot] : nullptr;
		FBountyCardInput In;
		In.Posting = Posting;
		if (Posting && Posting->IsPosting())
		{
			const ABountyArena* Arena = FindArena(Posting->Arena);
			In.ArenaName = Arena ? Arena->GetDisplayName() : FText::FromName(Posting->Arena);
			In.bOtherTaken = bAnyActive && Posting->State == EBountyState::Posted;
			In.bSeen = Posting->Serial <= Record.SeenUpTo;
			In.Purse = GetPurse(*Posting);
			In.Experience = GetExperience(*Posting);
			In.Guns = Tuning.GunsFor(Posting->Tier);
		}
		else
		{
			In.RefillIn = Record.RefillAt.IsValidIndex(Slot) && Record.RefillAt[Slot] > 0.0 ? Record.RefillAt[Slot] - Now : 0.0;
		}
		Cards.Add(BountyRules::MakeCard(In));
	}
	return Cards;
}

FText UBountyBoardSubsystem::DescribeNextRotation() const
{
	const FBountyBoardRecord& Record = GetRecord();
	const double In = BountyRules::RotationIn(Record.RotatedAt, GetNow(), FBountyTuning::Current().RotateMinutes);
	return FText::Format(LOCTEXT("NextRotation", "New postings in {0}"), BountyRules::DescribeWait(In));
}

#undef LOCTEXT_NAMESPACE
