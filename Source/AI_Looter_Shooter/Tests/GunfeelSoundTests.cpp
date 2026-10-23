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
	const TCHAR* Names[] = { TEXT("Tail"), TEXT("Mech"), TEXT("Low"), TEXT("DistantFrom"), TEXT("ProbeRange"),
		// The gun feel pass's: bursts, the end of a magazine, the hiss and rack, impacts, creature hits.
		TEXT("AutoTailEvery.Rifle"), TEXT("AutoTailEvery.SMG"), TEXT("AutoTailEvery.LMG"), TEXT("BurstEndTail"), TEXT("BurstGap"),
		TEXT("LowAmmo.Rifle"), TEXT("LowAmmo.SMG"), TEXT("LowAmmo.LMG"), TEXT("LowAmmo.Shotgun"), TEXT("LowAmmo.Revolver"),
		TEXT("LowAmmo.Sniper"), TEXT("LowAmmo.Launcher"), TEXT("LastRound"), TEXT("LastRoundDelay"), TEXT("HissFrom"), TEXT("Hiss"),
		TEXT("Rack"), TEXT("Impact.Base"), TEXT("Impact.Slope"), TEXT("Impact.Min"), TEXT("Impact.Max"), TEXT("ScatterLouder"),
		TEXT("ImpactHeavyFrom"), TEXT("ImpactHeavy"), TEXT("ImpactScatter"), TEXT("CreatureHeavyFrom"), TEXT("CreatureHeavy"),
		TEXT("CreatureHeavyShare"), TEXT("CreatureBlastWindow"), TEXT("CreatureCrit") };
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
	{
		FScopedSoundVariable Every(TEXT("Looter.Gunfeel.Sound.AutoTailEvery.Rifle"), 0.f);
		TestEqual(TEXT("A tail every 0 shots is every shot"), GunfeelSound::AutoTailEvery(EWeaponKind::Rifle), 1);
	}
	{
		FScopedSoundVariable Every(TEXT("Looter.Gunfeel.Sound.AutoTailEvery.SMG"), 5.f);
		TestEqual(TEXT("A changed spacing is read at once"), GunfeelSound::AutoTailEvery(EWeaponKind::SMG), 5);
		TestEqual(TEXT("...and only for its kind"), GunfeelSound::AutoTailEvery(EWeaponKind::Shotgun), 1);
	}
	{
		FScopedSoundVariable Rounds(TEXT("Looter.Gunfeel.Sound.LowAmmo.Rifle"), -3.f);
		TestEqual(TEXT("Rounds that click never go under none"), GunfeelSound::LowAmmoRounds(EWeaponKind::Rifle), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundBurstsTest, "Looter.Gunfeel.Sound.Bursts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundBurstsTest::RunTest(const FString& Parameters)
{
	// Which kinds space their tails, by the shipped numbers.
	TestEqual(TEXT("Rifle: every 2nd"), Def::AutoTailEvery(EWeaponKind::Rifle), 2);
	TestEqual(TEXT("SMG: every 3rd"), Def::AutoTailEvery(EWeaponKind::SMG), 3);
	TestEqual(TEXT("LMG: every 2nd"), Def::AutoTailEvery(EWeaponKind::LMG), 2);
	for (EWeaponKind Kind : { EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::Sniper, EWeaponKind::Launcher })
	{
		TestEqual(TEXT("A single-shot kind tails every shot"), Def::AutoTailEvery(Kind), 1);
	}

	// Rifle: shots 0, 2, 4 tail. SMG: 0, 3, 6. The first shot of a burst always does.
	int32 RifleTails = 0;
	int32 SMGTails = 0;
	for (int32 Shot = 0; Shot < 12; ++Shot)
	{
		RifleTails += GunfeelSound::TailPlays(Shot, Def::AutoTailRifle) ? 1 : 0;
		SMGTails += GunfeelSound::TailPlays(Shot, Def::AutoTailSMG) ? 1 : 0;
	}
	TestEqual(TEXT("Rifle: 6 tails in 12 shots"), RifleTails, 6);
	TestEqual(TEXT("SMG: 4 tails in 12 shots"), SMGTails, 4);
	TestTrue(TEXT("The first shot always tails"), GunfeelSound::TailPlays(0, 3));
	TestFalse(TEXT("The second of an every-2nd doesn't"), GunfeelSound::TailPlays(1, 2));
	TestTrue(TEXT("The third does"), GunfeelSound::TailPlays(2, 2));
	TestFalse(TEXT("The third of an every-3rd doesn't"), GunfeelSound::TailPlays(2, 3));
	TestTrue(TEXT("The fourth of an every-3rd does"), GunfeelSound::TailPlays(3, 3));
	TestTrue(TEXT("Every shot when the spacing is 1"), GunfeelSound::TailPlays(7, 1));
	TestTrue(TEXT("...or nothing sensible"), GunfeelSound::TailPlays(7, 0));

	// A burst: shots within the gap of the last are one.
	TestTrue(TEXT("Never fired: a new burst"), GunfeelSound::NewBurst(5.0, -1.0, Def::BurstGap));
	TestFalse(TEXT("14 Hz fire is one burst"), GunfeelSound::NewBurst(5.07, 5.0, Def::BurstGap));
	TestFalse(TEXT("Right at the gap is still one"), GunfeelSound::NewBurst(5.0 + Def::BurstGap, 5.0, Def::BurstGap));
	TestTrue(TEXT("A second later: a new burst"), GunfeelSound::NewBurst(6.0, 5.0, Def::BurstGap));
	TestTrue(TEXT("A new world's clock: a new burst"), GunfeelSound::NewBurst(1.0, 50.0, Def::BurstGap));

	// The burst's end: only after an automatic run that skipped its last tail.
	TestTrue(TEXT("A 10-shot burst whose last shot skipped its tail ends with one"), GunfeelSound::BurstEndTailPlays(10, 1, 1.f));
	TestFalse(TEXT("...not when the last shot tailed"), GunfeelSound::BurstEndTailPlays(10, 0, 1.f));
	TestFalse(TEXT("...not for a single shot"), GunfeelSound::BurstEndTailPlays(1, 1, 1.f));
	TestFalse(TEXT("...not when the setting is off"), GunfeelSound::BurstEndTailPlays(10, 1, 0.f));
	TestTrue(TEXT("Two shots whose second skipped"), GunfeelSound::BurstEndTailPlays(2, 1, 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundMagazineTest, "Looter.Gunfeel.Sound.Magazine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundMagazineTest::RunTest(const FString& Parameters)
{
	// Rounds that click by kind (the shipped numbers) against real magazines.
	TestEqual(TEXT("Rifle clicks the last 6 of 30"), GunfeelSound::LowAmmoWindow(Def::LowAmmoRounds(EWeaponKind::Rifle), 30), 6);
	TestEqual(TEXT("...but only half of a 10 round magazine"), GunfeelSound::LowAmmoWindow(Def::LowAmmoRounds(EWeaponKind::Rifle), 10), 5);
	TestEqual(TEXT("SMG clicks the last 10 of 40"), GunfeelSound::LowAmmoWindow(Def::LowAmmoRounds(EWeaponKind::SMG), 40), 10);
	TestEqual(TEXT("LMG clicks the last 15 of 100 (not 25)"), GunfeelSound::LowAmmoWindow(Def::LowAmmoRounds(EWeaponKind::LMG), 100), 15);
	TestEqual(TEXT("Revolver clicks the last 2 of 6"), GunfeelSound::LowAmmoWindow(Def::LowAmmoRounds(EWeaponKind::Revolver), 6), 2);
	TestEqual(TEXT("Shotgun clicks the last 2"), GunfeelSound::LowAmmoWindow(Def::LowAmmoRounds(EWeaponKind::Shotgun), 8), 2);
	TestEqual(TEXT("Sniper clicks the last 1 of 5"), GunfeelSound::LowAmmoWindow(Def::LowAmmoRounds(EWeaponKind::Sniper), 5), 1);
	TestEqual(TEXT("Launcher never clicks"), GunfeelSound::LowAmmoWindow(Def::LowAmmoRounds(EWeaponKind::Launcher), 3), 0);
	TestEqual(TEXT("A magazine under 4 never clicks"), GunfeelSound::LowAmmoWindow(6, 3), 0);

	// The click rises: pitch 1 at the window's first round up to the top at the last.
	const float Top = Def::LowAmmoPitchTop;
	TestEqual(TEXT("First click of the run"), GunfeelSound::LowAmmoPitch(6, 6, Top), 1.f);
	TestEqual(TEXT("Last click before empty"), GunfeelSound::LowAmmoPitch(1, 6, Top), Top);
	TestEqual(TEXT("Mid-run"), GunfeelSound::LowAmmoPitch(3, 6, Top), 1.f + (Top - 1.f) * 0.6f);
	TestEqual(TEXT("A window of one is the last round's pitch"), GunfeelSound::LowAmmoPitch(1, 1, Top), Top);
	TestEqual(TEXT("Above the window: silence"), GunfeelSound::LowAmmoPitch(7, 6, Top), 0.f);
	TestEqual(TEXT("Empty: no click (the last round's own sound)"), GunfeelSound::LowAmmoPitch(0, 6, Top), 0.f);
	TestEqual(TEXT("No window: silence"), GunfeelSound::LowAmmoPitch(3, 0, Top), 0.f);

	// The last round.
	for (EWeaponKind Kind : { EWeaponKind::Rifle, EWeaponKind::SMG, EWeaponKind::LMG, EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::Sniper })
	{
		TestTrue(TEXT("The last round has its sound"), GunfeelSound::LastRoundPlays(Kind, 0, 6, Def::LastRound));
	}
	TestFalse(TEXT("A round still in the magazine isn't the last"), GunfeelSound::LastRoundPlays(EWeaponKind::Rifle, 1, 30, Def::LastRound));
	TestFalse(TEXT("The launcher has none"), GunfeelSound::LastRoundPlays(EWeaponKind::Launcher, 0, 3, Def::LastRound));
	TestFalse(TEXT("Off at volume 0"), GunfeelSound::LastRoundPlays(EWeaponKind::Rifle, 0, 30, 0.f));
	TestFalse(TEXT("A one-round magazine's only shot isn't a last round"), GunfeelSound::LastRoundPlays(EWeaponKind::Rifle, 0, 1, Def::LastRound));

	// The LMG's hot barrel and the shotgun's rack.
	TestTrue(TEXT("A hot LMG hisses at a burst's end"), GunfeelSound::HeatHissPlays(EWeaponKind::LMG, 0.9f, Def::HissFrom, 20, Def::Hiss));
	TestFalse(TEXT("A cool one doesn't"), GunfeelSound::HeatHissPlays(EWeaponKind::LMG, 0.5f, Def::HissFrom, 20, Def::Hiss));
	TestFalse(TEXT("A single tap doesn't"), GunfeelSound::HeatHissPlays(EWeaponKind::LMG, 0.9f, Def::HissFrom, 1, Def::Hiss));
	TestFalse(TEXT("A rifle never hisses"), GunfeelSound::HeatHissPlays(EWeaponKind::Rifle, 0.9f, Def::HissFrom, 20, Def::Hiss));
	TestEqual(TEXT("The hiss starts at half"), GunfeelSound::HeatHissLoudness(Def::HissFrom, Def::HissFrom), 0.5f);
	TestEqual(TEXT("...is full at a glowing barrel"), GunfeelSound::HeatHissLoudness(1.f, Def::HissFrom), 1.f);
	TestEqual(TEXT("...and in between"), GunfeelSound::HeatHissLoudness(0.8f, Def::HissFrom), 0.75f);
	TestTrue(TEXT("The shooter hears their own rack"), GunfeelSound::RackAudible(true, 99999.f, Def::DistantFrom));
	TestTrue(TEXT("Someone near hears it"), GunfeelSound::RackAudible(false, 2000.f, Def::DistantFrom));
	TestFalse(TEXT("Someone far doesn't"), GunfeelSound::RackAudible(false, 3500.f, Def::DistantFrom));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundHitsTest, "Looter.Gunfeel.Sound.Hits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundHitsTest::RunTest(const FString& Parameters)
{
	// Impact loudness by the gun's mark scale (FBulletMarks::GunScale: shotgun 0.6, SMG 0.8, rifle 1, revolver 1.25, sniper 1.5).
	auto Loud = [](float Scale)
	{
		return GunfeelSound::ImpactLoudness(Scale, Def::ImpactBase, Def::ImpactSlope, Def::ImpactMin, Def::ImpactMax);
	};
	TestEqual(TEXT("The rifle is the reference"), Loud(1.f), 1.f);
	TestEqual(TEXT("A shotgun pellet is softer"), Loud(0.6f), 0.9f);
	TestEqual(TEXT("The SMG a little"), Loud(0.8f), 0.95f);
	TestEqual(TEXT("The revolver a little louder"), Loud(1.25f), 1.0625f);
	TestEqual(TEXT("The sniper louder still"), Loud(1.5f), 1.125f);
	TestEqual(TEXT("Never past the top"), Loud(9.f), Def::ImpactMax);
	TestEqual(TEXT("Never under the floor"), Loud(-9.f), Def::ImpactMin);
	TestTrue(TEXT("A blast's one cue is louder than a pellet's"), Loud(0.6f) * Def::ScatterLouder > Loud(0.6f));

	// The heavy layers: the revolver and the sniper, not the rifle, LMG or shotgun.
	TestTrue(TEXT("Revolver is heavy"), GunfeelSound::IsHeavy(1.25f, Def::ImpactHeavyFrom));
	TestTrue(TEXT("Sniper is heavy"), GunfeelSound::IsHeavy(1.5f, Def::ImpactHeavyFrom));
	TestFalse(TEXT("LMG is not"), GunfeelSound::IsHeavy(1.05f, Def::ImpactHeavyFrom));
	TestFalse(TEXT("Rifle is not"), GunfeelSound::IsHeavy(1.f, Def::ImpactHeavyFrom));
	TestFalse(TEXT("A threshold of 0 turns it off"), GunfeelSound::IsHeavy(9.f, 0.f));
	TestEqual(TEXT("Creatures share the threshold"), Def::CreatureHeavyFrom, Def::ImpactHeavyFrom);

	// A shotgun blast's pellets add up on one creature: one thump when they reach the share, none for the rest of the blast.
	GunfeelSound::FBlastTally Tally;
	const float From = Def::CreatureHeavyShare;
	const float Window = Def::CreatureBlastWindow;
	TestFalse(TEXT("A first pellet"), GunfeelSound::BlastIsHeavy(Tally, 10.00, 0.1f, Window, From));
	TestFalse(TEXT("A second"), GunfeelSound::BlastIsHeavy(Tally, 10.02, 0.1f, Window, From));
	TestFalse(TEXT("A third (0.3)"), GunfeelSound::BlastIsHeavy(Tally, 10.04, 0.1f, Window, From));
	TestTrue(TEXT("The fourth takes it past a third of its health"), GunfeelSound::BlastIsHeavy(Tally, 10.06, 0.1f, Window, From));
	TestFalse(TEXT("One thump to a blast"), GunfeelSound::BlastIsHeavy(Tally, 10.07, 0.1f, Window, From));
	TestFalse(TEXT("The next blast starts over"), GunfeelSound::BlastIsHeavy(Tally, 10.50, 0.1f, Window, From));
	TestTrue(TEXT("...and one big pellet is enough"), GunfeelSound::BlastIsHeavy(Tally, 11.00, 0.5f, Window, From));
	GunfeelSound::FBlastTally Never;
	TestFalse(TEXT("A share of 0 turns it off"), GunfeelSound::BlastIsHeavy(Never, 1.0, 5.f, Window, 0.f));

	// The crit crack by what the body is made of, and the confirms.
	TestEqual(TEXT("Spiders crack"), GunfeelSound::CritCueFor(ECreatureBlood::Ichor), FName(LooterSoundCue::Gunfeel::CritShell));
	TestEqual(TEXT("The Gravemother cracks"), GunfeelSound::CritCueFor(ECreatureBlood::DarkIchor), FName(LooterSoundCue::Gunfeel::CritShell));
	TestEqual(TEXT("Slimes pop"), GunfeelSound::CritCueFor(ECreatureBlood::Gel), FName(LooterSoundCue::Gunfeel::CritGel));
	TestEqual(TEXT("Clay uses the gel's"), GunfeelSound::CritCueFor(ECreatureBlood::Clay), FName(LooterSoundCue::Gunfeel::CritGel));
	TestEqual(TEXT("The Unpaid snap"), GunfeelSound::CritCueFor(ECreatureBlood::Embers), FName(LooterSoundCue::Gunfeel::CritCoal));
	TestEqual(TEXT("Miners snap"), GunfeelSound::CritCueFor(ECreatureBlood::MinerEmbers), FName(LooterSoundCue::Gunfeel::CritCoal));
	TestTrue(TEXT("No body, no crack"), GunfeelSound::CritCueFor(ECreatureBlood::None).IsNone());
	TestEqual(TEXT("A plain hit ticks"), GunfeelSound::HitConfirmCue(false, false), FName(LooterSoundCue::HitMarker));
	TestEqual(TEXT("A critical pings"), GunfeelSound::HitConfirmCue(true, false), FName(LooterSoundCue::Gunfeel::HitMarkerCrit));
	TestEqual(TEXT("A ward taking it is glassy"), GunfeelSound::HitConfirmCue(false, true), FName(LooterSoundCue::Gunfeel::HitMarkerWard));
	TestEqual(TEXT("...a critical too"), GunfeelSound::HitConfirmCue(true, true), FName(LooterSoundCue::Gunfeel::HitMarkerWard));
	TestEqual(TEXT("A plain kill"), GunfeelSound::KillConfirmCue(false, true), FName(LooterSoundCue::Kill));
	TestEqual(TEXT("A critical kill"), GunfeelSound::KillConfirmCue(true, true), FName(LooterSoundCue::Gunfeel::KillCrit));
	TestEqual(TEXT("An older bank without it: the plain kill"), GunfeelSound::KillConfirmCue(true, false), FName(LooterSoundCue::Kill));
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

	// The gun feel pass's layers and cues: the named constants are what CueFor builds.
	namespace Cue = LooterSoundCue::Gunfeel;
	TestEqual(TEXT("The rifle's last round"), GunshotSound::CueFor(EWeaponKind::Rifle, EGunshotLayer::LastRound), FName(Cue::RifleLastRound));
	TestEqual(TEXT("The SMG's last round"), GunshotSound::CueFor(EWeaponKind::SMG, EGunshotLayer::LastRound), FName(Cue::SMGLastRound));
	TestEqual(TEXT("The LMG's last round"), GunshotSound::CueFor(EWeaponKind::LMG, EGunshotLayer::LastRound), FName(Cue::LMGLastRound));
	TestEqual(TEXT("The shotgun's last round"), GunshotSound::CueFor(EWeaponKind::Shotgun, EGunshotLayer::LastRound), FName(Cue::ShotgunLastRound));
	TestEqual(TEXT("The revolver's last round"), GunshotSound::CueFor(EWeaponKind::Revolver, EGunshotLayer::LastRound), FName(Cue::RevolverLastRound));
	TestEqual(TEXT("The sniper's last round"), GunshotSound::CueFor(EWeaponKind::Sniper, EGunshotLayer::LastRound), FName(Cue::SniperLastRound));
	TestEqual(TEXT("The shotgun's rack"), GunshotSound::CueFor(EWeaponKind::Shotgun, EGunshotLayer::Rack), FName(Cue::ShotgunRack));
	TestEqual(TEXT("The LMG's hiss"), GunshotSound::CueFor(EWeaponKind::LMG, EGunshotLayer::HeatHiss), FName(Cue::LMGHeatHiss));
	TestEqual(TEXT("The contract's names"), FName(Cue::ImpactHeavy), FName(TEXT("Impact.Heavy")));
	TestEqual(TEXT("...all of them"), FName(Cue::CreatureHitHeavy), FName(TEXT("Creature.Hit.Heavy")));
	TestEqual(TEXT("...the kill"), FName(Cue::KillCrit), FName(TEXT("UI.Kill.Crit")));
	return true;
}

#endif
