#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureTrait.h"
#include "StoneSentinelRules.generated.h"

/**
 * A Stone Sentinel's numbers (AStoneSentinelCreature; the Elements matter job, Docs/Work/2026-10-29-ElementsMatter/CONTRACT.md;
 * made a threat 2026-10-31, Docs/Archive/Work/2026-10-31-Creatures/CONTRACT.md): its sleep and waking, its gait, its two-fist slam,
 * its stone and the window it leaves open. Distances at size 1 (they grow with it), seconds as they are. Its ranged boulder
 * and its shockwave have their own numbers (StoneSentinelMoveRules.h).
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

	/**
	 * Its waking: its brain held while the stone cracks loose, it stretches free of the years and drops into its stance with
	 * a thud (s). 1.6 so the drop lands on its Wake cue's settle.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wake", meta = (ClampMin = "0.2", Units = "s"))
	float WakeSeconds = 1.6f;

	/** How fast it grinds round while it wakes (degrees a second): slow, so the turn reads as stone moving. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wake", meta = (ClampMin = "10"))
	float WakeTurnRate = 75.f;

	/** Its drop into its stance at the waking's end: a jolt on a near player's view (0-1) within this (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wake", meta = (ClampMin = "0", ClampMax = "1"))
	float WakeSettleShake = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wake", meta = (ClampMin = "0", Units = "cm"))
	float WakeSettleShakeRadius = 1400.f;

	// --- Walking ---

	/** One stride (cm): a heavy footfall at each, its Step cue, a puff of grit and a jolt for a player near. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk", meta = (ClampMin = "20", Units = "cm"))
	float StepLength = 120.f;

	/**
	 * A footfall's jolt on a player's view (0-1, for StepShakeSeconds), full within a fifth of StepShakeRadius (cm) and
	 * nothing past it. 0.12 since 2026-10-31 (was a faint 0.05): the user wants its steps felt near it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk", meta = (ClampMin = "0", ClampMax = "1"))
	float StepShake = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk", meta = (ClampMin = "0.05", Units = "s"))
	float StepShakeSeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk", meta = (ClampMin = "0", Units = "cm"))
	float StepShakeRadius = 1100.f;

	// --- Grit off its joints (StoneSentinelCreatureChips.cpp: pebbles and a breath of dust off a shoulder, elbow, neck or knee) ---

	/** Pebbles a second: standing awake, at a full walk (added), in a wind-up (added), waking, and while it's open (Exposed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grit", meta = (ClampMin = "0"))
	float GritIdle = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grit", meta = (ClampMin = "0"))
	float GritWalking = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grit", meta = (ClampMin = "0"))
	float GritWindup = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grit", meta = (ClampMin = "0"))
	float GritWaking = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grit", meta = (ClampMin = "0"))
	float GritExposed = 3.f;

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

	/** Its breath after a slam before any next attack (s). 1.4 since 2026-10-31 (was 1.8): it presses harder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slam", meta = (ClampMin = "0", Units = "s"))
	float SlamCooldown = 1.4f;

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

	// --- Open (the rhythm to learn, 2026-10-31: after a heavy blow it's stuck bent over its fists, the soul-light pouring out of
	//     the cracks in its back; StoneSentinelCreatureHurt.cpp) ---

	/** How long it's open after a slam (s); the shockwave's and a burst boulder's are theirs (StoneSentinelMoveRules.h). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Open", meta = (ClampMin = "0", Units = "s"))
	float ExposedAfterSlam = 0.9f;

	/** The share of its stone's defense it keeps while open (0: only its tier's). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Open", meta = (ClampMin = "0", ClampMax = "1"))
	float ExposedStoneShare = 0.f;

	/** While open its back and shoulders (spine_02's and spine_01's hit zones) are critical too, as its head always is. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Open")
	bool bExposedTorsoCrits = true;

	// --- Going back to sleep ---

	/** Calm this long (no target) within SettleReach of its plinth, it climbs back on and is a statue again (s, cm, s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rest", meta = (ClampMin = "0", Units = "s"))
	float RestAfter = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rest", meta = (ClampMin = "0", Units = "cm"))
	float SettleReach = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rest", meta = (ClampMin = "0.1", Units = "s"))
	float SettleSeconds = 1.f;
};

/**
 * Where its body is in a move (StoneSentinelRules::SlamPoseAt and the other moves' poses), in degrees and cm. Laid over the
 * statue's rest pose by how awake it is (StoneSentinelCreaturePose.cpp). The rigid Tripo pieces bend cleanly only so far:
 * arms up to about 128 / 70, a bow of about 28 (2026-10-30: 172/35/-10 and 62/6/34 split its shoulders into shards).
 */
