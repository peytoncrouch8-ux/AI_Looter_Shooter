#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"

class AActor;
class FElementStatus;
struct FElementTuning;
enum class EElementAffinity : uint8;

/**
 * How much of each element status's pose a creature shows now (the "Elements matter" job, 2026-10-29: statuses you can
 * see on the body), eased in and out (FCreatureStatusPoseTracker). Each kind's pose code lays them over its own the way it
 * lays its stagger (GetStaggerAlpha), in its own character:
 *  - Burn (Ember): flailing at the flames.
 *  - Jolt (Thunder): locked up, a fast tremor while the jolt waits, and a hard jerk at its burst (JoltJerk).
 *  - Rot: hunched over, and a cough or retch every few seconds (Cough).
 *  - Salt (Grave-salt): stiff, its pose's clock slowed (ClockScale, SaltClock at full salt).
 *  - Smite (Soul-light): flinching from the light gathering in it (more as the burst nears).
 * Looks only: nothing here changes a creature's speed, damage or timing (the spiders' fight stays as it is, the user's rule).
 * A boss shows none of it (its fight is its show, as with its stagger).
 */
struct AI_LOOTER_SHOOTER_API FCreatureStatusPose
{
	float Burn = 0.f, Jolt = 0.f, Rot = 0.f, Salt = 0.f, Smite = 0.f;
	/** Times the kind's own pose clock (breathing, gait, sway): 1, or slower while salted. */
	float ClockScale = 1.f;
	/** The jolt's burst: 1 the moment it bursts, gone over JoltJerkSeconds (already times nothing: the kinds size it). */
	float JoltJerk = 0.f;
	/** The rot's cough now (0 to 1, already times Rot): a heave and a retch, every CoughEvery seconds. */
	float Cough = 0.f;
	/** The reactions' own clock (s) for flails and tremors; each creature's starts at its own offset, so a pack doesn't move in step. */
	float Time = 0.f;

	/** Any of it showing. */
	bool IsAny() const;
	/** A wave of the reactions' clock: Hz times a second, Phase in radians (-1 to 1). */
	float Wave(float Hz, float Phase = 0.f) const;
	/** The flames' flail (Burn's wave at FlailHz), and the jolt's tremor (at TremorHz); -1 to 1, not times their alphas. */
	float Flail(float Phase = 0.f) const;
	float Tremor(float Phase = 0.f) const;
};

/**
 * Turns a creature's statuses into its status pose over time: each alpha eases toward what its status asks for (in at
 * EaseIn a second, out at EaseOut), the jolt's burst is caught as it happens (the jolts waiting drop by one), the rot's
 * cough keeps its beat. Plain numbers: UCreatureStatusReactComponent runs one per creature, the tests drive one by hand.
 */
class AI_LOOTER_SHOOTER_API FCreatureStatusPoseTracker
{
public:
	/** Moves it on by DeltaSeconds toward what Status asks for. Dead (bAlive false), everything eases out and no jerk is caught. */
	void Advance(const FElementStatus& Status, const FElementTuning& Tuning, bool bAlive, float DeltaSeconds);

	const FCreatureStatusPose& Get() const { return Pose; }

	/** Nothing showing and nothing left to ease (its component may stop ticking). */
	bool IsAtRest() const;

	/** Everything off at once (a new life); its clock starts at TimeOffset. */
	void Reset(float TimeOffset = 0.f);

private:
	FCreatureStatusPose Pose;
	/** The jolts waiting at the last step, and the oldest one's charge then: a drop from a charged jolt is its burst. */
	int32 LastJolts = 0;
	float LastJoltCharge = 0.f;
	/** Seconds since the last jolt's burst (below 0: none yet). */
	float JerkAge = -1.f;
	/** The rot's cough's beat (s), running while the rot shows. */
	float CoughClock = 0.f;
};

