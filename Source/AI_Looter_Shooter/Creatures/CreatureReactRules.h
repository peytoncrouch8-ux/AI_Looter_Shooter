#pragma once

// The rules behind the creatures' reactions (the 2026-10-14 "creatures that react" polish), kept apart from any world so
// the tests can check them on their own: the notice beat as a creature first spots its prey, the hit flash on its body,
// and the level's budget for the dust and ash they throw. UCreatureReactComponent and UCreatureReactSubsystem run them.

#include "CoreMinimal.h"
#include "Creatures/CreatureTypes.h"

/**
 * The notice beat: half a second as a creature first spots its prey (from calm, or a search that never came to a chase)
 * in which it reacts where it stands, facing them (a spider rears, an Unpaid's coal flares and its head snaps round, a
 * miner lowers his pick, a slime shudders), before it comes. Once a hunt: a search that turns back into the chase it had
 * doesn't have another. Never for one already hurt (a shot gets it going at once, and a hit during the beat ends it), a
 * boss, or one held back. A pack's members each take their own length (their seeds), so they don't set off as one.
 */
struct AI_LOOTER_SHOOTER_API FCreatureNotice
{
	/** A beat lasts between these (s), by the creature's seed; with its prey already within reach, only this. */
	static constexpr float ShortestSeconds = 0.42f;
	static constexpr float LongestSeconds = 0.62f;
	static constexpr float PointBlankSeconds = 0.22f;
	/** Its pose rises over this share of the beat and gives way to the chase's over the last share. */
	static constexpr float RiseShare = 0.3f;
	static constexpr float FallShare = 0.3f;

	/** How long a creature with this seed (0 to 1) takes; point blank: short. */
	static float LengthFor(float Seed01, bool bPointBlank);

	/** Whether a hunt that just turned into a chase from From may have its beat. */
	static bool MayNotice(ECreatureState From, bool bHurt, bool bBoss, bool bChasedThisHunt);

	void Begin(float Seconds);
	void Advance(float DeltaSeconds);
	/** Over now (a hit, the prey lost); the hunt goes on. */
	void Stop();
	/** It has chased this hunt: a search turning back into the chase has no beat. */
	void NoteChase() { bChasedThisHunt = true; }
	/** The hunt is over (calm again, home, dead): the next one may have its beat. */
	void EndHunt();

	bool IsHolding() const { return bHolding; }
	bool HasChasedThisHunt() const { return bChasedThisHunt; }
	/** 0 to 1 through the beat. */
	float GetAlpha() const;
	/** How much of its notice pose shows (0 to 1): up quickly, held, eased off into the chase. */
	float GetStrength() const;
	/** Beats it has had (the tests). */
	int32 GetCount() const { return Count; }

private:
	float Length = 0.f;
	float Time = 0.f;
	bool bHolding = false;
	bool bChasedThisHunt = false;
	int32 Count = 0;
};

/**
 * The brief white flash on a creature's body when a shot lands (warmer on a crit): full at once, gone in under a tenth of
 * a second (its own clock, so a hit-stop holds it at full). Shots close together start it again, the stronger one standing.
 */
struct AI_LOOTER_SHOOTER_API FCreatureHitFlash
{
	static constexpr float Seconds = 0.09f;
	static constexpr float NormalStrength = 0.65f;
	static constexpr float CriticalStrength = 1.f;
	/** How bright M_FX_HitFlash's Intensity goes at full strength (the dummies' is 1.8). */
	static constexpr float Glow = 1.7f;
	/** The overlay stays on (at nothing) this long after a flash, so steady fire doesn't put it on and off every shot. */
	static constexpr float LingerSeconds = 0.4f;

	void Start(bool bCritical);
	void Advance(float DeltaSeconds);
	/** Over at once, linger and all (a death: the kind's death look shows, never a white body held by the kill's hit-stop). */
	void Stop() { Time = Seconds + LingerSeconds; Strength = 0.f; }
	bool IsOn() const { return Time < Seconds; }
	/** The overlay is wanted: flashing, or lingering after one. */
	bool IsWorn() const { return Time < Seconds + LingerSeconds; }
	float GetIntensity() const;
	bool IsCritical() const { return bCritical; }

private:
	float Time = 1000.f;
	float Strength = 0.f;
	bool bCritical = false;
};

/**
 * A level's allowance for something thrown often (the creatures' footfall dust, their ash): at most PerSecond on average,
 * Burst at once; what's asked past it is simply not thrown.
 */
struct AI_LOOTER_SHOOTER_API FCreatureDustBudget
{
	FCreatureDustBudget() = default;
	FCreatureDustBudget(float InPerSecond, float InBurst) : PerSecond(InPerSecond), Burst(InBurst) {}

	/** How many of Wanted may go now (Now: the world's clock, s), taken from the allowance. */
	int32 Take(double Now, int32 Wanted);

	float PerSecond = 40.f;
	float Burst = 12.f;

private:
	float Tokens = -1.f;
	double LastTime = 0.0;
};

/** The sounds of the reactions (cues without sounds play nothing until Main's sound pass makes them). */
namespace CreatureReactCue
{
	inline constexpr const TCHAR* SpiderNotice = TEXT("Creature.Spider.Notice");
	inline constexpr const TCHAR* SpiderEmerge = TEXT("Creature.Spider.Emerge");
	inline constexpr const TCHAR* SpiderTwitch = TEXT("Creature.Spider.Twitch");
	inline constexpr const TCHAR* SlimeNotice = TEXT("Creature.Slime.Notice");
	inline constexpr const TCHAR* SlimeBubble = TEXT("Creature.Slime.Bubble");
	inline constexpr const TCHAR* SlimeSwell = TEXT("Creature.Slime.Swell");
	inline constexpr const TCHAR* UnpaidNotice = TEXT("Creature.Unpaid.Notice");
	inline constexpr const TCHAR* UnpaidAsh = TEXT("Creature.Unpaid.Ash");
	inline constexpr const TCHAR* UnpaidRise = TEXT("Creature.Unpaid.Rise");
	inline constexpr const TCHAR* DeadMinerNotice = TEXT("Creature.DeadMiner.Notice");
	inline constexpr const TCHAR* BellRingerNotice = TEXT("Creature.BellRinger.Notice");
}
