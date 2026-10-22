#pragma once

#include "CoreMinimal.h"
#include "Bounties/BountyRules.h"
#include "Bounties/BountyTypes.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "BountyBoardSubsystem.generated.h"

class ABountyArena;
class ABountyBoard;
class ABountyChest;
class ABountyFight;
class APawn;
struct FCampaignRecord;
struct FMissionRewardsGiven;

/** The board's postings or a bounty's state changed (its screen draws again). */
DECLARE_MULTICAST_DELEGATE(FOnBountyBoardChanged);

/**
 * The bounty board's life in a level (Docs/Work/2026-10-22-Next/BOUNTY.md): its record (the session's, by the board's id:
 * FCampaignRecord::Bounties; in memory without one), its clock (the session's time played), and everything a bounty
 * does:
 *  - posting: a fresh board posts its slots (BountyRules::Draw over the arenas open now), every RotateMinutes of play
 *    the untaken postings come down and new ones go up, a slot paid out fills RefillMinutes of play later, an empty slot
 *    as soon as an arena is free (the story moved on);
 *  - taking one (one at a time): its fight (ABountyFight) made at its arena, now and on every level start while it's
 *    taken, and a mission tracker entry (UMissionSubsystem: tracked, the minimap's arrow);
 *  - giving it up: back on the board, its fight gone;
 *  - won as its mark dies (saved): a strongbox (ABountyChest) owed, dropped where the fight ends, stood at its arena on a
 *    later visit until it's opened;
 *  - paid at the board: experience and silver (BountyRules::ExperienceFor, PurseFor), the mission-complete banner.
 * A timer looks twice a second (the tests call Refresh themselves). The board (ABountyBoard) shows what it says; the
 * screen (UBountyBoardWidget) reads its cards and acts through it. One board per level.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UBountyBoardSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UBountyBoardSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Seconds between its looks. */
	static constexpr float RefreshSeconds = 0.5f;

	// --- The board ---

	void RegisterBoard(ABountyBoard* InBoard);
	void UnregisterBoard(ABountyBoard* InBoard);
	ABountyBoard* GetBoard() const { return Board.Get(); }

	/** Its board posts and opens: the board on (Looter.Bounty.Enabled) and its OpenWhen held. */
	bool IsBoardOpen(const ABountyBoard& InBoard) const;

	// --- What it shows ---

	/** The record as it stands (an empty one with no board or campaign). */
	const FBountyBoardRecord& GetRecord() const;

	/** The posting taken or won, or null. */
	const FBountyPosting* GetActive() const;

	/** The screen's cards, a slot each. */
	TArray<FBountyCardView> MakeCards() const;

	/** "New postings in 1 h 42 min". */
	FText DescribeNextRotation() const;

	/** What a posting pays now: the purse, the experience (at the player's level now), the strongbox's guns. */
	int32 GetPurse(const FBountyPosting& Posting) const;
	int64 GetExperience(const FBountyPosting& Posting) const;

	/** The taken bounty's fight while it's out, and a won one's strongbox while it stands. */
	ABountyFight* GetFight() const { return Fight.Get(); }
	ABountyChest* GetChest(int32 Serial) const;

	/** The bounty's strongbox is still owed: won and not looted yet (the board's paid line says whether it waits). */
	bool IsChestOwed(int32 Serial) const;

	/** An arena of the level by id (the open ones and the rest), or null. */
	ABountyArena* FindArena(FName ArenaId) const;

	FOnBountyBoardChanged OnChanged;

	// --- Acting (the screen, the console, the tests) ---

	/** Takes the posting in Slot: Posted, the board open, none taken. Its fight comes, it's tracked. */
	bool Take(int32 Slot);

	/** Gives the taken bounty up (not a won one): back on the board, its fight gone. */
	bool GiveUp();

	/**
	 * Pays the won bounty to Player (the one at the board): experience, silver, the banner; its slot fills later. False when
	 * none is won. OutGiven: what it gave.
	 */
	bool Collect(APawn* Player, FMissionRewardsGiven* OutGiven = nullptr);

	/** The player read the board: every posting up counts as seen. */
	void MarkRead();

	/** Looks now: posts, rotates and fills, the fight and the strongboxes, the board's look, the tracker (its timer does). */
	void Refresh();

	/** Rotates now (the console's Looter.Bounty.Post). */
	void RotateNow();

	/** Puts Posting in Slot (the console's Looter.Bounty.Make), replacing what's there unless it's taken or won; its serial is the next. */
	bool PostChosen(int32 Slot, FBountyPosting Posting);

	/** The taken bounty's mark dies (the console): killed if it's out, else the bounty is won as if it had. */
	bool WinNow();

	/** Forgets the board's record (the console): every posting, the fight, the strongboxes owed. */
	void Forget();

	/** The board's clock: the session's time played (s); a test's own clock when one is set. */
	double GetNow() const;
	void SetTestClock(double Seconds) { TestClock = Seconds; }

	/** Moves the test clock (or, in play, the board's own offset) on by Seconds: the console's Looter.Bounty.Clock. */
	void AdvanceClock(double Seconds);

	/** The hardest tier the story allows now. */
	EBountyTier GetHighestTier() const;