struct FStoneSentinelPose
{
	/** The upper arms' swing forward round the shoulders: 0 hanging, 90 straight ahead, 180 overhead. */
	float ArmSwing = 0.f;
	/** The forearms bent on further from the upper arms (degrees forward). */
	float ElbowBend = 0.f;
	/** The trunk bowed forward (degrees; below 0 leaning back). */
	float Bow = 0.f;
	/** The trunk turned on its hips (degrees about up, above 0 its right shoulder back): a throw's wind. */
	float Twist = 0.f;
	/** The arms spread out from its sides (added to their way's sideways share: 0.4 about 22 degrees wider). */
	float Spread = 0.f;
	/** The trunk leaning to its side (degrees, above 0 to its right). */
	float Lean = 0.f;
	/** Its hips sunk (cm): its weight dropping into a blow. Small: its feet sink with them. */
	float Crouch = 0.f;
	/** Its head tipped up (degrees above 0) or tucked (below), over its awake lift. */
	float HeadPitch = 0.f;
};

/** Its stride at a point of its gait (StoneSentinelRules::GaitAt): left 0, right 1. Degrees and cm, at a full walk. */
struct FStoneSentinelGait
{
	/** Each thigh's swing forward (degrees; below 0 back), and its knee's bend (the shin back). */
	float Thigh[2] = { 0.f, 0.f };
	float Knee[2] = { 0.f, 0.f };
	/** Each arm's swing (degrees on its hanging), opposite its own leg. */
	float Arm[2] = { 0.f, 0.f };
	/** Its hips dropping as a foot strikes (cm): tons settling onto it. */
	float Drop = 0.f;
	/** Its trunk rolling over the planted foot (degrees, above 0 to its right), twisting against its legs, lurching forward at a strike. */
	float Roll = 0.f;
	float Twist = 0.f;
	float Lurch = 0.f;
};

/** Its sounds (Art/Sounds/recipes/sentinel.py and sentinel_attacks.py; Audio/LooterSoundCues.h names the first six too). */
namespace StoneSentinelCue
{
	inline constexpr const TCHAR* Wake = TEXT("Creature.Sentinel.Wake");
	inline constexpr const TCHAR* Step = TEXT("Creature.Sentinel.Step");
	inline constexpr const TCHAR* WindUp = TEXT("Creature.Sentinel.WindUp");
	inline constexpr const TCHAR* Slam = TEXT("Creature.Sentinel.Slam");
	inline constexpr const TCHAR* Chip = TEXT("Creature.Sentinel.Chip");
	inline constexpr const TCHAR* Crumble = TEXT("Creature.Sentinel.Crumble");
	/** The rock torn out of the ground (~1.3 s): roots and earth tearing, stone grinding loose, dirt pouring off it as it lifts. */
	inline constexpr const TCHAR* Tear = TEXT("Creature.Sentinel.Tear");
	/** The throw (~0.7 s): the body's stone wrenched round, a great rush of air as the rock leaves its hands. */
	inline constexpr const TCHAR* Heave = TEXT("Creature.Sentinel.Heave");
	/** The rock in flight (~1.6 s, follows it): a low tumbling whoom, grit streaming off. */
	inline constexpr const TCHAR* BoulderFly = TEXT("Creature.Sentinel.BoulderFly");
	/** It lands (~1.8 s, 'Gun'): a crushing thud, the rock bursting, stones raining back, the valley rolling it. */
	inline constexpr const TCHAR* BoulderImpact = TEXT("Creature.Sentinel.BoulderImpact");
	/** Shot in its grip, it bursts over its head (~1.2 s, 'Gun'): a sharp shatter, stones raining on stone, its grinding reel. */
	inline constexpr const TCHAR* BoulderShatter = TEXT("Creature.Sentinel.BoulderShatter");
	/** The shockwave's tell (~1.4 s, swells): it rears and spreads its arms, a deep grind rising, the ground under it groaning. */
	inline constexpr const TCHAR* ShockWindUp = TEXT("Creature.Sentinel.ShockWindUp");
	/** Both fists driven into the ground (~1.8 s, 'Gun'): a huge boom, the ground splitting, a crackle rolling away. */
	inline constexpr const TCHAR* ShockSlam = TEXT("Creature.Sentinel.ShockSlam");
	/** Rock spikes bursting up near the listener (~0.6 s): stone punching through earth, a run of sharp cracks. */
	inline constexpr const TCHAR* ShockSpikes = TEXT("Creature.Sentinel.ShockSpikes");
	/** It's open (~1.2 s): a hollow crack through the block and a low soul-light hum under it: "now". */
	inline constexpr const TCHAR* Exposed = TEXT("Creature.Sentinel.Exposed");
	/** Pebbles off a joint (~0.3 s, quiet, Near): a few small stones ticking down. */
	inline constexpr const TCHAR* Pebbles = TEXT("Creature.Sentinel.Pebbles");
}

