#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureTrait.h"
#include "StoneSentinelRules.generated.h"

/**
 * A Stone Sentinel's numbers (AStoneSentinelCreature; the Elements matter job, Docs/Work/2026-10-29-ElementsMatter/CONTRACT.md):
 * its sleep and waking, its gait, its two-fist slam, its stone. Distances at size 1 (they grow with it), seconds as they are.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FStoneSentinelTuning
{
	GENERATED_BODY()

	// --- Waking ---

	/** A player this near (cm, flat) wakes it, or a shot. Near enough that a careless walk past a grave row springs it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wake", meta = (ClampMin = "100", Units = "cm"))
	float WakeDistance = 900.f;

	/** ...and no farther above or below it than this (cm): a player on a roof or down a cellar doesn't count. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wake", meta = (ClampMin = "50", Units = "cm"))
	float WakeRise = 400.f;

	/** Its waking: its brain held while the stone cracks loose and it grinds round to face them (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wake", meta = (ClampMin = "0.2", Units = "s"))
	float WakeSeconds = 1.6f;

	/** How fast it grinds round while it wakes (degrees a second): slow, so the turn reads as stone moving. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wake", meta = (ClampMin = "10"))
	float WakeTurnRate = 75.f;

	// --- Walking ---

	/** One stride (cm): a heavy footfall at each, its Step cue, a puff of grit and a faint jolt for a player near. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk", meta = (ClampMin = "20", Units = "cm"))
	float StepLength = 120.f;

	/** A footfall's jolt on a player's view (0-1) within this (cm). Faint: felt, never a shake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk", meta = (ClampMin = "0", ClampMax = "1"))
	float StepShake = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk", meta = (ClampMin = "0", Units = "cm"))
	float StepShakeRadius = 900.f;

	// --- The slam ---

	/** Its wind-up (s): both fists heaved overhead, then brought down at its end. Long and readable: back off in it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0.2", Units = "s"))
	float SlamWindup = 1.1f;

	/** The last part of the wind-up the fists take to come down (s): the blow lands as they meet the ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0.02", Units = "s"))
	float SlamFallSeconds = 0.14f;

	/** After the blow: fists in the dirt (the window to punish it), then back up (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0.1", Units = "s"))
	float SlamRecovery = 1.0f;

	/** The share of the recovery it stays bent over its fists before it straightens. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0", ClampMax = "1"))
	float SlamHoldShare = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0", Units = "s"))
	float SlamCooldown = 1.8f;

	/** The blow lands on a target this far off at most (cm, flat), in front (the cosine off its facing), this far up or down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "50", Units = "cm"))
	float SlamReach = 280.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "-1", ClampMax = "1"))
	float SlamFront = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "50", Units = "cm"))
	float SlamMaxRise = 220.f;

	/** It turns after its target for this share of the wind-up, then its aim is set: a sidestep late in it dodges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0", ClampMax = "1"))
	float SlamAimShare = 0.6f;

	/** Where the fists land without its fist sockets (cm ahead of its middle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0", Units = "cm"))
	float SlamImpactAhead = 160.f;

	/** The crack it leaves: run ahead from the fists this long, this wide; a burst round the fists; how long it stays open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0", Units = "cm"))
	float SlamCrackLength = 380.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "1", Units = "cm"))
	float SlamCrackWidth = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0", Units = "cm"))
	float SlamBurstRadius = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0", Units = "s"))
	float SlamCrackLingers = 2.5f;

	/** The blow's shake on a player's view (0-1), how long, and how far it's felt (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0", ClampMax = "1"))
	float SlamShake = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0.05", Units = "s"))
	float SlamShakeSeconds = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0", Units = "cm"))
	float SlamShakeRadius = 1500.f;

	// --- Stone ---

	/** The share of every hit its stone shrugs off, on top of its tier's (StoneSentinelRules::CombinedDefense; a crit pierces half). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0", ClampMax = "0.9"))
	float StoneDefense = 0.25f;

	/** From the killing blow to the body breaking up (s): it sags, cracks run over it, then it bursts into stone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0.1", Units = "s"))
	float CrumbleSeconds = 0.8f;

	/** Stone pieces thrown: by a hit, a crit, its waking, its crumbling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0"))
	int32 HitChips = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0"))
	int32 CritChips = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0"))
	int32 WakeChips = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0"))
	int32 CrumbleChips = 18;

	// --- Going back to sleep ---

	/** Calm this long (no target) within SettleReach of its plinth, it climbs back on and is a statue again (s, cm, s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rest", meta = (ClampMin = "0", Units = "s"))
	float RestAfter = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rest", meta = (ClampMin = "0", Units = "cm"))
	float SettleReach = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rest", meta = (ClampMin = "0.1", Units = "s"))
	float SettleSeconds = 1.f;
};

/** Where its body is in a move (StoneSentinelRules::SlamPoseAt): the arms' swing and the trunk's bow, in degrees. */
struct FStoneSentinelPose
{
	/** The upper arms' swing forward round the shoulders: 0 hanging, 90 straight ahead, 180 overhead. */
	float ArmSwing = 0.f;
	/** The forearms bent on further from the upper arms (degrees forward). */
	float ElbowBend = 0.f;
	/** The trunk bowed forward (degrees; below 0 leaning back). */
	float Bow = 0.f;
};