private:
	// --- The record and the story (BountyBoardSubsystem.cpp) ---
	FCampaignRecord* FindCampaign() const;
	FBountyBoardRecord& MutableRecord();
	FName GetBoardId() const;
	static void EnsureSlots(FBountyBoardRecord& Record);
	/** The arenas open now, by id. */
	TArray<FBountyArenaChoice> GatherOpenArenas() const;
	void LevelsAt(const ABountyArena& Arena, int32& OutLow, int32& OutHigh) const;
	bool FillSlot(FBountyBoardRecord& Record, int32 Slot, double Now);
	void Rotate(FBountyBoardRecord& Record, double Now);
	/** Something changed: saved soon, the board's look, the tracker, the screen. */
	void Changed(bool bSave);
	void SaveSoon() const;
	APawn* FindPlayer() const;
	int32 GetPlayerLevel() const;
	void HandleTimer();
	void HandleRunnerChanged();
	/** A new posting went up: a chime and a notice when the player is near the board. */
	void AnnouncePosted();

	// --- The fight, the strongboxes, the tracker (BountyBoardSubsystemFight.cpp) ---
	void EnsureFight();
	void DropFight();
	void HandleMarkDown(ABountyFight& InFight);
	void HandleFightDone(ABountyFight& InFight);
	/** The won bounty's state, as its mark's death or the fight's end says (both mean won). */
	void MarkWon(int32 Serial);
	void EnsureChests();
	ABountyChest* SpawnChest(const FBountyChestOwed& Owed, const FVector& Spot, float Yaw, bool bDrop);
	void HandleChestLooted(ABountyChest& Chest);
	void UpdateTracker();
	void ClearTracker();
	/** The mission-complete banner for a bounty paid (a side job's, its title the bounty's). */
	void Announce(const FBountyPosting& Posting, const FMissionRewardsGiven& Given);

	TWeakObjectPtr<ABountyBoard> Board;
	TWeakObjectPtr<ABountyFight> Fight;
	TMap<int32, TWeakObjectPtr<ABountyChest>> Chests;
	/** The record without a campaign (a test level with none). */
	FBountyBoardRecord LocalRecord;
	FName LastBoardId = TEXT("RansomsRest");
	int32 TrackerId = INDEX_NONE;
	FTimerHandle Timer;
	FDelegateHandle RunnerHandle;
	TOptional<double> TestClock;
	/** Seconds the console moved the clock on (play only; the session's time played is never changed). */
	double ClockOffset = 0.0;
	bool bMenuWorld = false;
	/** The story moved on since the last look: an empty slot waiting on an arena tries again. */
	bool bStoryChanged = true;
};
