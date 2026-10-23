#pragma once

#include "CoreMinimal.h"
#include "Bounties/BountyTypes.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureTrait.h"
#include "Creatures/EncounterGroup.h"
#include "Templates/SubclassOf.h"

class ACreatureBase;
struct FBountyTuning;
struct FChestLootOverride;
struct FLevelRules;
struct FXPCurve;

/** A kind of mark a bounty can name (BountyRules::Kinds): the creature, its posse and its support, how they arrive. */
struct AI_LOOTER_SHOOTER_API FBountyKindInfo
{
	/** Its id in postings and arenas ("Unpaid", "Spider", "Spitter", "DeadMiner"). */
	FName Id;
	/** The mark's class, its posse's, and the support that joins from the second push (a Bell-Ringer, a Venom Spitter). */
	TSubclassOf<ACreatureBase> Mark;
	TSubclassOf<ACreatureBase> Posse;
	TSubclassOf<ACreatureBase> Support;
	/** Its plain name after the rank's word on a card ("Gravebound Unpaid", "Hallowed spider"). */
	FText Plain;
	/** Its posse rises out of the ground (the dead), rather than coming their own way or dropping. */
	bool bRises = false;
};

/** A tier's numbers that aren't feel numbers (those are FBountyTuning's): its mark's rank, pushes, twists, odds. */
struct AI_LOOTER_SHOOTER_API FBountyTierInfo
{
	ECreatureRank MarkRank = ECreatureRank::Epic;
	/** Pushes in its fight; the mark comes with the last. */
	int32 Pushes = 2;
	/** Twists on the mark for sure. */
	int32 MarkTwists = 1;
	/** The chance of one more twist, and the share of that one that's a pack twist rather than another on the mark. */
	float ExtraTwistChance = 0.f;
	float ExtraIsPackShare = 1.f;
	/** Its weight among the tiers open when a posting is drawn. */
	float Weight = 1.f;
};

/** One arena a posting may be drawn for: its id, the kinds it hosts, its levels. */
struct AI_LOOTER_SHOOTER_API FBountyArenaChoice
{
	FName Id;
	TArray<FName> Kinds;
	int32 LevelLow = 1;
	int32 LevelHigh = 1;
};

/** What a posting is drawn from (BountyRules::Draw). */
struct AI_LOOTER_SHOOTER_API FBountyDrawInput
{
	/** The arenas open now (their story condition holds), in a fixed order (by id), so a draw is the same on every load. */
	TArray<FBountyArenaChoice> Arenas;
	/**
	 * Arenas the other slots and the taken bounty use, once for each posting (a repeated id counts twice). An arena another
	 * posting uses is drawn only when every arena has as many; one with PerArena postings is never drawn.
	 */
	TArray<FName> BusyArenas;
	/** Postings one arena holds at once (only one bounty is taken at a time, so they never stack two fights). */
	int32 PerArena = 3;
	/** Names up already, as "<Kind>.<Index>" (BountyRules::NameKey): never drawn twice at once. */
	TArray<FName> BusyNames;
	/**
	 * The other postings' mixes, as "<Arena>.<Kind>.<Tier>" (BountyRules::MixKey): a posting is drawn with a mix none of
	 * them has when its arena allows one (the board mixes tiers and kinds), and repeats one only when every mix is up.
	 */
	TArray<FName> BusyMixes;
	/** The board has no posting of the highest open tier yet and this is its last slot to fill: this one is of it. */
	bool bNeedHighest = false;
	/** The hardest tier the story allows now. */
	EBountyTier HighestTier = EBountyTier::Wanted;
	/** Its seed (BountyRules::SeedFor: the board's id and the serial) and its serial. */
	int32 Seed = 0;
	int32 Serial = 0;
	/** When it goes up (time played, s). */
	double Now = 0.0;
};

/** A bounty's fight as groups for its ABountyFight (BountyRules::MakeGroups). */
struct AI_LOOTER_SHOOTER_API FBountyFightPlan
{
	TArray<FEncounterGroup> Groups;
	int32 Pushes = 2;
	ECreatureRank MarkRank = ECreatureRank::Epic;
	TSubclassOf<ACreatureBase> Mark;
	/** Creatures in all, the mark among them. */
	int32 Total = 0;
};

