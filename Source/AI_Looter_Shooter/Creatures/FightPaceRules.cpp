#include "Creatures/FightPaceRules.h"
#include "Progression/LevelRules.h"

namespace
{
	/** Stands for "never" in a number of seconds (a pack that does no damage kills no one). */
	constexpr float Forever = 1.0e9f;
}

float FightPace::EnemyScale(int32 Level, float Growth)
{
	// Compounding, as the game grows them (FLevelRules::Compounded; the user, 2026-10-29 night).
	return FLevelRules::Compounded(Growth, Level);
}

float FightPace::StarterWard(int32 Level)
{
	// FIGHTS.md section 0 measured about 15 at level 1 and 41 at level 3: 13 more a level.
	return 15.f + 13.f * static_cast<float>(FMath::Max(Level, 1) - 1);
}

float FightPace::PlayerEHP(int32 Level, const FPlayerNumbers& Player, float Ward)
{
	return Player.BaseHealth * FLevelRules::Compounded(Player.HealthPerLevel, Level) + FMath::Max(Ward, 0.f);
}

float FightPace::TurnSeconds(const FKindNumbers& Kind)
{
	return FMath::Max(Kind.Windup + Kind.Recovery, KINDA_SMALL_NUMBER);
}

float FightPace::CycleSeconds(const FKindNumbers& Kind)
{
	return TurnSeconds(Kind) + FMath::Max(Kind.Cooldown, 0.f);
}

float FightPace::TurnsPerSecond(const FKindNumbers& Kind, int32 Count, const FFightNumbers& Fight)
{
	if (Count <= 0 || Fight.MeleeTokens <= 0)
	{
		return 0.f;
	}
	const float Turn = TurnSeconds(Kind);
	// Each creature starts an attack at most once a cycle, and not before its rest after a turn is over.
	const float OwnPace = static_cast<float>(Count) / FMath::Max(CycleSeconds(Kind), Turn + FMath::Max(Fight.TokenRest, 0.f));
	// More creatures than turns: someone always waits, so the turns pass from hand to hand and the rest costs nothing.
	float Rate = OwnPace;
	if (Count > Fight.MeleeTokens)
	{
		Rate = FMath::Min(Rate, static_cast<float>(Fight.MeleeTokens) / Turn);
	}
	if (Kind.MeleeGap > 0.f)
	{
		Rate = FMath::Min(Rate, 1.f / Kind.MeleeGap);
	}
	return Rate;
}

float FightPace::PackDps(const FKindNumbers& Kind, int32 Count, int32 Level, float RankDamage, const FFightNumbers& Fight)
{
	const float Hit = Kind.Damage * EnemyScale(Level, Fight.EnemyGrowth) * Fight.DamageScale * RankDamage;
	return TurnsPerSecond(Kind, Count, Fight) * Hit * FMath::Clamp(Kind.Uptime, 0.f, 1.f);
}

float FightPace::SecondsToDie(float EHP, float Dps)
{
	return Dps > KINDA_SMALL_NUMBER ? EHP / Dps : Forever;
}

float FightPace::GunDps(const FGunNumbers& Gun, int32 Level)
{
	const float Damage = Gun.Damage * Gun.DamageMultiplier * FLevelRules::Compounded(Gun.DamagePerLevel, Level);
	const float Shots = Gun.FireRate * Gun.FireRateMultiplier / 60.f;
	return Damage * Shots * FMath::Clamp(Gun.OnTarget, 0.f, 1.f);
}

float FightPace::SecondsToKill(float BaseHealth, int32 Level, float RankHealth, float RankDefense, const FGunNumbers& Gun, float EnemyGrowth)
{
	const float Health = BaseHealth * EnemyScale(Level, EnemyGrowth) * RankHealth;
	const float Dps = GunDps(Gun, Level) * (1.f - FMath::Clamp(RankDefense, 0.f, 0.9f));
	return Dps > KINDA_SMALL_NUMBER ? Health / Dps : Forever;
}

