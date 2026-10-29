#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"

/**
 * A creature's place in a fight (UFightDirectorSubsystem): Melee kinds wait their turn on the ring round the player and come
 * in when they get one; Ranged kinds (the Spitter) hold a spot of their own out in their band; Support kinds (the
 * Bell-Ringer) keep back by their own rules. The same enum names the kind of turn an attack takes (a Melee or a Ranged token).
 */
enum class EFightRole : uint8
{
	Melee,
	Ranged,
	Support,
};

/**
 * One target's attack turns of one kind (Saved/Fight/CONTRACT.md, FIGHT-AI 1), as plain data keyed by the creatures' ids, so
 * the tests check it without a level. At most Max creatures hold a turn at once; one that asks while they're all held waits in
 * line, and the next free turn goes to whoever has waited longest (one that stops asking for ForgetAfter seconds leaves the
 * line). A creature that just attacked rests a while before it may ask again. A staggered holder gives its turn back but
 * keeps the head of the line, so a short stagger costs it nothing and a long knock lets the next one in. A kind family's turns
 * may be spaced (its melee gap, FIGHTS.md section 2: the slime pack's leaps come one after another): one held back by its
 * family's gap doesn't hold up another family's waiter behind it.
 */
struct AI_LOOTER_SHOOTER_API FFightTokens
{
	struct FHolder
	{
		uint32 Id = 0;
		double Since = 0.0;
	};
	struct FWaiter
	{
		uint32 Id = 0;
		double Since = 0.0;
		double LastAsk = 0.0;
		/** Its kind family and that family's gap, as it last asked. */
		FName Family;
		float FamilyGap = 0.f;
		/** Put back at the head of the line by a stagger: taking its turn back isn't a new turn, so no gap holds it. */
		bool bKept = false;
	};
	struct FRest
	{
		uint32 Id = 0;
		double Until = 0.0;
	};

	/**
	 * Id asks for a turn at Now: true when it holds one (it had one, or a turn was free and it has waited longest). While
	 * resting it isn't even put in line. MinGap: a turn is never given within this many seconds of the last one given.
	 * Family / FamilyGap (FightRules::FamilyOf, its melee gap): a turn is never given to one of Family within FamilyGap
	 * seconds of the last turn its family was given; meanwhile the next waiter of another family may have it.
	 */
	bool TryTake(uint32 Id, double Now, int32 Max, float ForgetAfter, float MinGap = 0.f, FName Family = NAME_None, float FamilyGap = 0.f);

	bool Holds(uint32 Id) const;
	/** When Id took its turn (negative when it holds none). */
	double HeldSince(uint32 Id) const;
	bool IsWaiting(uint32 Id) const;
	bool IsResting(uint32 Id, double Now) const;

	/** Gives Id's turn back (true if it held one) and rests it RestSeconds (0: no rest, it may ask again at once). */
	bool Release(uint32 Id, double Now, float RestSeconds);
	/** Gives Id's turn back but keeps it at the head of the line (a stagger). True if it held one. */
	bool ReleaseKeepTurn(uint32 Id, double Now);
	/** Id is gone (dead, removed): no turn, no place in line, no rest. */
	void Forget(uint32 Id);
	/** Drops those in line that haven't asked within ForgetAfter seconds, and rests that are over. */
	void Prune(double Now, float ForgetAfter);

	int32 NumHeld() const { return Holders.Num(); }
	int32 NumWaiting() const { return Waiters.Num(); }
	bool IsEmpty() const { return Holders.IsEmpty() && Waiters.IsEmpty() && Rests.IsEmpty(); }
	const TArray<FHolder>& GetHolders() const { return Holders; }

private:
	/** Whether Waiter's family may begin a turn at Now (no gap, a kept turn, or its gap passed). */
	bool IsFamilyOpen(const FWaiter& Waiter, double Now) const;

	TArray<FHolder> Holders;
	TArray<FWaiter> Waiters;
	TArray<FRest> Rests;
	double LastGiven = -1.0e9;
	/** When each kind family was last given a turn (only families with a gap are kept). */
	TMap<FName, double> FamilyLastGiven;
};

/**
 * How a fight is run, as plain functions the tests check without a level (UFightDirectorSubsystem and the creatures play them;
 * the numbers are FFightTuning's): who keeps their own fight, the scales, the surround ring's slots and feints, the ranged
 * kinds' spots, and when a held turn is given back.
 */
namespace FightRules
{
	// --- Who and how much ---

	/** Bosses and Legendary monsters keep their own fights: no turns, no ring, their own speed and damage. */
	AI_LOOTER_SHOOTER_API bool KeepsOwnFight(ECreatureRank Rank, bool bBossRules);

	/** Its chase speed now: ChaseSpeed, times Scale unless it keeps its own fight. */
	AI_LOOTER_SHOOTER_API float ChaseSpeedFor(float ChaseSpeed, bool bOwnFight, float Scale);

	/** What its damage is multiplied by: Scale, or 1 for a boss or Legendary monster. */
	AI_LOOTER_SHOOTER_API float DamageScaleFor(bool bOwnFight, float Scale);

	/** A creature class's kind family for its melee gap: its native class (a Blueprint child and spiderlings count as their kind). */
	AI_LOOTER_SHOOTER_API FName FamilyOf(const UClass* Class);

