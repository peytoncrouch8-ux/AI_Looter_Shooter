#pragma once

#include "CoreMinimal.h"
#include "Creatures/FightRules.h"
#include "Creatures/FightTuning.h"
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
	};

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
	double LastRefresh = -1.0e9;
	uint64 TuningFrame = MAX_uint64;
	FFightTuning FrameTuning;
	FRandomStream Rolls;
};