/** The Stone Sentinels' rules as plain functions and numbers, so the tests check them without a level or its model. */
namespace StoneSentinelRules
{
	/** A dead miner's health at level 1 (ADeadMinerCreature; the test checks it still is) and the sentinel's share of it. */
	inline constexpr float DeadMinerHealth = 180.f;
	/** 6 since 2026-10-31 (the user: "they need to be harder to kill"; was 3): twice its old health. */
	inline constexpr float HealthShare = 6.f;

	/** Its slam's damage at level 1 (the fight's DamageScale on top), its experience, its walk. */
	inline constexpr float SlamDamage = 16.f;
	// 45, was 26 (the RR tuning the user took, 2026-10-29 night).
	inline constexpr int32 XPReward = 45;
	inline constexpr float WalkSpeed = 110.f;

	/**
	 * The rank whose loot it drops and whose Unpaid silver it pays at least (the RR tuning the user took, 2026-10-29 night):
	 * a Regular statue pays as a Restless one, its loot table (DA_LootTable_Rare: 60% a gun, Luck 0.5, 2-3 ammo) and a
	 * Restless Unpaid's purse; a higher tier its own. Loot/KillLootRules reads it.
	 */
	inline constexpr ECreatureRank LeastLootRank = ECreatureRank::Rare;

	/** How fast it comes on in a fight (cm/s), as felt: its ChaseSpeed is this over the fight's chase scale. */
	inline constexpr float FeltChaseSpeed = 170.f;

	/** It starts its slam from this far (cm, flat), and stands to swing at three quarters of it (the brain's hold). */
	inline constexpr float AttackRange = 260.f;

	/** Its health at level 1: six dead miners' worth of stone. */
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

	/** Its stone's share of defense now: all of it, or ExposedStoneShare of it while it's open. */
	AI_LOOTER_SHOOTER_API float StoneDefenseNow(bool bExposed, const FStoneSentinelTuning& Tuning);

	/** Whether a hit on Bone is critical: its head always; its back and shoulders too while it's open (bExposedTorsoCrits). */
	AI_LOOTER_SHOOTER_API bool IsCritBone(FName Bone, bool bExposed, const FStoneSentinelTuning& Tuning);

	/** Whether it still turns after its target StateTime into a wind-up of Windup (the first AimShare of it). */
	AI_LOOTER_SHOOTER_API bool TracksAim(float StateTime, float Windup, float AimShare);

	/** Blends two poses, every part of them (Alpha clamped 0-1). */
	AI_LOOTER_SHOOTER_API FStoneSentinelPose BlendPose(const FStoneSentinelPose& A, const FStoneSentinelPose& B, float Alpha);

	/** The pose AttackTime into its slam (the wind-up heave, the fists' fall at its end, bent over them, straightening). */
	AI_LOOTER_SHOOTER_API FStoneSentinelPose SlamPoseAt(float AttackTime, const FStoneSentinelTuning& Tuning);

	/** Its pose standing awake: arms hanging heavy, a slight bow. */
	AI_LOOTER_SHOOTER_API FStoneSentinelPose IdlePose();

	/**
	 * Its pose WakeTime into its waking (over the statue's rest by WakeShare): still at first as the stone cracks, a stretch
	 * free of the years (arms spread, head and trunk lifting), then its drop into its stance as it ends.
	 */
	AI_LOOTER_SHOOTER_API FStoneSentinelPose WakePoseAt(float WakeTime, float WakeSeconds);

	/** How far into its waking it is (0 a statue, 1 awake), eased: it grinds loose slowly, then all at once. */
	AI_LOOTER_SHOOTER_API float WakeShare(float WakeTime, float WakeSeconds);

	/** Whether a footfall comes between two points of its gait (each half of a stride: 0 the left foot, 0.5 the right). */
	AI_LOOTER_SHOOTER_API bool StepBetween(float FromPhase, float ToPhase);

	/**
	 * Its stride at Phase (0-1, the left foot down at 0, the right at 0.5), at a full walk: each foot planted most of the
	 * cycle and swung through quickly (stone doesn't glide), its hips dropping at each strike, its trunk rolling over the
	 * planted foot and twisting against its legs, its arms swinging a little against them.
	 */
	AI_LOOTER_SHOOTER_API FStoneSentinelGait GaitAt(float Phase);

	/** Stone pieces a hit throws: a crit's more, a heavy burst a few more. */
	AI_LOOTER_SHOOTER_API int32 ChipsFor(bool bCritical, float BurstShare, const FStoneSentinelTuning& Tuning);

	/**
	 * Its traits with Fleet taken out (a running statue is wrong, and its slow tread is the point): Rolled is a roll of one
	 * more than Count from the same seed, so the trait after the cut stands in; never more than Count.
	 */
	AI_LOOTER_SHOOTER_API TArray<ECreatureTrait> WithoutFleet(TConstArrayView<ECreatureTrait> Rolled, int32 Count);
}
