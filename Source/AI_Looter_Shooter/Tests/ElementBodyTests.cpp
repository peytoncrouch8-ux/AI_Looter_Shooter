#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelKeeper.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/CreatureTraitSettings.h"
#include "Creatures/ForemanCreature.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/PlaceholderCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Elements/ElementCallout.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementStatusComponent.h"
#include "Elements/ElementTuning.h"
#include "Tests/CreatureTraitTestWorld.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// Why the elements matter (the user's rulings, 2026-10-29 night; Docs/Work/2026-10-29-ElementsMatter): the bodies' table
// (Looter.Elements.Bodies), immunities on top of a body and an element gun's own hit scaling (Looter.Elements.Immune), and
// typed wards (Looter.Elements.Ward). The rules read a default-made FElementTuning (the cvars' defaults).

namespace
{
	using EA = EElementAffinity;
	using EB = EElementBody;
	using EE = ELooterElement;

	/** One row of the table: a body's side to each element, in Cinder, Hallow, Wardbreak, Thunder, Rot order. */
	struct FBodyRow
	{
		EB Body;
		const TCHAR* Name;
		EA Sides[5];
	};

	constexpr EE ElementOrder[] = { EE::Ember, EE::SoulLight, EE::GraveSalt, EE::Thunder, EE::Rot };