/** One card of the board's screen, in words (BountyRules::MakeCard; UBountyBoardWidget draws it). */
struct AI_LOOTER_SHOOTER_API FBountyCardView
{
	bool bEmpty = true;
	EBountyTier Tier = EBountyTier::Wanted;
	EBountyState State = EBountyState::Posted;
	/** The mark's name ("Old Stilts"). */
	FText Title;
	/** "Wanted", "Dangerous", "Deadly". */
	FText TierWord;
	/** "Gravebound Unpaid · level 4-5". */
	FText KindLine;
	/** The mark's traits: "Warded · Fleet". */
	FText Traits;
	/** Its pack twist in a line ("Big posse: one more in every push"); empty: none. */
	FText Posse;
	FText Flavour;
	/** "Last seen: the Scatter". */
	FText Where;
	/** "Strongbox, 2 guns or wards · +41 silver · +242 XP". */
	FText Reward;
	/** "TAKEN", "CLAIM IT", "NEW"; empty for a posting read before. */
	FText StateWord;
	/** An empty card's line: "Next posting in 7 min", "More bounties as the Reaches open up". */
	FText EmptyLine;
	bool bNew = false;
	bool bCanTake = false;
	bool bCanCollect = false;
	bool bCanGiveUp = false;
};

/** What a card is made from (BountyRules::MakeCard). */
struct AI_LOOTER_SHOOTER_API FBountyCardInput
{
	const FBountyPosting* Posting = nullptr;
	/** Its arena's name ("the Scatter"). */
	FText ArenaName;
	/** Another bounty is taken (this one can't be). */
	bool bOtherTaken = false;
	/** Read on the board before (no NEW). */
	bool bSeen = false;
	/** The purse and the experience it pays (at the player's level now), and its strongbox's guns. */
	int32 Purse = 0;
	int64 Experience = 0;
	int32 Guns = 0;
	/** An empty slot: seconds until it may fill (0 or less: it fills when an arena is free). */
	double RefillIn = 0.0;
};

/**
 * The bounty board's rules as plain functions (Docs/Work/2026-10-22-Next/BOUNTY.md; UBountyBoardSubsystem plays them,
 * Looter.Bounty.* tests check them without a level): the kinds and tiers, the draw, the timer, the fight's groups, the
 * pay, and the words.
 */
namespace BountyRules
{
	/** Slots on a board. */
	inline constexpr int32 SlotCount = 3;

	/** Every kind, in a fixed order; Find: by id, or null. */
	AI_LOOTER_SHOOTER_API TConstArrayView<FBountyKindInfo> Kinds();
	AI_LOOTER_SHOOTER_API const FBountyKindInfo* FindKind(FName Id);

	AI_LOOTER_SHOOTER_API const FBountyTierInfo& GetTier(EBountyTier Tier);

	/** A twist's trait on the mark, or none for a pack twist (Big posse, Swift posse). */
	AI_LOOTER_SHOOTER_API TOptional<ECreatureTrait> TraitOf(EBountyTwist Twist);
	AI_LOOTER_SHOOTER_API bool IsPackTwist(EBountyTwist Twist);

	/** Whether Twist fits a bounty whose mark is of Kind (CreatureTraitRules::Fits on the mark or its posse). */
	AI_LOOTER_SHOOTER_API bool TwistFits(EBountyTwist Twist, const FBountyKindInfo& Kind);

	/** The draw's seed for a board's serial: the same board and serial always draw the same posting. */
	AI_LOOTER_SHOOTER_API int32 SeedFor(FName BoardId, int32 Serial);

	/** A name's key in FBountyDrawInput::BusyNames. */
	AI_LOOTER_SHOOTER_API FName NameKey(FName Kind, int32 NameIndex);

	/** A posting's mix in FBountyDrawInput::BusyMixes: its arena, kind and tier. */
	AI_LOOTER_SHOOTER_API FName MixKey(FName Arena, FName Kind, EBountyTier Tier);

	/**
	 * A posting from In: an arena not busy (or none: an empty posting, Serial 0), a kind it hosts, a tier up to
	 * HighestTier by weight, its twists (fitting the kind), the mark's traits (its twists', then its rank's TraitCount
	 * more from the seed), a name not up yet. The same input always gives the same posting.
	 */
	AI_LOOTER_SHOOTER_API FBountyPosting Draw(const FBountyDrawInput& In);

	/** The mark's traits for a posting of Kind at Tier with Twists, from Seed (Draw's; the tests'). */
	AI_LOOTER_SHOOTER_API TArray<ECreatureTrait> MarkTraitsFor(const FBountyKindInfo& Kind, EBountyTier Tier,
		TConstArrayView<EBountyTwist> Twists, int32 Seed);

	/** Its fight: the posse and support by push, the mark with the last push (its group first in that push). */
	AI_LOOTER_SHOOTER_API FBountyFightPlan MakeGroups(const FBountyPosting& Posting);

	// --- The timer (time played, s) ---

	/** A rotation is due: never rotated, or RotateMinutes of play since the last. */
	AI_LOOTER_SHOOTER_API bool IsRotationDue(double RotatedAt, double Now, float RotateMinutes);

	/** Seconds until the next rotation (0 when due). */
	AI_LOOTER_SHOOTER_API double RotationIn(double RotatedAt, double Now, float RotateMinutes);

