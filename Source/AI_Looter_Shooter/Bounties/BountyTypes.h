#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureTrait.h"
#include "BountyTypes.generated.h"

/**
 * How hard a bounty is (Docs/Work/2026-10-22-Next/BOUNTY.md): its mark's rank, its pushes, its twists and its pay. Saved
 * by number in FBountyPosting: new tiers only ever go at the end.
 */
UENUM(BlueprintType)
enum class EBountyTier : uint8
{
	/** A Gravebound mark, two pushes, one twist: the board's first, from Main 3. */
	Wanted,
	/** A Hallowed mark, three pushes, a twist and sometimes a pack twist: from Main 5. */
	Dangerous,
	/** An Ethereal mark, three pushes, two twists: from Main 6. */
	Deadly,
};

/**
 * What a bounty has on top of its mark's rank (its "twist"): a trait on the mark, or something about its posse. Saved by
 * number: new ones only ever go at the end.
 */
UENUM(BlueprintType)
enum class EBountyTwist : uint8
{
	/** The mark is Fleet (ECreatureTrait::Fleet). */
	Fleet,
	/** The mark is Warded. */
	Warded,
	/** The mark is Mending. */
	Mending,
	/** The mark is Rallying: its posse takes less and moves faster round it. */
	Rallying,
	/** The mark is Vengeful. */
	Vengeful,
	/** The mark bursts when it dies (Grave-burst). */
	GraveBurst,
	/** Its posse is bigger: one more Basic in every push. */
	BigPosse,
	/** Its posse runs Fleet. */
	SwiftPosse,
};

/** Where a posting is. Saved by number: new states only ever go at the end. */
UENUM(BlueprintType)
enum class EBountyState : uint8
{
	/** Up on the board, to take. */
	Posted,
	/** Taken: its fight waits at its arena. */
	Taken,
	/** Its mark is dead: the bounty waits to be paid at the board. */
	Won,
};

/**
 * One bounty on a board, as drawn and saved whole (BountyRules::Draw), so a load never rolls it again: its mark's kind,
 * name and traits, its tier and twists, its arena and that arena's levels, and where it is. An empty slot has Serial 0.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FBountyPosting
{
	GENERATED_BODY()

	/** Its number on its board, from 1 (its draw's seed with the board's id). 0: an empty slot. */
	UPROPERTY()
	int32 Serial = 0;

	/** The mark's kind (BountyRules::Kinds: "Unpaid", "Spider", "Spitter", "DeadMiner"). */
	UPROPERTY()
	FName Kind;

	UPROPERTY()
	EBountyTier Tier = EBountyTier::Wanted;

	/** Its arena's id (ABountyArena::ArenaId: "Scatter"). */
	UPROPERTY()
	FName Arena;

	/** The levels where its arena stands (its zone's, before ranks), as the board read them when it was posted. */
	UPROPERTY()
	int32 LevelLow = 1;

	UPROPERTY()
	int32 LevelHigh = 1;

	/** Which of its kind's names the mark has (BountyRules::MarkName). */
	UPROPERTY()
	int32 NameIndex = 0;

	/** Its twists, the mark's first. */
	UPROPERTY()
	TArray<EBountyTwist> Twists;

	/** Every trait the mark has: its twists' and its rank's rolled ones (set on it exactly, as it spawns). */
	UPROPERTY()
	TArray<ECreatureTrait> MarkTraits;

	UPROPERTY()
	EBountyState State = EBountyState::Posted;

	/** When it went up, in the session's time played (s). */
	UPROPERTY()
	double PostedAt = 0.0;

	bool IsPosting() const { return Serial > 0; }
	bool HasTwist(EBountyTwist Twist) const { return Twists.Contains(Twist); }
};

/**
 * A strongbox owed from a save made before the playtest of 2026-10-27, when a won bounty dropped one at its fight and it
 * waited there until opened. Nothing makes these now (the reward is handed over at the board as the bounty is paid); a
 * save that still has one gets its loot at the board on the next visit (UBountyBoardSubsystem::HandOverOwed). Kept for
 * those saves: never reuse the struct or the record's list for anything else.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FBountyChestOwed
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Serial = 0;

	UPROPERTY()
	FName Arena;

	UPROPERTY()
	EBountyTier Tier = EBountyTier::Wanted;
};

/**
 * A bounty board's record (FCampaignRecord::Bounties, by the board's id: "RansomsRest"): its slots' postings, when each
 * empty slot may be filled again, the next serial, its rotations, what the player has read, how many it has paid, and an
 * older save's strongboxes still owed. New within session version 4: a session from before has none, which reads as a
 * fresh board.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FBountyBoardRecord
{
	GENERATED_BODY()

	/** The board's slots, in order (an empty one has Serial 0). */
	UPROPERTY()
	TArray<FBountyPosting> Slots;

	/** Per slot: the time played from which an empty slot takes a new posting (s; 0: at once). */
	UPROPERTY()
	TArray<double> RefillAt;

	/** The serial the next posting gets. */
	UPROPERTY()
	int32 NextSerial = 1;

	/** How many times it has rotated, and when it last did (time played, s; negative: never posted yet). */
	UPROPERTY()
	int32 Rotation = 0;

	UPROPERTY()
	double RotatedAt = -1.0;

	/** The highest serial the player has read on the board (a posting above it is new: the board's ?). */
	UPROPERTY()
	int32 SeenUpTo = 0;

	/** Bounties paid in all. */
	UPROPERTY()
	int32 Paid = 0;

	/** An older save's strongboxes still owed (FBountyChestOwed): handed over at the board, then gone. Never added to now. */
	UPROPERTY()
	TArray<FBountyChestOwed> OwedChests;

	/** The posting taken or won (one at a time), its slot, or INDEX_NONE. */
	int32 FindActiveSlot() const
	{
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (Slots[Index].IsPosting() && Slots[Index].State != EBountyState::Posted)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/** The slot holding Serial, or INDEX_NONE. */
	int32 FindSerial(int32 Serial) const
	{
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (Serial > 0 && Slots[Index].Serial == Serial)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/** It has never posted anything (a fresh board: an older save, a new game). */
	bool IsFresh() const { return RotatedAt < 0.0; }
};