/** The Stone Sentinels' rules as plain functions and numbers, so the tests check them without a level or its model. */
namespace StoneSentinelRules
{
	/** A dead miner's health at level 1 (ADeadMinerCreature; the test checks it still is) and the sentinel's share of it. */
	inline constexpr float DeadMinerHealth = 180.f;
	inline constexpr float HealthShare = 3.f;

	/** Its slam's damage at level 1 (the fight's DamageScale on top), its experience, its walk. */
	inline constexpr float SlamDamage = 16.f;
	inline constexpr int32 XPReward = 26;
	inline constexpr float WalkSpeed = 110.f;

	/** How fast it comes on in a fight (cm/s), as felt: its ChaseSpeed is this over the fight's chase scale. */
	inline constexpr float FeltChaseSpeed = 170.f;

	/** It starts its slam from this far (cm, flat), and stands to swing at three quarters of it (the brain's hold). */
	inline constexpr float AttackRange = 260.f;

	/** Its health at level 1: three dead miners' worth of stone. */
	AI_LOOTER_SHOOTER_API float MaxHealth();

	/** The ChaseSpeed that, times the fight's chase scale (Looter.Fight.ChaseScale), comes on at Felt. */
	AI_LOOTER_SHOOTER_API float ChaseSpeedFor(float Felt, float ChaseScale);

	/** Whether a player FlatDistance off (cm) and Rise above or below it wakes it (Tuning's WakeDistance and WakeRise). */
	AI_LOOTER_SHOOTER_API bool ShouldWake(float FlatDistance, float Rise, const FStoneSentinelTuning& Tuning);

	/**
	 * Whether the fists land on a target ToTarget from its middle (world), facing Forward, at its size: within SlamReach
	 * flat, SlamMaxRise up or down (at least size 1's), and in front (SlamFront). Right on top of it, it can't miss.
	 */
	AI_LOOTER_SHOOTER_API bool IsSlamHit(const FVector& Forward, const FVector& ToTarget, const FStoneSentinelTuning& Tuning, float Size = 1.f);

	/** Its stone's defense with its tier's on top: each takes its share of what the other left (at most 0.9). */
	AI_LOOTER_SHOOTER_API float CombinedDefense(float Stone, float Tier);

	/** Whether it still turns after its target StateTime into a wind-up of Windup (the first AimShare of it). */
	AI_LOOTER_SHOOTER_API bool TracksAim(float StateTime, float Windup, float AimShare);

	/** The pose AttackTime into its slam (the wind-up heave, the fists' fall at its end, bent over them, straightening). */
	AI_LOOTER_SHOOTER_API FStoneSentinelPose SlamPoseAt(float AttackTime, const FStoneSentinelTuning& Tuning);

	/** Its pose standing awake: arms hanging heavy, a slight bow. */
	AI_LOOTER_SHOOTER_API FStoneSentinelPose IdlePose();

	/** How far into its waking it is (0 a statue, 1 awake), eased: it grinds loose slowly, then all at once. */
	AI_LOOTER_SHOOTER_API float WakeShare(float WakeTime, float WakeSeconds);

	/** Whether a footfall comes between two points of its gait (each half of a stride: 0 the left foot, 0.5 the right). */
	AI_LOOTER_SHOOTER_API bool StepBetween(float FromPhase, float ToPhase);

	/** Stone pieces a hit throws: a crit's more, a heavy burst a few more. */
	AI_LOOTER_SHOOTER_API int32 ChipsFor(bool bCritical, float BurstShare, const FStoneSentinelTuning& Tuning);

	/**
	 * Its traits with Fleet taken out (a running statue is wrong, and its slow tread is the point): Rolled is a roll of one
	 * more than Count from the same seed, so the trait after the cut stands in; never more than Count.
	 */
	AI_LOOTER_SHOOTER_API TArray<ECreatureTrait> WithoutFleet(TConstArrayView<ECreatureTrait> Rolled, int32 Count);
}
