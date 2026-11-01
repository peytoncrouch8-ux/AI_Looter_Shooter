#pragma once

#include "CoreMinimal.h"
#include "Creatures/DredgerTuning.h"
#include "Creatures/DredgerTypes.h"

/** What a rifleman's cover search found at one candidate spot (DredgerRiflemanCover.cpp measures it; ScoreSpot weighs it). */
struct FDredgerSpotFacts
{
	/** Flat distance from the spot to the target (cm). */
	float ToTarget = 0.f;
	/** Flat distance from where he stands to the spot (cm). */
	float Travel = 0.f;
	/** How far round the target (degrees, 0-180) the spot lies from the bearing his squad wants him on. */
	float BearingOff = 0.f;
	/** Crouched there, the world stands between him and his target (close in front of him: cover, not a distant hill). */
	bool bCovered = false;
	/** Standing there, his rifle has a clear line to his target. */
	bool bCanShoot = false;
	/** Another Dredger has it, or one within ClaimRadius of it. */
	bool bClaimed = false;
	/** Getting there runs him past his target (across the line it would have him walk into). */
	bool bCrossesTarget = false;
	/** It is where he already is (a small bonus: no shuffling between equal spots). */
	bool bCurrent = false;
};

/** Everything that may break the soul-hose this frame (HoseBreak weighs it). */
struct FDredgerHoseFacts
{
	/** Flat distance from the hose-man to his victim (cm). */
	float Distance = 0.f;
	/** Seconds the world has cut the line so far. */
	float BlockedFor = 0.f;
	/** Seconds latched so far. */
	float LatchedFor = 0.f;
	bool bOwnerHeld = false;
	bool bOwnerDown = false;
	bool bTankBurst = false;
	bool bVictimGone = false;
};

/**
 * The Dredgers' fight as plain rules (no world, so the tests drive them; Looter.Creatures.Dredger.Rules). The numbers are
 * FDredgerTuning's (the Looter.Dredger.* cvars).
 */
namespace DredgerRules
{
	/** Two spots nearer than this (cm) are the same spot: a second rifleman never takes one already had. */
	inline constexpr float ClaimRadius = 300.f;
	/** Within this (cm) of his spot he is there. */
	inline constexpr float SpotArrive = 80.f;
	/** Cover counts only if the world stands between him and his target within this far of him (cm). */
	inline constexpr float CoverDepth = 300.f;
	/** A crouched rifleman's eye and a standing one's muzzle over his feet (cm, full size). */
	inline constexpr float CrouchEyeHeight = 100.f;
	inline constexpr float StandMuzzleHeight = 148.f;
	/** A target's chest over its middle (cm). */
	inline constexpr float ChestRise = 25.f;
	/** A missed bullet goes by this far off its target's chest (cm), at the least and at the most. */
	inline constexpr float MissNear = 55.f;
	inline constexpr float MissFar = 130.f;

	/**
	 * The share of a rifleman's bullets aimed to hit a target Distance away (cm): AccuracyNear within NearRange, Accuracy at
	 * 15 m, AccuracyFar past FarRange, straight lines between; a target crossing his line at CrossSpeed (cm/s) loses up to
	 * MovingPenalty of it (full at sprint speed, 600).
	 */
	AI_LOOTER_SHOOTER_API float HitChance(float Distance, float CrossSpeed, const FDredgerTuning& Tuning);

	/**
	 * Where a missed bullet is aimed: past the target's chest (Chest) as seen from Muzzle, Side01 picking left, right or over
	 * (0-1) and Spread01 how far off (MissNear to MissFar). It goes by close enough to be heard (UEnemyProjectileSubsystem's
	 * fly-by) and kicks up the ground or the wall behind.
	 */
	AI_LOOTER_SHOOTER_API FVector MissPoint(const FVector& Chest, const FVector& Muzzle, float Side01, float Spread01);

	/** Where to aim at a target moving at Velocity, a share Lead of the way its bullet's flight time takes it. */
	AI_LOOTER_SHOOTER_API FVector LeadPoint(const FVector& Muzzle, const FVector& Chest, const FVector& Velocity, float BulletSpeed, float Lead);

	/** How a rifleman keeps his distance: out past Far none (his brain brings him in), inside Near back off, else to his spot or there. */
	AI_LOOTER_SHOOTER_API EDredgerKeep ChooseKeep(float Distance, float Near, float Far, bool bHasSpot, float GapToSpot);

	/**
	 * The bearing offset (degrees) the Index-th rifleman on one target (0 first) wants round it from his squad's: 0, then
	 * +Angle, -Angle, +2 Angle, -2 Angle... (at most 150 either way): pairs flank from both sides.
	 */
	AI_LOOTER_SHOOTER_API float FlankOffset(int32 Index, float Angle);

	/** A spot's cost (lower is better); never usable (the largest float) where he couldn't fire from it at all. */
	AI_LOOTER_SHOOTER_API float ScoreSpot(const FDredgerSpotFacts& Spot, const FDredgerTuning& Tuning);

	/** The candidate spots a search looks at, round a bearing (degrees) from the target: (distance cm, bearing degrees) each. */
	AI_LOOTER_SHOOTER_API TArray<FVector2D> SpotCandidates(float Bearing, const FDredgerTuning& Tuning);

	/** A burst's attack: its recovery (the rounds after the first and a beat) and the cooldown that makes its period. */
	AI_LOOTER_SHOOTER_API float BurstRecovery(const FDredgerTuning& Tuning);
	AI_LOOTER_SHOOTER_API float BurstCooldown(const FDredgerTuning& Tuning);

	/** Whether the hose-man may throw his line now: in range, a clear line, rested. */
	AI_LOOTER_SHOOTER_API bool CanLatch(float Distance, bool bLineClear, float RestLeft, const FDredgerTuning& Tuning);

	/** Why the soul-hose breaks this frame (None: it holds). The first that fits: his fall, his tank, a stagger, the victim, range, the world, time. */
	AI_LOOTER_SHOOTER_API EDredgerHoseBreak HoseBreak(const FDredgerHoseFacts& Facts, const FDredgerTuning& Tuning);

	/** Health a second the hose drains: DrainPerSecond grown as his AttackDamage grew from his class's (his level and rank). */
	AI_LOOTER_SHOOTER_API float DrainPerSecond(float AttackDamage, float ClassAttackDamage, const FDredgerTuning& Tuning);

	/** The pull on the victim (a share of their walking input): PullShare, eased off within 2.5 m (it never drags them into him). */
	AI_LOOTER_SHOOTER_API float PullInput(float Distance, const FDredgerTuning& Tuning);

	/** Whether hits on the tank that took TankDamage of a MaxHealth burst it. */
	AI_LOOTER_SHOOTER_API bool TankBursts(float TankDamage, float MaxHealth, const FDredgerTuning& Tuning);

	/**
	 * A point T (0-1) along a hose from A to B hanging with Slack (cm at its middle) and wavering by Wave (cm) on Clock: the
	 * code-drawn line's shape (a sag, a ripple running along it).
	 */
	AI_LOOTER_SHOOTER_API FVector HosePoint(const FVector& A, const FVector& B, float T, float Slack, float Wave, float Clock);
}
