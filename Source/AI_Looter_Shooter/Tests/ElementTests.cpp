#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelKeeper.h"
#include "Combat/GraveSaltDamageType.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/ForemanCreature.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Elements/ElementDamageType.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementStatus.h"
#include "Elements/ElementStatusComponent.h"
#include "Elements/ElementTuning.h"
#include "Tests/CreatureTraitTestWorld.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The story elements (Looter.Elements.*, Phase 12.3): the weakness table, the chance a hit sets its status, each status's
// damage over its time (burns stacking to three, a smite's delayed burst, the salt's slow), and a status set and played
// out on a spider in a test level. The rules read a default-made FElementTuning (the cvars' defaults).

namespace
{
	/** An enum's value as expected (TestEqual wants numbers or strings). */
	template <typename TEnum>
	bool TestIs(FAutomationTestBase& Test, const TCHAR* What, TEnum Actual, TEnum Expected)
	{
		return Test.TestTrue(What, Actual == Expected);
	}

	/** Every piece a status deals over Seconds, in steps of Step. */
	float DealtOver(FElementStatus& Status, float Seconds, const FElementTuning& Tuning, ELooterElement Element, float Step = 1.f / 30.f,
		int32* OutPieces = nullptr)
	{
		float Dealt = 0.f;
		int32 Pieces = 0;
		for (float Time = 0.f; Time < Seconds - 0.0001f; Time += Step)
		{
			TArray<FElementTick> Ticks;
			Status.Advance(Step, Tuning, Ticks);
			for (const FElementTick& Tick : Ticks)
			{
				if (Tick.Element == Element)
				{
					Dealt += Tick.Damage;
					++Pieces;
				}
			}
		}
		if (OutPieces)
		{
			*OutPieces = Pieces;
		}
		return Dealt;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementWeaknessTest, "Looter.Elements.Weakness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementWeaknessTest::RunTest(const FString& Parameters)
{
	using namespace ElementRules;
	// Bodies by kind (Blueprint children keep their parent's).
	TestIs(*this, TEXT("A spider is chitin"), BodyOf(ASpiderCreature::StaticClass()), EElementBody::Chitin);
	TestIs(*this, TEXT("...the Spitter too"), BodyOf(ASpitterSpider::StaticClass()), EElementBody::Chitin);
	TestIs(*this, TEXT("...and the Gravemother"), BodyOf(AGravemotherCreature::StaticClass()), EElementBody::Chitin);
	TestIs(*this, TEXT("A slime is gel"), BodyOf(ASlimeCreature::StaticClass()), EElementBody::Gel);
	TestIs(*this, TEXT("The Unpaid are the coal's own"), BodyOf(AUnpaidCreature::StaticClass()), EElementBody::Unpaid);
	TestIs(*this, TEXT("A dead miner is the rest of the dead"), BodyOf(ADeadMinerCreature::StaticClass()), EElementBody::Undead);
	TestIs(*this, TEXT("...the Foreman"), BodyOf(AForemanCreature::StaticClass()), EElementBody::Undead);
	TestIs(*this, TEXT("...the Bell-Ringer"), BodyOf(ABellRingerCreature::StaticClass()), EElementBody::Undead);
	TestIs(*this, TEXT("...Abel"), BodyOf(AAbelKeeper::StaticClass()), EElementBody::Undead);
	TestIs(*this, TEXT("Nothing named: other"), BodyOf(nullptr), EElementBody::Other);

	const FElementTarget Spider{ EElementBody::Chitin, false, false };
	const FElementTarget Slime{ EElementBody::Gel, false, false };
	const FElementTarget Unpaid{ EElementBody::Unpaid, false, false };
	const FElementTarget Miner{ EElementBody::Undead, false, false };
	const FElementTarget WardedSpider{ EElementBody::Chitin, true, false };
	// Ember.
	TestIs(*this, TEXT("Ember: weak spiders"), Affinity(ELooterElement::Ember, Spider, false), EElementAffinity::Weak);
	TestIs(*this, TEXT("Ember: weak slimes"), Affinity(ELooterElement::Ember, Slime, false), EElementAffinity::Weak);
	TestIs(*this, TEXT("Ember: the Unpaid resist"), Affinity(ELooterElement::Ember, Unpaid, false), EElementAffinity::Resist);
	TestIs(*this, TEXT("Ember: a miner is neither"), Affinity(ELooterElement::Ember, Miner, false), EElementAffinity::Neutral);
	// Soul-light.
	TestIs(*this, TEXT("Soul-light: weak Unpaid"), Affinity(ELooterElement::SoulLight, Unpaid, false), EElementAffinity::Weak);
	TestIs(*this, TEXT("Soul-light: weak dead miners"), Affinity(ELooterElement::SoulLight, Miner, false), EElementAffinity::Weak);
	TestIs(*this, TEXT("Soul-light: slimes resist"), Affinity(ELooterElement::SoulLight, Slime, false), EElementAffinity::Resist);
	TestIs(*this, TEXT("Soul-light: spiders neither"), Affinity(ELooterElement::SoulLight, Spider, false), EElementAffinity::Neutral);
	// Grave-salt.
	TestIs(*this, TEXT("Grave-salt: spiders resist"), Affinity(ELooterElement::GraveSalt, Spider, false), EElementAffinity::Resist);
	TestIs(*this, TEXT("Grave-salt: a spirit's weak, a spider or not"), Affinity(ELooterElement::GraveSalt, WardedSpider, false), EElementAffinity::Weak);
	TestIs(*this, TEXT("Grave-salt: an Unpaid's body neither"), Affinity(ELooterElement::GraveSalt, Unpaid, false), EElementAffinity::Neutral);
	TestIs(*this, TEXT("Grave-salt: an Unpaid's coal (a crit) weak"), Affinity(ELooterElement::GraveSalt, Unpaid, true), EElementAffinity::Weak);
	TestIs(*this, TEXT("Thunder: weak slimes"), Affinity(ELooterElement::Thunder, Slime, false), EElementAffinity::Weak);
	TestIs(*this, TEXT("Thunder: the dead resist"), Affinity(ELooterElement::Thunder, Miner, false), EElementAffinity::Resist);
	TestIs(*this, TEXT("Thunder: a spider neither"), Affinity(ELooterElement::Thunder, Spider, false), EElementAffinity::Neutral);
	TestIs(*this, TEXT("Rot: weak spiders"), Affinity(ELooterElement::Rot, Spider, false), EElementAffinity::Weak);
	TestIs(*this, TEXT("Rot: slimes resist"), Affinity(ELooterElement::Rot, Slime, false), EElementAffinity::Resist);
	TestIs(*this, TEXT("Rot: the dead neither"), Affinity(ELooterElement::Rot, Unpaid, false), EElementAffinity::Neutral);
	TestIs(*this, TEXT("No element: neutral"), Affinity(ELooterElement::None, Spider, false), EElementAffinity::Neutral);

	const FElementTuning Tuning;
	TestEqual(TEXT("Weak: x1.75"), AffinityScale(EElementAffinity::Weak, Tuning), 1.75f);
	TestEqual(TEXT("Resist: x0.5"), AffinityScale(EElementAffinity::Resist, Tuning), 0.5f);
	TestEqual(TEXT("Neutral: x1"), AffinityScale(EElementAffinity::Neutral, Tuning), 1.f);

	TestEqual(TEXT("A spider's bestiary line"), WeaknessLine(ASpiderCreature::StaticClass()), FString(TEXT("WEAK TO CINDER AND ROT · RESISTS WARDBREAK")));
	TestTrue(TEXT("The Unpaid's names its coal"), WeaknessLine(AUnpaidCreature::StaticClass()).Contains(TEXT("ITS COAL TO WARDBREAK")));
	TestTrue(TEXT("Nothing for an unnamed kind"), WeaknessLine(nullptr).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementChanceTest, "Looter.Elements.Chance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementChanceTest::RunTest(const FString& Parameters)
{
	using namespace ElementRules;
	const FElementTuning Tuning;
	TestEqual(TEXT("A rifle's, Uncommon"), GunChance(EWeaponKind::Rifle, EWeaponRarity::Uncommon, Tuning), Tuning.ChanceRifle, 0.0001f);
	TestEqual(TEXT("...the same on a Common"), GunChance(EWeaponKind::Rifle, EWeaponRarity::Common, Tuning), Tuning.ChanceRifle, 0.0001f);
	TestEqual(TEXT("...three steps more on a Legendary"), GunChance(EWeaponKind::Rifle, EWeaponRarity::Legendary, Tuning),
		Tuning.ChanceRifle + 3.f * Tuning.ChanceRarityStep, 0.0001f);
	TestTrue(TEXT("A shotgun's pellet rolls lower than a rifle's round"),
		GunChance(EWeaponKind::Shotgun, EWeaponRarity::Rare, Tuning) < GunChance(EWeaponKind::Rifle, EWeaponRarity::Rare, Tuning));
	TestTrue(TEXT("A sniper's round higher than a revolver's"),
		GunChance(EWeaponKind::Sniper, EWeaponRarity::Rare, Tuning) > GunChance(EWeaponKind::Revolver, EWeaponRarity::Rare, Tuning));
	for (const EWeaponKind Kind : { EWeaponKind::Rifle, EWeaponKind::Revolver, EWeaponKind::SMG, EWeaponKind::Sniper, EWeaponKind::LMG })
	{
		const float Chance = GunChance(Kind, EWeaponRarity::Uncommon, Tuning);
		TestTrue(FString::Printf(TEXT("A %s's round: 15-35%%"), *UEnum::GetValueAsString(Kind)), Chance >= 0.15f && Chance <= 0.35f);
	}
	TestEqual(TEXT("Weak: x1.75"), HitChance(0.2f, EElementAffinity::Weak, Tuning), 0.35f, 0.0001f);
	TestEqual(TEXT("Resist: x0.5"), HitChance(0.2f, EElementAffinity::Resist, Tuning), 0.1f, 0.0001f);
	TestEqual(TEXT("Never past the most"), HitChance(0.8f, EElementAffinity::Weak, Tuning), Tuning.ChanceMax, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementBurnTest, "Looter.Elements.Burn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementBurnTest::RunTest(const FString& Parameters)
{
	const FElementTuning Tuning;
	{
		FElementStatus Status;
		Status.Apply(ELooterElement::Ember, 100.f, EElementAffinity::Neutral, false, Tuning);
		int32 Pieces = 0;
		const float Dealt = DealtOver(Status, Tuning.EmberSeconds + 0.2f, Tuning, ELooterElement::Ember, 1.f / 30.f, &Pieces);
		TestEqual(TEXT("A burn deals its share of the hit in all"), Dealt, 100.f * Tuning.EmberShare, 0.01f);
		TestTrue(TEXT("...in a piece each beat"), FMath::Abs(Pieces - FMath::RoundToInt32(Tuning.EmberSeconds / Tuning.TickSeconds)) <= 1);
		TestFalse(TEXT("...and is out"), Status.IsActive());
	}
	{
		FElementStatus Status;
		Status.Apply(ELooterElement::Ember, 100.f, EElementAffinity::Weak, false, Tuning);
		TestEqual(TEXT("On a creature weak to it: x1.75"), DealtOver(Status, 3.5f, Tuning, ELooterElement::Ember), 100.f * Tuning.EmberShare * 1.75f, 0.01f);
	}
	{
		FElementStatus Status;
		Status.Apply(ELooterElement::Ember, 100.f, EElementAffinity::Neutral, true, Tuning);
		TestTrue(TEXT("A boss burns half as long"), Status.GetBurnTimeLeft() <= Tuning.EmberSeconds * Tuning.BossDuration + 0.001f);
		TestEqual(TEXT("...for half"), DealtOver(Status, 3.5f, Tuning, ELooterElement::Ember), 100.f * Tuning.EmberShare * Tuning.BossDuration, 0.01f);
	}
	{
		FElementStatus Status;
		for (int32 Hit = 0; Hit < 5; ++Hit)
		{
			Status.Apply(ELooterElement::Ember, 10.f, EElementAffinity::Neutral, false, Tuning);
		}
		TestEqual(TEXT("Burns stack to three"), Status.NumBurns(), Tuning.EmberMaxStacks);
		TestEqual(TEXT("...a weaker fourth adds nothing"), Status.GetDamageLeft(ELooterElement::Ember), 3.f * 10.f * Tuning.EmberShare, 0.01f);
		Status.Apply(ELooterElement::Ember, 100.f, EElementAffinity::Neutral, false, Tuning);
		TestEqual(TEXT("...a stronger one takes the weakest's place"), Status.GetDamageLeft(ELooterElement::Ember),
			2.f * 10.f * Tuning.EmberShare + 100.f * Tuning.EmberShare, 0.01f);
		TestEqual(TEXT("...still three"), Status.NumBurns(), Tuning.EmberMaxStacks);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementSmiteTest, "Looter.Elements.Smite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementSmiteTest::RunTest(const FString& Parameters)
{
	const FElementTuning Tuning;
	FElementStatus Status;
	Status.Apply(ELooterElement::SoulLight, 100.f, EElementAffinity::Weak, false, Tuning);
	TestTrue(TEXT("A smite waits"), Status.NumSmites() == 1 && Status.IsActive());
	TArray<FElementTick> Ticks;
	Status.Advance(Tuning.SoulLightDelay * 0.9f, Tuning, Ticks);
	TestTrue(TEXT("...nothing before its time"), Ticks.IsEmpty());
	TestTrue(TEXT("...its light gathering"), Status.GetSmiteCharge(Tuning) > 0.85f);
	Status.Advance(Tuning.SoulLightDelay * 0.2f, Tuning, Ticks);
	if (TestEqual(TEXT("...then bursts once"), Ticks.Num(), 1))
	{
		TestTrue(TEXT("...a smite"), Ticks[0].bSmite && Ticks[0].Element == ELooterElement::SoulLight);
		TestEqual(TEXT("...for its share, weak x1.75"), Ticks[0].Damage, 100.f * Tuning.SoulLightShare * 1.75f, 0.01f);
	}
	TestFalse(TEXT("...and is spent"), Status.IsActive());

	FElementStatus Many;
	for (int32 Hit = 0; Hit < Tuning.SoulLightMaxPending + 2; ++Hit)
	{
		Many.Apply(ELooterElement::SoulLight, 10.f, EElementAffinity::Neutral, false, Tuning);
	}
	TestEqual(TEXT("Smites waiting at most four"), Many.NumSmites(), Tuning.SoulLightMaxPending);
	TestEqual(TEXT("...the light past it gathered, not lost"), Many.GetDamageLeft(ELooterElement::SoulLight),
		(Tuning.SoulLightMaxPending + 2) * 10.f * Tuning.SoulLightShare, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementSaltTest, "Looter.Elements.Salt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementSaltTest::RunTest(const FString& Parameters)
{
	const FElementTuning Tuning;
	FElementStatus Status;
	Status.Apply(ELooterElement::GraveSalt, 100.f, EElementAffinity::Neutral, false, Tuning);
	TestTrue(TEXT("Salted"), Status.IsSalted());
	TestEqual(TEXT("...for its time"), Status.GetSaltTimeLeft(), Tuning.GraveSaltSeconds, 0.001f);
	Status.Apply(ELooterElement::GraveSalt, 20.f, EElementAffinity::Neutral, false, Tuning);
	TestEqual(TEXT("A weaker salting keeps the stronger"), Status.GetDamageLeft(ELooterElement::GraveSalt), 100.f * Tuning.GraveSaltShare, 0.01f);
	TestEqual(TEXT("...dealt in all over its time"), DealtOver(Status, Tuning.GraveSaltSeconds + 0.2f, Tuning, ELooterElement::GraveSalt),
		100.f * Tuning.GraveSaltShare, 0.01f);
	TestFalse(TEXT("...then it's off"), Status.IsSalted());
	TestTrue(TEXT("A 30% slow"), FMath::IsNearlyEqual(Tuning.GraveSaltSlow, 0.7f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementJoltTest, "Looter.Elements.Jolt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementJoltTest::RunTest(const FString& Parameters)
{
	const FElementTuning Tuning;
	FElementStatus Status;
	Status.Apply(ELooterElement::Thunder, 100.f, EElementAffinity::Weak, false, Tuning);
	TestTrue(TEXT("A jolt waits"), Status.NumJolts() == 1 && Status.IsActive());
	TArray<FElementTick> Ticks;
	Status.Advance(Tuning.ThunderDelay * 0.5f, Tuning, Ticks);
	TestTrue(TEXT("...nothing before its time"), Ticks.IsEmpty());
	TestTrue(TEXT("...its charge building"), Status.GetJoltCharge(Tuning) > 0.4f);
	Status.Advance(Tuning.ThunderDelay * 0.6f, Tuning, Ticks);
	if (TestEqual(TEXT("...then bursts once"), Ticks.Num(), 1))
	{
		TestTrue(TEXT("...a jolt"), Ticks[0].bJolt && !Ticks[0].bSmite && Ticks[0].Element == ELooterElement::Thunder);
		TestEqual(TEXT("...for its share, weak x1.75"), Ticks[0].Damage, 100.f * Tuning.ThunderShare * 1.75f, 0.01f);
	}
	TestFalse(TEXT("...and is spent"), Status.IsActive());
	FElementStatus Many;
	for (int32 Hit = 0; Hit < Tuning.ThunderMaxPending + 2; ++Hit)
	{
		Many.Apply(ELooterElement::Thunder, 10.f, EElementAffinity::Neutral, false, Tuning);
	}
	TestEqual(TEXT("Jolts waiting at most four"), Many.NumJolts(), Tuning.ThunderMaxPending);
	TestEqual(TEXT("...the rest joined, not lost"), Many.GetDamageLeft(ELooterElement::Thunder), (Tuning.ThunderMaxPending + 2) * 10.f * Tuning.ThunderShare, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementRotTest, "Looter.Elements.Rot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementRotTest::RunTest(const FString& Parameters)
{
	const FElementTuning Tuning;
	FElementStatus Status;
	Status.Apply(ELooterElement::Rot, 100.f, EElementAffinity::Neutral, false, Tuning);
	TestTrue(TEXT("Rotting"), Status.IsRotting());
	TestEqual(TEXT("...for its time"), Status.GetRotTimeLeft(), Tuning.RotSeconds, 0.001f);
	// It grows: the second half deals more than the first.
	const float First = DealtOver(Status, Tuning.RotSeconds * 0.5f, Tuning, ELooterElement::Rot);
	TestTrue(TEXT("...half way, its growth"), FMath::IsNearlyEqual(Status.GetRotGrowth(), 0.5f, 0.02f));
	const float Second = DealtOver(Status, Tuning.RotSeconds * 0.5f + 0.2f, Tuning, ELooterElement::Rot);
	TestTrue(TEXT("The second half deals more"), Second > First * 1.5f);
	TestEqual(TEXT("...the whole share in all"), First + Second, 100.f * Tuning.RotShare, 0.05f);
	TestFalse(TEXT("...then it's off"), Status.IsRotting());
	Status.Apply(ELooterElement::Rot, 100.f, EElementAffinity::Neutral, false, Tuning);
	DealtOver(Status, 1.f, Tuning, ELooterElement::Rot);
	Status.Apply(ELooterElement::Rot, 40.f, EElementAffinity::Neutral, false, Tuning);
	TestEqual(TEXT("A new hit sets the time back"), Status.GetRotTimeLeft(), Tuning.RotSeconds, 0.001f);
	TestTrue(TEXT("...and adds to what is left"), Status.GetDamageLeft(ELooterElement::Rot) > 100.f * Tuning.RotShare * 0.8f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementWordsTest, "Looter.Elements.Words",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementWordsTest::RunTest(const FString& Parameters)
{
	using namespace ElementRules;
	TestIs(*this, TEXT("Parse Ember"), Parse(TEXT("Ember")), ELooterElement::Ember);
	TestIs(*this, TEXT("Parse Soul-light"), Parse(TEXT("Soul-light")), ELooterElement::SoulLight);
	TestIs(*this, TEXT("Parse SoulLight"), Parse(TEXT("SoulLight")), ELooterElement::SoulLight);
	TestIs(*this, TEXT("Parse Grave-salt"), Parse(TEXT(" grave-salt ")), ELooterElement::GraveSalt);
	TestIs(*this, TEXT("Parse Thunder"), Parse(TEXT("Thunder")), ELooterElement::Thunder);
	TestIs(*this, TEXT("Parse Rot (and Blight)"), Parse(TEXT("Blight")), ELooterElement::Rot);
	TestEqual(TEXT("Thunder's word"), Word(ELooterElement::Thunder), FString(TEXT("THUNDER")));
	TestTrue(TEXT("Rot deals its own damage type"), DamageTypeFor(ELooterElement::Rot) == URotElementDamageType::StaticClass());
	TestIs(*this, TEXT("...which counts as Rot"), ElementOf(GetDefault<URotElementDamageType>()), ELooterElement::Rot);
	TestIs(*this, TEXT("Parse nothing"), Parse(TEXT("")), ELooterElement::None);
	TestEqual(TEXT("A word"), Word(ELooterElement::SoulLight), FString(TEXT("HALLOW")));
	TestTrue(TEXT("None has none"), Word(ELooterElement::None).IsEmpty() && Description(ELooterElement::None).IsEmpty());
	TestEqual(TEXT("Cues by name"), Cue(ELooterElement::GraveSalt, EElementCue::Tick), FName(TEXT("Element.GraveSalt.Tick")));
	TestEqual(TEXT("...the kill's"), Cue(ELooterElement::Ember, EElementCue::Kill), FName(TEXT("Element.Ember.Kill")));
	TestTrue(TEXT("None plays nothing"), Cue(ELooterElement::None, EElementCue::Apply).IsNone());
	TestTrue(TEXT("Grave-salt deals the grenade's own damage type"), DamageTypeFor(ELooterElement::GraveSalt) == UGraveSaltDamageType::StaticClass());
	TestIs(*this, TEXT("...which counts as Grave-salt"), ElementOf(GetDefault<UGraveSaltDamageType>()), ELooterElement::GraveSalt);
	TestIs(*this, TEXT("Ember's knows itself"), ElementOf(GetDefault<UEmberElementDamageType>()), ELooterElement::Ember);
	TestIs(*this, TEXT("Soul-light's too"), ElementOf(GetDefault<USoulLightElementDamageType>()), ELooterElement::SoulLight);
	TestIs(*this, TEXT("Anything else: none"), ElementOf(GetDefault<UDamageType>()), ELooterElement::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementInPlayTest, "Looter.Elements.InPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementInPlayTest::RunTest(const FString& Parameters)
{
	using namespace CreatureTraitTestWorld;
	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	ACreatureBase* Spider = SpawnSpider(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	const FElementTuning Tuning;
	TestFalse(TEXT("A roll past the chance sets nothing"),
		UElementStatusComponent::TryApply(*Spider, ELooterElement::Ember, 0.2f, 100.f, false, 0.99f, FVector::ZeroVector, nullptr, nullptr));
	TestNull(TEXT("...and adds nothing to it"), UElementStatusComponent::Find(Spider));
	TestTrue(TEXT("A roll under it sets the burn"),
		UElementStatusComponent::TryApply(*Spider, ELooterElement::Ember, 0.2f, 100.f, false, 0.0f, FVector::ZeroVector, nullptr, nullptr));
	// Weak to Ember: x1.75 to the chance, so a roll of 0.3 against 0.2 comes up too.
	TestTrue(TEXT("A spider's weakness raises the chance"),
		UElementStatusComponent::TryApply(*Spider, ELooterElement::Ember, 0.2f, 100.f, false, 0.3f, FVector::ZeroVector, nullptr, nullptr));
	UElementStatusComponent* Status = UElementStatusComponent::Find(Spider);
	if (!TestNotNull(TEXT("It carries a status component now"), Status))
	{
		return false;
	}
	TestTrue(TEXT("...burning twice over"), Status->IsBurning() && Status->GetStatus().NumBurns() == 2);
	for (float Time = 0.f; Time < Tuning.EmberSeconds + 0.3f; Time += Frame)
	{
		Status->Advance(Frame);
	}
	TestEqual(TEXT("The burns dealt their share, weak x1.75 (a test level takes none of it: what was sent)"),
		Status->GetDealt(ELooterElement::Ember), 2.f * 100.f * Tuning.EmberShare * Tuning.Weak, 0.05f);
	TestFalse(TEXT("...and are out"), Status->IsBurning());

	TestTrue(TEXT("Salt on it (a spider resists, but the roll is 0)"),
		UElementStatusComponent::TryApply(*Spider, ELooterElement::GraveSalt, 0.2f, 100.f, false, 0.0f, FVector::ZeroVector, nullptr, nullptr));
	const UMovementSlowComponent* Slow = Spider->FindComponentByClass<UMovementSlowComponent>();
	TestTrue(TEXT("...slows it"), Slow && Slow->IsSlowed() && FMath::IsNearlyEqual(Slow->GetMultiplier(), Tuning.GraveSaltSlow, 0.001f));
	return true;
}

#endif
