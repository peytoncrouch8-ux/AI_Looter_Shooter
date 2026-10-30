#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/CreatureTraitRules.h"
#include "Creatures/CreatureTraitSettings.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/BossTestWorld.h"
#include "Tests/CreatureTraitTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The creature traits (UCreatureTraitComponent; the enemy tiers' Gravebound 1, Hallowed 2, Ethereal 3): their rules alone
// (which kinds each fits, the roll, the ward's and mending's sums), then the traits on a spider's own body and health:
// Warded, Mending, Fleet, Vengeful. Those that reach past it (Rallying, Grave-burst, Phasing, a new life):
// CreatureTraitPlayTests.cpp.

using namespace CreatureTraitTestWorld;

// ---------------------------------------------------------------------------
// The rules
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTraitRollTest, "Looter.Creatures.Traits.Roll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTraitRollTest::RunTest(const FString& Parameters)
{
	// Which kinds each trait fits, by class (most specific first).
	TestTrue(TEXT("A spider is a spider"), CreatureTraitRules::KindOf(*GetDefault<ASpiderCreature>()) == ECreatureTraitKind::Spider);
	TestTrue(TEXT("The Spitter is its own kind"), CreatureTraitRules::KindOf(*GetDefault<ASpitterSpider>()) == ECreatureTraitKind::Spitter);
	TestTrue(TEXT("A slime"), CreatureTraitRules::KindOf(*GetDefault<ASlimeCreature>()) == ECreatureTraitKind::Slime);
	TestTrue(TEXT("An Unpaid"), CreatureTraitRules::KindOf(*GetDefault<AUnpaidCreature>()) == ECreatureTraitKind::Unpaid);
	TestTrue(TEXT("A dead miner, not an Unpaid"), CreatureTraitRules::KindOf(*GetDefault<ADeadMinerCreature>()) == ECreatureTraitKind::DeadMiner);
	TestTrue(TEXT("A Bell-Ringer, not an Unpaid"), CreatureTraitRules::KindOf(*GetDefault<ABellRingerCreature>()) == ECreatureTraitKind::BellRinger);
	TestFalse(TEXT("The Spitter keeps away: never Fleet"), CreatureTraitRules::Fits(ECreatureTrait::Fleet, ECreatureTraitKind::Spitter));
	TestFalse(TEXT("...nor Grave-burst (it dies at range)"), CreatureTraitRules::Fits(ECreatureTrait::GraveBurst, ECreatureTraitKind::Spitter));
	TestTrue(TEXT("...but Vengeful"), CreatureTraitRules::Fits(ECreatureTrait::Vengeful, ECreatureTraitKind::Spitter));
	TestFalse(TEXT("The Bell-Ringer is never Fleet"), CreatureTraitRules::Fits(ECreatureTrait::Fleet, ECreatureTraitKind::BellRinger));
	TestFalse(TEXT("...nor Grave-burst"), CreatureTraitRules::Fits(ECreatureTrait::GraveBurst, ECreatureTraitKind::BellRinger));
	TestFalse(TEXT("...nor Vengeful"), CreatureTraitRules::Fits(ECreatureTrait::Vengeful, ECreatureTraitKind::BellRinger));
	TestTrue(TEXT("A slime phases (the Ethereal's mark, whatever the body)"), CreatureTraitRules::Fits(ECreatureTrait::Phasing, ECreatureTraitKind::Slime));

	// Every kind, many seeds, each tier's count: the right number, no repeats, only fitting ones, Phasing exactly for the
	// Ethereal (and first), the same traits from the same seed.
	const ECreatureTraitKind Kinds[] = { ECreatureTraitKind::Spider, ECreatureTraitKind::Slime, ECreatureTraitKind::Spitter, ECreatureTraitKind::Unpaid,
		ECreatureTraitKind::DeadMiner, ECreatureTraitKind::BellRinger, ECreatureTraitKind::Other };
	struct FTierCase
	{
		int32 Count;
		bool bEthereal;
	};
	const FTierCase Tiers[] = { { 1, false }, { 2, false }, { 3, true } };
	int32 Bad = 0;
	for (const ECreatureTraitKind Kind : Kinds)
	{
		int32 Fitting = 0;
		for (const ECreatureTrait Trait : CreatureTraits::All)
		{
			Fitting += Trait != ECreatureTrait::Phasing && CreatureTraitRules::Fits(Trait, Kind) ? 1 : 0;
		}
		for (const FTierCase& Tier : Tiers)
		{
			for (int32 Seed = 0; Seed < 400; ++Seed)
			{
				const TArray<ECreatureTrait> Rolled = CreatureTraitRules::Roll(Tier.Count, Tier.bEthereal, Kind, Seed * 7919 + 13);
				const int32 Wanted = Tier.bEthereal ? FMath::Min(Tier.Count, Fitting + 1) : FMath::Min(Tier.Count, Fitting);
				TSet<ECreatureTrait> Unique(Rolled);
				bool bFits = true;
				for (const ECreatureTrait Trait : Rolled)
				{
					bFits &= CreatureTraitRules::Fits(Trait, Kind);
				}
				const bool bPhasingRight = Tier.bEthereal ? (Rolled.Num() > 0 && Rolled[0] == ECreatureTrait::Phasing) : !Rolled.Contains(ECreatureTrait::Phasing);
				const bool bSame = CreatureTraitRules::Roll(Tier.Count, Tier.bEthereal, Kind, Seed * 7919 + 13) == Rolled;
				if (Rolled.Num() != Wanted || Unique.Num() != Rolled.Num() || !bFits || !bPhasingRight || !bSame)
				{
					++Bad;
				}
			}
		}
	}
	TestEqual(TEXT("Every roll the right count, no repeats, only fitting traits, Phasing first for an Ethereal and never otherwise, the same from the same seed"),
		Bad, 0);

	// Each of a spider's six comes up about as often as the others for a Gravebound (one trait).
	int32 Counts[CreatureTraits::NumTraits] = {};
	constexpr int32 Rolls = 6000;
	for (int32 Seed = 0; Seed < Rolls; ++Seed)
	{
		const TArray<ECreatureTrait> Rolled = CreatureTraitRules::Roll(1, false, ECreatureTraitKind::Spider, Seed * 104729 + 1);
		if (Rolled.Num() == 1)
		{
			++Counts[static_cast<int32>(Rolled[0])];
		}
	}
	for (const ECreatureTrait Trait : CreatureTraits::All)
	{
		const int32 Got = Counts[static_cast<int32>(Trait)];
		if (Trait == ECreatureTrait::Phasing)
		{
			TestEqual(TEXT("A Gravebound never phases"), Got, 0);
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s comes up about a sixth of the time (%d of %d)"), *CreatureTraitRules::Name(Trait), Got, Rolls),
			Got > Rolls / 6 * 0.8 && Got < Rolls / 6 * 1.2);
	}

	// Names both ways.
	ECreatureTrait Parsed = ECreatureTrait::Fleet;
	TestTrue(TEXT("\"grave-burst\" reads as Grave-burst"), CreatureTraitRules::Parse(TEXT("grave-burst"), Parsed) && Parsed == ECreatureTrait::GraveBurst);
	TestTrue(TEXT("\"PHASING\" reads as Phasing"), CreatureTraitRules::Parse(TEXT("PHASING"), Parsed) && Parsed == ECreatureTrait::Phasing);
	TestFalse(TEXT("\"Sturdy\" is no trait"), CreatureTraitRules::Parse(TEXT("Sturdy"), Parsed));
	TestEqual(TEXT("Grave-burst's word"), CreatureTraitRules::Word(ECreatureTrait::GraveBurst).ToString(), FString(TEXT("Grave-burst")));
	return true;
}

