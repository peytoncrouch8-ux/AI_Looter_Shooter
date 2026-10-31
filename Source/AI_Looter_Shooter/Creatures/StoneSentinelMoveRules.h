#pragma once

#include "CoreMinimal.h"
#include "Creatures/FightRules.h"
#include "Creatures/StoneSentinelRules.h"
#include "StoneSentinelMoveRules.generated.h"

/**
 * The Stone Sentinel's three attacks (the user, 2026-10-31: "they aren't a threat either ... have some other attack than just
 * a melee"; their pick: both a hurled boulder at range and a ground-slam shockwave up close). Each is the creature's one
 * attack, wound up, struck and recovered (ACreatureBase), with its own timing; StoneSentinelMoves::Choose picks one by range
 * and cooldowns so it presses at every distance.
 */
UENUM(BlueprintType)
enum class EStoneSentinelMove : uint8
{
	None,
	/** Both fists overhead and down on whoever stands in front (its melee, StoneSentinelCreatureSlam.cpp). */
	Slam,
	/** It rears, spreads its arms and drives both fists into the ground: a ring of rock spikes runs out (StoneSentinelCreatureShock.cpp). */
	Shockwave,
	/** It tears a rock out of the ground, lifts it overhead and hurls it at where its target will be (StoneSentinelCreatureBoulder.cpp). */
	Boulder,
};