	// --- The ring ---

	/** Whether MeleeHunters on one target make a ring (at least MinPack of them); fewer just come in. */
	AI_LOOTER_SHOOTER_API bool UsesRing(int32 MeleeHunters, int32 MinPack);

	/** A creature's ring radius: the ring's, or Margin past its own attack reach if that's farther (a slime's leap). */
	AI_LOOTER_SHOOTER_API float RingReach(float RingRadius, float AttackRange, float Margin);

	/** How far off it takes its ring spot and asks for its turn: ApproachShare of its ring radius (the flank fan outside it). */
	AI_LOOTER_SHOOTER_API float ApproachDistance(float RingReach, float ApproachShare);

	/** Two bearings' angle apart (degrees, 0 to 180). */
	AI_LOOTER_SHOOTER_API float AngleApart(float A, float B);

	/** Slot's bearing round the target (degrees, -180 to 180): Turn plus its share of the circle. */
	AI_LOOTER_SHOOTER_API float SlotBearing(int32 Slot, int32 Slots, float Turn);

	/** How much a bearing round the player lies in front of their view (1 straight ahead, 0 straight behind). */
	AI_LOOTER_SHOOTER_API float Frontness(float Bearing, float ViewYaw);

	/** One waiting creature's ask for a slot: its bearing round the target now, its slot from the last look, the slots it may take (bits). */
	struct FRingAsk
	{
		float Bearing = 0.f;
		int32 CurrentSlot = INDEX_NONE;
		uint32 Allowed = MAX_uint32;
	};

	/** The ring now: its slots, how far it has turned, the player's view, the bias against the front, the keep bonus, the open slots (bits). */
	struct FRingLayout
	{
		int32 Slots = 8;
		float Turn = 0.f;
		float ViewYaw = 0.f;
		float BehindBias = 0.35f;
		float KeepBonus = 0.15f;
		uint32 Open = MAX_uint32;
	};

	/**
	 * Each ask's slot (INDEX_NONE: none it may take is open): the cheapest pairs first, a pair costing its angle from the
	 * creature's bearing (a half circle = 1), plus BehindBias times the slot's frontness, less KeepBonus for its own slot. More
	 * asks than open slots: the rest share, each taking the cheapest open slot with half a point added for each one already there.
	 */
	AI_LOOTER_SHOOTER_API TArray<int32> AssignRingSlots(const TArray<FRingAsk>& Asks, const FRingLayout& Layout);

	/** Its distance from the target during a feint Since seconds old (Seconds long): in to Depth of Radius and back, eased; Radius outside it. */
	AI_LOOTER_SHOOTER_API float FeintRadius(float Radius, float Since, float Seconds, float Depth);

	/**
	 * Which way (flat, unit) to walk from Here to Spot round Target without cutting past it: straight at the spot when it's
	 * nearly on the same bearing, else round the circle on the short side, closing on the spot's distance as it goes.
	 */
	AI_LOOTER_SHOOTER_API FVector OrbitWay(const FVector& Here, const FVector& Target, const FVector& Spot);

	// --- Ranged spots ---

	/** The circular mean of Bearings (degrees); bOutAny false when there are none (or they cancel out). */
	AI_LOOTER_SHOOTER_API float MeanBearing(TConstArrayView<float> Bearings, bool& bOutAny);

	/**
	 * Bearings round the target a ranged creature might take its spot at, best first. With a melee pack (MeleeBearing, its
	 * mean): OffAngle or more round from it on either side (its own bearing first if that's already far enough round), so its
	 * shots come from another side than the pack; with none, near its own. bMustMove (it just shot and moves on): 30 degrees
	 * or more from its own. Bearings within 40 degrees of another ranged one's spot (Others) come later.
	 */
	AI_LOOTER_SHOOTER_API TArray<float> RangedBearings(float Current, bool bHasMelee, float MeleeBearing, float OffAngle, bool bMustMove,
		TConstArrayView<float> Others);

	/** Its spot's distance from the target: where it is, kept inside its band (Near to Far) a little in from each edge. */
	AI_LOOTER_SHOOTER_API float RangedDistance(float Current, float Near, float Far);

	// --- A held turn's upkeep ---

	enum class ETokenCheck : uint8
	{
		/** Keeps its turn. */
		Keep,
		/** Gives it back (out of the fight, or gone out of reach), no rest. */
		Return,
		/** Staggered or knocked: gives it back, keeping the head of the line. */
		KeepTurn,
		/** Held too long: taken back, with the rest after an attack. */
		TakeBack,
	};

	/** What the director's look sees of a holder. */
	struct FHolderLook
	{
		/** Alive, after this target, chasing or attacking. */
		bool bFighting = true;
		/** Staggered or knocked (its brain held by a hit). */
		bool bHeldByHit = false;
		/** In its attack now (wind-up to recovery). */
		bool bAttacking = false;
		/** Its distance from the target (flat), and how far is out of reach (0: no such limit). */
		float Distance = 0.f;
		float LeaveDistance = 0.f;
		float HeldSeconds = 0.f;
	};

	AI_LOOTER_SHOOTER_API ETokenCheck CheckHolder(const FHolderLook& Look, float MaxSeconds);
}
