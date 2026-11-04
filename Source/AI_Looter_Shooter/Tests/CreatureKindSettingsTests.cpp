#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bestiary/BestiaryEntry.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureKindSettings.h"
#include "Creatures/DredgerHoseMan.h"
#include "Creatures/DredgerRifleman.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/BossTestWorld.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureKindSettingsTest, "Looter.Creatures.KindSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureKindSettingsTest::RunTest(const FString& Parameters)
{
	// Kill speed (2026-11-03): the regular kinds' health lives in Project Settings > Game > Creature Kinds, set just under
	// the starter guns' breakpoints: a Regular dies to two shotgun blasts (8 x 12 = 96 each), three revolver shots (52) or
	// about nine rifle rounds (18). The class's constructor keeps its old number; the settings win for that exact class.
	const TPair<const UClass*, float> Wanted[] = {
		{ ASpiderCreature::StaticClass(), 150.f }, { AUnpaidCreature::StaticClass(), 140.f }, { ASpitterSpider::StaticClass(), 150.f },
		{ ADeadMinerCreature::StaticClass(), 150.f }, { ADredgerRifleman::StaticClass(), 120.f }, { ADredgerHoseMan::StaticClass(), 170.f } };
	for (const TPair<const UClass*, float>& Case : Wanted)
	{
		const FCreatureKindStats* Entry = UCreatureKindSettings::Get().Find(Case.Key);
		if (TestNotNull(FString::Printf(TEXT("%s has an entry"), *Case.Key->GetName()), Entry))
		{
			TestEqual(FString::Printf(TEXT("%s: health in the settings"), *Case.Key->GetName()), Entry->Health, Case.Value);
			// The spider's bite is the settings' 6 (change 4: five biting at once); every other kind keeps its class's (0).
			TestEqual(FString::Printf(TEXT("%s: damage entry"), *Case.Key->GetName()), Entry->AttackDamage, Case.Key == ASpiderCreature::StaticClass() ? 6.f : 0.f);
		}
		TestEqual(FString::Printf(TEXT("%s: BaseHealthOf reads the settings"), *Case.Key->GetName()), UCreatureKindSettings::BaseHealthOf(Case.Key), Case.Value);
		TestTrue(FString::Printf(TEXT("%s: two shotgun blasts"), *Case.Key->GetName()), Case.Value <= 2.f * 96.f);
		// Three revolver shots at the listed damage (a low roll owes a fourth now and then); the hose-man, the tank, takes four.
		const float Revolver = Case.Key == ADredgerHoseMan::StaticClass() ? 4.f * 52.f : 3.f * 52.f;
		TestTrue(FString::Printf(TEXT("%s: revolver shots"), *Case.Key->GetName()), Case.Value <= Revolver);
	}
	// A kind with no entry keeps its class's number, and a subclass never inherits its parent's entry (Abel keeps his own).
	TestEqual(TEXT("The slime keeps its class's 120"), UCreatureKindSettings::BaseHealthOf(ASlimeCreature::StaticClass()), 120.f);
	TestEqual(TEXT("HealthFor with no entry is the class's"), UCreatureKindSettings::HealthFor(ASlimeCreature::StaticClass(), 77.f), 77.f);
	TestEqual(TEXT("AttackDamageFor with a 0 entry is the class's"), UCreatureKindSettings::AttackDamageFor(AUnpaidCreature::StaticClass(), 8.f), 8.f);
	TestNull(TEXT("No class, no entry"), UCreatureKindSettings::Get().Find(nullptr));

	// In play: a spider spawned in a test world starts with the settings' health, and the Warden's Log says the same.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (TestNotNull(TEXT("Spider spawned"), Spider))
	{
		Spider->DispatchBeginPlay();
		BossTestWorld::NoLoot(Spider);
		const UHealthComponent* Health = Spider->FindComponentByClass<UHealthComponent>();
		if (TestNotNull(TEXT("Spider has health"), Health))
		{
			TestEqual(TEXT("A Regular spider at level 1 starts with 150"), Health->GetMaxHealth(), 150.f, 0.01f);
			TestEqual(TEXT("...and its bite is the settings' 6 (the class says 11)"), Spider->AttackDamage, 6.f, 0.01f);
			TestEqual(TEXT("BaseAttackDamageOf reads the settings"), UCreatureKindSettings::BaseAttackDamageOf(ASpiderCreature::StaticClass()), 6.f, 0.01f);
		}
	}
	UBestiaryEntry* Page = NewObject<UBestiaryEntry>();
	Page->ActorClass = ASpiderCreature::StaticClass();
	const FBestiaryStats Stats = Page->ReadStats();
	TestTrue(TEXT("The page reads health"), Stats.bHasHealth);
	TestEqual(TEXT("The page says 150 too"), Stats.Health, 150.f, 0.01f);
	return true;
}

#endif
