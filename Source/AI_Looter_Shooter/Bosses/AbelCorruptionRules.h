#pragma once

#include "CoreMinimal.h"

/**
 * Abel Ransom, Corrupted (Main 7 "Given to the Wind", MAINS_2_7.md section 11; the user's calls in DECISIONS.md: the
 * call-backs are a hold at his keeper's post while his bar is locked, the six dead are an optional save): the plain rules
 * of what the furnace did to him, so the tests check them without a fight. UAbelCorruptionComponent plays them.
 *
 *  - The call-back: at 66% and 33% his Smog bar locks (shots only spark) and he goes down on his knees for KneelSeconds; a
 *    hold of CallBackHold on the keeper's post fills a gold pip, boils a third of the smog off him for good and lets him up.
 *    Missed, he rises with the bar still locked, and kneels again RetryAfter later: it can't be skipped and can't be failed.
 *    At zero the last call: a FinalHold on the post, under a tendril's drain, fills the third pip and frees him.
 *  - The smog tendril: from his black lantern, after a one-second flare (shoot the lantern then to stop it), a line latches
 *    the player like a hose-man's: a drain and a slow pull. It breaks at 14 m, on a staggering hit, when the black lantern is
 *    shot during one of its flares (every 3 s while it holds), or after a while.
 *  - The drink: out over the canyon in "The Hose", a smog line from him to one of the six shrouded dead at a time. Three hits
 *    on its collar (where it meets the body) break it; held DrinkSeconds, the body is drunk out: he heals DrinkHeal of his
 *    health, it rises Restless, and the optional "save all six" is lost.
 *  - The fog surge: in "Keeper" the smog from the wreck below pours up over the deck: the height fog's density rises to
 *    SurgeDensity starting SurgeStart out (about 15 m of sight), eased in and given back.
 */
namespace AbelCorruptionRules
{
	/** Three gold pips under his bar: one at each call-back (66%, 33%) and the last at zero. */
	inline constexpr int32 Pips = 3;

	/** Knelt for a call-back this long (s); missed, the next kneel comes this long after he rises (s). */
	inline constexpr float KneelSeconds = 6.f;
	inline constexpr float RetryAfter = 12.f;

	/** The holds on the keeper's post: a call-back, and the last call at zero (s). */
	inline constexpr float CallBackHold = 1.5f;
	inline constexpr float FinalHold = 3.f;

	/** The tendril: thrown at a player within ThrowRange (cm), after a Tell flare (s), flying at Speed (cm/s), reaching this far. */
	inline constexpr float TendrilThrowRange = 1200.f;
	inline constexpr float TendrilTell = 1.f;
	inline constexpr float TendrilSpeed = 2400.f;
	inline constexpr float TendrilReach = 1600.f;
	/** Latched: it breaks past BreakRange (cm, "14 m"), or after Longest (s); the black lantern flares FlareSeconds every FlareEvery. */
	inline constexpr float TendrilBreakRange = 1400.f;
	inline constexpr float TendrilLongest = 6.f;
	inline constexpr float TendrilFlareEvery = 3.f;
	inline constexpr float TendrilFlareSeconds = 1.f;
	/** Its drain a second, as a share of his bite (AttackDamage, his level's), and its pull (a share of the player's walk). */
	inline constexpr float TendrilDrainShare = 0.3f;
	inline constexpr float TendrilPull = 0.3f;
	/** In "Keeper" a second tendril follows the first this long after (s). */
	inline constexpr float SecondTendrilAfter = 0.45f;

	/** The drink: each line held this long drinks a body out (s); this many hits on its collar break it; a gap between lines (s). */
	inline constexpr float DrinkSeconds = 12.f;
	inline constexpr int32 CollarHits = 3;
	inline constexpr float DrinkRest = 4.f;
	/** A body drunk out heals him this share of his health. */
	inline constexpr float DrinkHeal = 0.05f;

	/** The fog surge: the height fog's density and start distance (cm) at its height, eased in over SurgeIn and out over SurgeOut (s). */
	inline constexpr float SurgeDensity = 0.05f;
	inline constexpr float SurgeStart = 200.f;
	inline constexpr float SurgeIn = 3.f;
	inline constexpr float SurgeOut = 2.5f;

	/** Why a tendril lets go (None: it holds). */
	enum class EBreak : uint8
	{
		None,
		/** He fell, left the fight, or was freed. */
		OwnerDown,
		/** The player died or is gone. */
		VictimGone,
		/** Past TendrilBreakRange. */
		Distance,
		/** A staggering hit on him (his boss's stagger, or his kneel). */
		Staggered,
		/** The black lantern shot during its flare. */
		LanternShot,
		/** Held TendrilLongest. */
		Time,
	};

	/** What a latched tendril knows this frame. */
	struct FTendrilFacts
	{
		float Distance = 0.f;
		float LatchedFor = 0.f;
		bool bOwnerDown = false;
		bool bVictimGone = false;
		bool bStaggered = false;
		bool bLanternShot = false;
		/** The last call's tendril: it holds through the hold (no range, no time), only the hold's end or the player's death break it. */
		bool bFinal = false;
	};

	/** The first reason that fits (his fall, the player, the lantern, a stagger, the range, the time), or None. */
	AI_LOOTER_SHOOTER_API EBreak TendrilBreaks(const FTendrilFacts& Facts);

	/** The black lantern is flaring LatchedFor seconds into a tendril (its window to be shot): the first second of every three. */
	AI_LOOTER_SHOOTER_API bool IsLanternFlaring(float LatchedFor);

	/** How much smog is left on him with Filled pips (1: all of it, 0: freed), a third gone with each. */
	AI_LOOTER_SHOOTER_API float CorruptionFor(int32 Filled);

	/** The call-back a phase opens with (its index, 0 at 66%, 1 at 33%), or INDEX_NONE for the first phase. */
	AI_LOOTER_SHOOTER_API int32 CallBackFor(int32 Phase);

	/**
	 * The next of the dead to drink after Last (in order round the boards, Last + 1 first; INDEX_NONE to start at 0), the
	 * drunk passed over; INDEX_NONE when every one is drunk or there are none.
	 */
	AI_LOOTER_SHOOTER_API int32 NextDrink(const TArray<bool>& Drunk, int32 Last);

	/** The optional's word: the fight won with bodies on the boards and none of them drunk. */
	AI_LOOTER_SHOOTER_API bool AreSixSaved(int32 Bodies, int32 Drunk);

	/** The surge's share (0-1) moved on by DeltaSeconds toward On (1) or off (0), eased at SurgeIn / SurgeOut. */
	AI_LOOTER_SHOOTER_API float StepSurge(float Share, bool bOn, float DeltaSeconds);

	/** The fog's density and start distance at Share of the surge, from the level's own. */
	AI_LOOTER_SHOOTER_API float SurgeDensityAt(float OwnDensity, float Share);
	AI_LOOTER_SHOOTER_API float SurgeStartAt(float OwnStart, float Share);

	/** His body's tint under Corruption of smog (1: black, a hint of violet; 0: his own). */
	AI_LOOTER_SHOOTER_API FLinearColor SmogTint(const FLinearColor& Own, float Corruption);

	/** His coal's color: furnace orange while any smog is on him, the saint's gold-white once freed (RGB linear, A its strength). */
	AI_LOOTER_SHOOTER_API FLinearColor CoalColor(float Corruption);

	/** The optional's event and story flag, both "Abel.SixSaved" (Main 7 step 2's optional; W5's GivenToTheWind::SixSavedEvent). */
	AI_LOOTER_SHOOTER_API FName SixSavedEvent();
}