	FString SideName(EA Side)
	{
		switch (Side)
		{
		case EA::Weak: return TEXT("weak");
		case EA::Resist: return TEXT("resist");
		case EA::Immune: return TEXT("immune");
		default: return TEXT("neutral");
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementBodiesTest, "Looter.Elements.Bodies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementBodiesTest::RunTest(const FString& Parameters)
{
	using namespace ElementRules;
	const EA N = EA::Neutral;
	const EA W = EA::Weak;
	const EA R = EA::Resist;
	const EA I = EA::Immune;
	// The user's table, every body against every element (Cinder, Hallow, Wardbreak, Thunder, Rot).
	const FBodyRow Rows[] = {
		{ EB::Flesh, TEXT("Flesh"), { W, N, N, W, W } },
		{ EB::Chitin, TEXT("Chitin"), { W, N, R, N, W } },
		{ EB::Gel, TEXT("Gel"), { W, R, N, W, R } },
		{ EB::Bone, TEXT("Bone"), { N, W, N, R, N } },
		{ EB::Coal, TEXT("Coal"), { R, W, N, R, N } },
		{ EB::Spirit, TEXT("Spirit"), { I, W, W, I, I } },
		{ EB::Stone, TEXT("Stone"), { R, N, W, W, I } },
		{ EB::Iron, TEXT("Iron"), { R, N, I, W, W } },
	};
	for (const FBodyRow& Row : Rows)
	{
		FElementTarget Target;
		Target.Body = Row.Body;
		for (int32 Index = 0; Index < 5; ++Index)
		{
			const EA Side = Affinity(ElementOrder[Index], Target, false);
			TestTrue(FString::Printf(TEXT("%s to %s: %s (got %s)"), Row.Name, *Word(ElementOrder[Index]), *SideName(Row.Sides[Index]), *SideName(Side)),
				Side == Row.Sides[Index]);
		}
	}
	FElementTarget Coal;
	Coal.Body = EB::Coal;
	TestTrue(TEXT("Coal's crit (the coal itself) is weak to Wardbreak"), Affinity(EE::GraveSalt, Coal, true) == EA::Weak);

	// Each kind's own (its constructor's), Blueprint children keeping it.
	TestTrue(TEXT("A spider is chitin"), BodyOf(ASpiderCreature::StaticClass()) == EB::Chitin);
	TestTrue(TEXT("The Unpaid are coal"), BodyOf(AUnpaidCreature::StaticClass()) == EB::Coal);
	TestTrue(TEXT("Abel is bone"), BodyOf(AAbelKeeper::StaticClass()) == EB::Bone);
	TestTrue(TEXT("A placeholder lives: flesh"), BodyOf(APlaceholderCreature::StaticClass()) == EB::Flesh);

	// The story immunities.
	TestTrue(TEXT("The Gravemother: Rot"), StoryImmunityOf(AGravemotherCreature::StaticClass()) == EE::Rot);
	TestTrue(TEXT("The Foreman: Cinder"), StoryImmunityOf(AForemanCreature::StaticClass()) == EE::Ember);
	TestTrue(TEXT("Abel: Hallow"), StoryImmunityOf(AAbelKeeper::StaticClass()) == EE::SoulLight);
	TestTrue(TEXT("A spider: none"), StoryImmunityOf(ASpiderCreature::StaticClass()) == EE::None);
	TestEqual(TEXT("The Gravemother's bestiary line"), WeaknessLine(AGravemotherCreature::StaticClass()),
		FString(TEXT("WEAK TO CINDER · RESISTS WARDBREAK · IMMUNE TO ROT")));
	TestEqual(TEXT("A placeholder's: three weaknesses"), WeaknessLine(APlaceholderCreature::StaticClass()),
		FString(TEXT("WEAK TO CINDER, THUNDER AND ROT")));

	// The masks and the words.
	TestEqual(TEXT("None has no bit"), static_cast<int32>(MaskOf(EE::None)), 0);
	TestTrue(TEXT("Every element its own bit"), (MaskOf(EE::Ember) & MaskOf(EE::Rot)) == 0 && MaskOf(EE::Thunder) != 0);
	TestEqual(TEXT("BURNING"), StatusWord(EE::Ember), FString(TEXT("BURNING")));
	TestEqual(TEXT("SMITTEN"), StatusWord(EE::SoulLight), FString(TEXT("SMITTEN")));
	TestEqual(TEXT("SALTED"), StatusWord(EE::GraveSalt), FString(TEXT("SALTED")));
	TestEqual(TEXT("JOLTED"), StatusWord(EE::Thunder), FString(TEXT("JOLTED")));
	TestEqual(TEXT("ROTTING"), StatusWord(EE::Rot), FString(TEXT("ROTTING")));
	TestTrue(TEXT("...none for None"), StatusWord(EE::None).IsEmpty());
	TestEqual(TEXT("A ward's break cue"), Cue(EE::Thunder, EElementCue::WardBreak), FName(TEXT("Element.Thunder.WardBreak")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementImmuneTest, "Looter.Elements.Immune",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementImmuneTest::RunTest(const FString& Parameters)
{
	using namespace ElementRules;
	const FElementTuning Tuning;
	// The bullet's own scale: weak x2, resist x0.33, immune x0.1.
	TestEqual(TEXT("A gun's hit, weak: x2"), HitScale(EA::Weak, Tuning), 2.f);
	TestEqual(TEXT("...resisted: x0.33"), HitScale(EA::Resist, Tuning), 0.33f);
	TestEqual(TEXT("...immune: x0.1"), HitScale(EA::Immune, Tuning), 0.1f);
	TestEqual(TEXT("...neutral: x1"), HitScale(EA::Neutral, Tuning), 1.f);

	// A spirit on top of its body: an Ethereal spider keeps chitin's other weaknesses, immune wins.
	FElementTarget Spirit;
	Spirit.Body = EB::Chitin;
	Spirit.bSpirit = true;
	TestTrue(TEXT("An Ethereal spider: immune to Cinder (immune wins over chitin's weakness)"), Affinity(EE::Ember, Spirit, false) == EA::Immune);
	TestTrue(TEXT("...to Rot and Thunder"), Affinity(EE::Rot, Spirit, false) == EA::Immune && Affinity(EE::Thunder, Spirit, false) == EA::Immune);
	TestTrue(TEXT("...weak to Wardbreak and Hallow"), Affinity(EE::GraveSalt, Spirit, false) == EA::Weak && Affinity(EE::SoulLight, Spirit, false) == EA::Weak);
	// Touched: immune to its element, weak to its counter.
	FElementTarget Touched;
	Touched.Body = EB::Chitin;
	Touched.Touched = EE::Ember;
	TestTrue(TEXT("A Cinder-touched spider: immune to Cinder"), Affinity(EE::Ember, Touched, false) == EA::Immune);
	TestTrue(TEXT("...weak to Wardbreak, its counter (over chitin's resist)"), Affinity(EE::GraveSalt, Touched, false) == EA::Weak);
	TestTrue(TEXT("...still weak to Rot"), Affinity(EE::Rot, Touched, false) == EA::Weak);
	FElementTarget Story;
	Story.Body = EB::Chitin;
	Story.ImmuneMask = MaskOf(EE::Rot);
	TestTrue(TEXT("A story immunity wins over the body's weakness"), Affinity(EE::Rot, Story, false) == EA::Immune);

	// In a test level: a spider touched by Cinder.
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
	TestTrue(TEXT("A plain spider: Cinder weak, a Cinder gun's hit x2"), FMath::IsNearlyEqual(GunHitScale(EE::Ember, Spider, false, Tuning), 2.f));
	TestTrue(TEXT("...a plain gun's: never scaled"), FMath::IsNearlyEqual(GunHitScale(EE::None, Spider, false, Tuning), 1.f));
	TestTrue(TEXT("...Wardbreak resisted: x0.33"), FMath::IsNearlyEqual(GunHitScale(EE::GraveSalt, Spider, false, Tuning), 0.33f));
	TestTrue(TEXT("Nothing that isn't a creature scales"), FMath::IsNearlyEqual(GunHitScale(EE::Ember, nullptr, false, Tuning), 1.f));
	Spider->SetTouched(EE::Ember);
	TestTrue(TEXT("Touched by Cinder"), Spider->GetTouchedElement() == EE::Ember);
	TestEqual(TEXT("...its word"), Spider->GetTouchedWord(), FString(TEXT("Cinder-touched")));
	TestTrue(TEXT("...its target says so"), TargetOf(Spider).Touched == EE::Ember);
	TestTrue(TEXT("...a Cinder gun's hit x0.1"), FMath::IsNearlyEqual(GunHitScale(EE::Ember, Spider, false, Tuning), 0.1f));
	TestFalse(TEXT("...and no burn sets, whatever the roll"),
		UElementStatusComponent::TryApply(*Spider, EE::Ember, 1.f, 100.f, false, 0.f, FVector::ZeroVector, nullptr, nullptr));
	UElementStatusComponent* Status = UElementStatusComponent::FindOrAdd(*Spider);
	Status->Apply(EE::Ember, 100.f, EA::Immune, FVector::ZeroVector, nullptr, nullptr);
	TestFalse(TEXT("...not even set by hand at its immunity"), Status->IsBurning());
	TestTrue(TEXT("...Wardbreak, its counter, x2"), FMath::IsNearlyEqual(GunHitScale(EE::GraveSalt, Spider, false, Tuning), 2.f));

	// The number says so: NO EFFECT, once a fight, greyed, not bigger.
	const FElementNumberLook Look = ElementCallout::Judge(EE::Ember, EE::None, false, EA::Immune, true);
	TestTrue(TEXT("An immune hit's number: NO EFFECT"), Look.Callout == EA::Immune && Look.SizeScale == 1.f);
	TestEqual(TEXT("...its words"), ElementCallout::TagText(EA::Immune).ToString(), FString(TEXT("NO EFFECT")));
	TestFalse(TEXT("...greyed, not the element's colour"), ElementCallout::TagColor(EE::Ember, EA::Immune).Equals(ElementRules::UiColor(EE::Ember), 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementWardTest, "Looter.Elements.Ward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementWardTest::RunTest(const FString& Parameters)
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
	ACreatureBase* Other = SpawnSpider(World, FVector(0.0, 600.0, 0.0));
	UHealthComponent* Health = HealthOf(Spider);
	if (!TestNotNull(TEXT("A spider with health"), Health) || !TestNotNull(TEXT("Another"), Other))
	{
		return false;
	}
	// Touched by Rot first (its rank applied again sets its traits), then Warded: its ward is its own element.
	Spider->SetTouched(EE::Rot);
	UCreatureTraitComponent* Traits = Give(*Spider, { ECreatureTrait::Warded });
	TestTrue(TEXT("A Rot-touched creature's ward is Rot"), Traits->GetWardElement() == EE::Rot);
	UCreatureTraitComponent* OtherTraits = Give(*Other, { ECreatureTrait::Warded });
	TestTrue(TEXT("Another's ward is rolled from its seed: one of the five"), OtherTraits->GetWardElement() != EE::None);

	const FElementTuning Tuning;
	const float Full = Health->GetMaxHealth() * Settings().Warded.HealthShare;
	TestTrue(TEXT("A standing ward: the gun's hit isn't scaled by the body (the ward has its own rules)"),
		FMath::IsNearlyEqual(ElementRules::GunHitScale(EE::Ember, Spider, false, Tuning), 1.f));
	TestFalse(TEXT("...and no status sets through it"),
		UElementStatusComponent::TryApply(*Spider, EE::Ember, 1.f, 100.f, false, 0.f, FVector::ZeroVector, nullptr, nullptr));
	{
		const FHitElementScope Rot(EE::Rot);
		Hurt(Spider, Full * 0.2f);
	}
	TestEqual(TEXT("Its own element breaks it x2: a fifth takes two fifths"), Traits->GetWardFraction(), 0.6f, 0.001f);
	{
		const FHitElementScope Salt(EE::GraveSalt);
		Hurt(Spider, Full * 0.2f);
	}
	TestEqual(TEXT("Wardbreak on it x1.5: a fifth takes three tenths"), Traits->GetWardFraction(), 0.3f, 0.001f);
	{
		const FHitElementScope Cinder(EE::Ember);
		Hurt(Spider, Full * 0.1f);
	}
	TestEqual(TEXT("Anything else x1"), Traits->GetWardFraction(), 0.2f, 0.001f);
	Hurt(Spider, Full * 0.05f, true);
	TestEqual(TEXT("A plain crit x2, as ever"), Traits->GetWardFraction(), 0.1f, 0.001f);
	TestEqual(TEXT("...health untouched all the while"), Health->GetHealth(), Health->GetMaxHealth(), 0.01f);
	{
		const FHitElementScope Rot(EE::Rot);
		Hurt(Spider, Full * 0.1f);
	}
	TestFalse(TEXT("Its own element breaks the rest"), Traits->IsWardUp());
	TestTrue(TEXT("...no ward element once it's broken"), Traits->GetWardElement() == EE::None);
	TestEqual(TEXT("...the spill comes off health as it was: a twentieth of a full ward"), Health->GetHealth(),
		Health->GetMaxHealth() - Full * 0.05f, 0.05f);
	return true;
}

#endif
