#pragma once

#include "CoreMinimal.h"
#include "Creatures/FightEvent.h"
#include "Creatures/FightPressureRules.h"
#include "Creatures/FightRules.h"
#include "Creatures/FightTuning.h"
#include "Creatures/UnpaidFightRules.h"
#include "Math/RandomStream.h"
#include "Subsystems/WorldSubsystem.h"
#include "FightDirectorSubsystem.generated.h"

class ACreatureBase;
class APawn;

/**
 * Runs the fights in a played world (Phase 12.2, FIGHT-AI; Saved/Fight/CONTRACT.md), one per world, no tick: the creatures
 * ask it from their brains (ACreatureBase's chase, CreatureBaseFight.cpp), and it looks at every fight again once
 * Looter.Fight.RingRefresh has passed, at the first ask after (each target's turns' upkeep, the ring's slots and feints, the
 * ranged kinds' spots: one walk of the level's creatures and at most two traces a ring slot and four ranged spots).
 *  - Turns: per target, Looter.Fight.MeleeTokens creatures may be in a melee attack at once and RangedTokens in a spit
 *    (FFightTokens: the longest waiter gets the next, a rest after an attack, a turn held too long taken back, a staggered
 *    one keeping the head of the line).
 *  - The ring: with RingMinPack or more melee creatures on one target, each holds a slot round it (RingRadius, or past its
 *    own reach), the slot nearest its approach and not in front of the player's view if it can help it; the ring turns
 *    slowly (RingDrift) and waiting ones feint in now and then. Slots that are walled off or have no ground are skipped.
 *  - Ranged spots: a spot in its band at RangedOffAngle or more round from the melee pack, with a clear line at the target,
 *    a new one after a shot now and then (RangedRelocate).
 *  - A pack's moments (the Unpaid's kit): a frenzy that lets one pack take more turns at once for a while after a rally
 *    scream, one rally, howl and cinder at a time per target, one phase-step or flank at a time per target, and each
 *    creature's own chase boost.
 *  - Pressure (FIGHTS.md section 2, 2026-10-28; FightDirectorPressure.cpp): a kind family's melee turns at one target spaced
 *    by its melee gap (GetMeleeGap), what it reads of each player (still, hidden, hurt: GetPlayerRead) and the close-call ease
 *    for one nearly dead, one leap at a target at a time (MayLeap), the packs' leaders (IsPackLed, the lull after one falls),
 *    and the fight's moments for the fight log (NoteEvent, OnFightEvent).
 * Bosses and Legendary monsters never ask (ACreatureBase::UsesFightDirector). Looter.Fight.Director 0 turns it all off (Get
 * returns none). It is made in game and PIE worlds only, and in test levels a test lets it (bAllowInTestWorlds).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UFightDirectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

protected:
	/** The base class turns editor-preview worlds away before ShouldCreateSubsystem asks: a test level lets them in. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** The director of WorldContext's world, or none (no such world, or Looter.Fight.Director 0). */
	static UFightDirectorSubsystem* Get(const UObject* WorldContext);

	/** Tests set it before they make a test level, so it gets a director too (Tests/FightTests.cpp). */
	static bool bAllowInTestWorlds;

	// --- Turns ---

	/** Creature asks for a turn (Role: a melee attack or a ranged one) at Target: true when it has one. */
	bool TryTakeToken(const ACreatureBase& Creature, const APawn& Target, EFightRole Role);
	/** It holds a turn now, at anyone. */
	bool HoldsToken(const ACreatureBase& Creature) const;
	/** Its attack played out: its turn back with the rest after an attack; a ranged one may move on to a new spot. */
	void EndTurn(const ACreatureBase& Creature);
	/** It left the fight (calm, searching, breaking off): its turn back, no rest. */
	void ReleaseToken(const ACreatureBase& Creature);
	/** Staggered or knocked: its turn back, keeping the head of the line (Looter.Fight.TurnKeep). */
	void GiveBackForHit(const ACreatureBase& Creature);
	/** Dead or removed: nothing of it kept. */
	void ForgetCreature(const ACreatureBase& Creature);
	/** It can't use a turn now (an Unpaid out of its target's sight): its turn back, and it isn't put in line for Seconds. */
	void RestFromTurns(const ACreatureBase& Creature, float Seconds);

	// --- Places ---

	/**
	 * Where it waits for its turn now, round Target: its ring slot (with its feint) for a melee one when the ring is up, its
	 * spot for a ranged one; false when it has none (a small pack, a support kind, no slot open): it comes in as before.
	 */
	bool GetWaitSpot(const ACreatureBase& Creature, const APawn& Target, FVector& OutSpot);
	/** A ranged one's spot is no good (its lob blocked there): a new one at the next look. */
	void RequestNewSpot(const ACreatureBase& Creature);
	/** Its ring radius (the ring's, or past its own reach), and how far off it takes its slot and asks for its turn. */
	float GetRingReach(const ACreatureBase& Creature) const;
	float GetApproachDistance(const ACreatureBase& Creature) const;

	// --- A pack's moments (the Unpaid's kit, UnpaidCreatureMoves.cpp; round 2, 2026-10-27) ---

	/** PackTag's pack goes for Target together: for Seconds its creatures may hold up to Tokens melee turns there (more than Looter.Fight.MeleeTokens). */
	void OpenFrenzy(const APawn& Target, FName PackTag, float Seconds, int32 Tokens);
	/** Seconds left of PackTag's frenzy at Target (0: none). */
	float GetFrenzyLeft(const APawn& Target, FName PackTag) const;
	/** One rally scream at Target in Cooldown seconds, whoever screams: true when this one may (it's counted as given). */
	bool TryStartRally(const APawn& Target, float Cooldown);
	/** One howl at Target in Cooldown seconds, whoever howls: true when this one may (counted). */
	bool TryHowl(const APawn& Target, float Cooldown);
	/** One cinder thrown at Target in Gap seconds, whoever throws: true when this one may (counted). */
	bool TryTakeCinder(const APawn& Target, float Gap);
	/**
	 * One phase-step or flank at Target every Gap seconds, whoever goes (FUnpaidStepGate; 2026-10-28: a pack blinked on one
	 * frame): whether Creature may go now. bOwed (a press step owed) holds the next opening for it. Counted only by NoteStep.
	 */
	bool MayStep(const ACreatureBase& Creature, const APawn& Target, float Gap, bool bOwed);
	/** Creature phase-stepped or flanked at Target now: the next one waits the gap. */
	void NoteStep(const ACreatureBase& Creature, const APawn& Target);
	/** Creatures of PackTag fighting Target at the last look at the fights. */
	int32 CountPack(const APawn& Target, FName PackTag) const;
	/** A creature's own chase boost on top of Looter.Fight.ChaseScale (the Unpaid's closing and frenzy pace); 1 unless set. */
	void SetChaseBoost(const ACreatureBase& Creature, float Boost);
	float GetChaseBoost(const ACreatureBase& Creature) const;

	// --- Pressure (FIGHTS.md section 2; FightDirectorPressure.cpp) ---

	/**
	 * The gap (s) between melee turns of Creature's kind family (its native class) at one target: its GetMeleeGap times
	 * Looter.Fight.MeleeGap (0 turns every gap off). A turn waits for it; other families' turns don't.
	 */
	float GetMeleeGapFor(const ACreatureBase& Creature);

	/** What the director read of Target at its last look (a fresh look first if one is due): a default read when it runs no fight there. */
	FFightPlayerRead GetPlayerRead(const APawn& Target);

	/** One leap (a pounce) at Target in Gap seconds, whoever leaps: whether Creature may go now. Counted only by NoteLeap. */
	bool MayLeap(const ACreatureBase& Creature, const APawn& Target, float Gap) const;
	/** Creature leapt at Target now: the next leap there waits the gap. */
	void NoteLeap(const ACreatureBase& Creature, const APawn& Target);

	/** Leader leads its pack now (UCreaturePackComponent::SetLeader): its packmates near it count as led. */
	void NoteLeader(const ACreatureBase& Leader);
	/** Member's pack has a living leader within Looter.Fight.LeaderReach of it (a leader counts its own pack as led). */
	bool IsPackLed(const ACreatureBase& Member) const;
	/** Leader fell (PackLeaderRules::OnLeaderDown): its pack's frenzy ends at once, and the lull after it begins. */
	void NoteLeaderDown(const ACreatureBase& Leader);
	/** Seconds since a leader of Member's pack fell within Looter.Fight.LeaderReach of it (a large number: none has). */
	float GetSinceLeaderDown(const ACreatureBase& Member) const;
	/** Within Looter.Fight.LeaderLull seconds of its leader's fall: no rally, no frenzy (the Unpaid's kit reads it). */
	bool IsInLeaderLull(const ACreatureBase& Member) const;

	/** A fight's moment (Creature's, Event, its number): counted and handed to OnFightEvent. */
	void NoteEvent(const ACreatureBase& Creature, EFightEvent Event, float Value = 0.f);
	/** NoteEvent at Creature's world's director, if it has one (Looter.Fight.Director 0: nothing). */
	static void Note(const ACreatureBase& Creature, EFightEvent Event, float Value = 0.f);
	/** Every NoteEvent (the fight log binds it; nothing is spent while nobody listens). */
	FOnFightEvent OnFightEvent;
	/** How many of Event were noted in this world. */
	int32 GetEventCount(EFightEvent Event) const;

	// --- Tests and tools ---

	/** Looks at every fight now, whenever it last did. */
	void RefreshNow();
	/** Turns held at Target now, of Role's kind (Melee or Ranged). */
	int32 CountTokens(const APawn& Target, EFightRole Role) const;
	/** Whether Target has a ring round it now, and Creature's slot on it (INDEX_NONE: none). */
	bool IsRingUp(const APawn& Target) const;
	int32 GetSlot(const ACreatureBase& Creature, const APawn& Target) const;

