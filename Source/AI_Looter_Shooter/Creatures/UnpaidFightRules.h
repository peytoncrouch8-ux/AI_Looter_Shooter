#pragma once

#include "CoreMinimal.h"

/** The attack an Unpaid chose as its wind-up began (its kit, UnpaidCreatureMoves.cpp): none outside an attack, or the old kit's. */
enum class EUnpaidMove : uint8
{
	None,
	/** The shriek, then a flight at where its target will be. */
	Lunge,
	/** Close in: an arm drawn back, then two swipes across its front, one arm and then the other. */
	Rake,
};

/** What an Unpaid is doing between its turns, on the ring (no attack turn needed). */
enum class EUnpaidAction : uint8
{
	None,
	/** The coal swelling at its chest, then a cinder thrown at the player. */
	Cinder,
	/** Screaming its pack on: they come together once it ends. */
	Rally,
};

/** An Unpaid's kit as it fights (AUnpaidCreature::Kit): what it's doing, its clocks, and what it has worked out once. */
struct FUnpaidKitState
{
	EUnpaidMove Move = EUnpaidMove::None;
	EUnpaidAction Action = EUnpaidAction::None;
	/** Seconds into its Action. */
	float ActionTime = 0.f;
	/** Its rake's second swipe is done; its cinder has left its hand. */
	bool bSecondSwipe = false;
	bool bThrown = false;
	/** How far its lunge may fly this time (cm, at its size): the kit's reach, read as it struck. */
	float LungeReach = 0.f;
	/** Which arm swipes first (0 left, 1 right). */
	int32 LeadArm = 1;
	/** Seconds it has been waiting on the ring this time (0 off it). */
	float WaitTime = 0.f;
	/** Seconds until it may throw, or flank, again. */
	float CinderCooldown = 0.f;
	float FlankCooldown = 0.f;
	/** Its turn's flank has been rolled for (once a turn). */
	bool bFlankRolled = false;
	/** Seconds to its next look at its choices on the ring (not every frame). */
	float ThinkIn = 0.f;
	/** The chase boost it last told the fight director (written only when it changes). */
	float ToldBoost = 1.f;
	/** Whether it is a boss's add (they keep the old kit, so a boss fight's pacing is the boss's): -1 not looked yet. */
	int8 BossAdd = -1;
	/** The class's (or level's) recovery, before a move sets its own (a lunge's, captured with the wind-up as its rank applies). */
	float BaseRecovery = 0.7f;

	/** Back to calm (a death, coming back): its clocks start over; its rest timings stay (its rank's, captured once). */
	void Reset()
	{
		Move = EUnpaidMove::None;
		Action = EUnpaidAction::None;
		ActionTime = 0.f;
		bSecondSwipe = false;
		bThrown = false;
		LungeReach = 0.f;
		ToldBoost = 1.f;
		WaitTime = 0.f;
		CinderCooldown = 0.f;
		FlankCooldown = 0.f;
		bFlankRolled = false;
		ThinkIn = 0.f;
		BossAdd = -1;
	}
};

/** The kit's rules as plain functions (the tests check them without a level; AUnpaidCreature plays them). Distances in cm. */
namespace UnpaidFightRules
{
	/**
	 * Where to aim at a moving target: where they stand plus their flat run for the time something flying at Speed takes to
	 * reach them (at most MaxLeadSeconds), times LeadShare (0: where they stand; 1: a perfect lead). Up and down is ignored
	 * (a jump is no run): the point keeps their height.
	 */
	AI_LOOTER_SHOOTER_API FVector LeadPoint(const FVector& From, const FVector& TargetAt, const FVector& TargetVelocity, float Speed,
		float LeadShare, float MaxLeadSeconds);

	/** Its attack at Distance from its target: a rake when it's within RakeRange and Roll (0-1) comes under RakeChance, else a lunge. */
	AI_LOOTER_SHOOTER_API EUnpaidMove ChooseMove(float Distance, float RakeRange, float RakeChance, float Roll);

	/**
	 * Whether a swipe from Here, facing Facing (flat), catches Victim: within Radius flat, within the arc ArcDegrees wide
	 * across its front, and within UpDown above or below.
	 */
	AI_LOOTER_SHOOTER_API bool InSwipe(const FVector& Here, const FVector& Facing, const FVector& Victim, float Radius, float ArcDegrees,
		float UpDown);

	/** Direction turned toward Wanted (both flat) by at most MaxDegrees; a unit vector. */
	AI_LOOTER_SHOOTER_API FVector TurnToward(const FVector& Direction, const FVector& Wanted, float MaxDegrees);

	/**
	 * Where round the player a flank may come back, as bearings (degrees, world), best first: at their side and behind them
	 * (ViewYaw: the way they look), the side nearer its own bearing CreatureBearing first, wide sides before straight behind,
	 * the near-front sides last.
	 */
	AI_LOOTER_SHOOTER_API TArray<float> FlankBearings(float ViewYaw, float CreatureBearing);

	/** Whether its turn should open with a flank: it's in front of the player's eyes (Frontness at least Front) and between MinRange and MaxRange off. */
	AI_LOOTER_SHOOTER_API bool WantsFlank(float Frontness, float Distance, float Front, float MinRange, float MaxRange);

	/**
	 * How much faster than its chase it comes now: ClosingScale while farther than ClosingRange (not while it waits on the
	 * ring), times FrenzyChase in its pack's frenzy.
	 */
	AI_LOOTER_SHOOTER_API float ChaseBoost(float Distance, bool bWaiting, float ClosingRange, float ClosingScale, bool bFrenzy, float FrenzyChase);
}