	/** "1 h 42 min", "7 min", "under a minute". */
	AI_LOOTER_SHOOTER_API FText DescribeWait(double Seconds);

	// --- The pay ---

	/** The purse at the board: the tier's at level 1, grown with Level (the arena's top) as the economy grows. */
	AI_LOOTER_SHOOTER_API int32 PurseFor(EBountyTier Tier, int32 Level, const FBountyTuning& Tuning);

	/**
	 * The experience at the board: the tier's share of the step from ArenaLevel to the next, times the kills' falloff for
	 * a player above it (FLevelRules::KillXPShare), rounded; 0 for a player at the top level.
	 */
	AI_LOOTER_SHOOTER_API int64 ExperienceFor(EBountyTier Tier, int32 ArenaLevel, int32 PlayerLevel, const FXPCurve& Curve,
		const FLevelRules& Rules, const FBountyTuning& Tuning);

	/** The strongbox's loot: the tier's guns at the tuning's luck, its ammo pickups. */
	AI_LOOTER_SHOOTER_API FChestLootOverride ChestLoot(EBountyTier Tier, const FBountyTuning& Tuning);

	// --- Words (BountyRulesWords.cpp) ---

	AI_LOOTER_SHOOTER_API FText TierWord(EBountyTier Tier);
	AI_LOOTER_SHOOTER_API FText TwistWord(EBountyTwist Twist);
	/** A pack twist's line on a card ("Big posse: one more in every push"). */
	AI_LOOTER_SHOOTER_API FText PackLine(EBountyTwist Twist);

	/** How many names Kind has, and the Index-th (wrapped); "the mark" for an unknown kind. */
	AI_LOOTER_SHOOTER_API int32 NumNames(FName Kind);
	AI_LOOTER_SHOOTER_API FText MarkName(FName Kind, int32 Index);

	/** A flavour line for a posting (by its kind and serial). */
	AI_LOOTER_SHOOTER_API FText Flavour(FName Kind, int32 Serial);

	/** "Gravebound Unpaid · level 4-5": its mark's rank word, its kind's plain name, the mark's levels. */
	AI_LOOTER_SHOOTER_API FText KindLine(const FBountyPosting& Posting);

	/** The mark's traits in a line ("Warded · Fleet"), or "No tricks". */
	AI_LOOTER_SHOOTER_API FText TraitsLine(const FBountyPosting& Posting);

	/** "Bounty: Old Stilts": its title on the tracker and the banner. */
	AI_LOOTER_SHOOTER_API FText TitleFor(const FBountyPosting& Posting);

	/** Hob's lines at its beats: the first push (by kind), the mark's push, the mark down, the fight over. */
	AI_LOOTER_SHOOTER_API FText PushLine(const FBountyPosting& Posting);
	AI_LOOTER_SHOOTER_API FText MarkArriveLine(const FBountyPosting& Posting);
	AI_LOOTER_SHOOTER_API FText MarkDownLine(const FBountyPosting& Posting);
	AI_LOOTER_SHOOTER_API FText ClearedLine(const FBountyPosting& Posting);

	/** Hob, who says them. */
	AI_LOOTER_SHOOTER_API FText HobName();

	/** A card of the board's screen. */
	AI_LOOTER_SHOOTER_API FBountyCardView MakeCard(const FBountyCardInput& In);

	/**
	 * What the strongbox holds, in words: "Strongbox, 2 guns or wards". A Strongbox's table drops wards (ULootTable::
	 * bDropsShields), so any of its gun picks may come as a ward: the card never promises guns alone.
	 */
	AI_LOOTER_SHOOTER_API FText StrongboxWords(int32 Guns);

	/**
	 * The board's line once a bounty is paid: "Paid: +242 XP · +41 silver.", and "Your strongbox waits where it fell." only
	 * while its strongbox is still owed (not looted yet).
	 */
	AI_LOOTER_SHOOTER_API FText PaidLine(int64 Experience, int32 Silver, bool bStrongboxWaits);

	/** A posting for logs: "#4 Wanted Unpaid 'Hatless Gideon' at Scatter (2-3), twists Fleet, traits Fleet, Mending: Taken". */
	AI_LOOTER_SHOOTER_API FString Describe(const FBountyPosting& Posting);

	/** Names in code, for logs and commands, and back (any case); false when it's none. */
	AI_LOOTER_SHOOTER_API FString TierName(EBountyTier Tier);
	AI_LOOTER_SHOOTER_API FString TwistName(EBountyTwist Twist);
	AI_LOOTER_SHOOTER_API FString StateName(EBountyState State);
	AI_LOOTER_SHOOTER_API bool ParseTier(const FString& Text, EBountyTier& OutTier);
	AI_LOOTER_SHOOTER_API bool ParseTwist(const FString& Text, EBountyTwist& OutTwist);
}
