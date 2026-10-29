#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/FightPaceRules.h"
#include "Creatures/FightTuning.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Progression/LevelRules.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTypes.h"

// The fights' pace (FIGHTS.md section 5, lane MEASURE): the throughput model of Creatures/FightPaceRules.h over each kind's class
// defaults and the fight cvars' defaults, and a level-matched Uncommon rifle's kill times. It does not play a fight: it pins the
// numbers that decide how one goes (turns, rests, bites, health, the gun), so anyone who moves one out of its band, the guns'
// work included, sees this test fail. Each band has a Target (what the plan wants; shown as a warning when missed, never a
// failure) and a Guard (what is enforced: today's fights sit inside it, a doubled turn count or a halved health does not).
// The spiders are a known exception to the standing-player target: the user said "leave them" (2026-10-28), and a pack still
// kills a standing player in 3-4 s.

namespace
{
	/** One kind's numbers as its class starts, and its fitted uptime. */
	FightPace::FKindNumbers NumbersOf(EPaceKind Kind, const TCHAR* Name, const ACreatureBase& Defaults)
	{
		FightPace::FKindNumbers Numbers;
		Numbers.Name = Name;
		const UHealthComponent* Health = Defaults.FindComponentByClass<UHealthComponent>();
		Numbers.Health = Health ? Health->MaxHealth : 100.f;
		Numbers.Damage = Defaults.AttackDamage;
		Numbers.Windup = Defaults.AttackWindup;
		Numbers.Recovery = Defaults.AttackRecovery;
		Numbers.Cooldown = Defaults.AttackCooldown;
		// The kind's own gap times the cvar's scale (Looter.Fight.MeleeGap; 0 turns every gap off).
		Numbers.MeleeGap = Defaults.GetMeleeGap() * FFightTuning().MeleeGap;
		Numbers.Uptime = FightPace::FittedUptime(Kind);
		return Numbers;
	}

	/** The fight cvars' defaults and the level rules' (a default-made tuning, never what the user has set in this editor). */
	FightPace::FFightNumbers DefaultFight()
	{
		const FFightTuning Tuning;
		FightPace::FFightNumbers Fight;
		Fight.MeleeTokens = Tuning.MeleeTokens;
		Fight.TokenRest = Tuning.TokenRest;
		Fight.DamageScale = Tuning.DamageScale;
		Fight.EnemyGrowth = FLevelRules().EnemyGrowth;
		return Fight;
	}

	FightPace::FPlayerNumbers DefaultPlayer()
	{
		FightPace::FPlayerNumbers Player;
		Player.HealthPerLevel = FLevelRules().HealthPerLevel;
		return Player;
	}

	/** The rifle a level-matched player carries: FWeaponStats' defaults (20 damage, 600 rpm) at the Uncommon tier. */
	FightPace::FGunNumbers UncommonRifle()
	{
		const FWeaponStats Stats;
		const UWeaponDefinition* Definition = GetDefault<UWeaponDefinition>();
		const FWeaponRarityInfo& Uncommon = Definition->GetRarityInfo(EWeaponRarity::Uncommon);
		FightPace::FGunNumbers Gun;
		Gun.Damage = Stats.Damage;
		Gun.FireRate = Stats.FireRate;
		Gun.DamagePerLevel = Definition->DamagePerLevel;
		Gun.DamageMultiplier = Uncommon.DamageMultiplier;
		Gun.FireRateMultiplier = Uncommon.FireRateMultiplier;
		return Gun;
	}

	struct FPaceKind
	{
		EPaceKind Kind;
		const TCHAR* Name;
		FightPace::FKindNumbers Numbers;
	};

