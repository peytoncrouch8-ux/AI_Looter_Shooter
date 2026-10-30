#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/GraveSaltDamageType.h"
#include "Combat/HitReaction.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Elements/ElementCallout.h"
#include "Elements/ElementDamageType.h"
#include "Elements/ElementDeathLook.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementStatusComponent.h"
#include "Tests/CreatureTraitTestWorld.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/World/DamageNumberActor.h"
#include "UI/World/DamageNumberWidget.h"
#include "Weapons/WeaponFX.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The elements' feel (the loot polish): a number in its element's colour, the weakness called out once a fight, and a
// creature killed by a status dying the element's way, not the gun's. Looter.UI.DamageNumbers.Element (the number's
// colours, sizes, tag), Looter.Elements.Callout (the once-a-fight log, the hit's element, in a test level),
// Looter.Elements.StatusKill (a burn kills a spider: no gun knock) and Looter.Elements.DeathLook (the three deaths).

namespace
{
	/** The look a plain fact-set gives. */
	FElementNumberLook JudgeIt(ELooterElement Gun, ELooterElement Status, bool bCritical, EElementAffinity Affinity, bool bFirst)
	{
		return ElementCallout::Judge(Gun, Status, bCritical, Affinity, bFirst);
	}

	bool SameColor(const FLinearColor& A, const FLinearColor& B)
	{
		return A.Equals(B, 0.001f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageNumbersElementTest, "Looter.UI.DamageNumbers.Element",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDamageNumbersElementTest::RunTest(const FString& Parameters)
{
	using EA = EElementAffinity;
	using EE = ELooterElement;

	// No element: an ordinary number whatever else is true.
	const FElementNumberLook Plain = JudgeIt(EE::None, EE::None, true, EA::Weak, true);
	TestTrue(TEXT("No element, no look"), Plain.Element == EE::None && !Plain.bTint && Plain.Callout == EA::Neutral && Plain.SizeScale == 1.f);

	// A status's piece or smite: the element's colour, never tinted-by-crit; a gun's plain hit unchanged.
	const FElementNumberLook Burn = JudgeIt(EE::Ember, EE::Ember, false, EA::Neutral, true);
	TestTrue(TEXT("A burn's number is tinted"), Burn.Element == EE::Ember && Burn.bTint);
	TestTrue(TEXT("The status wins over the gun's element"), JudgeIt(EE::SoulLight, EE::GraveSalt, false, EA::Neutral, false).Element == EE::GraveSalt);
	const FElementNumberLook GunHit = JudgeIt(EE::Ember, EE::None, false, EA::Neutral, true);
	TestTrue(TEXT("A plain hit of an element gun is unchanged"), GunHit.Element == EE::Ember && !GunHit.bTint && GunHit.SizeScale == 1.f);
	TestTrue(TEXT("...its crit is tinted"), JudgeIt(EE::Ember, EE::None, true, EA::Neutral, true).bTint);

	// The call-out: weak is bigger with a WEAK tag, once; resist is a tag only; neutral says nothing.
	const FElementNumberLook Weak = JudgeIt(EE::Ember, EE::None, false, EA::Weak, true);
	TestTrue(TEXT("First weak hit of the fight: WEAK"), Weak.Callout == EA::Weak);
	TestEqual(TEXT("...a little bigger"), Weak.SizeScale, ElementCallout::WeakSize);
	TestTrue(TEXT("...but not tinted (a plain hit)"), !Weak.bTint);
	const FElementNumberLook Again = JudgeIt(EE::Ember, EE::None, false, EA::Weak, false);
	TestTrue(TEXT("The next one: no tag, normal size"), Again.Callout == EA::Neutral && Again.SizeScale == 1.f);
	const FElementNumberLook Resist = JudgeIt(EE::GraveSalt, EE::None, false, EA::Resist, true);
	TestTrue(TEXT("A resisted hit: RESIST, same size"), Resist.Callout == EA::Resist && Resist.SizeScale == 1.f);
	TestTrue(TEXT("A neutral hit: no tag"), JudgeIt(EE::Ember, EE::None, false, EA::Neutral, true).Callout == EA::Neutral);
	TestTrue(TEXT("A weak status piece also calls it out"), JudgeIt(EE::None, EE::SoulLight, false, EA::Weak, true).Callout == EA::Weak);

	// Colours: the interface's own element colours.
	FElementNumberLook Tick;
	Tick.bTint = true;
	Tick.Element = EE::Ember;
	TestTrue(TEXT("A burn's number is Ember's colour"), SameColor(ElementCallout::NumberColor(Tick, false, 0.f), LooterUI::Color::ElementEmber()));
	Tick.Element = EE::SoulLight;
	TestTrue(TEXT("...a smite's Soul-light's"), SameColor(ElementCallout::NumberColor(Tick, false, 0.f), LooterUI::Color::ElementSoulLight()));
	Tick.Element = EE::GraveSalt;
	TestTrue(TEXT("...salt's Grave-salt's"), SameColor(ElementCallout::NumberColor(Tick, false, 0.f), LooterUI::Color::ElementGraveSalt()));
	TestTrue(TEXT("A normal number is the text colour"), SameColor(ElementCallout::NumberColor(FElementNumberLook(), false, 0.f), LooterUI::Color::Text()));
	TestTrue(TEXT("A plain crit settles to the accent"), SameColor(ElementCallout::NumberColor(FElementNumberLook(), true, 0.f), LooterUI::Color::Accent()));
	TestTrue(TEXT("...white-hot at first"), SameColor(ElementCallout::NumberColor(FElementNumberLook(), true, 1.f), FLinearColor::White));
	FElementNumberLook CritSalt;
	CritSalt.Element = EE::GraveSalt;
	CritSalt.bTint = true;
	const FLinearColor Settled = ElementCallout::NumberColor(CritSalt, true, 0.f);
	TestFalse(TEXT("An element crit settles tinted, not the plain accent"), SameColor(Settled, LooterUI::Color::Accent()));
	TestTrue(TEXT("...between the accent and the element's colour"),
		Settled.B > LooterUI::Color::Accent().B && Settled.B < LooterUI::Color::ElementGraveSalt().B + 0.001f);
	TestTrue(TEXT("...still white-hot as it slams in"), SameColor(ElementCallout::NumberColor(CritSalt, true, 1.f), FLinearColor::White));
	// An untinted element look (a gun's plain hit) is the plain number.
	FElementNumberLook Untinted;
	Untinted.Element = EE::Ember;
	TestTrue(TEXT("A plain element hit keeps the text colour"), SameColor(ElementCallout::NumberColor(Untinted, false, 0.f), LooterUI::Color::Text()));

	// The tag: its words and colours.
	TestEqual(TEXT("WEAK"), ElementCallout::TagText(EA::Weak).ToString(), FString(TEXT("WEAK")));
	TestEqual(TEXT("RESIST"), ElementCallout::TagText(EA::Resist).ToString(), FString(TEXT("RESIST")));
	TestEqual(TEXT("NO EFFECT on an immune one"), ElementCallout::TagText(EA::Immune).ToString(), FString(TEXT("NO EFFECT")));
	const FElementNumberLook Immune = JudgeIt(EE::Ember, EE::None, false, EA::Immune, true);
	TestTrue(TEXT("...called out once a fight, same size"), Immune.Callout == EA::Immune && Immune.SizeScale == 1.f);
	TestFalse(TEXT("...greyed like a resist"), SameColor(ElementCallout::TagColor(EE::Ember, EA::Immune), LooterUI::Color::ElementEmber()));
	TestTrue(TEXT("Nothing for neutral"), ElementCallout::TagText(EA::Neutral).IsEmpty());
	TestTrue(TEXT("WEAK is the element's colour"), SameColor(ElementCallout::TagColor(EE::SoulLight, EA::Weak), LooterUI::Color::ElementSoulLight()));
	const FLinearColor Grey = ElementCallout::TagColor(EE::Ember, EA::Resist);
	TestTrue(TEXT("RESIST is greyed: the dim text colour, not the element's"),
		FMath::IsNearlyEqual(Grey.R, LooterUI::Color::TextDim().R, 0.001f) && !SameColor(Grey, LooterUI::Color::ElementEmber()) && Grey.A < 1.f);

	// Sizes: a weak number is a little bigger, a crit still the loudest.
	TestEqual(TEXT("Normal size"), UDamageNumberWidget::NumberSize(false, FElementNumberLook()), UDamageNumberWidget::NormalSize);
	TestEqual(TEXT("Crit size"), UDamageNumberWidget::NumberSize(true, FElementNumberLook()), UDamageNumberWidget::CriticalSize);
	TestTrue(TEXT("A weak hit's number is bigger"), UDamageNumberWidget::NumberSize(false, Weak) > UDamageNumberWidget::NormalSize);
	TestTrue(TEXT("...a little: under a crit's"), UDamageNumberWidget::NumberSize(false, Weak) < UDamageNumberWidget::CriticalSize);
	TestEqual(TEXT("A resisted hit's is not"), UDamageNumberWidget::NumberSize(false, Resist), UDamageNumberWidget::NormalSize);

	// The tag pops in big and settles.
	TestEqual(TEXT("The tag pops in at its pop size"), DamageNumberMotion::TagScaleAt(0.f), DamageNumberMotion::TagPopScale);
	TestEqual(TEXT("...settled by the end of the pop"), DamageNumberMotion::TagScaleAt(DamageNumberMotion::TagPopSeconds), 1.f, 0.001f);
	TestTrue(TEXT("...shrinking between"), DamageNumberMotion::TagScaleAt(0.05f) < DamageNumberMotion::TagScaleAt(0.f)
		&& DamageNumberMotion::TagScaleAt(0.05f) > DamageNumberMotion::TagScaleAt(0.1f));
	TestEqual(TEXT("...and stays"), DamageNumberMotion::TagScaleAt(2.f), 1.f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementCalloutTest, "Looter.Elements.Callout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementCalloutTest::RunTest(const FString& Parameters)
{
	using EA = EElementAffinity;
	using EE = ELooterElement;

	// The log: once a fight per creature, element and side.
	UDamageType* A = NewObject<UDamageType>(GetTransientPackage());
	UDamageType* B = NewObject<UDamageType>(GetTransientPackage());
	FElementCalloutLog Log;
	const float Fight = ElementCallout::FightSeconds;
	TestTrue(TEXT("The first weak hit is called out"), Log.TryClaim(A, EE::Ember, EA::Weak, 0.0, Fight));
	TestFalse(TEXT("...the next is not"), Log.TryClaim(A, EE::Ember, EA::Weak, 1.0, Fight));
	TestFalse(TEXT("...nor a while after, the fight going on"), Log.TryClaim(A, EE::Ember, EA::Weak, 19.0, Fight));
	TestTrue(TEXT("Another creature is"), Log.TryClaim(B, EE::Ember, EA::Weak, 19.5, Fight));
	TestTrue(TEXT("Another element on the first is"), Log.TryClaim(A, EE::SoulLight, EA::Weak, 20.0, Fight));
	TestTrue(TEXT("Its resist is a call-out of its own"), Log.TryClaim(A, EE::GraveSalt, EA::Resist, 20.0, Fight));
	TestFalse(TEXT("Neutral is never called out"), Log.TryClaim(A, EE::Ember, EA::Neutral, 21.0, Fight));
	TestFalse(TEXT("...nor no element"), Log.TryClaim(A, EE::None, EA::Weak, 21.0, Fight));
	TestFalse(TEXT("...nor nobody"), Log.TryClaim(nullptr, EE::Ember, EA::Weak, 21.0, Fight));
	TestTrue(TEXT("A pause longer than a fight starts a new one"), Log.TryClaim(A, EE::Ember, EA::Weak, 19.0 + Fight + 0.5, Fight));
	const double Later = 19.0 + Fight + 0.5;
	TestTrue(TEXT("A clock that started again starts over"), Log.TryClaim(A, EE::Ember, EA::Weak, Later - 100.0, Fight));
	TArray<UDamageType*> Crowd;
	for (int32 Index = 0; Index < 80; ++Index)
	{
		Crowd.Add(NewObject<UDamageType>(GetTransientPackage()));
		Log.TryClaim(Crowd.Last(), EE::Ember, EA::Weak, 1000.0 + Index * 0.01, Fight);
	}
	TestTrue(FString::Printf(TEXT("A crowd keeps the log small (%d)"), Log.Num()), Log.Num() <= 48);
	Log.Reset();
	TestEqual(TEXT("Reset empties it"), Log.Num(), 0);

	// The hit's element scope nests and restores.
	TestTrue(TEXT("No element outside a scope"), FHitElementScope::Current() == EE::None);
	{
		const FHitElementScope Outer(EE::Ember);
		TestTrue(TEXT("In a scope"), FHitElementScope::Current() == EE::Ember);
		{
			const FHitElementScope Inner(EE::GraveSalt);
			TestTrue(TEXT("Nested"), FHitElementScope::Current() == EE::GraveSalt);
		}
		TestTrue(TEXT("...restored"), FHitElementScope::Current() == EE::Ember);
	}
	TestTrue(TEXT("...and gone"), FHitElementScope::Current() == EE::None);

	// The element of a status's damage: its type, or what the status component is dealing (the salt's is the grenade's type).
	TestTrue(TEXT("Ember's damage type"), ElementCallout::StatusElementOf(GetDefault<UEmberElementDamageType>()) == EE::Ember);
	TestTrue(TEXT("Soul-light's damage type"), ElementCallout::StatusElementOf(GetDefault<USoulLightElementDamageType>()) == EE::SoulLight);
	TestTrue(TEXT("A gun's hit is no status"), ElementCallout::StatusElementOf(GetDefault<UWeaponDamageType>()) == EE::None);
	TestTrue(TEXT("...nor is the grenade's salt by itself"), ElementCallout::StatusElementOf(GetDefault<UGraveSaltDamageType>()) == EE::None);
	TestTrue(TEXT("...nor nothing"), ElementCallout::StatusElementOf(nullptr) == EE::None);
	{
		const UElementStatusComponent::FDealingScope Dealing(EE::GraveSalt);
		TestTrue(TEXT("The salt's piece, while the status deals it"), ElementCallout::StatusElementOf(GetDefault<UGraveSaltDamageType>()) == EE::GraveSalt);
		TestTrue(TEXT("...it is the dealing element"), UElementStatusComponent::DealingElement() == EE::GraveSalt);
	}
	TestTrue(TEXT("...and not after"), UElementStatusComponent::DealingElement() == EE::None);

	// In a test level, on a spider (weak to Ember, resists Grave-salt).
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
	ElementCallout::Log().Reset();
	const UDamageType* Shot = GetDefault<UWeaponDamageType>();
	const UDamageType* CritShot = GetDefault<UWeaponCritDamageType>();
	{
		const FHitElementScope Ember(EE::Ember);
		const FElementNumberLook First = ElementCallout::LookFor(Shot, Spider, false, 10.0);
		TestTrue(TEXT("An Ember hit on a spider: WEAK, bigger, not tinted"), First.Callout == EA::Weak && First.SizeScale > 1.f && !First.bTint);
		const FElementNumberLook Second = ElementCallout::LookFor(Shot, Spider, false, 10.5);
		TestTrue(TEXT("...once: the next is plain"), Second.Callout == EA::Neutral && Second.SizeScale == 1.f);
		TestTrue(TEXT("...a crit keeps its look, tinted"), ElementCallout::LookFor(CritShot, Spider, true, 11.0).bTint);
	}
	{
		const FHitElementScope Salt(EE::GraveSalt);
		TestTrue(TEXT("Grave-salt on a spider: RESIST"), ElementCallout::LookFor(Shot, Spider, false, 12.0).Callout == EA::Resist);
	}
	TestTrue(TEXT("A gun with no element: the plain number"),
		ElementCallout::LookFor(Shot, Spider, false, 13.0).Element == EE::None && ElementCallout::LookFor(CritShot, Spider, true, 13.0).Element == EE::None);
	{
		const UElementStatusComponent::FDealingScope Dealing(EE::Ember);
		const FElementNumberLook Tick = ElementCallout::LookFor(GetDefault<UEmberElementDamageType>(), Spider, false, 14.0);
		TestTrue(TEXT("A burn's piece: tinted Ember"), Tick.Element == EE::Ember && Tick.bTint);
		TestEqual(TEXT("...its weakness already called out this fight"), static_cast<int32>(Tick.Callout), static_cast<int32>(EA::Neutral));
	}
	ElementCallout::Log().Reset();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementStatusKillTest, "Looter.Elements.StatusKill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementStatusKillTest::RunTest(const FString& Parameters)
{
	using namespace CreatureTraitTestWorld;
	using EE = ELooterElement;
	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	ACreatureBase* Burned = SpawnSpider(World, FVector::ZeroVector);
	ACreatureBase* Shot = SpawnSpider(World, FVector(0.0, 500.0, 0.0));
	if (!TestNotNull(TEXT("A spider"), Burned) || !TestNotNull(TEXT("Another"), Shot))
	{
		return false;
	}
	const UHealthComponent* BurnedHealth = HealthOf(Burned);
	const UHealthComponent* ShotHealth = HealthOf(Shot);
	if (!TestNotNull(TEXT("Its health"), BurnedHealth) || !TestNotNull(TEXT("...and the other's"), ShotHealth))
	{
		return false;
	}

	// The rules: a status's kill has no gun's knock and no kind burst; a gun's does.
	FCreatureHit StatusHit;
	StatusHit.Element = EE::Ember;
	TestFalse(TEXT("A status's kill throws no corpse"), UCreatureHitReactionComponent::KnocksCorpse(StatusHit));
	TestFalse(TEXT("...and bursts as no kind's"), UCreatureHitReactionComponent::BurstsLikeItsKind(StatusHit));
	const FCreatureHit GunHit;
	TestTrue(TEXT("A gun's kill knocks the corpse"), UCreatureHitReactionComponent::KnocksCorpse(GunHit));
	TestTrue(TEXT("...and bursts like its kind"), UCreatureHitReactionComponent::BurstsLikeItsKind(GunHit));

	// Played: a burn's piece (as the status deals it) kills one spider; a plain hit kills the other.
	const float Overkill = BurnedHealth->GetMaxHealth() * 10.f;
	{
		const UElementStatusComponent::FDealingScope Dealing(EE::Ember);
		Burned->OnTakeAnyDamage.Broadcast(Burned, Overkill, GetDefault<UEmberElementDamageType>(), nullptr, nullptr);
	}
	Shot->OnTakeAnyDamage.Broadcast(Shot, ShotHealth->GetMaxHealth() * 10.f, GetDefault<UWeaponDamageType>(), nullptr, nullptr);
	if (!Burned->IsDead() || !Shot->IsDead())
	{
		AddWarning(TEXT("A damage broadcast didn't kill in this test level: the status kill's play isn't checked."));
		return true;
	}
	const UCreatureHitReactionComponent* BurnedReaction = Burned->FindComponentByClass<UCreatureHitReactionComponent>();
	const UCreatureHitReactionComponent* ShotReaction = Shot->FindComponentByClass<UCreatureHitReactionComponent>();
	if (!TestNotNull(TEXT("Their hit reactions"), BurnedReaction) || !TestNotNull(TEXT("...both"), ShotReaction))
	{
		return false;
	}
	const FCreatureHit& Last = BurnedReaction->GetLastTakenHit();
	TestTrue(TEXT("The burned spider's last hit was the status's"), Last.Element == EE::Ember);
	TestTrue(TEXT("...no gun's (a burn is not a shotgun blast)"), Last.Kind == EWeaponKind::None);
	TestFalse(TEXT("...so its corpse is not knocked"), UCreatureHitReactionComponent::KnocksCorpse(Last));
	TestTrue(TEXT("A plain hit's kill is no status's"), ShotReaction->GetLastTakenHit().Element == EE::None);
	TestTrue(TEXT("...and knocks as ever"), UCreatureHitReactionComponent::KnocksCorpse(ShotReaction->GetLastTakenHit()));
	TestTrue(TEXT("Nothing is dealing now"), UElementStatusComponent::DealingElement() == EE::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementDeathLookTest, "Looter.Elements.DeathLook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementDeathLookTest::RunTest(const FString& Parameters)
{
	using EE = ELooterElement;
	for (const EE Element : { EE::Ember, EE::SoulLight, EE::GraveSalt, EE::Thunder, EE::Rot })
	{
		TestTrue(FString::Printf(TEXT("%s: a body of pieces"), *ElementRules::Word(Element)), ElementDeathLook::AuraCount(Element, 1.f) > 0);
		TestTrue(TEXT("...more on a bigger body"), ElementDeathLook::AuraCount(Element, 3.f) > ElementDeathLook::AuraCount(Element, 1.f));
	}
	TestEqual(TEXT("None has no death"), ElementDeathLook::AuraCount(EE::None, 1.f), 0);
	TestTrue(TEXT("Crumbling salt sheds more than a burst of light"),
		ElementDeathLook::AuraCount(EE::GraveSalt, 1.f) > ElementDeathLook::AuraCount(EE::SoulLight, 1.f));

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FWeaponFX Effects;
	Effects.Initialize(WorldWrapper.GetTestWorld());
	const FVector Body(0.0, 0.0, 60.0);
	for (const EE Element : { EE::Ember, EE::SoulLight, EE::GraveSalt, EE::Thunder, EE::Rot })
	{
		const int32 Before = Effects.NumParticles();
		ElementDeathLook::Play(Effects, Element, Body, 40.f, 60.f, FVector(500.0, 0.0, 100.0));
		TestTrue(FString::Printf(TEXT("%s's death spawns its look (%d)"), *ElementRules::Word(Element), Effects.NumParticles() - Before),
			Effects.NumParticles() > Before);
	}
	const int32 Before = Effects.NumParticles();
	ElementDeathLook::Play(Effects, EE::None, Body, 40.f, 60.f, FVector::ZeroVector);
	TestEqual(TEXT("None spawns nothing"), Effects.NumParticles(), Before);
	Effects.Shutdown();
	return true;
}

#endif