/** The status reactions' rules and numbers (Looter.Creatures.StatusPose tests them). */
namespace CreatureStatusPose
{
	/** Alphas rise this much a second (full in about a sixth of a second) and fall this much (gone in 0.4 s). */
	inline constexpr float EaseIn = 6.f;
	inline constexpr float EaseOut = 2.5f;
	/** Salt's clock at full salt: the kind's pose runs at this share of its speed. */
	inline constexpr float SaltClock = 0.6f;
	/** The burn's flail and the jolt's tremor (Hz). */
	inline constexpr float FlailHz = 2.3f;
	inline constexpr float TremorHz = 21.f;
	/** The jolt's burst: its jerk fades over this long, and the lock-up holds this long after it (s). */
	inline constexpr float JoltJerkSeconds = 0.35f;
	inline constexpr float JoltLockSeconds = 0.45f;
	/** The rot's cough: one every CoughEvery seconds, lasting CoughSeconds, the first CoughLead into the rot. */
	inline constexpr float CoughEvery = 2.4f;
	inline constexpr float CoughSeconds = 0.75f;
	inline constexpr float CoughLead = 0.9f;
	/** The rot coughs only once its hunch shows this much. */
	inline constexpr float CoughFrom = 0.25f;

	/**
	 * The creature's status pose now (its UCreatureStatusReactComponent's, times Looter.Element.StatusPose); nothing on one
	 * without statuses, a boss, or null. Each kind's pose code reads it once a frame.
	 */
	AI_LOOTER_SHOOTER_API FCreatureStatusPose Of(const AActor* Creature);

	/** What each status asks for now (the alphas only, before easing): the tracker's goal. */
	AI_LOOTER_SHOOTER_API FCreatureStatusPose GoalOf(const FElementStatus& Status, const FElementTuning& Tuning);

	/** Pose with its alphas times Scale (0 hides it, 1 as it is), the clock's slowing too. */
	AI_LOOTER_SHOOTER_API FCreatureStatusPose Scaled(const FCreatureStatusPose& Pose, float Scale);

	/** The cough's shape at Into seconds into one (0 to 1): a heave, a retch on it, gone by CoughSeconds. */
	AI_LOOTER_SHOOTER_API float CoughShape(float Into);

	// --- The burn's panic (ACreatureBase::UpdateStatusBrain) ---

	/** What the panic's rule asks of a creature as a burn takes it. */
	struct FPanicAsk
	{
		ECreatureRank Rank = ECreatureRank::Basic;
		/** A boss, or one keeping its own fight (a Soulfed / Legendary monster). */
		bool bBoss = false;
		bool bOwnFight = false;
		/** A spider of any kind: its fight stays untouched (the user's rule); it gets the pose, not the panic. */
		bool bSpider = false;
		/** Fire resisted or nothing to it (the Unpaid's coal, stone, iron, spirits): it doesn't run from it. */
		bool bFireShrugged = false;
		bool bAlive = true;
		/** After a player (chasing or searching), not mid-attack, not held back or already running. */
		bool bHunting = false;
		bool bAttacking = false;
		bool bPassive = false;
		bool bRunning = false;
		/** Its panic's cooldown left (s). */
		float CooldownLeft = 0.f;
	};

	/** Whether it may panic at all: Basic or Restless, not a boss or Soulfed, not a spider, hurt by fire, hunting and free. */
	AI_LOOTER_SHOOTER_API bool MayPanic(const FPanicAsk& Ask);

	/** It panics: it may, and Roll (0 to 1) comes in under Chance. */
	AI_LOOTER_SHOOTER_API bool Panics(const FPanicAsk& Ask, float Roll, float Chance);

	/** Fire is resisted or nothing to a creature that stands Affinity to Ember. */
	AI_LOOTER_SHOOTER_API bool ShrugsFire(EElementAffinity Affinity);

	/** How long a panic runs (s), and its speed against its chase speed. */
	inline constexpr float PanicSeconds = 1.5f;
	inline constexpr float PanicSpeedShare = 1.1f;

	/** Looter.Element.BurnPanic (the chance a burn sets one panicking), .BurnPanicCooldown (s), and Looter.Element.StatusPose. */
	AI_LOOTER_SHOOTER_API float LiveBurnPanicChance();
	AI_LOOTER_SHOOTER_API float LiveBurnPanicCooldown();
	AI_LOOTER_SHOOTER_API float LiveStatusPoseScale();
}