/** The hurled boulder (AStoneSentinelBoulder; StoneSentinelCreatureBoulder.cpp). Distances at size 1 unless they say world. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FSentinelBoulderTuning
{
	GENERATED_BODY()

	/** It throws at a target between these (cm, flat, world), its lob's way clear, no more than MaxRise above it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0", Units = "cm"))
	float MinStart = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "100", Units = "cm"))
	float MaxStart = 2600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0", Units = "cm"))
	float MaxRise = 900.f;

	/** The tell (s): bent to the ground it tears the rock up at TearAt, has it overhead by LiftedAt, draws back (its trunk
	 *  wound round) until DrawnAt, and hurls it at Windup. Long: the rock overhead is the time to shoot it or move. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0.5", Units = "s"))
	float Windup = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0.05", Units = "s"))
	float TearAt = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0.1", Units = "s"))
	float LiftedAt = 1.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0.1", Units = "s"))
	float DrawnAt = 1.72f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0.1", Units = "s"))
	float Recovery = 0.9f;
	/** It turns after its target through this share of the tell (the rock goes where it's facing, led). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0", ClampMax = "1"))
	float AimShare = 0.9f;
	/** From one throw to the next one's readiness (s), and from first turning on someone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 5.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0", Units = "s"))
	float FirstDelay = 1.2f;
	/** Its breath after a throw before any next attack (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0", Units = "s"))
	float Breath = 0.7f;

	/** The rock (world cm across), torn from this far ahead of its middle (cm at its size), and its shootable reach round its middle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "20", Units = "cm"))
	float Size = 105.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0", Units = "cm"))
	float TearAhead = 125.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "10", Units = "cm"))
	float HitRadius = 65.f;

	/** The lob: pulled down this hard (cm/s^2), flying FlightNear at MinStart and FlightFar at MaxStart (s), between them by distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "100"))
	float Gravity = 1600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0.3", Units = "s"))
	float FlightNear = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0.3", Units = "s"))
	float FlightFar = 1.6f;
	/** It aims where its target will be: their run carried on for LeadShare of the flight, never more than MaxLead (cm) ahead.
	 *  Under a full lead a straight run is caught; a turn or a strafe sees it land beside them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0", ClampMax = "1"))
	float LeadShare = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0", Units = "cm"))
	float MaxLead = 500.f;

	/** It bursts on whoever is within SplashRadius (world cm) of where it lands: Strength times its slam at the middle, down
	 *  to EdgeShare of that at the edge, and a shove; a rock that strikes a body square does the middle's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0"))
	float Strength = 1.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "10", Units = "cm"))
	float SplashRadius = 230.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder", meta = (ClampMin = "0", ClampMax = "1"))
	float EdgeShare = 0.35f;

	/** Its counter: the rock in its grip takes HealthShare of the sentinel's most health to break (2-3 rifle shots); broken,
	 *  it bursts over its head for CounterShare of its health, it reels CounterStun (s) and is open CounterExposed (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder|Counter", meta = (ClampMin = "0.001", ClampMax = "1"))
	float HealthShare = 0.06f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder|Counter", meta = (ClampMin = "0", ClampMax = "1"))
	float CounterShare = 0.08f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder|Counter", meta = (ClampMin = "0.2", Units = "s"))
	float CounterStun = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boulder|Counter", meta = (ClampMin = "0", Units = "s"))
	float CounterExposed = 2.2f;
};

/** The ground-slam shockwave (AStoneSentinelShockwave; StoneSentinelCreatureShock.cpp). Distances at size 1. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FSentinelShockTuning
{
	GENERATED_BODY()

	/** It slams for a shockwave at a target within MaxStart (cm, flat), no more than MaxRise above or below it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0", Units = "cm"))
	float MaxStart = 650.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0", Units = "cm"))
	float MaxRise = 160.f;
	/** Hugged this close (cm, flat), it shakes them off with a shockwave rather than a slam whenever one is ready. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0", Units = "cm"))
	float HugDistance = 150.f;
	/** After this many slams in a row it breaks the run with a shockwave when it can. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "1"))
	int32 SlamRunBreak = 2;

	/** The tell (s): it rears back, arms spread wide (the ground cracking round its feet), and drives both fists down in the
	 *  last FallSeconds; the front leaves as they land. Fists in the ground through HoldShare of its Recovery: the window. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0.3", Units = "s"))
	float Windup = 1.35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0.05", Units = "s"))
	float FallSeconds = 0.22f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0.1", Units = "s"))
	float Recovery = 1.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0", ClampMax = "1"))
	float HoldShare = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0", Units = "s"))
	float ExposedSeconds = 1.6f;
	/** From one shockwave to the next one's readiness (s), from first turning on someone, and its breath after (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 6.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0", Units = "s"))
	float FirstDelay = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0", Units = "s"))
	float Breath = 0.9f;
	/** The tell's ring of cracks spreads to this (cm) round its feet over the wind-up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0", Units = "cm"))
	float TellRadius = 260.f;

	/** The front: from StartRadius out to MaxRadius (cm) at Speed (cm/s); it hurts across Band (cm) behind its edge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Front", meta = (ClampMin = "0", Units = "cm"))
	float StartRadius = 120.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Front", meta = (ClampMin = "100", Units = "cm"))
	float MaxRadius = 900.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Front", meta = (ClampMin = "100", Units = "cm/s"))
	float Speed = 950.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Front", meta = (ClampMin = "10", Units = "cm"))
	float Band = 100.f;
	/** Gaps in the ring: Gaps of GapDegrees each, evenly round, never within KeepClear degrees (past a gap's edge) of where its
	 *  target stood as it struck: step round into one, jump it, or get out past MaxRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Front", meta = (ClampMin = "0", ClampMax = "6"))
	int32 Gaps = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Front", meta = (ClampMin = "0", ClampMax = "90"))
	float GapDegrees = 34.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Front", meta = (ClampMin = "0", ClampMax = "90"))
	float KeepClear = 30.f;
	/** A body in the air at least JumpClear (cm) over the ring's ground is jumped clear; one more than MaxRise above or below it
	 *  is on another level (cm, world). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Front", meta = (ClampMin = "0", Units = "cm"))
	float JumpClear = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Front", meta = (ClampMin = "0", Units = "cm"))
	float BodyMaxRise = 150.f;
	/** The ground steps more than this (cm) between rows, or a wall stands across it: the front stops on that side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Front", meta = (ClampMin = "10", Units = "cm"))
	float MaxStep = 110.f;

	/** It hurts Strength times its slam, throws them up and back (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Hit", meta = (ClampMin = "0"))
	float Strength = 1.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Hit", meta = (ClampMin = "0"))
	float ThrowUp = 520.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Hit", meta = (ClampMin = "0"))
	float ThrowBack = 320.f;

	/** The spikes: a row every RowSpacing (cm) out, one every SpikeSpacing (cm) round it, SpikeTall near it to SpikeShort at
	 *  the ring's edge (cm, world), rising RiseSeconds, standing HoldSeconds and sinking SinkSeconds (s); Sectors (a ground
	 *  trace each a row) carry them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Spikes", meta = (ClampMin = "30", Units = "cm"))
	float RowSpacing = 130.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Spikes", meta = (ClampMin = "30", Units = "cm"))
	float SpikeSpacing = 85.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Spikes", meta = (ClampMin = "10", Units = "cm"))
	float SpikeTall = 140.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Spikes", meta = (ClampMin = "10", Units = "cm"))
	float SpikeShort = 75.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Spikes", meta = (ClampMin = "0.02", Units = "s"))
	float RiseSeconds = 0.09f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Spikes", meta = (ClampMin = "0", Units = "s"))
	float HoldSeconds = 0.18f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Spikes", meta = (ClampMin = "0.05", Units = "s"))
	float SinkSeconds = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Spikes", meta = (ClampMin = "8", ClampMax = "64"))
	int32 Sectors = 32;
};

/** What its choice of attack looks at (StoneSentinelMoves::Choose). */
struct FStoneSentinelMoveAsk
{
	/** Its target: how far off (cm, flat) and how far above it (cm; below 0 under it). */
	float Distance = 0.f;
	float Rise = 0.f;
	/** Its size (its slam's and shockwave's reach grow with it; the boulder's range is the world's). */
	float Size = 1.f;
	bool bShockReady = false;
	bool bBoulderReady = false;
	/** The lob's way to them is clear (no roof, no wall). */
	bool bLobClear = false;
	/** Its last attack and how many of it in a row. */
	EStoneSentinelMove Last = EStoneSentinelMove::None;
	int32 Streak = 0;
};