// ---------------------------------------------------------------------------
// Warded
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTraitWardTest, "Looter.Creatures.Traits.Ward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTraitWardTest::RunTest(const FString& Parameters)
{
	// The sums: the ward takes a hit first, a crit counts twice against it, a breaking hit spills the rest over.
	float Ward = 100.f;
	TestEqual(TEXT("A hit under the ward: all held"), CreatureTraitRules::AbsorbWard(Ward, 40.f, false, 2.f), 0.f);
	TestEqual(TEXT("...the ward down by it"), Ward, 60.f);
	TestEqual(TEXT("A crit for 20 counts 40"), CreatureTraitRules::AbsorbWard(Ward, 20.f, true, 2.f), 0.f);
	TestEqual(TEXT("...20 left"), Ward, 20.f);
	TestEqual(TEXT("A hit for 50 breaks it: 30 spill over"), CreatureTraitRules::AbsorbWard(Ward, 50.f, false, 2.f), 30.f);
	TestEqual(TEXT("...broken"), Ward, 0.f);
	TestEqual(TEXT("Broken, all of a hit goes through"), CreatureTraitRules::AbsorbWard(Ward, 25.f, false, 2.f), 25.f);
	Ward = 10.f;
	TestEqual(TEXT("A crit for 15 breaks 10 with 5 of itself; 10 spills"), CreatureTraitRules::AbsorbWard(Ward, 15.f, true, 2.f), 10.f);
	// Typed wards (2026-10-29 night): the type's scale first, then the crit's on top; the spill is the hit as it was.
	Ward = 100.f;
	TestEqual(TEXT("Its own element x2: a 30 hit takes 60"), CreatureTraitRules::AbsorbWard(Ward, 30.f, false, 2.f, 2.f), 0.f);
	TestEqual(TEXT("...40 left"), Ward, 40.f);
	TestEqual(TEXT("...and a crit of it x4: a 15 hit breaks it with 10, 5 spills"), CreatureTraitRules::AbsorbWard(Ward, 15.f, true, 2.f, 2.f), 5.f);
	using EE = ELooterElement;
	TestEqual(TEXT("A shock ward to Thunder: x2"), CreatureTraitRules::WardElementScale(EE::Thunder, EE::Thunder, 2.f, 1.5f), 2.f);
	TestEqual(TEXT("...to Wardbreak: x1.5"), CreatureTraitRules::WardElementScale(EE::Thunder, EE::GraveSalt, 2.f, 1.5f), 1.5f);
	TestEqual(TEXT("...to Cinder: x1"), CreatureTraitRules::WardElementScale(EE::Thunder, EE::Ember, 2.f, 1.5f), 1.f);
	TestEqual(TEXT("...to a plain gun: x1"), CreatureTraitRules::WardElementScale(EE::Thunder, EE::None, 2.f, 1.5f), 1.f);
	TestEqual(TEXT("A true ward to Wardbreak: the larger, never both (x2, not x3)"), CreatureTraitRules::WardElementScale(EE::GraveSalt, EE::GraveSalt, 2.f, 1.5f), 2.f);
	TestTrue(TEXT("Ward rolls: shock half"), CreatureTraitRules::WardElementAt(0.f) == EE::Thunder && CreatureTraitRules::WardElementAt(0.49f) == EE::Thunder);
	TestTrue(TEXT("...a true ward a fifth"), CreatureTraitRules::WardElementAt(0.5f) == EE::GraveSalt && CreatureTraitRules::WardElementAt(0.69f) == EE::GraveSalt);
	TestTrue(TEXT("...Cinder, Hallow, Rot a tenth each"), CreatureTraitRules::WardElementAt(0.75f) == EE::Ember
		&& CreatureTraitRules::WardElementAt(0.85f) == EE::SoulLight && CreatureTraitRules::WardElementAt(0.95f) == EE::Rot);
	TestTrue(TEXT("...the same seed the same ward"), CreatureTraitRules::RollWardElement(1234) == CreatureTraitRules::RollWardElement(1234));

	// On a spider: its health untouched while the ward holds, the ward its share of the most health.
	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACreatureBase* Spider = SpawnSpider(World, FVector::ZeroVector);
	UHealthComponent* Health = HealthOf(Spider);
	if (!TestNotNull(TEXT("A spider with health"), Health))
	{
		return false;
	}
	UCreatureTraitComponent* Traits = Give(*Spider, { ECreatureTrait::Warded });
	const float Max = Health->GetMaxHealth();
	const float Full = Max * Settings().Warded.HealthShare;
	TestTrue(TEXT("Warded: its health has a shield"), Health->Shield.IsBound());
	TestEqual(TEXT("A full ward"), Traits->GetWardFraction(), 1.f);
	Hurt(Spider, Full * 0.5f);
	TestEqual(TEXT("Half the ward's worth: health untouched"), Health->GetHealth(), Max, 0.01f);
	TestEqual(TEXT("...half the ward left"), Traits->GetWardFraction(), 0.5f, 0.001f);
	Hurt(Spider, Full * 0.2f, true);
	TestEqual(TEXT("A crit for a fifth of it takes two fifths"), Traits->GetWardFraction(), 0.1f, 0.001f);
	TestEqual(TEXT("...health still untouched"), Health->GetHealth(), Max, 0.01f);
	Hurt(Spider, Full * 0.3f);
	TestFalse(TEXT("A hit past what's left breaks it"), Traits->IsWardUp());
	TestEqual(TEXT("...the rest of the hit spills onto health"), Health->GetHealth(), Max - Full * 0.2f, 0.05f);
	const UCreatureHitReactionComponent* React = Spider->FindComponentByClass<UCreatureHitReactionComponent>();
	TestTrue(TEXT("Broken, it staggers at once (always: the typed wards)"), React && React->IsStaggered());
	Traits->Advance(Frame);
	TestTrue(TEXT("Broken, it's dazed"), Spider->GetKnock() && Spider->GetKnock()->IsDizzy());
	Hurt(Spider, 10.f);
	TestEqual(TEXT("After it, hits go to health"), Health->GetHealth(), Max - Full * 0.2f - 10.f, 0.05f);
	Traits->SetTraits(TArray<ECreatureTrait>());
	TestFalse(TEXT("Its traits taken: no shield"), Health->Shield.IsBound());
	return true;
}

