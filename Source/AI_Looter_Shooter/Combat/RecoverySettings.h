#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Math/RandomStream.h"
#include "RecoverySettings.generated.h"

/**
 * How the player gets health back (Docs/Polish/BorderlandsComparison.md, item 3): the revenant's own trait, "wounds that
 * close" (health refills slowly once nothing has hurt them for a while), and the soul-motes some kills leave behind (small
 * floating wisps that heal when walked into). Every number is here so the feel can be tuned in one place;
 * UPlayerVitalsSubsystem holds the copy the game uses (UPlayerVitalsSubsystem::SettingsFor).
 *
 * The numbers answer one worry: with no healing, every fight drained a bar that never refilled until the player died, so a
 * long hike wore them down and failing hurt. Regeneration alone would make fights free, so it waits long enough that a
 * fight still costs something, and the motes reward the kill that wins one.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FRecoverySettings
{
	GENERATED_BODY()

	// --- Wounds that close ---

	/**
	 * Seconds without being hurt before health starts to come back. Long enough that a fight in progress never regenerates
	 * (a gap in the shooting is not a rest), short enough that the walk to the next camp restores the player.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wounds That Close", meta = (ClampMin = "0", Units = "s"))
	float RegenDelaySeconds = 6.f;

	/**
	 * Seconds the rate takes to rise from nothing to its full value once it starts. A ramp makes the first moment of healing
	 * gentle, so it reads as the wounds beginning to close rather than a switch flipping.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wounds That Close", meta = (ClampMin = "0", Units = "s"))
	float RegenRampSeconds = 1.5f;

	/** Share of the most health, per second, healed at the ramp's top: 0.12 takes a little over eight seconds to heal a bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wounds That Close", meta = (ClampMin = "0", ClampMax = "1"))
	float RegenRatePerSecond = 0.12f;

	// --- Soul-motes: how often they drop ---

	/**
	 * The chance a kill leaves one mote, by the dead creature's rank. They climb with the rank (the tougher the fight, the
	 * likelier the reward); Gravebound and every rank above it share the top chance. A boss always leaves 2 to 3.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0", ClampMax = "1"))
	float DropChanceBasic = 0.12f;

	/** "Restless". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0", ClampMax = "1"))
	float DropChanceRestless = 0.25f;

	/** "Gravebound", and "Soulfed" above it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0", ClampMax = "1"))
	float DropChanceGravebound = 0.5f;

	/** How many a boss leaves (from, to, both included): enough to heal a hurt player once the fight is won. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0"))
	int32 BossMotesMin = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0"))
	int32 BossMotesMax = 3;

	// --- Soul-motes: what one does ---

	/** Share of the most health one heals. Small, so the choice is to take a few (and fight on) rather than to stop and rest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0", ClampMax = "1"))
	float MoteHealShare = 0.15f;

	/** Seconds a mote waits before it is gone: long enough to finish a fight and walk back for it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "1", Units = "s"))
	float MoteLifeSeconds = 30.f;

	/** The last seconds of its life it flickers and shrinks away, so it never just vanishes under the player's eyes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0", Units = "s"))
	float MoteFadeSeconds = 5.f;

	/** How far a hurt player's capsule can be from a mote (cm) for it to start drifting toward them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0", Units = "cm"))
	float MoteMagnetRadius = 380.f;

	/** How close (cm, to the player's capsule) it is picked up: running past within about a meter is enough. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0", Units = "cm"))
	float MoteCollectRadius = 110.f;

	/**
	 * The pull's speed (cm/s) at the edge of the magnet's reach and right beside the player. The top is faster than a
	 * sprint (7.9 m/s), so a mote can't be outrun once it has started.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0", Units = "cm/s"))
	float MoteMagnetSpeedMin = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0", Units = "cm/s"))
	float MoteMagnetSpeedMax = 1200.f;

	/** How high over the ground it floats (cm): about chest height, so it reads in the grass and is easy to run into. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0", Units = "cm"))
	float MoteHoverHeight = 95.f;

	/** Seconds after dropping before it can be picked up, so it visibly rises out of the body first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul-motes", meta = (ClampMin = "0", Units = "s"))
	float MoteCollectDelay = 0.6f;

	// --- The rules, as plain functions (the tests check them without a world) ---

	/**
	 * The share of the most health that wounds-that-close heals between two moments of a run (seconds since it began):
	 * the area under the rate's ramp (0 up to RegenRatePerSecond over RegenRampSeconds, then level). Exact, so the total
	 * doesn't depend on the frame rate.
	 */
	float RegenShareBetween(float From, float To) const;

	/** The chance a kill of this rank leaves a mote (a boss: 1; it leaves BossMotesMin to BossMotesMax). */
	float DropChance(ECreatureRank Rank) const;

	/** How many motes one kill of this rank leaves, drawn from Random (a seeded stream repeats it). */
	int32 RollMotes(ECreatureRank Rank, FRandomStream& Random) const;

	/** What one mote heals a player of this much health at most. */
	float MoteHeal(float MaxHealth) const { return MaxHealth * MoteHealShare; }

	/**
	 * How much of a mote is left to see Age seconds into its life: 1 until the last MoteFadeSeconds, then easing down to 0
	 * at MoteLifeSeconds (its size; the flicker is the actor's own).
	 */
	float MoteVisible(float Age) const;
};
