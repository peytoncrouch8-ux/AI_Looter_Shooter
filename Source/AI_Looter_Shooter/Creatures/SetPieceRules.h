#pragma once

#include "CoreMinimal.h"
#include "Creatures/EncounterSpawner.h"

/**
 * A set piece's feel numbers (ASetPieceSpawner; Docs/Pipeline.md "Fight: Layout"), as the Looter.Fight.SetPiece.* cvars
 * set them (FSetPieceTuning::Current), so the user tunes them live. Each set piece's own data (its pushes, beats, spots)
 * comes from the layout; these scale or override them for every set piece at once. A default-made one holds the defaults,
 * which the tests read.
 */
struct AI_LOOTER_SHOOTER_API FSetPieceTuning
{
	/** Every beat's camera shake, times (0 turns the shakes off). */
	float ShakeScale = 1.f;

	/** The next push comes once this many or fewer of the last are left standing; below 0: each set piece's own. */
	int32 PushWhenLeft = -1;

	/** Every beat's dust from the roof (a cave-in), times. */
	float DustScale = 1.f;

	/** The reward's shower at the end: guns in it, their luck (a Strongbox's 2 and 1), its ammo pickups, extra boxes. */
	int32 RewardGuns = 2;
	float RewardLuck = 1.f;
	int32 RewardAmmo = 2;
	int32 RewardBonusAmmo = 1;

	/** The tuning as the cvars hold it now (read each time: a live change counts at once). */
	static FSetPieceTuning Current();
};

/**
 * A set piece's rules as plain functions (ASetPieceSpawner plays them; Looter.SetPiece.Rules checks them without a
 * level): when the next push comes, when it's held off, when it's done, whose talk it shuts, and its beats' numbers.
 */
namespace SetPieceRules
{
	/**
	 * Whether its next push comes now: it's fighting (Engaged), a wave is left (WavesStarted of PlannedWaves), and the last
	 * is down to PushWhenLeft or fewer (Remaining: alive and still owed). A push comes on top of what's left, not after it.
	 */
	AI_LOOTER_SHOOTER_API bool IsPushDue(EEncounterState State, int32 WavesStarted, int32 PlannedWaves, int32 Remaining, int32 PushWhenLeft);

	/** The push threshold a set piece plays: the tuning's override when it has one (0 or more), else its own. */
	AI_LOOTER_SHOOTER_API int32 PushThreshold(int32 Own, int32 Override);

	/**
	 * Whether it's done: begun (WavesStarted), every wave's creatures cleared (State Cleared), and none of its placed
	 * creatures (a balcony's Bell-Ringer, the high ground's Spitters) alive or owed. A Legendary's lair cleared by its
	 * monster being away isn't a fight, so it needs a wave begun.
	 */
	AI_LOOTER_SHOOTER_API bool IsDone(EEncounterState State, int32 WavesStarted, int32 PlacedAlive, int32 PlacedOwed);

	/** Whether the speakers it shuts stay shut: begun, not done, and still on (a story that ended lets them talk again). */
	AI_LOOTER_SHOOTER_API bool KeepsHushed(EEncounterState State, int32 WavesStarted, bool bDone);

	/** Whether it's held off springing: any of the spawners it waits on is fighting (the mine's lower shift still out). */
	AI_LOOTER_SHOOTER_API bool IsHeld(TConstArrayView<EEncounterState> Waited);

	/** The beat for wave Wave among beats for BeatWaves (each its wave, counted from 1): the first that's for it, or INDEX_NONE. */
	AI_LOOTER_SHOOTER_API int32 FindBeat(TConstArrayView<int32> BeatWaves, int32 Wave);

	/** A beat's shake as played: its strength times the tuning's scale, kept to 0-1. */
	AI_LOOTER_SHOOTER_API float ShakeStrength(float Strength, float Scale);

	/** A beat's dust puffs as played: its count times the tuning's scale, rounded, at most MaxDustPuffs. */
	AI_LOOTER_SHOOTER_API int32 DustPuffs(int32 Count, float Scale);

	/** The most dust puffs one beat sifts down (one roof trace each, once). */
	inline constexpr int32 MaxDustPuffs = 24;

	/** Its reward's level: the highest of its creatures' (the guns follow the fight), at least 1. */
	AI_LOOTER_SHOOTER_API int32 RewardLevel(int32 HighestCreatureLevel);
}
