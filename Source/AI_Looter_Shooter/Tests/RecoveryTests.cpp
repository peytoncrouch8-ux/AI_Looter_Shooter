#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Combat/RecoverySettings.h"
#include "Combat/WoundsClose.h"
#include "Tests/AutomationCommon.h"
#include "Tests/RecoveryTestWorld.h"

// The recovery loop's first half (Docs/Polish/BorderlandsComparison.md, item 3): the wounds that close (their wait, ramp,
// rate, and the things that stop or hold them) and how often a kill leaves a soul-mote. The motes themselves are
// RecoveryMoteTests.cpp's. The subsystem that ticks the wounds only exists in played worlds, so the tests drive the same
// rule object (FWoundsClose::Tick) against a bare health component.

using namespace RecoveryTestWorld;

namespace
{
	/** Quarter seconds are exact in floats, so the sums below add up to the closed form with no rounding to forgive. */
	constexpr float Quarter = 0.25f;

	/** Steps a rule through Steps quarter seconds against a fixed health, adding up what it gave. */
	float StepQuarters(FWoundsClose& Wounds, const FRecoverySettings& Settings, int32 Steps, float Health = 10.f, float MaxHealth = 100.f,
		bool bHurt = false, bool bHeld = false)
	{
		float Given = 0.f;
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			Given += Wounds.Step(Settings, Quarter, Health, MaxHealth, bHurt, bHeld);
		}
		return Given;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryWoundsRulesTest, "Looter.Recovery.WoundsRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecoveryWoundsRulesTest::RunTest(const FString& Parameters)
{
	// The decided numbers: a 6 s wait, a 1.5 s ramp up to 12% of the most health a second.
	const FRecoverySettings Settings;
	TestEqual(TEXT("The wait is 6 s"), Settings.RegenDelaySeconds, 6.f);
	TestEqual(TEXT("The ramp is 1.5 s"), Settings.RegenRampSeconds, 1.5f);
	TestEqual(TEXT("The rate is 12% a second"), Settings.RegenRatePerSecond, 0.12f);
	TestEqual(TEXT("A mote heals 15% of the most health"), Settings.MoteHealShare, 0.15f);
	TestEqual(TEXT("...100 health: 15"), Settings.MoteHeal(100.f), 15.f, 0.0001f);
	TestEqual(TEXT("...200 health: 30"), Settings.MoteHeal(200.f), 30.f, 0.0001f);

	// The ramp's area, exactly: half the top rate over the ramp, then the full rate.
	TestEqual(TEXT("The ramp heals 9% of the bar"), Settings.RegenShareBetween(0.f, 1.5f), 0.09f, 0.00001f);
	TestEqual(TEXT("...a quarter of it, a quarter second in: 0.25%"), Settings.RegenShareBetween(0.f, Quarter), 0.0025f, 0.00001f);
	TestEqual(TEXT("After the ramp: 12% a second"), Settings.RegenShareBetween(3.f, 4.f), 0.12f, 0.00001f);
	TestEqual(TEXT("Nothing runs backwards"), Settings.RegenShareBetween(2.f, 1.f), 0.f);

	// The wait: nothing for six seconds, then a gentle start.
	{
		FWoundsClose Wounds;
		TestEqual(TEXT("A hurt gives nothing"), Wounds.Step(Settings, Quarter, 40.f, 100.f, true, false), 0.f);
		TestEqual(TEXT("5.75 s on: still nothing"), StepQuarters(Wounds, Settings, 23), 0.f);
		TestFalse(TEXT("...and not regenerating"), Wounds.IsRegenerating());
		TestEqual(TEXT("6 s on: the wait is over, nothing healed yet"), StepQuarters(Wounds, Settings, 1), 0.f);
		TestFalse(TEXT("...the run has not begun"), Wounds.IsRegenerating());
		const float First = StepQuarters(Wounds, Settings, 1);
		TestEqual(TEXT("The first quarter second heals a quarter point of 100"), First, 0.25f, 0.0001f);
		TestTrue(TEXT("...and it is regenerating"), Wounds.IsRegenerating());

		// The ramp climbs: 9 points by 1.5 s of running, then 3 points a quarter second.
		const float ToRamp = First + StepQuarters(Wounds, Settings, 5);
		TestEqual(TEXT("1.5 s into the run: 9 points"), ToRamp, 9.f, 0.001f);
		const float AtTop = StepQuarters(Wounds, Settings, 1);
		TestEqual(TEXT("Past the ramp: 12 points a second (3 a quarter)"), AtTop, 3.f, 0.001f);
		TestTrue(TEXT("The start was gentler than the top"), First < AtTop * 0.2f);
		TestEqual(TEXT("A second at the top: 12 more"), StepQuarters(Wounds, Settings, 4), 12.f, 0.001f);
	}

	// However the seconds are cut, the same health comes back (a slow frame heals what its short ones would).
	{
		float Totals[3] = {};
		const float Slices[3] = { 0.25f, 0.03125f, 0.0078125f };
		for (int32 Case = 0; Case < 3; ++Case)
		{
			FWoundsClose Wounds;
			Wounds.Step(Settings, 0.1f, 0.f, 1000.f, true, false);
			const int32 Steps = FMath::RoundToInt32(10.f / Slices[Case]);
			for (int32 Step = 0; Step < Steps; ++Step)
			{
				Totals[Case] += Wounds.Step(Settings, Slices[Case], 0.f, 1000.f, false, false);
			}
		}
		// 10 s after the hurt: 6 waited, 4 healing: 9% over the ramp, 2.5 s at 12%, of 1000.
		TestEqual(TEXT("10 s in quarter seconds: 390"), Totals[0], 390.f, 0.05f);
		TestEqual(TEXT("...in 1/32 s: the same"), Totals[1], 390.f, 0.05f);
		TestEqual(TEXT("...in 1/128 s: the same"), Totals[2], 390.f, 0.05f);
	}

	// It stops at full health, never past it, and the run ends.
	{
		FWoundsClose Wounds;
		Wounds.QuietSeconds = 6.f;
		TestEqual(TEXT("Five points short, a long step: five, not more"), Wounds.Step(Settings, 10.f, 95.f, 100.f, false, false), 5.f, 0.0001f);
		TestEqual(TEXT("At full health: nothing"), Wounds.Step(Settings, Quarter, 100.f, 100.f, false, false), 0.f);
		TestFalse(TEXT("...and the run is over"), Wounds.IsRegenerating());
		TestEqual(TEXT("A hair under full counts as full (no run that chases rounding)"), Wounds.Step(Settings, Quarter, 99.995f, 100.f, false, false), 0.f);
	}

	// Any hurt stops it and starts the wait over, the ramp from nothing.
	{
		FWoundsClose Wounds;
		Wounds.QuietSeconds = 6.f;
		StepQuarters(Wounds, Settings, 8);
		TestTrue(TEXT("Two seconds into a run"), Wounds.IsRegenerating());
		TestEqual(TEXT("A hurt: nothing"), Wounds.Step(Settings, Quarter, 40.f, 100.f, true, false), 0.f);
		TestFalse(TEXT("...the run is over"), Wounds.IsRegenerating());
		TestEqual(TEXT("...5.75 s later: nothing"), StepQuarters(Wounds, Settings, 23), 0.f);
		TestEqual(TEXT("...6 s: the wait is over"), StepQuarters(Wounds, Settings, 1), 0.f);
		TestEqual(TEXT("...then it starts at the bottom of the ramp"), StepQuarters(Wounds, Settings, 1), 0.25f, 0.0001f);
		// A hurt in the very step the wait would end beats the heal.
		FWoundsClose Late;
		Late.QuietSeconds = 5.9f;
		TestEqual(TEXT("A hurt as the wait ends: nothing"), Late.Step(Settings, 1.f, 40.f, 100.f, true, false), 0.f);
		TestEqual(TEXT("...and the wait is whole again"), Late.QuietSeconds, 0.f);
	}

	// A scene or a death holds it: nothing heals, the wait already served is kept, the ramp starts over.
	{
		FWoundsClose Wounds;
		Wounds.Step(Settings, Quarter, 40.f, 100.f, true, false);
		StepQuarters(Wounds, Settings, 12);
		TestEqual(TEXT("Three seconds waited"), Wounds.QuietSeconds, 3.f);
		TestEqual(TEXT("Held a hundred seconds: nothing"), Wounds.Step(Settings, 100.f, 40.f, 100.f, false, true), 0.f);
		TestEqual(TEXT("...and the three seconds are kept, not added to"), Wounds.QuietSeconds, 3.f);
		TestEqual(TEXT("2.75 s more: nothing"), StepQuarters(Wounds, Settings, 11), 0.f);
		TestEqual(TEXT("...3 s more: the wait is over"), StepQuarters(Wounds, Settings, 1), 0.f);
		TestTrue(TEXT("...and then it heals"), StepQuarters(Wounds, Settings, 1) > 0.f);

		// Held in the middle of a run: the run is over, and the ramp starts again after it.
		StepQuarters(Wounds, Settings, 8);
		TestTrue(TEXT("In a run"), Wounds.IsRegenerating());
		TestEqual(TEXT("Held: nothing"), Wounds.Step(Settings, Quarter, 40.f, 100.f, false, true), 0.f);
		TestFalse(TEXT("...not regenerating"), Wounds.IsRegenerating());
		TestEqual(TEXT("Let go: a quarter point again, not the top rate"), StepQuarters(Wounds, Settings, 1), 0.25f, 0.0001f);
	}

	// Nothing a bad number can do: a zero ramp is the full rate at once, a zero rate heals nothing.
	{
		FRecoverySettings Odd;
		Odd.RegenRampSeconds = 0.f;
		FWoundsClose Wounds;
		Wounds.QuietSeconds = 6.f;
		TestEqual(TEXT("No ramp: 3 points a quarter second from the first"), StepQuarters(Wounds, Odd, 1), 3.f, 0.001f);
		Odd.RegenRatePerSecond = 0.f;
		TestEqual(TEXT("No rate: nothing"), StepQuarters(Wounds, Odd, 4), 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryWoundsOnHealthTest, "Looter.Recovery.WoundsOnHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecoveryWoundsOnHealthTest::RunTest(const FString& Parameters)
{
	// The same rule against a real health component: damage and a drain both restart the wait, healing doesn't, and the
	// dead get nothing.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHealthComponent* Health = nullptr;
	ACharacter* Player = SpawnPlayer(World, FVector::ZeroVector, Health);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Its health"), Health))
	{
		return false;
	}
	const FRecoverySettings Settings;
	FWoundsClose Wounds;

	// What counts as a hurt.
	TestEqual(TEXT("Unhurt: count 0"), static_cast<int32>(Health->GetHurtCount()), 0);
	Hurt(Player, 40.f);
	TestEqual(TEXT("Damage is a hurt"), static_cast<int32>(Health->GetHurtCount()), 1);
	TestEqual(TEXT("...health 60"), Health->GetHealth(), 60.f, 0.001f);
	Health->Heal(1.f);
	TestEqual(TEXT("Healing is not a hurt"), static_cast<int32>(Health->GetHurtCount()), 1);
	Health->Heal(-5.f);
	Hurt(Player, 0.f);
	TestEqual(TEXT("Damage of nothing is not a hurt"), static_cast<int32>(Health->GetHurtCount()), 1);
	Health->SetHealth(60.f);

	// The wait, the start, and the restart on damage.
	TestEqual(TEXT("The first look (no time passes): nothing yet"), Wounds.Tick(Settings, *Health, 0.f, false), 0.f);
	for (int32 Step = 0; Step < 23; ++Step)
	{
		Wounds.Tick(Settings, *Health, Quarter, false);
	}
	TestEqual(TEXT("5.75 s without a hurt: health as it was"), Health->GetHealth(), 60.f, 0.001f);
	Wounds.Tick(Settings, *Health, Quarter, false);
	TestEqual(TEXT("6 s: the wait is over, nothing healed"), Health->GetHealth(), 60.f, 0.001f);
	TestEqual(TEXT("A quarter second of running: a quarter point"), Wounds.Tick(Settings, *Health, Quarter, false), 0.25f, 0.0001f);
	TestEqual(TEXT("...in the health itself"), Health->GetHealth(), 60.25f, 0.001f);
	TestTrue(TEXT("...and regenerating"), Wounds.IsRegenerating());

	Hurt(Player, 1.f);
	TestEqual(TEXT("A hit: nothing healed"), Wounds.Tick(Settings, *Health, Quarter, false), 0.f);
	TestFalse(TEXT("...the run is over"), Wounds.IsRegenerating());
	for (int32 Step = 0; Step < 23; ++Step)
	{
		Wounds.Tick(Settings, *Health, Quarter, false);
	}
	TestEqual(TEXT("...and the wait is whole again (5.75 s)"), Health->GetHealth(), 59.25f, 0.001f);

	// A drain (a Hungry iron's toll) is a hurt too, or the wounds would close it at once.
	Wounds.Tick(Settings, *Health, Quarter, false);
	Wounds.Tick(Settings, *Health, Quarter, false);
	TestTrue(TEXT("Running again"), Wounds.IsRegenerating());
	const float Taken = Health->Drain(5.f);
	TestEqual(TEXT("A drain takes its five"), Taken, 5.f, 0.001f);
	TestEqual(TEXT("...and is a hurt"), static_cast<int32>(Health->GetHurtCount()), 3);
	Wounds.Tick(Settings, *Health, Quarter, false);
	TestFalse(TEXT("...which stops the run"), Wounds.IsRegenerating());
	const float AfterDrain = Health->GetHealth();
	for (int32 Step = 0; Step < 23; ++Step)
	{
		Wounds.Tick(Settings, *Health, Quarter, false);
	}
	TestEqual(TEXT("...and restarts the wait"), Health->GetHealth(), AfterDrain, 0.001f);

	// Held (a scene): nothing heals however long, and it picks up where the wait stood.
	for (int32 Step = 0; Step < 20; ++Step)
	{
		TestEqual(TEXT("Held: nothing"), Wounds.Tick(Settings, *Health, 10.f, true), 0.f);
	}
	TestEqual(TEXT("...health unchanged"), Health->GetHealth(), AfterDrain, 0.001f);
	Wounds.Tick(Settings, *Health, Quarter, false);
	TestEqual(TEXT("...a quarter second more completes the wait that was left (5.75 + 0.25)"), Health->GetHealth(), AfterDrain, 0.001f);
	Wounds.Tick(Settings, *Health, Quarter, false);
	TestTrue(TEXT("...and then it heals"), Health->GetHealth() > AfterDrain);

	// Right up to full health, then it stops.
	Wounds.QuietSeconds = Settings.RegenDelaySeconds;
	for (int32 Step = 0; Step < 200; ++Step)
	{
		Wounds.Tick(Settings, *Health, Quarter, false);
	}
	TestEqual(TEXT("Given enough time, health is full"), Health->GetHealth(), Health->GetMaxHealth(), 0.011f);
	Wounds.Tick(Settings, *Health, Quarter, false);
	TestFalse(TEXT("...and the run is over"), Wounds.IsRegenerating());
	TestTrue(TEXT("...with nothing past the most"), Health->GetHealth() <= Health->GetMaxHealth());

	// The dead get nothing, and the rule forgets its wait.
	Wounds.QuietSeconds = Settings.RegenDelaySeconds;
	Wounds.RunSeconds = 2.f;
	Hurt(Player, 1.0e6f);
	TestTrue(TEXT("Dead"), Health->IsDead());
	for (int32 Step = 0; Step < 10; ++Step)
	{
		TestEqual(TEXT("Dead: nothing healed"), Wounds.Tick(Settings, *Health, 10.f, false), 0.f);
	}
	TestEqual(TEXT("...health stays at nothing"), Health->GetHealth(), 0.f);
	TestFalse(TEXT("...no run"), Wounds.IsRegenerating());
	TestEqual(TEXT("...the wait is forgotten"), Wounds.QuietSeconds, 0.f);
	Hurt(Player, 5.f);
	TestEqual(TEXT("A hit on the dead is not a hurt"), static_cast<int32>(Health->GetHurtCount()), 4);

	// A new life waits the whole delay.
	Health->ResetHealth();
	Hurt(Player, 30.f);
	Wounds.Tick(Settings, *Health, Quarter, false);
	for (int32 Step = 0; Step < 23; ++Step)
	{
		Wounds.Tick(Settings, *Health, Quarter, false);
	}
	TestEqual(TEXT("After a respawn and a hit: the wait again"), Health->GetHealth(), 70.f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryMoteDropsTest, "Looter.Recovery.MoteDrops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecoveryMoteDropsTest::RunTest(const FString& Parameters)
{
	// The chance of a mote by rank, rolled from a seed: 12% Basic, 25% Restless, 50% Gravebound and above, 2 to 3 from a boss.
	const FRecoverySettings Settings;
	struct FRankChance
	{
		ECreatureRank Rank;
		double Chance;
	};
	const FRankChance Chances[] = { { ECreatureRank::Basic, 0.12 }, { ECreatureRank::Rare, 0.25 }, { ECreatureRank::Epic, 0.5 },
		{ ECreatureRank::Legendary, 0.5 } };
	constexpr int32 Kills = 40000;
	double Last = -1.0;
	for (const FRankChance& Case : Chances)
	{
		FRandomStream Random(20261008 + static_cast<int32>(Case.Rank));
		int32 Motes = 0;
		int32 Most = 0;
		for (int32 Kill = 0; Kill < Kills; ++Kill)
		{
			const int32 Count = Settings.RollMotes(Case.Rank, Random);
			Motes += Count;
			Most = FMath::Max(Most, Count);
		}
		const double Share = static_cast<double>(Motes) / Kills;
		TestEqual(FString::Printf(TEXT("Rank %d: the table says %.0f%%"), static_cast<int32>(Case.Rank), Case.Chance * 100.0),
			static_cast<double>(Settings.DropChance(Case.Rank)), Case.Chance, 0.0001);
		TestTrue(FString::Printf(TEXT("Rank %d: %d kills left motes on %.2f%% (decided %.0f%%, within a point and a half)"), static_cast<int32>(Case.Rank),
			Kills, Share * 100.0, Case.Chance * 100.0), FMath::Abs(Share - Case.Chance) <= 0.015);
		TestEqual(FString::Printf(TEXT("Rank %d: one mote at most"), static_cast<int32>(Case.Rank)), Most, 1);
		TestTrue(FString::Printf(TEXT("Rank %d: at least as likely as the rank below"), static_cast<int32>(Case.Rank)), Share >= Last - 0.01);
		Last = Share;
	}

	// The same seed rolls the same kills.
	{
		FRandomStream A(77);
		FRandomStream B(77);
		bool bSame = true;
		for (int32 Kill = 0; Kill < 500; ++Kill)
		{
			bSame &= Settings.RollMotes(ECreatureRank::Rare, A) == Settings.RollMotes(ECreatureRank::Rare, B);
		}
		TestTrue(TEXT("A seeded stream repeats its rolls"), bSame);
	}

	// A boss always leaves two or three.
	{
		FRandomStream Random(5);
		int32 Counts[4] = {};
		for (int32 Kill = 0; Kill < 3000; ++Kill)
		{
			const int32 Count = Settings.RollMotes(ECreatureRank::Boss, Random);
			if (Count >= 0 && Count < 4)
			{
				++Counts[Count];
			}
		}
		TestEqual(TEXT("A boss: never none"), Counts[0], 0);
		TestEqual(TEXT("...never one"), Counts[1], 0);
		TestTrue(TEXT("...two on some"), Counts[2] > 1000);
		TestTrue(TEXT("...three on some"), Counts[3] > 1000);
		TestEqual(TEXT("...always two or three (nothing more)"), Counts[2] + Counts[3], 3000);
		TestEqual(TEXT("A boss's chance is 1"), Settings.DropChance(ECreatureRank::Boss), 1.f);
	}

	// Numbers other than the defaults still hold: a chance of 0 never drops, of 1 always does.
	{
		FRecoverySettings Odd;
		Odd.DropChanceBasic = 0.f;
		Odd.DropChanceRestless = 1.f;
		FRandomStream Random(9);
		bool bNone = true;
		bool bAlways = true;
		for (int32 Kill = 0; Kill < 1000; ++Kill)
		{
			bNone &= Odd.RollMotes(ECreatureRank::Basic, Random) == 0;
			bAlways &= Odd.RollMotes(ECreatureRank::Rare, Random) == 1;
		}
		TestTrue(TEXT("A chance of nothing never drops"), bNone);
		TestTrue(TEXT("A chance of one always does"), bAlways);
	}
	return true;
}

#endif