/** Its attacks' rules as plain functions, so the tests check them without a level. */
namespace StoneSentinelMoves
{
	/**
	 * The attack it makes now, or None (it closes in). Within its slam's reach: the slam, but a ready shockwave when it's
	 * hugged or after SlamRunBreak slams in a row. Out to the shockwave's start: a ready shockwave. Out from the boulder's
	 * MinStart to its MaxStart: a ready boulder whose lob is clear. The shockwave is tried before the boulder where they meet.
	 */
	AI_LOOTER_SHOOTER_API EStoneSentinelMove Choose(const FStoneSentinelMoveAsk& Ask, const FSentinelShockTuning& Shock,
		const FSentinelBoulderTuning& Boulder);

	/** How far off (cm, flat) Move may begin, at Size. */
	AI_LOOTER_SHOOTER_API float StartRange(EStoneSentinelMove Move, const FSentinelShockTuning& Shock, const FSentinelBoulderTuning& Boulder,
		float Size);

	/** The fight's turn the attack takes: the boulder a ranged one (spread from other throwers' shots), the rest melee. */
	AI_LOOTER_SHOOTER_API EFightRole TokenRoleFor(EStoneSentinelMove Move);

	// --- The boulder ---

	/** The lob's flight (s) for a throw Distance (cm, flat): FlightNear at MinStart to FlightFar at MaxStart, held at both ends. */
	AI_LOOTER_SHOOTER_API float FlightSeconds(float Distance, const FSentinelBoulderTuning& Boulder);

