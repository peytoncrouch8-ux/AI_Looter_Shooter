#pragma once

#include "CoreMinimal.h"
#include "Creatures/UnpaidFightRules.h"

struct FUnpaidMoveTuning;

/**
 * The Unpaid's varied fight (2026-10-29; the user's brief "Unpaid fights: variety and a AAA failed-lunge shriek"): the rules
 * as plain functions and structs, so the tests check them without a level. AUnpaidCreature plays them
 * (UnpaidCreatureVariety.cpp, UnpaidCreatureShriek.cpp); the pack's board lives in UUnpaidPackSubsystem.
 *
 * Why (the diagnosis, Docs/Pipeline/Enemies.md "Fight: varied moves"): every turn began on the ring at 4.8 m, inside the lunge's
 * 5.2 m start and outside the rake's 2.6 m, so the old two-way choice (rake within 2.6 m, else lunge) picked the lunge nearly
 * every time, and 4 Unpaid on 3 turns never waited long enough on the ring for a rally (2.5 s) or more than a moment's cinder.
 * Now each turn picks from every move by range and situation, weighted, each move with its own cooldown, never a third time
 * in a row, never a move a pack-mate is committing at the same player.
 */

/** Where an Unpaid is in its failed-lunge shriek. */
enum class EUnpaidShriekPhase : uint8
{
	None,
	/** Rearing back, head up, jaw dropping, the coal pulsing: shoot the coal now and it chokes on it. */
	Tell,
	/** The scream: the ring rolls out, the player is rattled, its pack rallied. */
	Scream,
	/** Hunched, panting, before it rejoins the fight. */
	Recover,
};

/** What an Unpaid's turn knows as it picks its move (UnpaidMoveRules::Weights). Distances in cm at size 1. */
struct AI_LOOTER_SHOOTER_API FUnpaidMoveContext
{
	/** Flat distance to its target, at size 1 (the creature's own divided by its size). */
	float Distance = 0.f;
	/** The kit's lunge start and rake reach (FUnpaidFightTuning LungeStart, RakeRange). */
	float LungeStart = 520.f;
	float RakeRange = 260.f;
	/** Its last two attacks (newest first): no move a third time in a row. */
	EUnpaidMove Last = EUnpaidMove::None;
	EUnpaidMove BeforeLast = EUnpaidMove::None;
	/** Seconds left of each move's own cooldown (indexed by EUnpaidMove). */
	float Cooldowns[NumUnpaidMoves] = {};
	/** The moves its pack-mates are committing at this player now (bits, UnpaidMoveRules::Bit): never two the same at once. */
	uint32 Taken = 0;
	/** A pack-mate comes at the player from the front now (a lunge, a rake...), or from their side or behind (a phase-strike, a flank). */
	bool bMateFront = false;
	bool bMateSide = false;
	/** Its pack's blink gate lets it phase-step now (Looter.Unpaid.PackStepGap). */
	bool bMayBlink = true;
	/** The player stands still or hides too long while 3+ fight them (the director's read). */
	bool bCamping = false;
	/** Its last lunge missed: a feint is likelier (it reads the dodge). */
	bool bLastLungeMissed = false;
	/** Others of its pack fighting this player now. */
	int32 Mates = 0;
	/** Picked as its attack begins (no plan before it): only moves that start from here, no hang-back, no closing in. */
	bool bNow = false;
	/**
	 * The player is open now (reloading, swapping guns, an empty gun, the flask at the lips: UnpaidOpeningRules, 2026-10-31): the
	 * quick commits weigh more, the feint less, no hang-back. bPunisher: it claimed the punish of that opening; bInFront: it is
	 * in front of the player's view (its punish blinks round to their back), else behind or at a side (it lunges).
	 */
	bool bOpening = false;
	bool bPunisher = false;
	bool bInFront = true;
};

/**
 * Which moves each of a pack is committing at one player now (UUnpaidPackSubsystem keeps one per player): a move from its plan
 * to its turn's end. Plain, so the tests check it with their own clock.
 */
struct AI_LOOTER_SHOOTER_API FUnpaidMoveBoard
{
	struct FEntry
	{
		uint32 Id = 0;
		EUnpaidMove Move = EUnpaidMove::None;
		double Since = 0.0;
	};