float FightPace::FittedUptime(EPaceKind Kind)
{
	// Fitted to Looter.Fight.Sim on the night of 2026-11-03 (six Basic at level 3 on a standing level 3 player, the pack that
	// presses: 5 melee turns, the ring at 3 m, DamageScale 1.2 then): the Unpaid killed in 15.2 s (25 attacks, 19 landed), the
	// spiders in 18.6 (bite 6), the slimes in 28.7; about 3 s of each is the walk in from the arc. The paper fits of the evening
	// (0.55 / 0.65 / 0.9) ran 1.3-2.3x too hot: a spider circles and skips back, a slime hops and misses, an Unpaid feints.
	switch (Kind)
	{
	case EPaceKind::Spider:
		return 0.4f;
	case EPaceKind::Slime:
		return 0.4f;
	case EPaceKind::Unpaid:
		return 0.45f;
	default:
		return 1.f;
	}
}

FightPace::FBandPair FightPace::StandingDeathBands(EPaceKind Kind)
{
	FBandPair Bands;
	Bands.Target = { 8.f, 12.f };
	Bands.Guard = { 6.f, 16.f };
	switch (Kind)
	{
	case EPaceKind::Spider:
		// "Leave the spiders" was lifted (the user, 2026-11-03), but the smaller damage scale alone doesn't bring a pack there: six
		// bite at their own pace (11 every 2.1 s each), so a still player lasts about 5 s (4.7-5.6; it was 2.7-3.2). Known short
		// of the target until the spider's own numbers change (its bite in Creature Kinds, or a melee gap); the guard still
		// catches a doubled pace (2.4 s) and lets a fixed one in.
		Bands.Guard = { 3.5f, 16.f };
		break;
	case EPaceKind::Spitter:
		// A ranged kind: no melee turns to count, so no standing-death band (the lobs are the Spitter's tests').
		Bands.Target = { 0.f, Forever };
		Bands.Guard = { 0.f, Forever };
		break;
	default:
		break;
	}
	return Bands;
}

FightPace::FBandPair FightPace::BasicKillBands(EPaceKind Kind)
{
	FBandPair Bands;
	// Kill speed (2026-11-03, the user's call: Borderlands' pace): a Regular dies to a level-matched Uncommon rifle at 60% on
	// the body in about a second (spider 150 and Unpaid 140 health in UCreatureKindSettings: two shotgun blasts, three
	// revolver shots), where the bands of 2026-10-21 asked 2.0-2.5 s and the loop read as plinking.
	switch (Kind)
	{
	case EPaceKind::Spider:
		Bands.Target = { 0.9f, 1.4f };
		Bands.Guard = { 0.6f, 1.9f };
		break;
	case EPaceKind::Unpaid:
		Bands.Target = { 0.8f, 1.3f };
		Bands.Guard = { 0.6f, 1.8f };
		break;
	case EPaceKind::Slime:
		Bands.Target = { 0.8f, 1.5f };
		Bands.Guard = { 0.6f, 1.9f };
		break;
	case EPaceKind::Spitter:
		Bands.Target = { 0.9f, 1.5f };
		Bands.Guard = { 0.6f, 2.0f };
		break;
	}
	return Bands;
}

FightPace::FBandPair FightPace::RankKillBands(EPaceKind Kind, bool bGravebound)
{
	const FBandPair Basic = BasicKillBands(Kind);
	FBandPair Bands;
	// Restless: x2 health less 10% defense is x2.2; Gravebound: x3 less 20% is x3.75 (the spider's 4-6 s and 7-10 s).
	if (bGravebound)
	{
		Bands.Target = { Basic.Target.Low * 3.4f, Basic.Target.High * 4.0f };
		Bands.Guard = { Basic.Guard.Low * 3.0f, Basic.Guard.High * 4.5f };
	}
	else
	{
		Bands.Target = { Basic.Target.Low * 2.0f, Basic.Target.High * 2.4f };
		Bands.Guard = { Basic.Guard.Low * 1.8f, Basic.Guard.High * 2.7f };
	}
	return Bands;
}
