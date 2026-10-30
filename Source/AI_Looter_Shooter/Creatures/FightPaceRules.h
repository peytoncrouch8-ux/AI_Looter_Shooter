#pragma once

#include "CoreMinimal.h"

/**
 * The kinds the pace model knows (Docs/Work/2026-10-28-Core/FIGHTS.md, section 1 and 5): the four whose fights the job tunes.
 */
enum class EPaceKind : uint8
{
	Spider,
	Spitter,
	Slime,
	Unpaid,
};

/**
 * The fights' pace as plain arithmetic the tests (Looter.Fight.Pace) check without a level: how fast a pack of one kind
 * wears a standing player down, and how fast a level-matched gun wears a creature down. It is the throughput model of FIGHTS.md
 * section 0, over numbers the caller reads from the classes' defaults and the cvars, so a change to a creature, the tokens, the
 * damage scale or the gun that moves a fight out of its band shows up as a failed test, not as a surprise in play.
 *
 *   hits a second = min(tokens / turn, n / cycle)     turn = windup + recovery, cycle = turn + cooldown
 *   (a pack no bigger than its tokens has no waiter, so a creature's rest after a turn counts; a melee gap caps it again)
 *   damage a second = hits a second x hit x damage scale x land share x uptime
 *
 * Uptime is the share of a turn that really lands a hit once the walking in, the ring and the kit's moves are paid for. It is
 * not derivable from a class's numbers: it is fitted to the measured fights (FIGHTS.md section 0), per kind, and DEMO refits
 * it after the baseline run.
 */
namespace FightPace
{
	/** One kind's level 1 numbers (its class defaults) and its fitted uptime. */
	struct FKindNumbers
	{
		const TCHAR* Name = TEXT("");
		float Health = 100.f;
		float Damage = 10.f;
		float Windup = 0.55f;
		float Recovery = 0.45f;
		float Cooldown = 1.1f;
		/** Melee turns of this kind at one target begin at least this far apart (s); 0: no gap. */
		float MeleeGap = 0.f;
		/** The share of a turn that lands a hit on a standing player (fitted; see above). */
		float Uptime = 1.f;
	};

	/** The fight's cvars (Looter.Fight.*) and the level rules the model reads. */
	struct FFightNumbers
	{
		int32 MeleeTokens = 3;
		float TokenRest = 0.6f;
		float DamageScale = 1.8f;
		/** An enemy's health and damage grow by this share of the level below's a level, compounding (FLevelRules::EnemyGrowth). */
		float EnemyGrowth = 0.09f;
	};

	/** The player's health; the ward comes from StarterWard or the caller. */
	struct FPlayerNumbers
	{
		float BaseHealth = 100.f;
		/** FLevelRules::HealthPerLevel. */
		float HealthPerLevel = 0.06f;
	};

	/** The gun a level-matched player carries. */
	struct FGunNumbers
	{
		float Damage = 20.f;
		/** Rounds a minute. */
		float FireRate = 600.f;
		/** The share of the shots that land on the body (the user's 60% on target). */
		float OnTarget = 0.6f;
		/** The gun's damage grows by this share of the level below's a level, compounding (UWeaponDefinition::DamagePerLevel). */
		float DamagePerLevel = 0.09f;
		/** The rarity's multipliers (Uncommon 1.10 and 1.03). */
		float DamageMultiplier = 1.f;
		float FireRateMultiplier = 1.f;
	};

	/** A range of seconds. */
	struct FBand
	{
		float Low = 0.f;
		float High = 0.f;
		bool Contains(float Value) const { return Value >= Low && Value <= High; }
	};

	/**
	 * What the plan wants (Target, FIGHTS.md section 1: shown in the log, never a failure) and what the test enforces (Guard:
	 * wide enough that today's fights sit inside, narrow enough that a doubled turn count or a halved health is caught).
	 */
	struct FBandPair
	{
		FBand Target;
		FBand Guard;
	};

	// --- Levels ---

	/** An enemy's health and damage at Level against its level 1 values: (1 + Growth) ^ (Level - 1). */
	AI_LOOTER_SHOOTER_API float EnemyScale(int32 Level, float Growth);

	/** The ward the player's first gun brings, at Level: FIGHTS.md's two measured points (about 15 at level 1, 41 at level 3), carried on in a line. */
	AI_LOOTER_SHOOTER_API float StarterWard(int32 Level);

	/** The player's health and ward together at Level. */
	AI_LOOTER_SHOOTER_API float PlayerEHP(int32 Level, const FPlayerNumbers& Player, float Ward);

	// --- A pack's pressure on a standing player ---

	/** Seconds one melee turn takes: windup to the end of its recovery. */
	AI_LOOTER_SHOOTER_API float TurnSeconds(const FKindNumbers& Kind);

	/** Seconds from one attack's start to the same creature's next: the turn and its cooldown. */
	AI_LOOTER_SHOOTER_API float CycleSeconds(const FKindNumbers& Kind);

	/** Melee turns a second that Count of Kind take at one target (see the header). */
	AI_LOOTER_SHOOTER_API float TurnsPerSecond(const FKindNumbers& Kind, int32 Count, const FFightNumbers& Fight);

	/** Damage a second Count of Kind at Level do to a player who stands still. RankDamage: the rank's damage multiplier (1 for Basic). */
	AI_LOOTER_SHOOTER_API float PackDps(const FKindNumbers& Kind, int32 Count, int32 Level, float RankDamage, const FFightNumbers& Fight);

	/** Seconds a player with EHP lasts under Dps (a very large number when it is 0). */
	AI_LOOTER_SHOOTER_API float SecondsToDie(float EHP, float Dps);

	// --- The gun's pressure on a creature ---

	/** What a level-matched gun does to the body a second at Level (the gun's level is the creature's). */
	AI_LOOTER_SHOOTER_API float GunDps(const FGunNumbers& Gun, int32 Level);

	/**
	 * Seconds a gun at Level takes to kill a creature of the same level with BaseHealth at level 1: its health scaled by the
	 * rank's multiplier and the level, less the rank's defense (a body shot gets none of the crit's piercing).
	 */
	AI_LOOTER_SHOOTER_API float SecondsToKill(float BaseHealth, int32 Level, float RankHealth, float RankDefense, const FGunNumbers& Gun,
		float EnemyGrowth);

	// --- The bands ---

	/** The kind's fitted uptime on a standing player. */
	AI_LOOTER_SHOOTER_API float FittedUptime(EPaceKind Kind);

	/** A standing player facing 6 or more of one kind dies in this long. Spiders: the known exception (the user's "leave them", 2026-10-28). */
	AI_LOOTER_SHOOTER_API FBandPair StandingDeathBands(EPaceKind Kind);

	/** A level-matched Uncommon gun kills a Basic one of the kind in this long. */
	AI_LOOTER_SHOOTER_API FBandPair BasicKillBands(EPaceKind Kind);
	/**
	 * The same for a Restless (bGravebound false) and a Gravebound one: the Basic band stretched by what the rank adds (health x2
	 * and x3, less the defense), which lands the spider on FIGHTS.md's 4-6 s and 7-10 s.
	 */
	AI_LOOTER_SHOOTER_API FBandPair RankKillBands(EPaceKind Kind, bool bGravebound);
}
