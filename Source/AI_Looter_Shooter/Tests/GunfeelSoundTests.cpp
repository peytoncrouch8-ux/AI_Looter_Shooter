#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponMakers.h"
#include "Audio/LooterSoundCues.h"
#include "Weapons/GunfeelSoundTuning.h"
#include "Weapons/GunshotSound.h"
#include "Weapons/MakerSoundFlavor.h"
#include "Weapons/WeaponSounds.h"
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

// The gun makers' sound flavours (2026-10-28, the user's pick): Roundhouse heavy, Saint Maud ringing, Windfall rattly,
// Meridian clean; a gun with no maker (or a key the table doesn't know) sounds as its kind always did.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundMakerFlavorsTest, "Looter.Gunfeel.Sound.Maker.Flavors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundMakerFlavorsTest::RunTest(const FString& Parameters)
{
	using namespace MakerSoundFlavor;
	for (const FWeaponMaker& Maker : WeaponMakers::All())
	{
		TestNotNull(*FString::Printf(TEXT("%s has a sound flavour"), *Maker.Key.ToString()), ShippedFor(Maker.Key));
		TestFalse(*FString::Printf(TEXT("%s's flavour is heard"), *Maker.Key.ToString()), IsPlain(ShippedFlavor(Maker.Key)));
	}
	TestEqual(TEXT("One flavour per maker"), Shipped().Num(), WeaponMakers::All().Num());

	const FMakerSoundFlavor Roundhouse = ShippedFlavor(TEXT("Roundhouse"));
	TestTrue(TEXT("Roundhouse is lower"), Roundhouse.Pitch < 1.f && Roundhouse.Pitch >= 0.9f);
	TestTrue(TEXT("...with more chest thump"), Roundhouse.Low > 1.f);
	TestTrue(TEXT("...and no layer of its own"), ExtraCueFor(TEXT("Roundhouse")).IsNone());

	const FMakerSoundFlavor SaintMaud = ShippedFlavor(TEXT("SaintMaud"));
	TestTrue(TEXT("Saint Maud is a touch brighter"), SaintMaud.Pitch > 1.f && SaintMaud.Pitch <= 1.1f);
	TestTrue(TEXT("...its tail louder and brighter"), SaintMaud.Tail > 1.f && TailPitchOf(SaintMaud) > SaintMaud.Pitch);
	TestTrue(TEXT("...and its barrel rings"), SaintMaud.Extra > 0.f);
	TestEqual(TEXT("...with the ring's cue"), ExtraCueFor(TEXT("SaintMaud")), FName(MakerSoundCue::SaintMaudRing));

	const FMakerSoundFlavor Windfall = ShippedFlavor(TEXT("Windfall"));
	TestTrue(TEXT("Windfall's action is louder"), Windfall.Mech > 1.f);
	TestTrue(TEXT("...never the same pitch twice"), Windfall.MechJitter > 0.f
		&& MechPitch(Windfall, 0.f) < MechPitch(Windfall, 1.f));
	TestEqual(TEXT("...and rattles"), ExtraCueFor(TEXT("Windfall")), FName(MakerSoundCue::WindfallRattle));

	const FMakerSoundFlavor Meridian = ShippedFlavor(TEXT("Meridian"));
	TestEqual(TEXT("Meridian keeps the kind's pitch"), Meridian.Pitch, 1.f);
	TestTrue(TEXT("...with a short, quiet tail"), Meridian.Tail < 1.f);
	TestTrue(TEXT("...and a tight action"), Meridian.Mech < 1.f && FMath::IsNearlyZero(Meridian.MechJitter));
	TestTrue(TEXT("...and no layer of its own"), ExtraCueFor(TEXT("Meridian")).IsNone());

	// Subtle enough that the gun still sounds like its kind.
	for (const FShipped& Entry : Shipped())
	{
		const FString Key = Entry.Key;
		TestTrue(*(Key + TEXT(": pitch within 10%")), FMath::Abs(Entry.Flavor.Pitch - 1.f) <= 0.1f
			&& FMath::Abs(TailPitchOf(Entry.Flavor) - 1.f) <= 0.12f);
		TestTrue(*(Key + TEXT(": no layer past +4 dB or under -4 dB")),
			Entry.Flavor.Core <= 1.6f && Entry.Flavor.Mech <= 1.6f && Entry.Flavor.Low <= 1.6f && Entry.Flavor.Tail <= 1.6f
			&& Entry.Flavor.Core >= 0.6f && Entry.Flavor.Mech >= 0.6f && Entry.Flavor.Low >= 0.6f && Entry.Flavor.Tail >= 0.6f);
	}

	// No maker, or one gone from the table: plain.
	TestTrue(TEXT("No maker is plain"), IsPlain(ShippedFlavor(NAME_None)));
	TestTrue(TEXT("An unknown maker is plain"), IsPlain(ShippedFlavor(TEXT("NoSuchMaker"))));
	TestTrue(TEXT("...live too"), IsPlain(For(TEXT("NoSuchMaker"))) && IsPlain(For(NAME_None)));
	TestTrue(TEXT("...with no layer of its own"), ExtraCueFor(TEXT("NoSuchMaker")).IsNone());
	TestTrue(TEXT("A default flavour is plain"), IsPlain(FMakerSoundFlavor()));
	TestEqual(TEXT("A plain action's pitch never moves"), MechPitch(FMakerSoundFlavor(), 0.f), 1.f);

	// The scale: 0 plain, 1 as shipped, 2 twice as far; nothing goes under silence.
	TestTrue(TEXT("Scale 0 is plain"), IsPlain(Scaled(Roundhouse, 0.f)));
	TestEqual(TEXT("Scale 1 keeps the pitch"), Scaled(Roundhouse, 1.f).Pitch, Roundhouse.Pitch);
	TestTrue(TEXT("Scale 2 doubles the move"), FMath::IsNearlyEqual(Scaled(Roundhouse, 2.f).Low, 1.f + 2.f * (Roundhouse.Low - 1.f)));
	TestEqual(TEXT("...and the extra layer"), Scaled(SaintMaud, 2.f).Extra, 2.f * SaintMaud.Extra);
	FMakerSoundFlavor Quiet;
	Quiet.Tail = 0.2f;
	TestEqual(TEXT("A volume never goes under silence"), Scaled(Quiet, 3.f).Tail, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundMakerLiveTest, "Looter.Gunfeel.Sound.Maker.Live",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundMakerLiveTest::RunTest(const FString& Parameters)
{
	using namespace MakerSoundFlavor;
	TestNotNull(TEXT("The scale exists"), IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Gunfeel.Sound.Maker.Scale")));
	const TCHAR* Fields[] = { TEXT("Pitch"), TEXT("TailPitch"), TEXT("Core"), TEXT("Mech"), TEXT("Low"), TEXT("Tail"),
		TEXT("MechJitter"), TEXT("Extra") };
	for (const FShipped& Entry : Shipped())
	{
		for (const TCHAR* Field : Fields)
		{
			const FString Name = FString::Printf(TEXT("Looter.Gunfeel.Sound.Maker.%s.%s"), Entry.Key, Field);
			TestNotNull(*FString::Printf(TEXT("%s exists"), *Name), IConsoleManager::Get().FindConsoleVariable(*Name));
		}
	}
	{
		FScopedSoundVariable Scale(TEXT("Looter.Gunfeel.Sound.Maker.Scale"), 1.f);
		FScopedSoundVariable Pitch(TEXT("Looter.Gunfeel.Sound.Maker.Roundhouse.Pitch"), 0.8f);
		TestEqual(TEXT("A changed maker pitch is read at once"), For(TEXT("Roundhouse")).Pitch, 0.8f);
		{
			FScopedSoundVariable Off(TEXT("Looter.Gunfeel.Sound.Maker.Scale"), 0.f);
			TestTrue(TEXT("Scale 0 makes every maker plain"), IsPlain(For(TEXT("Roundhouse"))) && IsPlain(For(TEXT("SaintMaud"))));
		}
		{
			FScopedSoundVariable Twice(TEXT("Looter.Gunfeel.Sound.Maker.Scale"), 2.f);
			TestTrue(TEXT("Scale 2 doubles a live move"), FMath::IsNearlyEqual(For(TEXT("Roundhouse")).Pitch, 0.6f));
		}
		{
			FScopedSoundVariable Below(TEXT("Looter.Gunfeel.Sound.Maker.Scale"), -1.f);
			TestEqual(TEXT("A negative scale is none"), MakerSoundFlavor::Scale(), 0.f);
		}
	}
	return true;
}


// ---------------------------------------------------------------------------
// Gun identity (2026-10-28, Docs/Work/2026-10-28-Core/GUNS.md lane C): each kind's layer balance, the body punch, the
// sights' foley. Looter.Gunfeel.Identity 0 is today's sound.
// ---------------------------------------------------------------------------

namespace
{
	const EWeaponKind AllKinds[] = { EWeaponKind::Rifle, EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::SMG,
		EWeaponKind::Sniper, EWeaponKind::LMG, EWeaponKind::Launcher };

	const TCHAR* KindName(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return TEXT("Shotgun");
		case EWeaponKind::Revolver: return TEXT("Revolver");
		case EWeaponKind::SMG: return TEXT("SMG");
		case EWeaponKind::Sniper: return TEXT("Sniper");
		case EWeaponKind::LMG: return TEXT("LMG");
		case EWeaponKind::Launcher: return TEXT("Launcher");
		default: return TEXT("Rifle");
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundKindLayersTest, "Looter.Gunfeel.Sound.KindLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundKindLayersTest::RunTest(const FString& Parameters)
{
	// The variables exist for every kind.
	for (const EWeaponKind Kind : AllKinds)
	{
		for (const TCHAR* Layer : { TEXT("Mech"), TEXT("Low"), TEXT("Tail") })
		{
			const FString Name = FString::Printf(TEXT("Looter.Gunfeel.Sound.%s.%s"), KindName(Kind), Layer);
			TestNotNull(*FString::Printf(TEXT("%s exists"), *Name), IConsoleManager::Get().FindConsoleVariable(*Name));
		}
	}
	// The shipped balance: the rifle is the reference; the SMG buzzes (action up, thump and tail down); the heavy guns
	// carry the chest and the valley.
	const GunfeelSound::FKindMix Rifle = Def::KindMix(EWeaponKind::Rifle);
	TestTrue(TEXT("Rifle: 1/1/1"), Rifle.Mech == 1.f && Rifle.Low == 1.f && Rifle.Tail == 1.f);
	const GunfeelSound::FKindMix SMG = Def::KindMix(EWeaponKind::SMG);
	TestTrue(TEXT("SMG: action up, thump and tail down"), SMG.Mech > 1.f && SMG.Low < 1.f && SMG.Tail < 1.f);
	TestTrue(TEXT("LMG: more chest"), Def::KindMix(EWeaponKind::LMG).Low > 1.f);
	TestTrue(TEXT("Revolver: a longer tail, a quieter action"),
		Def::KindMix(EWeaponKind::Revolver).Tail > 1.f && Def::KindMix(EWeaponKind::Revolver).Mech < 1.f);
	TestTrue(TEXT("Shotgun: the biggest thump"), Def::KindMix(EWeaponKind::Shotgun).Low >= Def::KindMix(EWeaponKind::Sniper).Low
		&& Def::KindMix(EWeaponKind::Shotgun).Low >= Def::KindMix(EWeaponKind::LMG).Low);
	TestTrue(TEXT("Sniper: the longest tail"), Def::KindMix(EWeaponKind::Sniper).Tail > Def::KindMix(EWeaponKind::Revolver).Tail
		&& Def::KindMix(EWeaponKind::Sniper).Tail > Def::KindMix(EWeaponKind::Shotgun).Tail);
	TestTrue(TEXT("Launcher: more chest, the rest plain"),
		Def::KindMix(EWeaponKind::Launcher).Low > 1.f && Def::KindMix(EWeaponKind::Launcher).Mech == 1.f);
	for (const EWeaponKind Kind : AllKinds)
	{
		const GunfeelSound::FKindMix Mix = Def::KindMix(Kind);
		TestTrue(*FString::Printf(TEXT("%s: no layer moved more than half either way"), KindName(Kind)),
			Mix.Mech >= 0.5f && Mix.Mech <= 1.5f && Mix.Low >= 0.5f && Mix.Low <= 1.5f && Mix.Tail >= 0.5f && Mix.Tail <= 1.5f);
	}

	// Read live (Identity on): a changed variable is heard at once, never under silence.
	if (IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Gunfeel.Identity")))
	{
		FScopedSoundVariable Identity(TEXT("Looter.Gunfeel.Identity"), 1.f);
		TestEqual(TEXT("The live SMG mech starts at the shipped one"), GunfeelSound::KindMix(EWeaponKind::SMG).Mech, SMG.Mech);
		FScopedSoundVariable Low(TEXT("Looter.Gunfeel.Sound.SMG.Low"), 0.4f);
		TestEqual(TEXT("A changed layer is read at once"), GunfeelSound::KindMix(EWeaponKind::SMG).Low, 0.4f);
		FScopedSoundVariable Tail(TEXT("Looter.Gunfeel.Sound.SMG.Tail"), -1.f);
		TestEqual(TEXT("...and never goes under silence"), GunfeelSound::KindMix(EWeaponKind::SMG).Tail, 0.f);
		TestEqual(TEXT("...and only for its kind"), GunfeelSound::KindMix(EWeaponKind::Rifle).Low, 1.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundPunchTest, "Looter.Gunfeel.Sound.Punch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundPunchTest::RunTest(const FString& Parameters)
{
	// Every Nth shot of a burst, the first always (by the shipped numbers).
	TestEqual(TEXT("Rifle: every 3rd"), Def::PunchEvery(EWeaponKind::Rifle), 3);
	TestEqual(TEXT("SMG: none"), Def::PunchEvery(EWeaponKind::SMG), 0);
	TestEqual(TEXT("LMG: every 2nd"), Def::PunchEvery(EWeaponKind::LMG), 2);
	for (const EWeaponKind Kind : { EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::Sniper, EWeaponKind::Launcher })
	{
		TestEqual(TEXT("A single-shot kind punches every shot"), Def::PunchEvery(Kind), 1);
	}
	auto Count = [](int32 Every)
	{
		int32 Plays = 0;
		for (int32 Shot = 0; Shot < 12; ++Shot)
		{
			Plays += GunfeelSound::PunchPlays(Shot, Every) ? 1 : 0;
		}
		return Plays;
	};
	TestEqual(TEXT("Rifle: 4 punches in 12 shots"), Count(Def::PunchEvery(EWeaponKind::Rifle)), 4);
	TestEqual(TEXT("LMG: 6 in 12"), Count(Def::PunchEvery(EWeaponKind::LMG)), 6);
	TestEqual(TEXT("SMG: none in 12"), Count(Def::PunchEvery(EWeaponKind::SMG)), 0);
	TestEqual(TEXT("A single shot kind: every one"), Count(1), 12);
	TestTrue(TEXT("The first shot of a burst punches"), GunfeelSound::PunchPlays(0, 3));
	TestFalse(TEXT("...the second of an every-3rd doesn't"), GunfeelSound::PunchPlays(1, 3));
	TestTrue(TEXT("...the fourth does"), GunfeelSound::PunchPlays(3, 3));
	TestFalse(TEXT("Never for 0"), GunfeelSound::PunchPlays(0, 0));
	TestFalse(TEXT("...or a negative count"), GunfeelSound::PunchPlays(0, -2));

	// The shooter's own gun only (a thump in the chest, 2D), and only above silence.
	TestTrue(TEXT("Own gun, audible"), GunfeelSound::PunchAudible(true, 1.f));
	TestFalse(TEXT("Someone else's gun: none"), GunfeelSound::PunchAudible(false, 1.f));
	TestFalse(TEXT("Silence: none"), GunfeelSound::PunchAudible(true, 0.f));

	// The rifle's sits at -4 dB; the others at full.
	TestTrue(TEXT("Rifle punch at -4 dB"), FMath::Abs(Def::PunchGain(EWeaponKind::Rifle) - FMath::Pow(10.f, -4.f / 20.f)) < 0.01f);
	for (const EWeaponKind Kind : { EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::SMG, EWeaponKind::Sniper, EWeaponKind::LMG, EWeaponKind::Launcher })
	{
		TestEqual(*FString::Printf(TEXT("%s: full punch"), KindName(Kind)), Def::PunchGain(Kind), 1.f);
	}

	// The variables and the cues.
	for (const EWeaponKind Kind : AllKinds)
	{
		for (const TCHAR* Which : { TEXT("PunchEvery"), TEXT("PunchGain") })
		{
			const FString Name = FString::Printf(TEXT("Looter.Gunfeel.Sound.%s.%s"), Which, KindName(Kind));
			TestNotNull(*FString::Printf(TEXT("%s exists"), *Name), IConsoleManager::Get().FindConsoleVariable(*Name));
		}
		TestEqual(*FString::Printf(TEXT("%s: the punch's cue"), KindName(Kind)), GunshotSound::CueFor(Kind, EGunshotLayer::Punch),
			FName(*FString::Printf(TEXT("Weapon.%s.Punch"), KindName(Kind))));
	}
	TestNotNull(TEXT("The global punch volume exists"), IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Gunfeel.Sound.Punch")));
	namespace Shot = LooterSoundCue::Gunshot;
	TestEqual(TEXT("Rifle cue constant"), FName(Shot::RiflePunch), GunshotSound::CueFor(EWeaponKind::Rifle, EGunshotLayer::Punch));
	TestEqual(TEXT("Revolver cue constant"), FName(Shot::RevolverPunch), GunshotSound::CueFor(EWeaponKind::Revolver, EGunshotLayer::Punch));
	TestEqual(TEXT("Launcher cue constant"), FName(Shot::LauncherPunch), GunshotSound::CueFor(EWeaponKind::Launcher, EGunshotLayer::Punch));

	// Live volume (Identity on): the global one times the kind's.
	if (IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Gunfeel.Identity")))
	{
		FScopedSoundVariable Identity(TEXT("Looter.Gunfeel.Identity"), 1.f);
		TestTrue(TEXT("Rifle: live volume is the -4 dB one"), FMath::Abs(GunfeelSound::PunchVolume(EWeaponKind::Rifle) - Def::PunchRifleGain) < 0.001f);
		TestEqual(TEXT("Shotgun: full"), GunfeelSound::PunchVolume(EWeaponKind::Shotgun), 1.f);
		{
			FScopedSoundVariable Half(TEXT("Looter.Gunfeel.Sound.Punch"), 0.5f);
			TestEqual(TEXT("The global volume scales every kind"), GunfeelSound::PunchVolume(EWeaponKind::Shotgun), 0.5f);
		}
		FScopedSoundVariable Never(TEXT("Looter.Gunfeel.Sound.PunchEvery.Rifle"), -4.f);
		TestEqual(TEXT("A negative spacing is none"), GunfeelSound::PunchEvery(EWeaponKind::Rifle), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundAimInTest, "Looter.Gunfeel.Sound.AimIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundAimInTest::RunTest(const FString& Parameters)
{
	TSet<FName> Seen;
	for (const EWeaponKind Kind : AllKinds)
	{
		const FName Cue = WeaponSounds::AimIn(Kind);
		TestEqual(*FString::Printf(TEXT("%s: its own sights"), KindName(Kind)), Cue, FName(*FString::Printf(TEXT("Weapon.%s.AimIn"), KindName(Kind))));
		Seen.Add(Cue);
		// A bank without the kind's cue falls back to the general one.
		TestEqual(*FString::Printf(TEXT("%s: falls back to the general sound"), KindName(Kind)), WeaponSounds::AimIn(Kind, false), FName(LooterSoundCue::AimIn));
	}
	TestEqual(TEXT("Seven kinds, seven cues"), Seen.Num(), 7);
	TestEqual(TEXT("The sights lowering"), WeaponSounds::AimOut(), FName(TEXT("Weapon.AimOut")));
	TestEqual(TEXT("...is the constant"), FName(LooterSoundCue::AimOut), WeaponSounds::AimOut());
	TestEqual(TEXT("The general rising sound is unchanged"), FName(LooterSoundCue::AimIn), FName(TEXT("Weapon.AimIn")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSoundSwitchTest, "Looter.Gunfeel.Sound.Switch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSoundSwitchTest::RunTest(const FString& Parameters)
{
	// Looter.Gunfeel.Identity 0 is today's sound: plain balance, no punch, the general sights, no lowering sound.
	if (!TestNotNull(TEXT("The master switch exists"), IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Gunfeel.Identity"))))
	{
		return false;
	}
	FScopedSoundVariable Off(TEXT("Looter.Gunfeel.Identity"), 0.f);
	for (const EWeaponKind Kind : AllKinds)
	{
		const GunfeelSound::FKindMix Mix = GunfeelSound::KindMix(Kind);
		TestTrue(*FString::Printf(TEXT("%s: plain balance"), KindName(Kind)), Mix.Mech == 1.f && Mix.Low == 1.f && Mix.Tail == 1.f);
		TestEqual(*FString::Printf(TEXT("%s: no punch"), KindName(Kind)), GunfeelSound::PunchVolume(Kind), 0.f);
		TestEqual(*FString::Printf(TEXT("%s: the general sights"), KindName(Kind)), GunshotSound::AimInCue(nullptr, Kind), FName(LooterSoundCue::AimIn));
	}
	TestTrue(TEXT("No lowering sound"), GunshotSound::AimOutCue(nullptr).IsNone());
	return true;
}

#endif
