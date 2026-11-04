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
	// Refit for the pack that presses (the audit's change 4, 2026-11-03: 5 melee turns, the ring at 3 m). Paper fits until
	// Looter.Fight.Sim measures them again (Docs/Work/2026-11-03-Phase3/FIGHTS.md has the working):
	switch (Kind)
	{
	case EPaceKind::Spider:
		// 0.75 was fitted on 10-28 (a pack killed a still level 3 in 3.2 s, nearly every bite landing). Since 10-31 a spider
		// circles before most bites (0.55-1.15 s, its turn held) and skips back after some, about 0.8 s more a cycle, which the
		// five turns no longer hide: 6 bite about 2 a second against the model's 2.9.
		return 0.65f;
	case EPaceKind::Slime:
		// FIGHTS.md section 0's "about 9 s"; its ring (530 cm, past its leap) is unchanged, and the model counts its turns itself.
		return 0.9f;
	case EPaceKind::Unpaid:
		// DEMO's 10-29 run (6 Basic, level 3, still): dead in 10.4 s, 175 taken, 3 turns held throughout: 0.43 at the old numbers.
		// Five turns let the pack start about 1.9 attacks a second instead of 1.5 (their walk in from the ring had kept a
		// turn busy): x1.3.
		return 0.55f;
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
