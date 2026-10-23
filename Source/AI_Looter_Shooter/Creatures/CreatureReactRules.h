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
 * The brief flash on a creature's body when a shot lands (warmer on a crit): full at once, gone in under a tenth of a
 * second (its own clock, so a hit-stop holds it at full). Mostly a rim of light round its silhouette, barely a lift over
 * the body (Even, Rim, RimPower on M_FX_HitFlash), so the creature stays solid and readable while it's hit.
 * Steady fire only nudges it: a hit soon after the last one flashes at RepeatShare of a fresh hit's strength, back to
 * full as the gap grows to QuietSeconds. Under full auto every shot flashed full and the overlay never went off: the
 * spider was a pale see-through ghost the whole fight (the user, 2026-10-22). Shots landing together (a shotgun's
 * pellets) don't add up: the stronger flash stands.
 */
struct AI_LOOTER_SHOOTER_API FCreatureHitFlash
{
	static constexpr float Seconds = 0.09f;
	static constexpr float NormalStrength = 0.65f;
	static constexpr float CriticalStrength = 1.f;
	/** How bright M_FX_HitFlash's Intensity goes at full strength (the dummies' is 1.8; 1.7 here washed the body white). */
	static constexpr float Glow = 1.2f;
	/** A hit within RepeatSeconds of the last flashes at RepeatShare (a crit at CriticalRepeatShare: crits still read in
	 *  steady fire), rising to full as the gap grows to QuietSeconds (a rifle's full auto is 0.08-0.12 s a shot). */
	static constexpr float RepeatSeconds = 0.15f;
	static constexpr float QuietSeconds = 0.45f;
	static constexpr float RepeatShare = 0.3f;
	static constexpr float CriticalRepeatShare = 0.65f;
	/** The flash's shape on a creature (M_FX_HitFlash: Even + (1 - |N.V|)^RimPower x Rim). The material's defaults
	 *  (0.35, 1.5, 2: an even glow, the dummies' and the powers' tints) lit thin legs and bristles all over. */
	static constexpr float Even = 0.08f;
	static constexpr float Rim = 1.6f;
	static constexpr float RimPower = 3.f;
	/** The overlay stays on (at nothing) this long after a flash, so steady fire doesn't put it on and off every shot. */
	static constexpr float LingerSeconds = 0.4f;

	/** A fresh hit's share of its full strength, GapSeconds after the last hit. */
	static float ShareFor(float GapSeconds, bool bCritical);

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