	/** Id commits Move now (None releases it). */
	void Commit(uint32 Id, EUnpaidMove Move, double Now);
	void Release(uint32 Id);
	/** The moves the others commit (bits), and whether any of them comes from the front, or from the player's side or back. */
	uint32 TakenBy(uint32 ExceptId) const;
	bool AnyFront(uint32 ExceptId) const;
	bool AnySide(uint32 ExceptId) const;
	/** Any of the others committed an attack at or after Time (world seconds): what a hang-back waits for. */
	bool AnyCommittedSince(uint32 ExceptId, double Time) const;
	EUnpaidMove MoveOf(uint32 Id) const;
	/** Forgets entries older than MaxAge (a creature gone without a word). */
	void Prune(double Now, float MaxAge);
	bool IsEmpty() const { return Entries.IsEmpty(); }

	TArray<FEntry> Entries;
};

/** An Unpaid's varied fight as it fights (AUnpaidCreature::Kit.Moves; UnpaidCreatureVariety.cpp, UnpaidCreatureShriek.cpp). */
struct AI_LOOTER_SHOOTER_API FUnpaidMoveState
{
	/** The move its turn chose before its attack began (a close one closes in first), and how long ago. */
	EUnpaidMove Planned = EUnpaidMove::None;
	float PlanTime = 0.f;
	/** Its last two attacks, newest first. */
	EUnpaidMove Last = EUnpaidMove::None;
	EUnpaidMove BeforeLast = EUnpaidMove::None;
	/** Seconds left of each move's cooldown. */
	float Cooldowns[NumUnpaidMoves] = {};
	/** What the pack's board holds for it (written only when it changes). */
	EUnpaidMove Told = EUnpaidMove::None;
	/** Its last lunge missed; this attack's miss has been counted (once an attack). */
	bool bLastLungeMissed = false;
	bool bMissNoted = false;
	/** A feint's or a low swipe's own flight speed (cm/s; 0: the lunge's). */
	float FlightSpeed = 0.f;
	/** A feint's slide aside after its flight: seconds left, its way, its speed. */
	float SlideLeft = 0.f;
	FVector SlideWay = FVector::ZeroVector;
	float SlideSpeed = 0.f;
	/** A phase-strike: its tell's whisper played once it's back; the arm it claws with (0 left, 1 right). */
	bool bStrikeTold = false;
	int32 StrikeArm = 1;
	/**
	 * A hang-back: seconds left, seconds in, the director's rest it last asked for, which way it circles (+1 / -1), and when it
	 * began (world seconds: a pack-mate's commit after it ends the wait).
	 */
	float HangLeft = 0.f;
	float HangTime = 0.f;
	float HangRest = 0.f;
	float HangWay = 1.f;
	double HangSince = 0.0;
	/** Rallied by a pack-mate's shriek: seconds left of its faster chase. */
	float RalliedLeft = 0.f;

	// --- The failed-lunge shriek ---
	EUnpaidShriekPhase Shriek = EUnpaidShriekPhase::None;
	float ShriekTime = 0.f;
	/** A missed lunge rolled a shriek: it comes as its turn ends. */
	bool bShriekOwed = false;
	/** Seconds since its last shriek (its own cooldown). */
	float SinceShriek = 1000.f;
	/** Its coal shot through the tell: exposed this long more (a bigger crit spot). */
	float ExposedLeft = 0.f;
	/** Its scream's pitch (its voice's, varied per Unpaid so a pack doesn't sound copied). */
	float ShriekPitch = 1.f;
	/** The scream's ring and the player's rattle went out (once a shriek). */
	bool bScreamed = false;
	/** Its hat lifted off its head by the scream, and where it sat (relative to its bone). */
	bool bHatLifted = false;
	FVector HatRestLocation = FVector::ZeroVector;
	FQuat HatRestTurn = FQuat::Identity;
	/** Its coal's light (UUnpaidPackSubsystem's pool; INDEX_NONE: none). */
	int32 Light = INDEX_NONE;
	/** Particles owed from earlier frames, so a low frame rate throws the same number a second. */
	float MotesOwed = 0.f;
	float WispsOwed = 0.f;
	float EmbersOwed = 0.f;
	float FrostOwed = 0.f;

