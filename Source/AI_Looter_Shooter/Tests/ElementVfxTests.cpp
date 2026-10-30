#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Elements/ElementRules.h"
#include "Weapons/ElementVfxTuning.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTypes.h"
#include "Weapons/WeaponFX.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The element guns' own looks ("Elements matter" lane F, 2026-10-29): the flash leaning to the element with its sparks, the
// trail behind the tracer, the held gun's idle pieces, the flare on an element mark and the bigger deaths. Looter.Vfx.Element.*.

namespace
{
	const ELooterElement AllElements[] = { ELooterElement::Ember, ELooterElement::SoulLight, ELooterElement::GraveSalt, ELooterElement::Thunder,
		ELooterElement::Rot };

	/** Runs the effects Seconds on in steps (the camera at Camera), so what was spawned plays out. */
	void PlayOut(FWeaponFX& Effects, float Seconds, const FVector& Camera)
	{
		for (float Time = 0.f; Time < Seconds; Time += 0.05f)
		{
			Effects.Tick(0.05f, Camera);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementVfxRulesTest, "Looter.Vfx.Element.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementVfxRulesTest::RunTest(const FString& Parameters)
{
	// The flash's heart stays a gunshot's (it barely leans), its bloom takes most of the element's hue; none at Amount 0.
	const FLinearColor Base(1.f, 0.45f, 0.12f);
	const FLinearColor Hue = ElementRules::FxColor(ELooterElement::Thunder);
	TestTrue(TEXT("No tint at 0"), VfxRules::MuzzleElementTint(EVfxFlashLayer::Bloom, Base, Hue, 0.f).Equals(Base, 0.0001f));
	const float CoreLean = FLinearColor::Dist(VfxRules::MuzzleElementTint(EVfxFlashLayer::Core, Base, Hue, 1.f), Base);
	const float BloomLean = FLinearColor::Dist(VfxRules::MuzzleElementTint(EVfxFlashLayer::Bloom, Base, Hue, 1.f), Base);
	TestTrue(TEXT("The bloom leans further than the core"), BloomLean > CoreLean && CoreLean > 0.f);
	TestTrue(TEXT("Never past the element's hue"), FLinearColor::Dist(VfxRules::MuzzleElementTint(EVfxFlashLayer::Bloom, Base, Hue, 5.f), Base)
		<= FLinearColor::Dist(Hue, Base) + 0.0001f);

	// Fast guns trail every other round; every element gives idle pieces, a plain gun none.
	TestEqual(TEXT("SMG every other"), FElementVfxTuning::TrailEvery(EWeaponKind::SMG), 2);
	TestEqual(TEXT("Rifle every round"), FElementVfxTuning::TrailEvery(EWeaponKind::Rifle), 1);
	for (const ELooterElement Element : AllElements)
	{
		TestTrue(FString::Printf(TEXT("%s idles"), *ElementRules::Word(Element)), FElementVfxTuning::IdleRate(Element) > 0.f);
	}
	TestEqual(TEXT("A plain gun doesn't"), FElementVfxTuning::IdleRate(ELooterElement::None), 0.f);
	const FElementVfxTuning Defaults;
	TestTrue(TEXT("Everything on by default"), Defaults.Muzzle > 0.f && Defaults.Trail > 0.f && Defaults.Idle > 0.f && Defaults.bMarks
		&& Defaults.MarkFlare > 0.f && Defaults.Death > 0.f && Defaults.TrailReach > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementVfxLooksTest, "Looter.Vfx.Element.Looks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementVfxLooksTest::RunTest(const FString& Parameters)
{
	// Each element's muzzle sparks (first person and from behind), trail piece, idle piece and mark flare spawn pieces, and all
	// of them are gone a few seconds on; None spawns nothing.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FWeaponFX Effects;
	Effects.Initialize(WorldWrapper.GetTestWorld());
	const FVector Camera(-300.0, 0.0, 100.0);
	PlayOut(Effects, 0.1f, Camera);
	for (const ELooterElement Element : AllElements)
	{
		const FString Word = ElementRules::Word(Element);
		for (const bool bFirstPerson : { true, false })
		{
			FVfxMuzzle Muzzle;
			Muzzle.Kind = EWeaponKind::Rifle;
			Muzzle.Location = FVector(0.0, 12.0, -10.0);
			Muzzle.bFirstPerson = bFirstPerson;
			Muzzle.Away = FVector(0.0, 0.7, -0.7);
			Muzzle.Element = Element;
			const int32 Before = Effects.NumParticles();
			Effects.SpawnElementMuzzle(Muzzle);
			TestTrue(FString::Printf(TEXT("%s: muzzle sparks (%s)"), *Word, bFirstPerson ? TEXT("first person") : TEXT("from behind")),
				Effects.NumParticles() > Before);
		}
		int32 Before = Effects.NumParticles();
		for (int32 Piece = 0; Piece < 6; ++Piece)
		{
			Effects.SpawnElementTrail(Element, FVector(200.0 + Piece * 120.0, 0.0, 100.0), FVector(320.0 + Piece * 120.0, 0.0, 100.0), 1.f);
		}
		TestTrue(FString::Printf(TEXT("%s: a trail"), *Word), Effects.NumParticles() > Before);
		Before = Effects.NumParticles();
		Effects.SpawnElementIdle(Element, FVector(40.0, 10.0, -10.0), FVector(100.0, 0.0, 0.0), 1.f);
		TestTrue(FString::Printf(TEXT("%s: an idle piece"), *Word), Effects.NumParticles() > Before);
		Before = Effects.NumParticles();
		Effects.SpawnElementMark(Element, FVector(500.0, 0.0, 0.0), FVector(-1.0, 0.0, 0.0), 1.f);
		TestTrue(FString::Printf(TEXT("%s: a flare on its mark"), *Word), Effects.NumParticles() > Before);
	}
	const int32 Before = Effects.NumParticles();
	FVfxMuzzle Plain;
	Effects.SpawnElementMuzzle(Plain);
	Effects.SpawnElementTrail(ELooterElement::None, FVector::ZeroVector, FVector(100.0, 0.0, 0.0), 1.f);
	Effects.SpawnElementIdle(ELooterElement::None, FVector::ZeroVector, FVector::ZeroVector, 1.f);
	Effects.SpawnElementMark(ELooterElement::None, FVector::ZeroVector, FVector::UpVector, 1.f);
	TestEqual(TEXT("None spawns nothing"), Effects.NumParticles(), Before);
	PlayOut(Effects, 4.f, Camera);
	TestEqual(TEXT("All gone a few seconds on"), Effects.NumParticles(), 0);
	Effects.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementVfxDeathsTest, "Looter.Vfx.Element.Deaths", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FElementVfxDeathsTest::RunTest(const FString& Parameters)
{
	// Each bigger death spawns its layer; Wardbreak's shatter waits (its shards fly only after the body has stood as salt), and
	// everything plays out.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FWeaponFX Effects;
	Effects.Initialize(WorldWrapper.GetTestWorld());
	const FVector Camera(-500.0, 0.0, 100.0);
	const FVector Body(0.0, 0.0, 60.0);
	const FVector Front(-32.0, 0.0, 0.0);
	for (const ELooterElement Element : AllElements)
	{
		const int32 Before = Effects.NumParticles();
		Effects.SpawnElementDeath(Element, Body, 40.f, 60.f, Front, 1.f);
		TestTrue(FString::Printf(TEXT("%s: a bigger death (%d pieces)"), *ElementRules::Word(Element), Effects.NumParticles() - Before),
			Effects.NumParticles() > Before);
		PlayOut(Effects, 8.f, Camera);
		TestEqual(FString::Printf(TEXT("%s: played out"), *ElementRules::Word(Element)), Effects.NumParticles(), 0);
	}
	// Half the share throws fewer pieces; none at 0.
	Effects.SpawnElementDeath(ELooterElement::GraveSalt, Body, 40.f, 60.f, Front, 1.f);
	const int32 Full = Effects.NumParticles();
	PlayOut(Effects, 8.f, Camera);
	Effects.SpawnElementDeath(ELooterElement::GraveSalt, Body, 40.f, 60.f, Front, 0.5f);
	TestTrue(TEXT("Half a share, fewer pieces"), Effects.NumParticles() < Full);
	PlayOut(Effects, 8.f, Camera);
	Effects.SpawnElementDeath(ELooterElement::GraveSalt, Body, 40.f, 60.f, Front, 0.f);
	Effects.SpawnElementDeath(ELooterElement::None, Body, 40.f, 60.f, Front, 1.f);
	TestEqual(TEXT("Nothing at 0 or for None"), Effects.NumParticles(), 0);
	Effects.Shutdown();
	return true;
}

#endif
