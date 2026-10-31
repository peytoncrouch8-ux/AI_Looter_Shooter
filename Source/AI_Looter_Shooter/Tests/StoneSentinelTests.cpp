#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Stone Sentinel (AStoneSentinelCreature, StoneSentinelRules): its rules alone (waking, the slam's wind-up and reach,
// its stone, its pose, its gait, no Fleet), then its class's numbers. Neither needs its model: the rules are plain numbers
// and the class's defaults are made without it (a stand-in takes its place).

#include "Combat/HealthComponent.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/FightRules.h"
#include "Creatures/FightTuning.h"
#include "Creatures/StoneSentinelCreature.h"
#include "Creatures/StoneSentinelRules.h"
#include "Elements/ElementBody.h"
#include "Elements/ElementRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneSentinelRulesTest, "Looter.Creatures.Sentinel.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoneSentinelRulesTest::RunTest(const FString& Parameters)
{
	const FStoneSentinelTuning Tuning;

	// It wakes for a player about 9 m off on its own level of ground (or a shot), never one far above or below.
	TestTrue(TEXT("It wakes at about 9 m"), Tuning.WakeDistance >= 800.f && Tuning.WakeDistance <= 1000.f);
	TestTrue(TEXT("8.5 m off: it wakes"), StoneSentinelRules::ShouldWake(850.f, 0.f, Tuning));
	TestFalse(TEXT("9.5 m off: still a statue"), StoneSentinelRules::ShouldWake(950.f, 0.f, Tuning));
	TestFalse(TEXT("Right over it on a roof 5 m up: still a statue"), StoneSentinelRules::ShouldWake(300.f, 500.f, Tuning));
	TestTrue(TEXT("...2 m up a slope: it wakes"), StoneSentinelRules::ShouldWake(300.f, -200.f, Tuning));

	// The slam: a long wind-up, its aim set late in it, 2.8 m in front.
	TestTrue(TEXT("A 1.1 s wind-up"), FMath::IsNearlyEqual(Tuning.SlamWindup, 1.1f));
	TestTrue(TEXT("It turns after them early in the heave"), StoneSentinelRules::TracksAim(0.5f, Tuning.SlamWindup, Tuning.SlamAimShare));
	TestFalse(TEXT("...not in its last part: a late sidestep dodges"),
		StoneSentinelRules::TracksAim(Tuning.SlamWindup * 0.8f, Tuning.SlamWindup, Tuning.SlamAimShare));
	TestTrue(TEXT("A 2.8 m reach"), FMath::IsNearlyEqual(Tuning.SlamReach, 280.f));
	const FVector Forward = FVector::ForwardVector;
	TestTrue(TEXT("2.7 m in front: hit"), StoneSentinelRules::IsSlamHit(Forward, FVector(270.0, 0.0, 0.0), Tuning));
	TestFalse(TEXT("3 m in front: missed"), StoneSentinelRules::IsSlamHit(Forward, FVector(300.0, 0.0, 0.0), Tuning));
	TestFalse(TEXT("2 m behind: missed"), StoneSentinelRules::IsSlamHit(Forward, FVector(-200.0, 0.0, 0.0), Tuning));
	TestTrue(TEXT("2 m off at 60 degrees: hit"), StoneSentinelRules::IsSlamHit(Forward, FVector(100.0, 173.0, 0.0), Tuning));
	TestFalse(TEXT("2 m in front, 3 m up: missed"), StoneSentinelRules::IsSlamHit(Forward, FVector(200.0, 0.0, 300.0), Tuning));
	TestTrue(TEXT("A bigger one reaches farther (3.2 m at 1.2x)"), StoneSentinelRules::IsSlamHit(Forward, FVector(320.0, 0.0, 0.0), Tuning, 1.2f));
	TestTrue(TEXT("It starts its slam from inside its reach"), StoneSentinelRules::AttackRange < Tuning.SlamReach);

	// The pose follows the slam: fists up through the heave, down at its end (the blow), bent over them, standing again.
	const FStoneSentinelPose Start = StoneSentinelRules::SlamPoseAt(0.f, Tuning);
	const FStoneSentinelPose Top = StoneSentinelRules::SlamPoseAt(Tuning.SlamWindup - Tuning.SlamFallSeconds, Tuning);
	const FStoneSentinelPose Blow = StoneSentinelRules::SlamPoseAt(Tuning.SlamWindup, Tuning);
	const FStoneSentinelPose Held = StoneSentinelRules::SlamPoseAt(Tuning.SlamWindup + Tuning.SlamRecovery * Tuning.SlamHoldShare * 0.5f, Tuning);
	const FStoneSentinelPose End = StoneSentinelRules::SlamPoseAt(Tuning.SlamWindup + Tuning.SlamRecovery, Tuning);
	const FStoneSentinelPose Idle = StoneSentinelRules::IdlePose();
	TestTrue(TEXT("It starts standing"), FMath::IsNearlyEqual(Start.ArmSwing, Idle.ArmSwing, 1.f));
	TestTrue(TEXT("Its fists are up over its hood at the heave's top, elbows bent"), Top.ArmSwing >= 120.f && Top.ElbowBend >= 60.f);
	TestTrue(TEXT("...and down in front at the blow, bowed over them"), Blow.ArmSwing < 90.f && Blow.Bow >= 20.f);
	TestTrue(TEXT("It stays down a moment: the window"), FMath::IsNearlyEqual(Held.ArmSwing, Blow.ArmSwing, 0.5f));
	TestTrue(TEXT("It stands again by the recovery's end"), FMath::IsNearlyEqual(End.ArmSwing, Idle.ArmSwing, 1.f)
		&& FMath::IsNearlyEqual(End.Bow, Idle.Bow, 1.f));

	// Stone: six dead miners' health (twice its first, 2026-10-31), a quarter of each hit shrugged off, a tier's on top (never added).
	TestTrue(TEXT("Six dead miners' health"), FMath::IsNearlyEqual(StoneSentinelRules::MaxHealth(), 6.f * StoneSentinelRules::DeadMinerHealth));
	TestTrue(TEXT("Defense 0.25"), FMath::IsNearlyEqual(Tuning.StoneDefense, 0.25f));
	TestTrue(TEXT("...a Regular one's is its stone's"), FMath::IsNearlyEqual(StoneSentinelRules::CombinedDefense(0.25f, 0.f), 0.25f));
	TestTrue(TEXT("...a Restless one keeps 0.75 x 0.9"), FMath::IsNearlyEqual(StoneSentinelRules::CombinedDefense(0.25f, 0.1f), 0.325f, 0.001f));
	TestTrue(TEXT("...never past 0.9"), StoneSentinelRules::CombinedDefense(0.9f, 0.9f) <= 0.9f);

	// Slow: it comes on at about 170 cm/s once the fight's chase scale is on.
	const float ChaseScale = FFightTuning().ChaseScale;
	TestTrue(TEXT("About 170 cm/s in a fight"), FMath::IsNearlyEqual(
		StoneSentinelRules::ChaseSpeedFor(StoneSentinelRules::FeltChaseSpeed, ChaseScale) * ChaseScale, 170.f, 1.f));

	// Its waking eases from statue to awake; its stride has a footfall each half.
	TestTrue(TEXT("A statue as it starts waking"), FMath::IsNearlyZero(StoneSentinelRules::WakeShare(0.f, Tuning.WakeSeconds)));
	TestTrue(TEXT("...awake at its end"), FMath::IsNearlyEqual(StoneSentinelRules::WakeShare(Tuning.WakeSeconds, Tuning.WakeSeconds), 1.f));
	TestTrue(TEXT("...slowly at first"), StoneSentinelRules::WakeShare(Tuning.WakeSeconds * 0.5f, Tuning.WakeSeconds) < 0.5f);
	TestTrue(TEXT("A footfall at the half stride"), StoneSentinelRules::StepBetween(0.4f, 0.6f));
	TestFalse(TEXT("...none between"), StoneSentinelRules::StepBetween(0.1f, 0.3f));
	TestTrue(TEXT("...and one at the stride's turn"), StoneSentinelRules::StepBetween(0.9f, 1.05f));

	// Chips: a crit knocks more off; no Fleet in its traits, the next one standing in.
	TestTrue(TEXT("A crit knocks more stone off"), StoneSentinelRules::ChipsFor(true, 0.f, Tuning) > StoneSentinelRules::ChipsFor(false, 0.f, Tuning));
	const TArray<ECreatureTrait> Rolled = { ECreatureTrait::Fleet, ECreatureTrait::Warded, ECreatureTrait::Mending };
	const TArray<ECreatureTrait> Kept = StoneSentinelRules::WithoutFleet(Rolled, 2);
	TestTrue(TEXT("Never Fleet: the next trait stands in"), Kept.Num() == 2 && Kept[0] == ECreatureTrait::Warded && Kept[1] == ECreatureTrait::Mending);
	const TArray<ECreatureTrait> RallyFirst = { ECreatureTrait::Rallying, ECreatureTrait::Fleet };
	const TArray<ECreatureTrait> One = StoneSentinelRules::WithoutFleet(RallyFirst, 1);
	TestTrue(TEXT("...and never more than its tier's count"), One.Num() == 1 && One[0] == ECreatureTrait::Rallying);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneSentinelStatsTest, "Looter.Creatures.Sentinel.Stats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoneSentinelStatsTest::RunTest(const FString& Parameters)
{
	const AStoneSentinelCreature* Defaults = GetDefault<AStoneSentinelCreature>();
	const ADeadMinerCreature* Miner = GetDefault<ADeadMinerCreature>();
	const UHealthComponent* Health = Defaults->FindComponentByClass<UHealthComponent>();
	const UHealthComponent* MinerHealth = Miner->FindComponentByClass<UHealthComponent>();
	if (TestNotNull(TEXT("It has health"), Health) && TestNotNull(TEXT("The dead miner has health"), MinerHealth))
	{
		TestTrue(TEXT("The rules' dead miner is today's"), FMath::IsNearlyEqual(MinerHealth->MaxHealth, StoneSentinelRules::DeadMinerHealth));
		TestTrue(TEXT("Six dead miners' health"), FMath::IsNearlyEqual(Health->MaxHealth, 6.f * MinerHealth->MaxHealth));
	}
	TestTrue(TEXT("Its slam winds up 1.1 s"), FMath::IsNearlyEqual(Defaults->AttackWindup, 1.1f));
	TestTrue(TEXT("It starts its slam from its reach"), FMath::IsNearlyEqual(Defaults->AttackRange, StoneSentinelRules::AttackRange));
	TestTrue(TEXT("It hits harder than a dead miner"), Defaults->AttackDamage > Miner->AttackDamage);
	TestTrue(TEXT("...and is worth more"), Defaults->XPReward > Miner->XPReward);
	const float ChaseScale = FFightTuning().ChaseScale;
	const float Felt = FightRules::ChaseSpeedFor(Defaults->ChaseSpeed, false, ChaseScale);
	TestTrue(FString::Printf(TEXT("It comes on at about 170 cm/s (%.0f)"), Felt), FMath::IsNearlyEqual(Felt, 170.f, 5.f));
	TestTrue(TEXT("Slower than a dead miner"), Defaults->ChaseSpeed < Miner->ChaseSpeed);
	TestTrue(TEXT("It stays by its plinth"), FMath::IsNearlyZero(Defaults->WanderRadius));
	TestTrue(TEXT("Its head is the crit"), Defaults->CriticalSpotBones.Contains(FName(TEXT("head"))));

	// Stone, as the elements see it: weak to Wardbreak and Thunder, resists Cinder, Rot does nothing; never Touched.
	TestEqual(TEXT("Its body is Stone"), static_cast<int32>(Defaults->GetElementBody()), static_cast<int32>(EElementBody::Stone));
	TestEqual(TEXT("...by its class too"), static_cast<int32>(ElementRules::BodyOf(AStoneSentinelCreature::StaticClass())),
		static_cast<int32>(EElementBody::Stone));
	FElementTarget Stone;
	Stone.Body = EElementBody::Stone;
	TestTrue(TEXT("Weak to Wardbreak"), ElementRules::Affinity(ELooterElement::GraveSalt, Stone, false) == EElementAffinity::Weak);
	TestTrue(TEXT("Weak to Thunder"), ElementRules::Affinity(ELooterElement::Thunder, Stone, false) == EElementAffinity::Weak);
	TestTrue(TEXT("Resists Cinder"), ElementRules::Affinity(ELooterElement::Ember, Stone, false) == EElementAffinity::Resist);
	TestTrue(TEXT("Immune to Rot"), ElementRules::Affinity(ELooterElement::Rot, Stone, false) == EElementAffinity::Immune);
	TestFalse(TEXT("Never Touched"), Defaults->bMayBeTouched);
	return true;
}

#endif