	/** Back to calm (death, coming back). The light must have been given back first. */
	void Reset();
};

namespace UnpaidMoveRules
{
	/** No move a third time in a row (the brief: "never the same move three times in a row"). */
	inline constexpr int32 MaxRun = 2;

	inline uint32 Bit(EUnpaidMove Move) { return 1u << static_cast<uint32>(Move); }
	inline int32 Index(EUnpaidMove Move) { return static_cast<int32>(Move); }

	/** An attack (not None, not a hang-back). */
	AI_LOOTER_SHOOTER_API bool IsAttack(EUnpaidMove Move);
	/** It comes at the player from in front of them (a lunge, a feint, a rake, a low swipe): a phase-strike pairs with it. */
	AI_LOOTER_SHOOTER_API bool IsFrontal(EUnpaidMove Move);
	AI_LOOTER_SHOOTER_API const TCHAR* Name(EUnpaidMove Move);
	/** Move after Last and BeforeLast would be its third in a row. */
	AI_LOOTER_SHOOTER_API bool WouldRepeat(EUnpaidMove Move, EUnpaidMove Last, EUnpaidMove BeforeLast);
	/** How near (cm at size 1) a move begins from: a lunge, feint or phase-strike from the lunge's start, a rake from its reach, a low swipe from its own. */
	AI_LOOTER_SHOOTER_API float StartDistance(EUnpaidMove Move, const FUnpaidMoveContext& Context, const FUnpaidMoveTuning& Tuning);
	/** A close move (rake, low swipe) planned farther out walks in to its start first. */
	AI_LOOTER_SHOOTER_API bool ClosesIn(EUnpaidMove Move);

	/**
	 * Each move's weight now (Out, indexed by EUnpaidMove), 0 where it may not be picked: out of its range, cooling down, a
	 * pack-mate's at the same player now, or a third in a row. A phase-strike weighs more while a pack-mate presses from the
	 * front (one pressures, another flanks), the frontal moves more while one is at the player's side; a hang-back only while
	 * pack-mates fight and none is at the player's side (it waits for the next of them to commit).
	 */
	AI_LOOTER_SHOOTER_API void Weights(const FUnpaidMoveContext& Context, const FUnpaidMoveTuning& Tuning, float Out[NumUnpaidMoves]);

	/**
	 * The move Roll (0-1) picks by Weights. Nothing open: a hang-back while pack-mates fight (it waits for one to commit);
	 * alone, or picking as the attack begins, any move in range that is no third in a row and no pack-mate's (cooldowns
	 * waived), else the lunge.
	 */
	AI_LOOTER_SHOOTER_API EUnpaidMove Choose(const FUnpaidMoveContext& Context, const FUnpaidMoveTuning& Tuning, float Roll);

	/** A missed lunge turns into a shriek: Roll under Chance, its own last shriek Cooldown ago, its pack's PackGap ago. */
	AI_LOOTER_SHOOTER_API bool ShriekOnMiss(float Roll, float Chance, float SinceOwn, float Cooldown, float SincePack, float PackGap);
	/** The shriek's phase Time seconds in (None past its end). */
	AI_LOOTER_SHOOTER_API EUnpaidShriekPhase ShriekPhaseAt(float Time, float Tell, float Scream, float Recover);
	/** A hit interrupts it: its coal shot (bCoalHit) during the tell. Never once it screams: then it's committed. */
	AI_LOOTER_SHOOTER_API bool InterruptsShriek(EUnpaidShriekPhase Phase, bool bCoalHit);
	/** How hard the scream rattles a player Distance off (0-1): full within a third of Radius, a third of it at Radius, nothing past. */
	AI_LOOTER_SHOOTER_API float RattleShare(float Distance, float Radius);
	/** A low swipe passes under a player whose feet are Clear (cm) or more over its own, or who is in the air. */
	AI_LOOTER_SHOOTER_API bool ClearsLowSwipe(float FeetOverIts, bool bAirborne, float Clear);
}
