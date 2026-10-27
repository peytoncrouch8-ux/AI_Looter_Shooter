#pragma once

#include "CoreMinimal.h"

/**
 * The grenade kinds' numbers beside the grave-salt jar (round 4, the user's pick 2026-10-27: "grenade types: a few kinds
 * found as loot"). Each kind's burst is the salt jar's (FGraveSaltTuning, the Looter.Grenade.* cvars) times its own
 * shares, so a change to the salt jar moves every kind with it and the early ramp (EarlyShare) holds for all of them.
 * The Looter.Grenade.Kinds.* / .Pitch.* / .Scatter.* / .Cinder.* cvars set them (Current(), read at each throw or burst);
 * a default-made one holds the defaults, which the tests read.
 *
 * A boss's cap (FGraveSaltTuning::BossShareCap, 8%) is the whole jar's: a jar that bursts more than once (the scatter's
 * brood, the cinder's bed) splits it, so no kind takes more of a boss's bar than the salt jar does.
 */
struct AI_LOOTER_SHOOTER_API FGrenadeKindTuning
{
	// --- Drops (Looter.Grenade.Kinds.*) ---

	/** Looter.Grenade.Kinds.FromLevel: a dropped tin is always plain salt below this player level (the tutorial teaches the salt jar). */
	int32 FromLevel = 2;
	/** Looter.Grenade.Kinds.Share: from then on, the chance a dropped tin is one of the three other kinds (else salt). */
	float SpecialShare = 0.35f;
	/** Looter.Grenade.Kinds.PitchWeight / .ScatterWeight / .CinderWeight: how the special drops split between them (0: never). */
	float PitchWeight = 1.f;
	float ScatterWeight = 1.f;
	float CinderWeight = 1.f;

	// --- The pitch jar (Looter.Grenade.Pitch.*): sticks where it lands, tars what it doesn't kill ---

	/** .Damage / .Radius / .Core: its burst's heart, reach and full-damage core as shares of the salt jar's (900, 4 m, 1 m). */
	float PitchDamage = 1.f;
	float PitchRadius = 0.75f;
	float PitchCore = 1.5f;
	/** .StuckFuse (s): stuck on a creature, it bursts within this long (a stuck jar never rides a charge into the player). */
	float PitchStuckFuse = 0.9f;
	/** .Slow / .SlowSeconds: what it doesn't kill is tarred: walking at this share of its speed this long (never a boss). */
	float PitchSlow = 0.5f;
	float PitchSlowSeconds = 3.f;

	// --- The scatter jar (Looter.Grenade.Scatter.*): bursts, and flings a brood of small jars that pop round it ---

	/** .Damage / .Radius: its own burst, as shares of the salt jar's. */
	float ScatterDamage = 0.5f;
	float ScatterRadius = 0.75f;
	/** .Count: the small jars it flings (spread evenly round, a little jitter). */
	int32 ScatterCount = 4;
	/** .BroodDamage / .BroodRadius: each small jar's burst, as shares of the salt jar's (360 heart at full, 2 m). */
	float BroodDamage = 0.4f;
	float BroodRadius = 0.5f;
	/** .Speed / .Lift (cm/s): how hard they're flung out across the ground and up. */
	float BroodSpeed = 520.f;
	float BroodLift = 380.f;
	/** .FuseMin / .FuseMax (s): each small jar's fuse, so they pop one after another. */
	float BroodFuseMin = 0.45f;
	float BroodFuseMax = 0.8f;

	// --- The cinder jar (Looter.Grenade.Cinder.*): a smaller burst that leaves a bed of burning cinders ---

	/** .Damage / .Radius: its burst, as shares of the salt jar's. */
	float CinderDamage = 0.6f;
	float CinderRadius = 0.85f;
	/** .BedRadius (cm) / .BedSeconds: the burning bed left on the ground. */
	float BedRadius = 260.f;
	float BedSeconds = 5.f;
	/** .BedShare: what the bed deals a second to what stands in it, as a share of the salt jar's heart (90 a second at full). */
	float BedShare = 0.1f;
	/** .BedTick (s): how often it burns what stands in it. */
	float BedTick = 0.5f;

	/** The tuning as the cvars hold it now. */
	static FGrenadeKindTuning Current();
};
