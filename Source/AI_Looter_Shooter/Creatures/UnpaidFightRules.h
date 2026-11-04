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
	// --- The varied kit (2026-10-29, the user's "Unpaid fights: variety"; UnpaidCreatureVariety.cpp, Looter.Unpaid.Variety) ---
	/** A lunge's tell and flight that stops short and slides aside: no blow, it baits the dodge. */
	Feint,
	/** Fades out, comes back at the player's side or behind them and claws after a clear tell of its own. */
	PhaseStrike,
	/** Sinks low and skims in along the ground, claws sweeping at the feet: a jump clears it. */
	LowSwipe,
	/** No attack: its turn given back, it circles wide and waits for a pack-mate to commit. */
	HangBack,
};

/** How many EUnpaidMove values there are (per-move arrays are this long, indexed by the value). */
inline constexpr int32 NumUnpaidMoves = static_cast<int32>(EUnpaidMove::HangBack) + 1;

/** What an Unpaid is doing between its turns, on the ring (no attack turn needed). */
enum class EUnpaidAction : uint8
{
	None,
	/** The coal swelling at its chest, then a cinder thrown at the player. */
	Cinder,
	/** Screaming its pack on: they come together once it ends. */
	Rally,
	/** A lunge missed: it rears up and shrieks at the player instead of lunging again (UnpaidCreatureShriek.cpp). */
	Shriek,
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
	/** One of its rake's two swipes landed (the tally counts a rake that landed once, not each swipe). */
	bool bRakeLanded = false;
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
		bRakeLanded = false;
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
 * The one gate every Unpaid blink at one player asks (2026-11-03, the audit's change 11: the teleport was decided in seven places,
 * up to ~50 blinks a minute at a player): the phase-step, the press step, the step to sight, the flank, the phase-strike, the
 * fade-back and the Phasing trait each keep their own reasons to go; this alone says whether they may. Never two blinks at one
 * player within Gap seconds, never more than PerMinute in any minute, across all the Unpaid after them (2026-10-28: a pack once
 * blinked on one frame). One that owes a press step (hit from past its edge) goes ahead of the rest: its ask holds the next
 * opening for it a moment (OwedHold), so the others wait; one that finds nowhere to land stops asking and the hold runs out.
 * Plain, so the tests check it with their own clock (UUnpaidPackSubsystem keeps one per player: MayBlink, NoteBlink).
 */
struct AI_LOOTER_SHOOTER_API FUnpaidBlinkGate
{
	/** An owed step's ask holds the next opening this long (s) after it last asked. */
	static constexpr float OwedHold = 0.25f;
	/** The budget's window (s). */
	static constexpr double Minute = 60.0;

	/**
	 * Whether Id may blink now (bOwed: it owes a press step): Gap since the last blink, fewer than PerMinute in the minute before
	 * Now (0: no budget), nobody else's owed step first; Gap and PerMinute both 0: the gate is off. Asking owed holds the next
	 * opening for it; nothing is counted until Note.
	 */
	bool MayBlink(uint32 Id, bool bOwed, double Now, float Gap, int32 PerMinute);
	/** The gap alone: UFightDirectorSubsystem::MayStep's old ask, which no Unpaid makes since 2026-11-03. */
	bool MayStep(uint32 Id, bool bOwed, double Now, float Gap) { return MayBlink(Id, bOwed, Now, Gap, 0); }
	/** Id blinked at Now: the next waits the gap and the minute counts it; its owed hold, if any, is spent. */
	void Note(uint32 Id, double Now);
	/** Blinks noted in the minute before Now. */
	int32 CountInMinute(double Now) const;

	double LastStep = -1.0e9;
	uint32 OwedId = 0;
	double OwedAsked = -1.0e9;
	/** When each blink of the last minute went, oldest first (older ones dropped as Note adds). */
	TArray<double> Recent;
};

/** The director's old name for it (its FTargetFight still holds one); goes with UFightDirectorSubsystem::MayStep and NoteStep. */
using FUnpaidStepGate = FUnpaidBlinkGate;

/** Where a blink may land round its player (UnpaidBlinkRules::MayLand; Looter.Unpaid.BlinkNearestBehind, .FlankArcMin, .FlankArcMax). */
struct AI_LOOTER_SHOOTER_API FUnpaidBlinkLanding
{
	/** Behind the player's back (past BehindFrom off their view), never nearer them than this (cm). */
	float NearestBehind = 250.f;
	/** A flank or a phase-strike comes back within these angles off the way they look (degrees): at the edge of their view, never straight behind. */
	float ArcMin = 60.f;
	float ArcMax = 120.f;
};

/** The one gate's landing rule as plain functions (the tests check them without a level; UUnpaidPackSubsystem::MayLandAt plays them). */
namespace UnpaidBlinkRules
{
	/** "Behind their back": more than this off the way they look (degrees). */
	inline constexpr float BehindFrom = 90.f;

	/** How far round from the way a player at PlayerAt looks (ViewYaw) Spot lies, seen from above (degrees, 0 to 180). */
	AI_LOOTER_SHOOTER_API float AngleOffView(const FVector& PlayerAt, float ViewYaw, const FVector& Spot);

	/**
	 * Whether a blink may land at Spot round a player at PlayerAt looking along ViewYaw: never behind their back nearer than
	 * Landing.NearestBehind; a flank's (bFlank: a flank or a phase-strike) only within its arc of their view, so they see it come
	 * at the edge of the screen. Half a degree's slack for spots worked out from bearings on the arc's edges.
	 */
	AI_LOOTER_SHOOTER_API bool MayLand(const FVector& PlayerAt, float ViewYaw, const FVector& Spot, bool bFlank, const FUnpaidBlinkLanding& Landing);
}

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
	 * Where round the player a flank (or a phase-strike) may come back, as bearings (degrees, world), best first: only within
	 * ArcMin to ArcMax off the way they look (ViewYaw; the one gate's arc: at the edge of their view, never straight behind), its
	 * middle first, then toward their front, toward their back, the arc's front edge, its back edge; each its own side first
	 * (the side nearer its bearing CreatureBearing: the shorter way round, never across their eyes).
	 */
	AI_LOOTER_SHOOTER_API TArray<float> FlankBearings(float ViewYaw, float CreatureBearing, float ArcMin, float ArcMax);

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
	 * (Looter.Unpaid.FadeBack, 0 since 2026-11-03: off, one flee rule) is rolled apart, once a life.
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