	/** Where it aims: its target's Feet with their flat Velocity carried on for LeadShare of Seconds, at most MaxLead ahead. */
	AI_LOOTER_SHOOTER_API FVector AimPoint(const FVector& Feet, const FVector& Velocity, float Seconds, const FSentinelBoulderTuning& Boulder);

	/** The launch velocity that carries it from Start to Target in Seconds under the boulder's Gravity. */
	AI_LOOTER_SHOOTER_API FVector LobVelocity(const FVector& Start, const FVector& Target, float Seconds, const FSentinelBoulderTuning& Boulder);

	/** Where a lob launched from Start at Velocity is After seconds, under the boulder's Gravity. */
	AI_LOOTER_SHOOTER_API FVector LobPoint(const FVector& Start, const FVector& Velocity, float After, const FSentinelBoulderTuning& Boulder);

	/** The share of its damage a body Side cm (flat, past its own radius) from where it burst takes: 1 at the middle down to
	 *  EdgeShare at SplashRadius, 0 past it. */
	AI_LOOTER_SHOOTER_API float SplashShare(float Side, const FSentinelBoulderTuning& Boulder);

	// --- The shockwave ---

	/** The front's radius Age seconds after the fists land (cm, at Size): out from StartRadius at Speed, held at MaxRadius. */
	AI_LOOTER_SHOOTER_API float FrontRadius(float Age, const FSentinelShockTuning& Shock, float Size);

	/** Seconds the front takes to run its whole way (at any size: everything grows together). */
	AI_LOOTER_SHOOTER_API float FrontSeconds(const FSentinelShockTuning& Shock);

	/**
	 * The middles of its gaps (degrees, world bearing from its middle), Gaps of them evenly round, turned so the one nearest
	 * TargetBearing is no nearer than KeepClear past its edge (the target stands mid-ring between two), wobbled by Seed.
	 */
	AI_LOOTER_SHOOTER_API TArray<float> MakeGaps(float TargetBearing, const FSentinelShockTuning& Shock, int32 Seed);

	/** Whether Bearing (degrees) lies in one of Gaps (their middles), each GapDegrees wide. */
	AI_LOOTER_SHOOTER_API bool IsInGap(float Bearing, TConstArrayView<float> Gaps, float GapDegrees);

	/** Whether a body BodyRadius round, Distance (cm, flat) from its middle, meets the front as it ran from FromRadius to
	 *  ToRadius this moment (its hurting band reaches Band behind its edge). */
	AI_LOOTER_SHOOTER_API bool FrontCrosses(float Distance, float BodyRadius, float FromRadius, float ToRadius, float Band);

	/** Whether the front catches a body FeetAbove (cm) the ring's ground there: on its level (BodyMaxRise either way), and on
	 *  the ground or too low in a jump (bFalling and under JumpClear). */
	AI_LOOTER_SHOOTER_API bool Catches(bool bFalling, float FeetAbove, const FSentinelShockTuning& Shock);

	// --- Poses (StoneSentinelPoses.cpp) ---

	/** The pose AttackTime into a throw: bent to the ground, the rock torn up and lifted overhead, drawn back with its trunk
	 *  wound round, hurled forward at the wind-up's end, the follow-through, standing again. */
	AI_LOOTER_SHOOTER_API FStoneSentinelPose ThrowPoseAt(float AttackTime, const FSentinelBoulderTuning& Boulder);

	/** The pose AttackTime into a shockwave: rearing back with its arms spread wide, both fists driven down, held in the
	 *  ground (the window), standing again. */
	AI_LOOTER_SHOOTER_API FStoneSentinelPose ShockPoseAt(float AttackTime, const FSentinelShockTuning& Shock);

	/** Its reel Time into Seconds after a rock bursts over its head: arms flung up, head ducked and rocked back, sagging, standing. */
	AI_LOOTER_SHOOTER_API FStoneSentinelPose ReelPoseAt(float Time, float Seconds);
}
