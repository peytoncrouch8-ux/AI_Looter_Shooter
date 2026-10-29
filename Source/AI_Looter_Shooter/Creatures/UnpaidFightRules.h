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

/** Where a hurt Unpaid is in its fade-back (FIGHTS.md lane 5, 2026-10-29; UnpaidCreatureFadeBack.cpp). */
enum class EUnpaidFadeBack : uint8
{
	None,
	/** Phase-stepping away from its target. */
	Stepping,
	/** Back in, at the ring's outer edge, waiting before it comes on again. */
	Waiting,
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
	/** Its flank has been rolled for this wait on the ring (once a wait). */
	bool bFlankRolled = false;
	/**
	 * Its next turn opens with a flank: it holds its attack (CanStartAttack) until its turn comes, then fades round. Without
	 * this the turn and the attack came in one frame from the ring (inside its lunge's reach) and no flank ever went (DEMO
	 * 2026-10-27: none in 35 turns).
	 */
	bool bFlankArmed = false;
	/** Seconds to its next look at its choices on the ring (not every frame). */
	float ThinkIn = 0.f;
	/** The chase boost it last told the fight director (written only when it changes). */
	float ToldBoost = 1.f;
	/** Whether it is a boss's add (they keep the old kit, so a boss fight's pacing is the boss's): -1 not looked yet. */
	int8 BossAdd = -1;
	/** The class's (or level's) recovery, before a move sets its own (a lunge's, captured with the wind-up as its rank applies). */
	float BaseRecovery = 0.7f;
	/**
	 * Its fade-back: where it is in one, seconds left of its wait, whether it has gone this life (once a life), and its
	 * chance's roll (-1 not rolled yet, 0 lost, 1 won: rolled once a life, the first time it may).
	 */
	EUnpaidFadeBack Fade = EUnpaidFadeBack::None;
	float FadeLeft = 0.f;
	bool bFadedBack = false;
	int8 FadeRoll = -1;
	/** Seconds left of its coal guttering (its pack's leader fell near it). */
	float GutterLeft = 0.f;
	/** Its lunge was struck at a camping target: it lands a little wider (Looter.Unpaid.CampHitRadius). */
	bool bCampLunge = false;

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
		bFlankArmed = false;
		ThinkIn = 0.f;
		BossAdd = -1;
		Fade = EUnpaidFadeBack::None;
		FadeLeft = 0.f;
		bFadedBack = false;
		FadeRoll = -1;
		GutterLeft = 0.f;
		bCampLunge = false;
	}
};

/**
 * What an Unpaid last saw of its target (AUnpaidCreature::SeesTarget: one trace, kept a moment so a frame's asks cost no
 * more), and whose target its step clock was last jittered for (a new target jitters it: UnpaidCreatureSight.cpp).
 */
struct FUnpaidSightState
{
	uint32 TargetId = 0;
	bool bClear = true;
	/** Seconds until it looks again (counted down by its own frames, so a test's ticks count too). */
	float RecheckIn = 0.f;
	uint32 StepTargetId = 0;
	/**
	 * A blind spell: it let a turn go for want of sight and stays out of the line for turns until it sees its target again
	 * (2026-10-28 play-check: let go and taken again 4-7 times a second across the pack). BlindHoldLeft: seconds left of the
	 * director's rest it last set, renewed before it runs out.
	 */
	bool bBlindSpell = false;
	float BlindHoldLeft = 0.f;
};

/**
 * One phase-step or flank at a time at one target, across all the Unpaid after them (2026-10-28 play-check: a pack past
 * FarBehind blinked on one frame as it noticed the player or a wave spawned, 5-6 at once): at most one every Gap seconds.
 * One that owes a press step (hit from past its edge) goes ahead of the rest: its ask holds the next opening for it a moment
 * (OwedHold), so the others wait; one that finds nowhere to land stops asking and the hold runs out. Plain, so the tests
 * check it with their own clock (UFightDirectorSubsystem keeps one per target: MayStep, NoteStep).
 */
struct AI_LOOTER_SHOOTER_API FUnpaidStepGate
{
	/** An owed step's ask holds the next opening this long (s) after it last asked. */
	static constexpr float OwedHold = 0.25f;

	/** Whether Id may step now (bOwed: it owes a press step). Asking owed holds the next opening for it; nothing is counted until Note. */
	bool MayStep(uint32 Id, bool bOwed, double Now, float Gap);
	/** Id stepped (or flanked) at Now: the next waits the gap; its owed hold, if any, is spent. */
	void Note(uint32 Id, double Now);

	double LastStep = -1.0e9;
	uint32 OwedId = 0;
	double OwedAsked = -1.0e9;
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
	 * Its step clock (seconds since its last phase-step) as it spawns or notices a target: set back to Roll (0-1) times Jitter
	 * short of its Cooldown, so each of a pack waits its own moment more (they no longer blink on one frame); a clock already
	 * further back (a step just taken) is kept.
	 */
	AI_LOOTER_SHOOTER_API float JitterStepClock(float SinceLastStep, float Cooldown, float Jitter, float Roll);

	/**
	 * How much faster than its chase it comes now: ClosingScale while farther than ClosingRange (not while it waits on the
	 * ring), times FrenzyChase in its pack's frenzy.
	 */
	AI_LOOTER_SHOOTER_API float ChaseBoost(float Distance, bool bWaiting, float ClosingRange, float ClosingScale, bool bFrenzy, float FrenzyChase);

	/**
	 * Which rally cooldown its scream takes, or below 0 when it may not scream (FIGHTS.md lane 5, 2026-10-29): in a led pack
	 * (bPackLed) only the leader screams, at LeaderRallyCooldown; within the lull after a leader's fall (bInLull) none does;
	 * a leaderless pack as before, any of it at RallyCooldown.
	 */
	AI_LOOTER_SHOOTER_API float RallyCooldownFor(bool bPackLed, bool bIsLeader, bool bInLull, float RallyCooldown, float LeaderRallyCooldown);

	/**
	 * Whether a hurt one may fade back after its turn: a Basic, alive under Below of its health, not yet this life. Its chance
	 * (Looter.Unpaid.FadeBack) is rolled apart, once a life.
	 */
	AI_LOOTER_SHOOTER_API bool MayFadeBack(bool bBasic, float HealthShare, float Below, bool bFadedThisLife);

	/**
	 * Where a fade-back may land, best first, at FeetZ: away from Target on the side Here is on (straight away, then fanned 25
	 * and 50 degrees either way), between Near and Far from Target (the middle first, then the far, the near).
	 */
	AI_LOOTER_SHOOTER_API TArray<FVector> FadeBackSpots(const FVector& Here, const FVector& Target, float Near, float Far, float FeetZ);

	/** The arrival hold after a step or flank that landed LandedGap (cm) from its target: NearHold within NearRange (the dais), else Hold. */
	AI_LOOTER_SHOOTER_API float ArrivalHoldFor(float LandedGap, float Hold, float NearHold, float NearRange);
}