private:
	/** A creature's place in one target's fight. */
	struct FPlace
	{
		int32 Slot = INDEX_NONE;
		float Radius = 0.f;
		double FeintStart = -1.0e9;
		/** A ranged one's spot, kept round the target as it moves: its bearing and distance. */
		bool bHasSpot = false;
		bool bRelocate = false;
		float SpotBearing = 0.f;
		float SpotDistance = 0.f;
	};

	/** One target's fight. */
	struct FTargetFight
	{
		FFightTokens Melee;
		FFightTokens Ranged;
		TMap<uint32, FPlace> Places;
		float RingTurn = 0.f;
		int32 Slots = 8;
		bool bRing = false;
		/** A pack's frenzy: whose, until when, and its turns. */
		FName FrenzyPack;
		double FrenzyUntil = -1.0e9;
		int32 FrenzyTokens = 0;
		/** The last rally, howl and cinder at this target. */
		double LastRally = -1.0e9;
		double LastHowl = -1.0e9;
		double LastCinder = -1.0e9;
		/** The phase-steps and flanks at this target (one a gap, an owed one first). */
		FUnpaidStepGate Steps;
		/** Fighters by pack tag at the last look. */
		TMap<FName, int32> PackCounts;
		/** The player read (FightDirectorPressure.cpp): how long still and hidden, the close-call ease, the leaps' gate. */
		FFightStillWatch Still;
		FFightSightWatch Sight;
		FFightEaseWatch Ease;
		FFightLeapGate Leaps;
		FFightPlayerRead Read;
	};

	/** A leader's fall, kept a while for its pack's lull: whose pack (its tag, or its class without one), where, when. */
	struct FLeaderDown
	{
		FName PackTag;
		TWeakObjectPtr<const UClass> Class;
		FVector Where = FVector::ZeroVector;
		double Time = 0.0;
	};

	/** The melee turns at once for Creature at Fight now: Looter.Fight.MeleeTokens, or its pack's frenzy's if more; one fewer while the ease is on. */
	int32 MeleeTokensFor(const ACreatureBase& Creature, const FTargetFight& Fight) const;
	/** The ranged turns at once at Fight now: Looter.Fight.RangedTokens, one fewer while the ease is on. */
	int32 RangedTokensFor(const FTargetFight& Fight) const;
	/** Reads Target at a look (still, hidden, hurt, camping) and opens the close-call ease if it's due (FightDirectorPressure.cpp). */
	void ReadPlayer(const APawn& Target, FTargetFight& Fight, const TArray<ACreatureBase*>& Fighters, double Time);
	/** Whether any of Fighters (the first few: HiddenTraceMax) has a clear eye line at Target. */
	bool AnyFighterSees(const APawn& Target, const TArray<ACreatureBase*>& Fighters) const;

	/** The tuning for this frame (the cvars read once a frame). */
	const FFightTuning& Tuning();
	double Now() const;
	void RefreshIfDue();
	void Refresh(double Time);
	void RefreshTarget(const APawn& Target, FTargetFight& Fight, const TArray<ACreatureBase*>& Fighters, double Time, float Elapsed);
	void KeepUpTurns(const APawn& Target, FFightTokens& Tokens, bool bMelee, const TMap<uint32, ACreatureBase*>& ById, double Time);
	void PlaceRing(const APawn& Target, FTargetFight& Fight, const TArray<ACreatureBase*>& Melee, double Time);
	void PlaceRanged(const APawn& Target, FTargetFight& Fight, const TArray<ACreatureBase*>& Ranged, const TArray<ACreatureBase*>& Melee);
	/** The ring's slots that lie open (bits): a clear line from the target and ground near its own level. */
	uint32 OpenSlots(const APawn& Target, float Turn, int32 Slots, float Radius) const;
	/** A ranged spot that will do: ground under it near the target's level and a clear line from there at the target. */
	bool IsRangedSpotUsable(const APawn& Target, const FVector& Spot) const;

	TMap<TWeakObjectPtr<const APawn>, FTargetFight> Fights;
	/** Creatures' own chase boosts by id (SetChaseBoost; none means 1). */
	TMap<uint32, float> ChaseBoosts;
	/** The packs' leaders (NoteLeader), and the falls of the last minute (NoteLeaderDown). */
	TArray<TWeakObjectPtr<const ACreatureBase>> Leaders;
	TArray<FLeaderDown> LeaderDowns;
	/** NoteEvent's counts, by event. */
	int32 EventCounts[NumFightEvents] = {};
	double LastRefresh = -1.0e9;
	uint64 TuningFrame = MAX_uint64;
	FFightTuning FrameTuning;
	FRandomStream Rolls;
};