	TArray<FPaceKind> AllKinds()
	{
		return {
			{ EPaceKind::Spider, TEXT("Spider"), NumbersOf(EPaceKind::Spider, TEXT("Spider"), *GetDefault<ASpiderCreature>()) },
			{ EPaceKind::Spitter, TEXT("Spitter"), NumbersOf(EPaceKind::Spitter, TEXT("Spitter"), *GetDefault<ASpitterSpider>()) },
			{ EPaceKind::Slime, TEXT("Slime"), NumbersOf(EPaceKind::Slime, TEXT("Slime"), *GetDefault<ASlimeCreature>()) },
			{ EPaceKind::Unpaid, TEXT("Unpaid"), NumbersOf(EPaceKind::Unpaid, TEXT("Unpaid"), *GetDefault<AUnpaidCreature>()) },
		};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightPaceModelTest, "Looter.Fight.Pace.Model",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFightPaceModelTest::RunTest(const FString& Parameters)
{
	// Levels: 8% of the level 1 value a level, for the enemy, the player's health and the gun alike.
	TestNearlyEqual(TEXT("An enemy at level 1 is its class"), FightPace::EnemyScale(1, 0.08f), 1.f);
	TestNearlyEqual(TEXT("...at level 11, x1.8"), FightPace::EnemyScale(11, 0.08f), 1.8f);
	TestNearlyEqual(TEXT("The starter ward is 15 at level 1 and 41 at level 3"), FightPace::StarterWard(1), 15.f);
	TestNearlyEqual(TEXT("...41 at level 3"), FightPace::StarterWard(3), 41.f);
	TestNearlyEqual(TEXT("A level 3 player: 116 health and a 41 ward is 157"), FightPace::PlayerEHP(3, FightPace::FPlayerNumbers(), 41.f), 157.f, 0.01f);

	// The turns: a spider's numbers (0.55 / 0.45 / 1.1).
	FightPace::FKindNumbers Bite;
	Bite.Windup = 0.55f;
	Bite.Recovery = 0.45f;
	Bite.Cooldown = 1.1f;
	// The model's own arithmetic, with its own numbers (3 turns, 0.6 s rest), not the cvars': the Bands test reads those.
	const FightPace::FFightNumbers Fight;
	TestNearlyEqual(TEXT("A turn is windup and recovery"), FightPace::TurnSeconds(Bite), 1.0f);
	TestNearlyEqual(TEXT("...a cycle adds the cooldown"), FightPace::CycleSeconds(Bite), 2.1f);
	TestNearlyEqual(TEXT("One alone bites every cycle"), FightPace::TurnsPerSecond(Bite, 1, Fight), 1.f / 2.1f, 0.001f);
	TestNearlyEqual(TEXT("Six go at their own pace, 6 in 2.1 s (the 3 turns at once would allow 3 a second)"), FightPace::TurnsPerSecond(Bite, 6, Fight), 6.f / 2.1f, 0.001f);
	TestTrue(TEXT("Twelve don't bite faster than the turns pass (3 a second)"), FightPace::TurnsPerSecond(Bite, 12, Fight) <= 3.f + 0.001f);
	FightPace::FFightNumbers MoreTurns = Fight;
	MoreTurns.MeleeTokens = Fight.MeleeTokens + 2;
	TestTrue(TEXT("Two more turns at once speed a big pack up"), FightPace::TurnsPerSecond(Bite, 12, MoreTurns) > FightPace::TurnsPerSecond(Bite, 12, Fight));
	FightPace::FKindNumbers Gap = Bite;
	Gap.MeleeGap = 0.9f;
	TestTrue(TEXT("A melee gap of 0.9 s caps a pack at 1.1 turns a second"), FightPace::TurnsPerSecond(Gap, 12, Fight) <= 1.f / 0.9f + 0.001f);
	TestNearlyEqual(TEXT("No creatures, no turns"), FightPace::TurnsPerSecond(Bite, 0, Fight), 0.f);

	// The damage and the clock.
	FightPace::FKindNumbers Hitter = Bite;
	Hitter.Damage = 10.f;
	Hitter.Uptime = 1.f;
	FightPace::FFightNumbers Plain = Fight;
	Plain.DamageScale = 2.f;
	TestNearlyEqual(TEXT("A hit is the bite x the level's scale x the damage scale x the rank"),
		FightPace::PackDps(Hitter, 1, 11, 1.5f, Plain), (1.f / 2.1f) * 10.f * 1.8f * 2.f * 1.5f, 0.01f);
	Hitter.Uptime = 0.5f;
	TestNearlyEqual(TEXT("Uptime halves what lands"), FightPace::PackDps(Hitter, 1, 1, 1.f, Plain), (1.f / 2.1f) * 10.f * 2.f * 0.5f, 0.01f);
	TestNearlyEqual(TEXT("100 health under 20 a second lasts 5 s"), FightPace::SecondsToDie(100.f, 20.f), 5.f);
	TestTrue(TEXT("Under no damage it lasts for ever"), FightPace::SecondsToDie(100.f, 0.f) > 1.0e6f);

	// The gun: a Common rifle is 200 a second on paper and 120 on the body (the plan's number).
	const FightPace::FGunNumbers Common;
	TestNearlyEqual(TEXT("A Common rifle puts 120 a second on the body"), FightPace::GunDps(Common, 1), 120.f, 0.01f);
	TestNearlyEqual(TEXT("...8% more a level"), FightPace::GunDps(Common, 3), 120.f * 1.16f, 0.01f);
	TestNearlyEqual(TEXT("A level-matched gun kills the same in the same time at any level"),
		FightPace::SecondsToKill(230.f, 1, 1.f, 0.f, Common, 0.08f), FightPace::SecondsToKill(230.f, 9, 1.f, 0.f, Common, 0.08f), 0.01f);
	TestNearlyEqual(TEXT("A Common rifle takes the spider's 230 health in 1.9 s"), FightPace::SecondsToKill(230.f, 1, 1.f, 0.f, Common, 0.08f), 230.f / 120.f, 0.01f);
	TestTrue(TEXT("Defense slows a kill"), FightPace::SecondsToKill(230.f, 1, 1.f, 0.2f, Common, 0.08f) > FightPace::SecondsToKill(230.f, 1, 1.f, 0.f, Common, 0.08f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightPaceBandsTest, "Looter.Fight.Pace.Bands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFightPaceBandsTest::RunTest(const FString& Parameters)
{
	const FightPace::FFightNumbers Fight = DefaultFight();
	const FightPace::FPlayerNumbers Player = DefaultPlayer();
	const FightPace::FGunNumbers Gun = UncommonRifle();
	const TArray<FPaceKind> Kinds = AllKinds();

	// A standing player and six of a kind (the plan's "full pack"), at levels 1, 3 and 5: how long until they die.
	for (const FPaceKind& Each : Kinds)
	{
		const FightPace::FBandPair Bands = FightPace::StandingDeathBands(Each.Kind);
		if (Each.Kind == EPaceKind::Spitter)
		{
			continue;
		}
		for (const int32 Level : { 1, 3, 5 })
		{
			const float Ward = FightPace::StarterWard(Level);
			const float Dps = FightPace::PackDps(Each.Numbers, 6, Level, 1.f, Fight);
			const float Seconds = FightPace::SecondsToDie(FightPace::PlayerEHP(Level, Player, Ward), Dps);
			const FString Line = FString::Printf(TEXT("6 %s, level %d: a standing player dies in %.1f s (%.0f damage a second; guard %.0f-%.0f, target %.0f-%.0f)"),
				Each.Name, Level, Seconds, Dps, Bands.Guard.Low, Bands.Guard.High, Bands.Target.Low, Bands.Target.High);
			TestTrue(*Line, Bands.Guard.Contains(Seconds));
			if (!Bands.Target.Contains(Seconds))
			{
				AddWarning(FString::Printf(TEXT("%s: outside the target%s"), *Line, Each.Kind == EPaceKind::Spider ? TEXT(" (known exception: the spiders stay as they are)") : TEXT("")));
			}
			else
			{
				AddInfo(Line);
			}
		}
	}

	// Kill times with a level-matched Uncommon rifle, 60% on the body: Basic, Restless, Gravebound.
	const ECreatureRank Ranks[] = { ECreatureRank::Basic, ECreatureRank::Rare, ECreatureRank::Epic };
	for (const FPaceKind& Each : Kinds)
	{
		for (const int32 Level : { 1, 3, 5 })
		{
			for (const ECreatureRank Rank : Ranks)
			{
				const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Rank);
				const float Seconds = FightPace::SecondsToKill(Each.Numbers.Health, Level, Info.HealthMultiplier, Info.Defense, Gun, Fight.EnemyGrowth);
				const FightPace::FBandPair Bands = Rank == ECreatureRank::Basic ? FightPace::BasicKillBands(Each.Kind)
					: FightPace::RankKillBands(Each.Kind, Rank == ECreatureRank::Epic);
				const FString Line = FString::Printf(TEXT("%s %s, level %d: dies in %.2f s to a level-matched Uncommon rifle (guard %.1f-%.1f, target %.1f-%.1f)"),
					*UCreatureRankSettings::GetTierName(Rank), Each.Name, Level, Seconds, Bands.Guard.Low, Bands.Guard.High, Bands.Target.Low, Bands.Target.High);
				TestTrue(*Line, Bands.Guard.Contains(Seconds));
				if (!Bands.Target.Contains(Seconds))
				{
					AddWarning(FString::Printf(TEXT("%s: outside the target"), *Line));
				}
				else
				{
					AddInfo(Line);
				}
			}
		}
	}
	return true;
}

#endif
