#pragma once

#include "CoreMinimal.h"
#include "Creatures/EncounterSpawner.h"

/**
 * A set piece's feel numbers (ASetPieceSpawner; Docs/Pipeline/Enemies.md "Fight: Layout"), as the Looter.Fight.SetPiece.* cvars
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

	/** An arena set piece's fog gates (ASetPieceSpawner::ArenaGates) close; off, they never do. */
	bool bArena = true;

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

	// --- An arena's fog gates (ASetPieceSpawner::ArenaGates; SetPieceSpawnerArena.cpp) ---

	/**
	 * Where one fog wall (an ABossSeal of the Gate shape) stands across an arena's opening: its origin, the yaw that turns the
	 * seal's +Y along the wall (so its GateEnd is (0, Length, Rise)) and the +X, the arena's side, flat and unit length.
	 */
	struct FGateFrame
	{
		FVector Origin = FVector::ZeroVector;
		double Yaw = 0.0;
		double Length = 0.0;
		/** How much higher (cm) the far end stands than the origin. */
		double Rise = 0.0;
		FVector Ahead = FVector::ForwardVector;
	};

	/** The frame of a wall from Start to End with ArenaPoint (a point inside the arena) ahead of it; the ends swap if they must. */
	AI_LOOTER_SHOOTER_API FGateFrame GateFrame(const FVector& Start, const FVector& End, const FVector& ArenaPoint);

	/** Whether Point is on the arena's side of the wall, at least Margin (cm) in from it. */
	AI_LOOTER_SHOOTER_API bool IsAheadOfGate(const FGateFrame& Frame, const FVector& Point, double Margin);

	/**
	 * Whether the gates close now: it's fighting (Engaged) with a push begun, not won, the arena is on, and the living player
	 * stands inside every gate (nobody is shut out, or caught in the wall as it rises).
	 */
	AI_LOOTER_SHOOTER_API bool ShouldRaiseGates(bool bUp, bool bArenaOn, EEncounterState State, int32 WavesStarted, bool bDone,
		bool bPlayerAlive, bool bPlayerInside);

	/**
	 * Whether the gates open now: they're up and it is won (done, or its own creatures all down: Cleared), taken away or off
	 * (not Engaged any more), the player died, or the arena was switched off.
	 */
	AI_LOOTER_SHOOTER_API bool ShouldDropGates(bool bUp, bool bArenaOn, EEncounterState State, bool bDone, bool bPlayerAlive);
}
