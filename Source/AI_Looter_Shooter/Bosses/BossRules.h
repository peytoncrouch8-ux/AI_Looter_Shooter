#pragma once

#include "CoreMinimal.h"
#include "Bosses/BossTypes.h"

/**
 * The boss fight's rules as plain functions, so the tests check them without a fight: which phase a share of health is in,
 * how many adds a wave may raise, when an untargetable spell ends, where adds rise, how crits build to a stagger, and where
 * a loot shower's pieces fly. (Where a volley's pellets fly is UEnemyProjectileSubsystem::VolleyDirections.)
 */
namespace BossRules
{
	/**
	 * The most adds a boss has alive at once, whatever its data says: "at most 10 adds plus Abel in the boss fight"
	 * (Docs/Areas/RansomsRest.md, Performance plan).
	 */
	inline constexpr int32 MaxAliveAdds = 10;

	/** A share of health this close over a phase's line still counts as on it, so a hit landing on the line isn't lost to rounding. */
	inline constexpr float ShareTolerance = 0.0001f;

	/**
	 * The phases as a fight plays them: highest share first (the order they come in as health falls), each share kept in
	 * 0-1, and the first one starting at full health. No phases make one plain phase.
	 */
	AI_LOOTER_SHOOTER_API TArray<FBossPhase> Ordered(const TArray<FBossPhase>& Phases);

	/** The phase a boss is in at a share of its health: the last of the Ordered phases whose share it has fallen to. */
	AI_LOOTER_SHOOTER_API int32 PhaseAt(const TArray<FBossPhase>& OrderedPhases, float HealthShare);

	/**
	 * How many of a wave rise now: what it asks for, up to the cap less the adds alive. The cap is the wave's own (MaxAlive)
	 * when it sets one, never more than the boss's, and never more than MaxAliveAdds.
	 */
	AI_LOOTER_SHOOTER_API int32 AddsToSpawn(int32 Wanted, int32 Alive, int32 WaveCap, int32 BossCap);

	/**
	 * Whether an untargetable spell is over: its time is up (when it has one), or it waits for the adds and they are all
	 * dead with no wave of its phase still to come (a wave a moment later doesn't end it early).
	 */
	AI_LOOTER_SHOOTER_API bool UntargetableEnds(const FBossUntargetable& Spell, float Elapsed, int32 AliveAdds, bool bWavesPending);

	/** Count points evenly round a circle of Radius about Center, level with it, the first at StartDegrees (0 = +X). */
	AI_LOOTER_SHOOTER_API TArray<FVector> RingPoints(const FVector& Center, float Radius, int32 Count, float StartDegrees);

	// --- The weak spot's stagger (FBossStagger) ---

	/**
	 * A stagger's build-up (0 nothing, 1 it staggers) after a critical hit of CritDamage on a boss of MaxHealth: each crit
	 * adds its share of what staggers it. Never past 1; a boss that never staggers (CritShare 0) builds nothing.
	 */
	AI_LOOTER_SHOOTER_API float AddCritToStagger(float BuildUp, float CritDamage, float MaxHealth, const FBossStagger& Stagger);

	/** The build-up DeltaSeconds later: a full one drains to nothing over the stagger's DrainSeconds. */
	AI_LOOTER_SHOOTER_API float DrainStagger(float BuildUp, float DeltaSeconds, const FBossStagger& Stagger);

	// --- The loot shower (FBossLootShowerSettings) ---

	/** How far the Index-th piece of a shower lands (cm): spread between MinReach and MaxReach, never two alike in a row. */
	AI_LOOTER_SHOOTER_API float ShowerReach(int32 Index, const FBossLootShowerSettings& Shower);

	/**
	 * The Index-th piece's throw: up at the shower's UpSpeed, out along a turn of the golden angle from StartYaw (so pieces
	 * thrown one after another spread evenly all round), fast enough to land ShowerReach away on level ground under Gravity.
	 */
	AI_LOOTER_SHOOTER_API FVector ShowerThrow(int32 Index, float StartYaw, const FBossLootShowerSettings& Shower, float Gravity = 980.f);
}
