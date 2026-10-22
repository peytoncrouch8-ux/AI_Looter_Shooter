#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/LooterSoundCues.h"
#include "Weapons/GunfeelSoundTuning.h"
#include "Weapons/GunshotSound.h"
#include "Weapons/WeaponTypes.h"
#include "HAL/IConsoleManager.h"

// The sound side of the gunplay pass (Docs/Plan.md 12.1, GUNFEEL-SOUND): which of a shot's layers play and the tail the
// surroundings pick, driven with explicit numbers or the shipped defaults (GunfeelSound::Default), never a value the
// player may have set in play. The variables are checked to exist and to be read live, and put back as they were.

namespace
{
	namespace Def = GunfeelSound::Default;
	using GunfeelSound::ESurroundings;

	/** A console variable, set to Value for the scope and put back after. */
	struct FScopedSoundVariable
	{
		explicit FScopedSoundVariable(const TCHAR* Name, float Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Was = Variable->GetFloat();
				Variable->Set(*FString::SanitizeFloat(Value), ECVF_SetByConsole);
			}
		}
		~FScopedSoundVariable()
		{
			if (Variable)
			{
				Variable->Set(*FString::SanitizeFloat(Was), ECVF_SetByConsole);
			}
		}
		IConsoleVariable* Variable = nullptr;
		float Was = 0.f;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundVariablesTest, "Looter.Gunfeel.Sound.Variables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundVariablesTest::RunTest(const FString& Parameters)
{
	const TCHAR* Names[] = { TEXT("Tail"), TEXT("Mech"), TEXT("Low"), TEXT("DistantFrom"), TEXT("ProbeRange") };
	for (const TCHAR* Name : Names)
	{
		const FString Full = FString(TEXT("Looter.Gunfeel.Sound.")) + Name;
		TestNotNull(*FString::Printf(TEXT("%s exists"), *Full), IConsoleManager::Get().FindConsoleVariable(*Full));
	}
	TestEqual(TEXT("Shots past 30 m are far ones"), Def::DistantFrom, 3000.f);
	TestEqual(TEXT("The probe looks 8 m round"), Def::ProbeRange, 800.f);
	TestEqual(TEXT("The layers start at full volume"), Def::Tail + Def::Mech + Def::Low, 3.f);
	{
		FScopedSoundVariable Tail(TEXT("Looter.Gunfeel.Sound.Tail"), 0.4f);
		TestEqual(TEXT("A changed tail volume is read at once"), GunfeelSound::TailVolume(), 0.4f);
	}
	{
		FScopedSoundVariable Mech(TEXT("Looter.Gunfeel.Sound.Mech"), -2.f);
		TestEqual(TEXT("A volume never goes under silence"), GunfeelSound::MechVolume(), 0.f);
	}
	{
		FScopedSoundVariable Far(TEXT("Looter.Gunfeel.Sound.DistantFrom"), 4500.f);
		TestEqual(TEXT("A changed far distance is read at once"), GunfeelSound::DistantFrom(), 4500.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundSurroundingsTest, "Looter.Gunfeel.Sound.Surroundings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundSurroundingsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Nothing round: open land"), GunfeelSound::Classify(false, 0), ESurroundings::Open);
	TestEqual(TEXT("One wall is still open land"), GunfeelSound::Classify(false, 1), ESurroundings::Open);
	TestEqual(TEXT("Walls on two sides: a street"), GunfeelSound::Classify(false, 2), ESurroundings::Town);
	TestEqual(TEXT("Walls all round, no roof: a yard"), GunfeelSound::Classify(false, 4), ESurroundings::Town);
	TestEqual(TEXT("A roof alone (a porch, a ledge) isn't a room"), GunfeelSound::Classify(true, 0), ESurroundings::Town);
	TestEqual(TEXT("A roof and one wall: still a porch"), GunfeelSound::Classify(true, 1), ESurroundings::Town);
	TestEqual(TEXT("A roof and walls on two sides: inside"), GunfeelSound::Classify(true, 2), ESurroundings::Inside);
	TestEqual(TEXT("A room"), GunfeelSound::Classify(true, 4), ESurroundings::Inside);

	TestEqual(TEXT("Open land's tail"), GunfeelSound::TailFor(ESurroundings::Open), EGunshotLayer::TailOpen);
	TestEqual(TEXT("A town's tail"), GunfeelSound::TailFor(ESurroundings::Town), EGunshotLayer::TailTown);
	TestEqual(TEXT("A room's tail"), GunfeelSound::TailFor(ESurroundings::Inside), EGunshotLayer::TailInside);

	TestTrue(TEXT("Never looked: look"), GunfeelSound::ProbeStale(10.0, -1.0));
	TestFalse(TEXT("Looked a tenth of a second ago: keep it"), GunfeelSound::ProbeStale(10.1, 10.0));
	TestTrue(TEXT("A quarter second on: look again"), GunfeelSound::ProbeStale(10.25, 10.0));
	TestTrue(TEXT("A new world's clock (time went back): look again"), GunfeelSound::ProbeStale(1.0, 50.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundLayersTest, "Looter.Gunfeel.Sound.Layers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundLayersTest::RunTest(const FString& Parameters)
{
	// The shooter's own gun: the punch, the action, the chest thump and the tail, however far its camera is.
	const GunfeelSound::FShotLayers Own = GunfeelSound::Choose(true, 99999.f, Def::DistantFrom, ESurroundings::Inside);
	TestTrue(TEXT("Own: the punch"), Own.bCore);
	TestTrue(TEXT("Own: the action"), Own.bMech);
	TestTrue(TEXT("Own: the chest thump"), Own.bLow);
	TestTrue(TEXT("Own: the tail"), Own.bTail);
	TestEqual(TEXT("Own: the room's tail inside"), Own.Tail, EGunshotLayer::TailInside);
	TestFalse(TEXT("Own: never the far shot"), Own.bDistant);

	// Someone else's, near: the punch and the tail, none of the shooter's own layers.
	const GunfeelSound::FShotLayers Near = GunfeelSound::Choose(false, 1200.f, Def::DistantFrom, ESurroundings::Town);
	TestTrue(TEXT("Near: the punch"), Near.bCore);
	TestTrue(TEXT("Near: the tail"), Near.bTail);
	TestEqual(TEXT("Near: the town's tail"), Near.Tail, EGunshotLayer::TailTown);
	TestFalse(TEXT("Near: not the action"), Near.bMech);
	TestFalse(TEXT("Near: not the chest thump"), Near.bLow);
	TestFalse(TEXT("Near: not the far shot"), Near.bDistant);

	// Someone else's, far: the far shot alone.
	const GunfeelSound::FShotLayers Far = GunfeelSound::Choose(false, 3500.f, Def::DistantFrom, ESurroundings::Open);
	TestTrue(TEXT("Far: the far shot"), Far.bDistant);
	TestFalse(TEXT("Far: no punch"), Far.bCore);
	TestFalse(TEXT("Far: no tail (the far take has its field)"), Far.bTail);
	TestFalse(TEXT("Exactly at the edge is still near"), GunfeelSound::Choose(false, 3000.f, 3000.f, ESurroundings::Open).bDistant);

	// The cues the layers play.
	TestEqual(TEXT("The rifle's action"), GunshotSound::CueFor(EWeaponKind::Rifle, EGunshotLayer::Mech), FName(TEXT("Weapon.Rifle.Mech")));
	TestEqual(TEXT("The shotgun's room"), GunshotSound::CueFor(EWeaponKind::Shotgun, EGunshotLayer::TailInside),
		FName(TEXT("Weapon.Shotgun.TailInside")));
	TestEqual(TEXT("The revolver's far shot"), GunshotSound::CueFor(EWeaponKind::Revolver, EGunshotLayer::Distant),
		FName(TEXT("Weapon.Revolver.Distant")));
	TestEqual(TEXT("Fire is still the whole shot"), GunshotSound::CueFor(EWeaponKind::Shotgun, EGunshotLayer::Fire),
		FName(LooterSoundCue::ShotgunFire));
	TestEqual(TEXT("The punch's cue follows the same naming"), FName(LooterSoundCue::Gunshot::RevolverCore),
		FName(TEXT("Weapon.Revolver.Core")));
	return true;
}

#endif
