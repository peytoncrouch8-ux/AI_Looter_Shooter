#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/SpiderCreature.h"
#include "Tests/CreatureTraitTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/PartEffectRules.h"
#include "Weapons/PartEffectSubsystem.h"
#include "Weapons/PartEffectTuning.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponTrigger.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The part effects (Looter.PartEffects.*, Phase 12.3): the trigger's plan for a Burst or Overcharge gun, the Explosive
// round's split and falloff, the Overcharge's wind-up, the Twin's two ways, a gun's effect and element from its parts,
// and the Ricochet's pick of a creature it can see. The rules read a default-made FPartEffectTuning (the cvars' defaults).

namespace
{
	template <typename TEnum>
	bool TestIs(FAutomationTestBase& Test, const TCHAR* What, TEnum Actual, TEnum Expected)
	{
		return Test.TestTrue(What, Actual == Expected);
	}

	FWeaponPartOption& AddOption(UWeaponDefinition& Gun, FName Slot, FName Key)
	{
		static UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		FWeaponPartSlot* Found = Gun.Parts.FindByPredicate([Slot](const FWeaponPartSlot& Part) { return Part.Name == Slot; });
		if (!Found)
		{
			Found = &Gun.Parts.AddDefaulted_GetRef();
			Found->Name = Slot;
		}
		FWeaponPartOption& Option = Found->Options.AddDefaulted_GetRef();
		Option.Key = Key;
		Option.Mesh = Cube;
		return Option;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPartEffectTriggerTest, "Looter.PartEffects.Trigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPartEffectTriggerTest::RunTest(const FString& Parameters)
{
	const FPartEffectTuning Tuning;
	const WeaponTrigger::FTriggerPlan Plain = WeaponTrigger::PlanFor(EWeaponFireMode::FullAuto, 0, EPartEffect::None, Tuning);
	TestTrue(TEXT("No effect: the definition's trigger"), Plain.FireMode == EWeaponFireMode::FullAuto && Plain.ChargeSeconds == 0.f
		&& Plain.PullIntervalScale == 1.f && Plain.BurstIntervalScale == 1.f);
	const WeaponTrigger::FTriggerPlan Twin = WeaponTrigger::PlanFor(EWeaponFireMode::SemiAuto, 0, EPartEffect::Twin, Tuning);
	TestTrue(TEXT("A bullet effect leaves the trigger alone"), Twin.FireMode == EWeaponFireMode::SemiAuto && Twin.ChargeSeconds == 0.f);

	const WeaponTrigger::FTriggerPlan Burst = WeaponTrigger::PlanFor(EWeaponFireMode::FullAuto, 0, EPartEffect::Burst, Tuning);
	TestTrue(TEXT("Burst: a full-auto gun fires bursts"), Burst.FireMode == EWeaponFireMode::Burst);
	TestEqual(TEXT("...of three"), Burst.BurstCount, 3);
	TestTrue(TEXT("...quick within, a breath between"), Burst.BurstIntervalScale < 1.f && Burst.PullIntervalScale > 1.f);
	TestEqual(TEXT("A burst gun keeps its own count"), WeaponTrigger::PlanFor(EWeaponFireMode::Burst, 4, EPartEffect::Burst, Tuning).BurstCount, 4);

	const WeaponTrigger::FTriggerPlan Charge = WeaponTrigger::PlanFor(EWeaponFireMode::FullAuto, 0, EPartEffect::Overcharge, Tuning);
	TestTrue(TEXT("Overcharge: one wound-up round a pull"), Charge.FireMode == EWeaponFireMode::SemiAuto);
	TestEqual(TEXT("...winding up 0.6 s"), Charge.ChargeSeconds, 0.6f, 0.0001f);
	TestEqual(TEXT("...at half the fire rate"), Charge.PullIntervalScale, 2.f, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPartEffectRulesTest, "Looter.PartEffects.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPartEffectRulesTest::RunTest(const FString& Parameters)
{
	using namespace PartEffectRules;
	const FPartEffectTuning Tuning;
	// Explosive: 60% to what it hit, 40% splashed (what it hit too).
	TestEqual(TEXT("A round that dealt 60 directly splashes 40"), SplashDamage(60.f, Tuning), 40.f, 0.001f);
	TestEqual(TEXT("Nothing dealt, nothing splashed"), SplashDamage(0.f, Tuning), 0.f);
	TestEqual(TEXT("Full at its heart"), SplashShareAt(0.f, Tuning), 1.f);
	TestEqual(TEXT("Its edge share at 1.5 m"), SplashShareAt(150.f, Tuning), Tuning.ExplosiveEdgeShare, 0.0001f);
	TestEqual(TEXT("Nothing past it"), SplashShareAt(151.f, Tuning), 0.f);
	// Overcharge.
	TestEqual(TEXT("Half wound after 0.3 s"), ChargeAfter(0.3f, Tuning), 0.5f, 0.0001f);
	TestEqual(TEXT("Full at 0.6 s and after"), ChargeAfter(2.f, Tuning), 1.f);
	TestEqual(TEXT("A full charge: double damage"), ChargeDamageScale(1.f, Tuning), 2.f, 0.0001f);
	TestEqual(TEXT("Half a charge: half again"), ChargeDamageScale(0.5f, Tuning), 1.5f, 0.0001f);
	TestEqual(TEXT("None: a plain round"), ChargeDamageScale(0.f, Tuning), 1.f);
	// Twin: two ways, the angle apart, either side of the aim.
	FVector Left;
	FVector Right;
	TwinDirections(FVector::ForwardVector, FVector::UpVector, Tuning, Left, Right);
	TestEqual(TEXT("The twins fly 0.8 degrees apart"), static_cast<float>(FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Left, Right)))), Tuning.TwinAngle, 0.01f);
	TestTrue(TEXT("...one either side, level"), Left.Y * Right.Y < 0.f && FMath::IsNearlyZero(Left.Z) && FMath::IsNearlyZero(Right.Z));
	// Ricochet's pick.
	const TArray<FVector> Spots = { FVector(500.f, 0.f, 0.f), FVector(300.f, 0.f, 0.f), FVector(900.f, 0.f, 0.f) };
	TestEqual(TEXT("The nearest within reach"), PickNearest(FVector::ZeroVector, Spots, 800.f), 1);
	TestEqual(TEXT("None within reach"), PickNearest(FVector(0.f, 5000.f, 0.f), Spots, 800.f), static_cast<int32>(INDEX_NONE));
	// Words.
	TestIs(*this, TEXT("Parse a word"), Parse(TEXT("ricochet")), EPartEffect::Ricochet);
	TestIs(*this, TEXT("...any of them"), Parse(TEXT("Overcharge")), EPartEffect::Overcharge);
	TestIs(*this, TEXT("Nothing: none"), Parse(TEXT("")), EPartEffect::None);
	TestTrue(TEXT("Every effect has its word and line"), !Word(EPartEffect::Twin).IsEmpty() && !Description(EPartEffect::Explosive).IsEmpty()
		&& Word(EPartEffect::None).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPartEffectPartsTest, "Looter.PartEffects.Parts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPartEffectPartsTest::RunTest(const FString& Parameters)
{
	// A gun's effect and element are its first part's that carries one, in slot order.
	UWeaponDefinition* Gun = NewObject<UWeaponDefinition>(GetTransientPackage());
	AddOption(*Gun, TEXT("Body"), TEXT("Plain"));
	FWeaponPartOption& Burner = AddOption(*Gun, TEXT("Barrel"), TEXT("Burner"));
	Burner.Element = ELooterElement::Ember;
	Burner.Effect = EPartEffect::Ricochet;
	FWeaponPartOption& Salter = AddOption(*Gun, TEXT("Magazine"), TEXT("Salter"));
	Salter.Element = ELooterElement::GraveSalt;
	Salter.Effect = EPartEffect::Twin;
	const TArray<FName> Kept = { TEXT("Plain"), TEXT("Burner"), TEXT("Salter") };
	const FWeaponLook Look = WeaponParts::Pick(*Gun, 1, EWeaponRarity::Epic, Kept);
	TestIs(*this, TEXT("The first part's element"), WeaponParts::ElementOf(Look), ELooterElement::Ember);
	TestIs(*this, TEXT("The first part's effect"), WeaponParts::EffectOf(Look), EPartEffect::Ricochet);
	const TArray<FName> NoBarrel = { TEXT("Plain"), NAME_None, TEXT("Salter") };
	const FWeaponLook Later = WeaponParts::Pick(*Gun, 1, EWeaponRarity::Epic, NoBarrel);
	TestIs(*this, TEXT("Without it, the next part's element"), WeaponParts::ElementOf(Later), ELooterElement::GraveSalt);
	TestIs(*this, TEXT("...and effect"), WeaponParts::EffectOf(Later), EPartEffect::Twin);
	const TArray<FName> Plain = { TEXT("Plain"), NAME_None, NAME_None };
	const FWeaponLook Bare = WeaponParts::Pick(*Gun, 1, EWeaponRarity::Epic, Plain);
	TestTrue(TEXT("A gun of plain parts has neither"), WeaponParts::ElementOf(Bare) == ELooterElement::None && WeaponParts::EffectOf(Bare) == EPartEffect::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPartEffectBounceTest, "Looter.PartEffects.Bounce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPartEffectBounceTest::RunTest(const FString& Parameters)
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
	const UPartEffectSubsystem* Effects = World->GetSubsystem<UPartEffectSubsystem>();
	ACreatureBase* Struck = SpawnSpider(World, FVector::ZeroVector);
	ACreatureBase* Near = SpawnSpider(World, FVector(400.0, 0.0, 0.0));
	ACreatureBase* Hidden = SpawnSpider(World, FVector(0.0, 300.0, 0.0));
	if (!TestNotNull(TEXT("The effects run in a test level"), Effects) || !TestNotNull(TEXT("Spiders"), Struck) || !Near || !Hidden)
	{
		return false;
	}
	// A wall between the struck spider and the nearer one: the bounce goes to the one it can see.
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 150.0, 100.0), FVector(400.0, 20.0, 300.0));
	const FVector From = Struck->GetActorLocation();
	TestTrue(TEXT("It bounces to the one it can see, past the nearer behind a wall"), Effects->FindBounceTarget(From, 800.f, Struck) == Near);
	TestNull(TEXT("Nothing within a short reach"), Effects->FindBounceTarget(From, 200.f, Struck));
	TestTrue(TEXT("Never back into the one it struck"), Effects->FindBounceTarget(Near->GetActorLocation(), 250.f, Near) == nullptr);
	return true;
}

#endif
