#pragma once

#include "CoreMinimal.h"
#include "Misc/DateTime.h"
#include "BossShrineRules.generated.h"

/** Where a vigil stone (ABossShrine) stands with its boss. */
UENUM(BlueprintType)
enum class EBossShrineState : uint8
{
	/** Its boss was never beaten in this session: the stone isn't there yet. */
	Dormant,
	/** The story holds the call back (Abel's until Main 6 is finished). */
	NotYet,
	/** Its boss is out there now (alive in its arena, or due there as the player comes): nothing to call. */
	Present,
	/** Beaten too lately: the call back waits out the rest of its five minutes of play. */
	Waiting,
	/** Ring the bell: its boss rises in its arena to fight again. */
	Ready,
};

/**
 * A vigil stone's rules as plain functions (the user's, 2026-10-12: "a pedestal with a menu that spawns next to the boss arenas
 * after you kill the boss the first time so you can respawn the boss. 5 minute wait, or the boss respawns after a save and
 * quit"), apart from the world so the tests read them:
 *  - the stone appears once its boss has been beaten (the first defeat), and rises beside the arena the first time the player
 *    comes near it after that, once no scene is playing and the boss is gone;
 *  - its boss can be called back while it isn't out there, five minutes of play after its own last death. Each stone counts
 *    from its own boss's death in the session's time played (which a save keeps), so a Save & Quit and Continue never readies
 *    a stone early, nor the others with it (the user, 2026-10-27: "vigil stones seem to be linked, each one should have its
 *    own countdown"; this replaced the 10-12 "or at once after a Save & Quit").
 */
namespace BossShrineRules
{
	/** Time played from a boss's death to its call back (5 minutes; paused time doesn't count). */
	inline constexpr double CallBackSeconds = 5.0 * 60.0;

	/** After its boss's death, the stone waits this long (s) before it may rise: the body falls and the loot flies first. */
	inline constexpr float RiseDelay = 3.f;

	/** What a stone's state is read from. */
	struct FInputs
	{
		/** Its boss has been beaten at least once (kept by the stone, the campaign, its lair or the story). */
		bool bEverBeaten = false;
		/** The story lets it be called now (the stone's CallableWhen). */
		bool bStoryAllows = true;
		/** Its boss is out there now. */
		bool bBossPresent = false;
		/** When it was last beaten, in the session's time played; negative: not kept (beaten before the stones kept it). */
		double DefeatedAt = -1.0;
		/** The session's time played now. */
		double PlayedNow = 0.0;
	};

	struct FVerdict
	{
		EBossShrineState State = EBossShrineState::Dormant;
		/** While Waiting: how much play is left before it's Ready (s). */
		double SecondsLeft = 0.0;
	};

	AI_LOOTER_SHOOTER_API FVerdict Evaluate(const FInputs& In);

	/** Play left before a boss beaten at DefeatedAt can be called back at PlayedNow (0 once it can, or when not kept). */
	AI_LOOTER_SHOOTER_API double SecondsLeft(double DefeatedAt, double PlayedNow);

	/**
	 * Whether the stone rises now: its boss beaten, the stone not risen yet, no scene playing, the boss not out there,
	 * RiseDelay passed since the death this visit (SinceDefeat; a death before the visit counts as long passed) and the
	 * player within Radius (cm).
	 */
	AI_LOOTER_SHOOTER_API bool ShouldRise(bool bEverBeaten, bool bRisen, bool bScenePlaying, bool bBossPresent, float SinceDefeat,
		float PlayerDistance, float Radius);

	/** The countdown for people: minutes and seconds, the seconds rounded up ("4:59", "0:01"). */
	AI_LOOTER_SHOOTER_API FText Countdown(double Seconds);

	/** The menu's status line for a state ("The Gravemother walks again: lay her to rest first."). */
	AI_LOOTER_SHOOTER_API FText StatusText(EBossShrineState State, const FText& BossName, double SecondsLeft);

	/**
	 * When it was last laid to rest, for people: the clock ("Today 14:32") and how much play ago ("Today 14:32, 3 min of play
	 * ago"; the time played is kept through a save, so it reads the same after a Save & Quit); "Before the stones kept count"
	 * when not kept.
	 */
	AI_LOOTER_SHOOTER_API FText LastKillText(double DefeatedAt, double PlayedNow, const FDateTime& DefeatedOn, const FDateTime& Now);
}