// ---------------------------------------------------------------------------
// Mending
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTraitMendingTest, "Looter.Creatures.Traits.Mending",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTraitMendingTest::RunTest(const FString& Parameters)
{
	const FMendingTraitTuning& Tuning = Settings().Mending;
	TestEqual(TEXT("Unhit not long enough: nothing"), CreatureTraitRules::MendAmount(Tuning.UnhitSeconds - 0.1f, Tuning.UnhitSeconds, 0.03f, 1000.f, 1.f), 0.f);
	TestEqual(TEXT("Long enough: 3% a second"), CreatureTraitRules::MendAmount(Tuning.UnhitSeconds, Tuning.UnhitSeconds, 0.03f, 1000.f, 1.f), 30.f, 0.001f);

	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ACreatureBase* Spider = SpawnSpider(WorldWrapper.GetTestWorld(), FVector::ZeroVector);
	UHealthComponent* Health = HealthOf(Spider);
	if (!TestNotNull(TEXT("A spider with health"), Health))
	{
		return false;
	}
	UCreatureTraitComponent* Traits = Give(*Spider, { ECreatureTrait::Mending });
	const float Max = Health->GetMaxHealth();
	Hurt(Spider, Max * 0.5f);
	Traits->Advance(Tuning.UnhitSeconds - 0.1f);
	TestFalse(TEXT("Not yet unhit long enough: not mending"), Traits->IsMending());
	TestEqual(TEXT("...not healed"), Health->GetHealth(), Max * 0.5f, 0.01f);
	Traits->Advance(0.2f);
	TestTrue(TEXT("Unhit long enough: mending"), Traits->IsMending());
	TestEqual(TEXT("...3% of its health a second"), Health->GetHealth(), Max * (0.5f + Tuning.HealSharePerSecond * 0.2f), 0.05f);
	Traits->Advance(1.f);
	TestEqual(TEXT("...another second, another 3%"), Health->GetHealth(), Max * (0.5f + Tuning.HealSharePerSecond * 1.2f), 0.05f);
	const float Before = Health->GetHealth();
	Hurt(Spider, 1.f);
	TestFalse(TEXT("Any hit breaks it off"), Traits->IsMending());
	Traits->Advance(1.f);
	TestEqual(TEXT("...and the wait starts again"), Health->GetHealth(), Before - 1.f, 0.05f);
	Traits->Advance(60.f);
	TestEqual(TEXT("Long left alone, it's whole again"), Health->GetHealth(), Max, 0.05f);
	Traits->Advance(Frame);
	TestFalse(TEXT("...and stops mending"), Traits->IsMending());
	return true;
}

