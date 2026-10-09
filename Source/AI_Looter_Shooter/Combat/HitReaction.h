#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"

/**
 * One creature's reactions to the hits it takes, as plain rules over the world's clock (no engine state, so the tests
 * drive it): a hit-stop (its own time all but stopped for an instant, never the world's) and a stagger (its brain paused
 * and a shove back). UCreatureHitReactionComponent plays them on the creature.
 *
 *  - Hit-stop: only on a critical hit or the killing one. A crit's is 45 ms and comes at most every HitStopCooldown (a
 *    rifle on a head would otherwise freeze it half the time); a kill's is 65 ms, a critical kill's 80, always.
 *  - Stagger: on a heavy hit, a crit or a burst of damage worth a fifth of its health inside BurstWindow (a shotgun's
 *    pellets at close range land together). Basic 0.32 s, Restless 0.26, Gravebound 0.2; then StaggerCooldown before the
 *    next, so nothing is stun-locked. Soulfed monsters and bosses never stagger (their fights are made around their
 *    moves), and bosses get no hit-stop either: a boss's own code runs its big moments.
 *  - A melee strike (UPlayerMeleeComponent) is the panic button against a lunge, so it always lands heavy: a hit-stop of
 *    MeleeHitStop with no cooldown, and a stagger MeleeStaggerScale times the rank's whatever its share of health, its
 *    cooldown skipped (the strike's own 0.6 s keeps it from stun-locking much). Not while a stagger still runs, and the
 *    stagger cooldown starts after it as usual, so guns can't chain one straight on.
 */
struct AI_LOOTER_SHOOTER_API FHitReaction
{
	/** What one hit starts: seconds of hit-stop and of stagger (0: none). */
	struct FResponse
	{
		float HitStopSeconds = 0.f;
		float StaggerSeconds = 0.f;
		/** The share of its health the hit's burst has taken so far (a shotgun blast's pellets together): the shove's strength. */
		float BurstShare = 0.f;
	};

	/**
	 * The creature took Damage (of MaxHealth) at Now; bKilled: this hit killed it. bMelee: a melee strike's (so is any hit
	 * while an FMeleeScope is open).
	 */
	FResponse OnHit(double Now, float Damage, float MaxHealth, bool bCritical, bool bKilled, ECreatureRank Rank, bool bMelee = false);

	/**
	 * Marks the damage dealt while it lives as a melee strike's. Whoever hears the hit without being told what dealt it
	 * (UCreatureHitReactionComponent hears it through the health's OnDamaged, which carries no damage type) gets it this
	 * way: the strike opens one around its damage, which the engine and the health hand on at once, on the game thread.
	 */
	struct FMeleeScope
	{
		FMeleeScope();
		~FMeleeScope();
		FMeleeScope(const FMeleeScope&) = delete;
		FMeleeScope& operator=(const FMeleeScope&) = delete;

		/** A melee strike's damage is being dealt right now. */
		static bool IsOpen();
	};

	bool IsHitStopped(double Now) const { return Now < HitStopEnd; }
	bool IsStaggered(double Now) const { return Now < StaggerEnd; }

	/** When the latest hit-stop ends (the world's seconds). */
	double GetHitStopEnd() const { return HitStopEnd; }

	/** Forgets everything (a creature coming back). */
	void Reset() { *this = FHitReaction(); }

	/** How far its own time runs during a hit-stop: all but stopped (not quite 0, which some motion code divides by). */
	static constexpr float HitStopDilation = 0.02f;
	static constexpr float CritHitStop = 0.045f;
	static constexpr float KillHitStop = 0.065f;
	static constexpr float CritKillHitStop = 0.08f;
	static constexpr float HitStopCooldown = 0.25f;

	/** Damage landing within this long of the burst's first hit counts together (a shotgun blast's pellets). */
	static constexpr float BurstWindow = 0.08f;
	/** A burst worth this share of its health is a heavy hit. */
	static constexpr float HeavyShare = 0.2f;
	static constexpr float StaggerCooldown = 1.1f;

	/** A melee strike's hit-stop: a touch longer than a crit's, so the blow's weight is felt; a kill's own wins if longer. */
	static constexpr float MeleeHitStop = 0.06f;
	/** A melee strike staggers this many times as long as a heavy hit does (Basic 0.48 s): time to step back and shoot. */
	static constexpr float MeleeStaggerScale = 1.5f;

	/** How long a heavy hit staggers a creature of Rank (0: never). */
	static float StaggerSecondsFor(ECreatureRank Rank);
	/** Whether a creature of Rank gets hit-stops at all. */
	static bool HitStopsFor(ECreatureRank Rank);

private:
	double HitStopEnd = -1.0;
	/** A crit's hit-stop can come again from here. */
	double HitStopReady = -1.0;
	double StaggerEnd = -1.0;
	/** A stagger can come again from here. */
	double StaggerReady = -1.0;
	double BurstStart = -1000.0;
	float BurstDamage = 0.f;
};