// ---------------------------------------------------------------------------
// Fleet and Vengeful
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTraitFleetTest, "Looter.Creatures.Traits.Fleet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTraitFleetTest::RunTest(const FString& Parameters)
{
	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACreatureBase* Spider = SpawnSpider(World, FVector::ZeroVector);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(150.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A spider"), Spider) || !TestNotNull(TEXT("A player"), Player))
	{
		return false;
	}
	const FFleetTraitTuning& Tuning = Settings().Fleet;
	const float Chase = Spider->ChaseSpeed;
	const float Walk = Spider->WalkSpeed;
	const float Windup = Spider->AttackWindup;
	UCreatureTraitComponent* Traits = Give(*Spider, { ECreatureTrait::Fleet });
	TestEqual(TEXT("Fleet: it chases 25% faster"), Spider->ChaseSpeed, Chase * Tuning.SpeedScale, 0.01f);
	TestEqual(TEXT("...and walks 25% faster"), Spider->WalkSpeed, Walk * Tuning.SpeedScale, 0.01f);
	Spider->DevPutInState(ECreatureState::Attack, Player);
	Traits->Advance(Frame);
	TestEqual(TEXT("An attack: its wind-up 15% quicker"), Spider->AttackWindup, Windup * Tuning.WindupScale, 0.0001f);
	Spider->DevPutInState(ECreatureState::Chase, Player);
	Traits->Advance(Frame);
	TestEqual(TEXT("The attack over: its own wind-up again"), Spider->AttackWindup, Windup, 0.0001f);
	Traits->SetTraits(TArray<ECreatureTrait>());
	TestEqual(TEXT("Fleet taken: its own pace again"), Spider->ChaseSpeed, Chase, 0.01f);
	TestEqual(TEXT("...walking too"), Spider->WalkSpeed, Walk, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTraitVengefulTest, "Looter.Creatures.Traits.Vengeful",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTraitVengefulTest::RunTest(const FString& Parameters)
{
	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ACreatureBase* Spider = SpawnSpider(WorldWrapper.GetTestWorld(), FVector::ZeroVector);
	UHealthComponent* Health = HealthOf(Spider);
	if (!TestNotNull(TEXT("A spider with health"), Health))
	{
		return false;
	}
	const FVengefulTraitTuning& Tuning = Settings().Vengeful;
	const float Chase = Spider->ChaseSpeed;
	const float Bite = Spider->AttackDamage;
	UCreatureTraitComponent* Traits = Give(*Spider, { ECreatureTrait::Vengeful });
	const float Max = Health->GetMaxHealth();
	Hurt(Spider, Max * 0.6f);
	TestFalse(TEXT("At 40% it's not enraged"), Traits->IsEnraged());
	Hurt(Spider, Max * 0.15f);
	TestTrue(TEXT("Under 30% it enrages"), Traits->IsEnraged());
	TestEqual(TEXT("...25% more damage"), Spider->AttackDamage, Bite * Tuning.DamageScale, 0.01f);
	TestEqual(TEXT("...25% faster"), Spider->ChaseSpeed, Chase * Tuning.SpeedScale, 0.01f);
	TestTrue(TEXT("...its tier glow flaring"), Traits->GetGlowScale() > Tuning.GlowScale);
	Health->Heal(Max * 0.5f);
	TestTrue(TEXT("Healed over the line, it stays enraged"), Traits->IsEnraged());
	BossTestWorld::Kill(Spider);
	TestFalse(TEXT("Dead, its rage is gone"), Traits->IsEnraged());
	TestEqual(TEXT("...its bite as it was"), Spider->AttackDamage, Bite, 0.01f);
	TestEqual(TEXT("...its pace as it was"), Spider->ChaseSpeed, Chase, 0.01f);
	return true;
}

#endif
