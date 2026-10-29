#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/CombatRules.h"
#include "Player/ViewKick.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/WeaponRecoil.h"
#include "Weapons/WeaponTypes.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

// The kick side of the gunplay pass (Docs/Plan.md 12.1, GUNFEEL-KICK): each rule driven with explicit numbers or the
// shipped defaults (GunfeelKick::Default), never a value the player may have set in play. The variables are only checked
// to exist and to be read live, and put back as they were.

namespace
{
	namespace Def = GunfeelKick::Default;

	/** A console variable, set to Value for the scope and put back after. */
	struct FScopedVariable
	{
		explicit FScopedVariable(const TCHAR* Name, float Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Was = Variable->GetFloat();
				Variable->Set(*FString::SanitizeFloat(Value), ECVF_SetByConsole);
			}
		}
		~FScopedVariable()
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

// ---------------------------------------------------------------------------
// The variables
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickVariablesTest, "Looter.Gunfeel.Kick.Variables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickVariablesTest::RunTest(const FString& Parameters)
{
	// Every number that affects feel is a Looter.Gunfeel.* variable (the contract's names).
	const TCHAR* Names[] = {
		TEXT("Kick.Rifle"), TEXT("Kick.Shotgun"), TEXT("Kick.Revolver"), TEXT("Recoil.Rifle"), TEXT("Recoil.Shotgun"),
		TEXT("Recoil.Revolver"), TEXT("Recoil.ShotgunSettle"), TEXT("Muzzle.Scale1P"), TEXT("Rumble.Scale"), TEXT("Rumble.Rifle"),
		TEXT("Rumble.RifleSeconds"), TEXT("Rumble.Shotgun"), TEXT("Rumble.ShotgunSeconds"), TEXT("Rumble.Revolver"),
		TEXT("Rumble.RevolverSeconds"), TEXT("Rumble.Hurt"), TEXT("LowAmmo.Share"), TEXT("LowAmmo.PitchTop"), TEXT("Recoil.RifleClimb"),
		TEXT("Recoil.RifleClimbMax"), TEXT("Spread.RifleBloom"), TEXT("Spread.BloomMax"), TEXT("Spread.BloomRecovery"),
		TEXT("Shotgun.FalloffStart"), TEXT("Shotgun.FalloffEnd"), TEXT("Shotgun.FalloffFloor"), TEXT("Crit.Revolver"),
		TEXT("Aim.Time.Rifle"), TEXT("Aim.Time.Shotgun"), TEXT("Aim.Time.Revolver"), TEXT("Aim.Move.Rifle"), TEXT("Aim.Move.Shotgun"),
		TEXT("Aim.Move.Revolver"), TEXT("Swap.Rifle"), TEXT("Swap.Shotgun"), TEXT("Swap.Revolver"),
		// The gun feel pass (2026-10-22): the sight, the pattern, the first shot, the sustained cap, the rack.
		TEXT("Recoil.AimedFlip"), TEXT("Recoil.AimedRoll"), TEXT("Recoil.AimedBack"), TEXT("Kick.FirstShot"), TEXT("Spread.FirstShot"),
		TEXT("Recoil.Pattern"), TEXT("Recoil.SideRandom"), TEXT("Kick.SustainCap"), TEXT("Kick.SustainFloor"), TEXT("Kick.Rack"),
		TEXT("Recoil.RifleDrift"), TEXT("Recoil.RifleDriftFrom"), TEXT("Recoil.SMGShimmy"), TEXT("Recoil.LMGSway"),
		TEXT("Recoil.LMGSwayShots"), TEXT("Spread.FirstShotRest"), TEXT("Spread.FirstShotEase") };
	for (const TCHAR* Name : Names)
	{
		const FString Full = FString(TEXT("Looter.Gunfeel.")) + Name;
		TestNotNull(*FString::Printf(TEXT("%s exists"), *Full), IConsoleManager::Get().FindConsoleVariable(*Full));
	}

	// They are read each time they are asked, so a change in play shows on the next shot.
	{
		FScopedVariable Kick(TEXT("Looter.Gunfeel.Kick.Shotgun"), 2.5f);
		TestEqual(TEXT("A changed kick is read at once"), GunfeelKick::KickScale(EWeaponKind::Shotgun), 2.5f);
		TestFalse(TEXT("...for its own kind only"), FMath::IsNearlyEqual(GunfeelKick::KickScale(EWeaponKind::Revolver), 2.5f));
	}
	{
		FScopedVariable Swap(TEXT("Looter.Gunfeel.Swap.Revolver"), 0.5f);
		TestEqual(TEXT("A changed swap time is read at once"), GunfeelKick::SwapScale(EWeaponKind::Revolver), 0.5f);
	}
	{
		FScopedVariable Aim(TEXT("Looter.Gunfeel.Aim.Time.Rifle"), 1.7f);
		TestEqual(TEXT("A gun with no kind of its own is the rifle"), GunfeelKick::AimTimeScale(EWeaponKind::None), 1.7f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickIdentityTest, "Looter.Gunfeel.Kick.Identity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickIdentityTest::RunTest(const FString& Parameters)
{
	// The shipped numbers give each gun its character: the shotgun shoves hardest, the six-gun is quickest on its feet and
	// to the eye, the shotgun slowest. Since gun identity (2026-10-28) the view kick's per-kind weight is ViewKicks::ForShot's
	// own table, so every Kick.<Kind> starts at 1; Identity 0's old scales (Default::Today) keep today's order.
	namespace Today = Def::Today;
	for (const float Scale : { Def::KickRifle, Def::KickShotgun, Def::KickRevolver, Def::KickSMG, Def::KickSniper, Def::KickLMG, Def::KickLauncher })
	{
		TestEqual(TEXT("Every kind's view kick scale starts at 1 (one table to read)"), Scale, 1.f);
	}
	TestTrue(TEXT("The shotgun shoves, the rifle ticks"), Today::KickShotgun >= Today::KickRevolver && Today::KickRevolver < Today::KickRifle + 0.5f
		&& Today::KickRifle > 1.f && Def::RecoilShotgun > Def::RecoilRifle);
	TestTrue(TEXT("The six-gun comes up quickest, the shotgun slowest"), Def::AimTimeRevolver < Def::AimTimeRifle && Def::AimTimeRifle < Def::AimTimeShotgun);
	TestTrue(TEXT("...is swapped quickest, the shotgun slowest"), Def::SwapRevolver < Def::SwapRifle && Def::SwapRifle < Def::SwapShotgun);
	TestTrue(TEXT("...and walks fastest aimed, the shotgun slowest"), Def::AimMoveRevolver > Def::AimMoveRifle && Def::AimMoveRifle > Def::AimMoveShotgun);
	TestTrue(TEXT("The shotgun settles slower"), Def::ShotgunSettle < 1.f);
	TestTrue(TEXT("The shotgun rumbles hardest and longest; the rifle is a tick"),
		Def::RumbleShotgun > Def::RumbleRevolver && Def::RumbleRevolver > Def::RumbleRifle
		&& Def::RumbleShotgunSeconds > Def::RumbleRevolverSeconds && Def::RumbleRevolverSeconds > Def::RumbleRifleSeconds);
	TestEqual(TEXT("The user's call: the revolver's crit is 1.75x"), Def::CritRevolver, 1.75f);
	// The gun feel pass spread the kinds apart (2026-10-22): heaviest first, the SMG a hum (Identity 0's scales).
	TestTrue(TEXT("Today's view kick runs launcher/sniper, shotgun, revolver, rifle, LMG, SMG"),
		Today::KickLauncher >= Today::KickSniper && Today::KickSniper > Today::KickShotgun && Today::KickShotgun > Today::KickRevolver
		&& Today::KickRevolver > Today::KickRifle && Today::KickRifle > Today::KickLMG && Today::KickLMG > Today::KickSMG);
	// Gun identity's table at a base gun: the light guns low, the heavy three up high but apart (Looter.Gunfeel.Identity.*).
	FRandomStream Random(2);
	auto Hip = [&Random](EWeaponKind Kind) { return ViewKicks::ForShot(Kind, ViewKicks::BaseShotRecoil, 0.f, Random).Pitch; };
	TestTrue(TEXT("Identity's lift runs shotgun, sniper, revolver, launcher, LMG, rifle, SMG"),
		Hip(EWeaponKind::Shotgun) > Hip(EWeaponKind::Sniper) && Hip(EWeaponKind::Sniper) > Hip(EWeaponKind::Revolver)
		&& Hip(EWeaponKind::Revolver) > Hip(EWeaponKind::Launcher) && Hip(EWeaponKind::Launcher) > Hip(EWeaponKind::LMG)
		&& Hip(EWeaponKind::LMG) > Hip(EWeaponKind::Rifle) && Hip(EWeaponKind::Rifle) > Hip(EWeaponKind::SMG));
	return true;
}

// ---------------------------------------------------------------------------
// View kick by kind
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickViewTest, "Looter.Gunfeel.Kick.ViewKick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickViewTest::RunTest(const FString& Parameters)
{
	// A shot's view kick times the kind's weight (UPlayerViewComponent::AddShotKick), at a base Common (Recoil 1.4): the rifle a
	// crack, the shotgun the biggest shove, and a burst of any of them still stays within the view's limits (FViewKickStack
	// holds the sum).
	FRandomStream Random(5);
	const float Base = ViewKicks::BaseShotRecoil;
	const FViewKick Rifle = ViewKicks::ForShot(EWeaponKind::Rifle, Base, 0.f, Random) * Def::KickRifle;
	const FViewKick Shotgun = ViewKicks::ForShot(EWeaponKind::Shotgun, Base, 0.f, Random) * Def::KickShotgun;
	const FViewKick Revolver = ViewKicks::ForShot(EWeaponKind::Revolver, Base, 0.f, Random) * Def::KickRevolver;
	TestTrue(FString::Printf(TEXT("The rifle's crack (%.2f degrees)"), Rifle.Pitch), Rifle.Pitch > 0.4f && Rifle.Pitch < 1.f);
	TestTrue(FString::Printf(TEXT("The shotgun's shove is the most (%.2f degrees)"), Shotgun.Pitch), Shotgun.Pitch > Revolver.Pitch && Shotgun.Pitch > 1.5f);
	TestTrue(TEXT("...and widens the view the most"), Shotgun.FieldOfView > Revolver.FieldOfView && Shotgun.FieldOfView > Rifle.FieldOfView);
	TestTrue(TEXT("...and pushes the camera back"), Shotgun.Push > 0.f && Rifle.Push == 0.f && Revolver.Push == 0.f);
	TestTrue(TEXT("A single kick stays within the view's limits"), Shotgun.Pitch <= FViewKickStack::MaxTurn && Shotgun.FieldOfView <= FViewKickStack::MaxFieldOfView
		&& Shotgun.Push <= FViewKickStack::MaxPush);

	// Identity 0's kick is today's: the old table times the old scale (a rifle 0.32 x 1.3 at Recoil 1).
	const FViewKick OldRifle = ViewKicks::ForShotToday(EWeaponKind::Rifle, 1.f, 0.f, Random) * Def::Today::KickRifle;
	TestEqual(TEXT("Today's rifle tick"), OldRifle.Pitch, 0.32f * 1.3f, 0.0001f);

	FViewKickStack Burst;
	for (int32 Shot = 0; Shot < 20; ++Shot)
	{
		Burst.Add(ViewKicks::ForShot(EWeaponKind::Shotgun, 2.f, 0.f, Random) * Def::KickShotgun);
		Burst.Tick(0.016f);
		TestTrue(TEXT("A heavy burst never turns the view past the limit"), FMath::Abs(Burst.GetRotation().Pitch) <= FViewKickStack::MaxTurn + KINDA_SMALL_NUMBER);
		TestTrue(TEXT("...or widens it past the limit"), FMath::Abs(Burst.GetFieldOfView()) <= FViewKickStack::MaxFieldOfView + KINDA_SMALL_NUMBER);
		TestTrue(TEXT("...or pushes the camera past the limit"), FMath::Abs(Burst.GetPush()) <= FViewKickStack::MaxPush + KINDA_SMALL_NUMBER);
	}
	return true;
}

// ---------------------------------------------------------------------------
// The rifle's climb and bloom
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickClimbTest, "Looter.Gunfeel.Kick.RifleClimb",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickClimbTest::RunTest(const FString& Parameters)
{
	FWeaponRecoilProfile Profile;
	Profile.AimKick = 0.5f;
	Profile.AimKickSide = 0.f;
	Profile.MuzzleTwist = 0.f;
	Profile.Roll = 0.f;
	Profile.ClimbPerShot = Def::RifleClimb;
	Profile.ClimbMax = Def::RifleClimbMax;

	// The first shot of a burst kicks as the profile says; each next one a little more, up to the most; a pause starts over.
	FRandomStream Random(9);
	FWeaponRecoil Recoil;
	TestEqual(TEXT("The first shot: no climb"), Recoil.GetClimb(Profile), 1.f);
	for (int32 Shot = 0; Shot < 5; ++Shot)
	{
		Recoil.AddShot(Profile, Random);
		Recoil.Tick(0.1f);
	}
	TestEqual(TEXT("After five shots: 30% more"), Recoil.GetClimb(Profile), 1.f + 5.f * Def::RifleClimb, 0.0001f);
	for (int32 Shot = 0; Shot < 30; ++Shot)
	{
		Recoil.AddShot(Profile, Random);
		Recoil.Tick(0.1f);
	}
	TestEqual(TEXT("A long burst: held to the most"), Recoil.GetClimb(Profile), Def::RifleClimbMax, 0.0001f);
	for (int32 Pause = 0; Pause < 5; ++Pause)
	{
		Recoil.Tick(0.1f);
	}
	Recoil.AddShot(Profile, Random);
	TestEqual(TEXT("After a pause the burst starts over"), Recoil.GetClimb(Profile), 1.f + Def::RifleClimb, 0.0001f);

	// The aim really climbs further than a gun that doesn't (the same shots from the same random stream).
	FWeaponRecoilProfile Flat = Profile;
	Flat.ClimbPerShot = 0.f;
	Flat.ClimbMax = 1.f;
	float Climbing = 0.f;
	float Steady = 0.f;
	FWeaponRecoil ClimbingRecoil;
	FWeaponRecoil SteadyRecoil;
	FRandomStream RandomA(21);
	FRandomStream RandomB(21);
	for (int32 Shot = 0; Shot < 14; ++Shot)
	{
		ClimbingRecoil.AddShot(Profile, RandomA);
		SteadyRecoil.AddShot(Flat, RandomB);
		for (int32 Step = 0; Step < 10; ++Step)
		{
			Climbing += ClimbingRecoil.Tick(0.01f).Pitch;
			Steady += SteadyRecoil.Tick(0.01f).Pitch;
		}
	}
	TestTrue(FString::Printf(TEXT("A climbing burst ends higher (%.2f against %.2f degrees)"), Climbing, Steady), Climbing > Steady * 1.15f);

	// A profile that doesn't climb (the shotgun's, the six-gun's, an old one) is unchanged by the rule.
	FWeaponRecoil Plain;
	for (int32 Shot = 0; Shot < 10; ++Shot)
	{
		Plain.AddShot(Flat, Random);
		Plain.Tick(0.05f);
	}
	TestEqual(TEXT("No climb, no growth"), Plain.GetClimb(Flat), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickBloomTest, "Looter.Gunfeel.Kick.RifleBloom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickBloomTest::RunTest(const FString& Parameters)
{
	// Each hip shot opens the rifle's cone by a share of its own; through the sight a quarter as much; held to the most.
	const float One = GunfeelKick::BloomAfterShot(EWeaponKind::Rifle, 0.f, 0.f, Def::RifleBloom, Def::BloomMax);
	TestEqual(TEXT("One hip shot opens it 15%"), One, Def::RifleBloom, 0.0001f);
	TestEqual(TEXT("Through the sight: a quarter as much"), GunfeelKick::BloomAfterShot(EWeaponKind::Rifle, 0.f, 1.f, Def::RifleBloom, Def::BloomMax), Def::RifleBloom * 0.25f, 0.0001f);
	TestTrue(TEXT("Half way up the sight: in between"), GunfeelKick::BloomAfterShot(EWeaponKind::Rifle, 0.f, 0.5f, Def::RifleBloom, Def::BloomMax) < One);
	float Bloom = 0.f;
	for (int32 Shot = 0; Shot < 40; ++Shot)
	{
		Bloom = GunfeelKick::BloomAfterShot(EWeaponKind::Rifle, Bloom, 0.f, Def::RifleBloom, Def::BloomMax);
	}
	TestEqual(TEXT("A long burst: twice its spread at the most"), 1.f + Bloom, Def::BloomMax, 0.0001f);
	TestEqual(TEXT("The shotgun doesn't bloom"), GunfeelKick::BloomAfterShot(EWeaponKind::Shotgun, 0.2f, 0.f, Def::RifleBloom, Def::BloomMax), 0.2f);
	TestEqual(TEXT("...nor does the six-gun"), GunfeelKick::BloomAfterShot(EWeaponKind::Revolver, 0.2f, 0.f, Def::RifleBloom, Def::BloomMax), 0.2f);

	// And closes once firing stops.
	TestEqual(TEXT("No time, no change"), GunfeelKick::BloomRecovered(1.f, 0.f, Def::BloomRecovery), 1.f);
	const float Later = GunfeelKick::BloomRecovered(1.f, 0.5f, Def::BloomRecovery);
	TestTrue(FString::Printf(TEXT("Half a second later it is mostly closed (%.2f)"), Later), Later > 0.1f && Later < 0.3f);
	TestTrue(TEXT("...a second later all but gone"), GunfeelKick::BloomRecovered(1.f, 1.f, Def::BloomRecovery) < 0.06f);
	TestEqual(TEXT("A negative time never opens it"), GunfeelKick::BloomRecovered(0.5f, -3.f, Def::BloomRecovery), 0.5f);
	return true;
}

// ---------------------------------------------------------------------------
// The shotgun's falloff and the revolver's crit
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickFalloffTest, "Looter.Gunfeel.Kick.ShotgunFalloff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickFalloffTest::RunTest(const FString& Parameters)
{
	const float Range = 1000.f;
	auto Falloff = [Range](float Distance)
	{
		return GunfeelKick::ShotgunFalloff(Distance, Range, Def::ShotgunFalloffStart, Def::ShotgunFalloffEnd, Def::ShotgunFalloffFloor);
	};
	TestEqual(TEXT("Full damage out to 60% of its range"), Falloff(600.f), 1.f);
	TestEqual(TEXT("...and point blank"), Falloff(0.f), 1.f);
	TestEqual(TEXT("A quarter of it at 1.6 times the range"), Falloff(1600.f), Def::ShotgunFalloffFloor, 0.0001f);
	TestEqual(TEXT("...and from there on"), Falloff(5000.f), Def::ShotgunFalloffFloor, 0.0001f);
	TestEqual(TEXT("Half way between: half way down"), Falloff(1100.f), (1.f + Def::ShotgunFalloffFloor) * 0.5f, 0.0001f);
	float Last = 2.f;
	for (float Distance = 0.f; Distance <= 2000.f; Distance += 50.f)
	{
		TestTrue(TEXT("Never gains damage with distance"), Falloff(Distance) <= Last + KINDA_SMALL_NUMBER);
		Last = Falloff(Distance);
	}

	// Today's falloff (every other gun's): full to the range, down to half at twice it. The shotgun's is sharper at its range.
	FWeaponStats Stats;
	Stats.Range = Range;
	TestEqual(TEXT("A rifle's rule is today's: full at its range"), GunfeelKick::DamageShareAtDistance(EWeaponKind::Rifle, Stats, Range), 1.f);
	TestEqual(TEXT("...half at twice it"), GunfeelKick::DamageShareAtDistance(EWeaponKind::Rifle, Stats, Range * 2.f), FWeaponStats::RangeFalloffFloor, 0.0001f);
	TestEqual(TEXT("...the six-gun's the same"), GunfeelKick::DamageShareAtDistance(EWeaponKind::Revolver, Stats, Range * 1.5f), Stats.DamageAtDistance(Range * 1.5f), 0.0001f);
	TestTrue(TEXT("A shotgun's pellet at its range is already weaker than a rifle's"), Falloff(Range) < Stats.DamageAtDistance(Range));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickCritTest, "Looter.Gunfeel.Kick.RevolverCrit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickCritTest::RunTest(const FString& Parameters)
{
	using namespace GunfeelKick;
	TestEqual(TEXT("A revolver's crit is 1.75x"), CritMultiplier(EWeaponKind::Revolver, LooterCombat::CriticalHitMultiplier, Def::CritRevolver), 1.75f);
	TestEqual(TEXT("The rifle keeps the game's 1.5x"), CritMultiplier(EWeaponKind::Rifle, LooterCombat::CriticalHitMultiplier, Def::CritRevolver), 1.5f);
	TestEqual(TEXT("...and the shotgun"), CritMultiplier(EWeaponKind::Shotgun, LooterCombat::CriticalHitMultiplier, Def::CritRevolver), 1.5f);
	TestEqual(TEXT("A cursed iron's triple wins on a revolver"), CritMultiplier(EWeaponKind::Revolver, 3.f, Def::CritRevolver), 3.f);
	TestEqual(TEXT("...and Cold's extra quarter (1.875) over the 1.75x"), CritMultiplier(EWeaponKind::Revolver, LooterCombat::CriticalHitMultiplier * 1.25f, Def::CritRevolver), 1.875f);
	TestEqual(TEXT("A rifle with Cold keeps its own"), CritMultiplier(EWeaponKind::Rifle, LooterCombat::CriticalHitMultiplier * 1.25f, Def::CritRevolver), 1.875f);

	// What it deals: 100 at the roll's middle is 175 on a revolver crit, 150 on a rifle's.
	TestEqual(TEXT("A revolver crit deals 175"), LooterCombat::HitDamage(100.f, true, 0.5f, CritMultiplier(EWeaponKind::Revolver, 1.5f, Def::CritRevolver)), 175.f, 0.01f);
	TestEqual(TEXT("A rifle crit deals 150"), LooterCombat::HitDamage(100.f, true, 0.5f, CritMultiplier(EWeaponKind::Rifle, 1.5f, Def::CritRevolver)), 150.f, 0.01f);
	TestEqual(TEXT("No crit, no multiplier"), LooterCombat::HitDamage(100.f, false, 0.5f, CritMultiplier(EWeaponKind::Revolver, 1.5f, Def::CritRevolver)), 100.f, 0.01f);
	return true;
}

// ---------------------------------------------------------------------------
// The low-ammo click
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickLowAmmoTest, "Looter.Gunfeel.Kick.LowAmmo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickLowAmmoTest::RunTest(const FString& Parameters)
{
	auto Pitch = [](int32 Left, int32 Magazine)
	{
		return GunfeelKick::LowAmmoPitch(Left, Magazine, Def::LowAmmoShare, Def::LowAmmoPitchTop);
	};
	// A 30-round magazine clicks for its last 7 rounds (a quarter, rounded down): the first at pitch 1, the last at the top.
	TestEqual(TEXT("Plenty left: no click"), Pitch(8, 30), 0.f);
	TestEqual(TEXT("The first click of the last quarter: pitch 1"), Pitch(7, 30), 1.f, 0.0001f);
	TestEqual(TEXT("The last round: the top pitch"), Pitch(1, 30), Def::LowAmmoPitchTop, 0.0001f);
	TestEqual(TEXT("Empty (the reload's own): no click"), Pitch(0, 30), 0.f);
	float Last = 0.f;
	for (int32 Left = 7; Left >= 1; --Left)
	{
		TestTrue(FString::Printf(TEXT("%d left rises (%.3f)"), Left, Pitch(Left, 30)), Pitch(Left, 30) >= Last);
		Last = Pitch(Left, 30);
	}
	// Small magazines: a six-gun's last round, a shotgun's last two; under four rounds never.
	TestEqual(TEXT("A six-gun's last round clicks at the top"), Pitch(1, 6), Def::LowAmmoPitchTop, 0.0001f);
	TestEqual(TEXT("...the one before it doesn't"), Pitch(2, 6), 0.f);
	TestEqual(TEXT("A shotgun of eight: two rounds left clicks at 1"), Pitch(2, 8), 1.f, 0.0001f);
	TestEqual(TEXT("...one left at the top"), Pitch(1, 8), Def::LowAmmoPitchTop, 0.0001f);
	TestEqual(TEXT("A magazine of three never clicks"), Pitch(1, 3), 0.f);
	TestEqual(TEXT("...nor does a single shot"), Pitch(1, 1), 0.f);
	TestEqual(TEXT("More left than the magazine holds (a bad number): no click"), Pitch(40, 30), 0.f);
	return true;
}

// ---------------------------------------------------------------------------
// Rumble
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickRumbleTest, "Looter.Gunfeel.Kick.Rumble",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickRumbleTest::RunTest(const FString& Parameters)
{
	// Strength is the base times the player's setting times the global scale, held to 0-1.
	TestEqual(TEXT("At 100% the base shows"), GunfeelKick::RumbleStrength(Def::RumbleShotgun, 1.f, Def::RumbleScale), Def::RumbleShotgun, 0.0001f);
	TestEqual(TEXT("At 50% half"), GunfeelKick::RumbleStrength(0.8f, 0.5f, 1.f), 0.4f, 0.0001f);
	TestEqual(TEXT("At 0% nothing"), GunfeelKick::RumbleStrength(0.8f, 0.f, 1.f), 0.f);
	TestEqual(TEXT("The scale on top"), GunfeelKick::RumbleStrength(0.4f, 1.f, 1.5f), 0.6f, 0.0001f);
	TestEqual(TEXT("Never past full"), GunfeelKick::RumbleStrength(0.9f, 1.f, 3.f), 1.f);
	TestEqual(TEXT("A setting past 100% (a bad save) is held to it"), GunfeelKick::RumbleStrength(0.5f, 4.f, 1.f), 0.5f);
	TestEqual(TEXT("A negative scale is nothing"), GunfeelKick::RumbleStrength(0.5f, 1.f, -1.f), 0.f);

	// A hit taken: a quarter of the bar or more is the full rumble, a scratch a quarter of it, nothing for no damage.
	const GunfeelKick::FRumble Full = GunfeelKick::RumbleForHurt(0.25f, Def::RumbleHurt);
	TestEqual(TEXT("A quarter of the bar: the full hurt rumble"), Full.Intensity, Def::RumbleHurt, 0.0001f);
	TestEqual(TEXT("...past a quarter is no more"), GunfeelKick::RumbleForHurt(0.9f, Def::RumbleHurt).Intensity, Def::RumbleHurt, 0.0001f);
	const GunfeelKick::FRumble Scratch = GunfeelKick::RumbleForHurt(0.01f, Def::RumbleHurt);
	TestTrue(TEXT("A scratch is smaller and shorter"), Scratch.Intensity < Full.Intensity && Scratch.Seconds < Full.Seconds && Scratch.Intensity > 0.f);
	TestEqual(TEXT("...a quarter of the full one"), Scratch.Intensity, Def::RumbleHurt * 0.25f, 0.0001f);
	const GunfeelKick::FRumble Middle = GunfeelKick::RumbleForHurt(0.1f, Def::RumbleHurt);
	TestTrue(TEXT("In between grows with the share"), Middle.Intensity > Scratch.Intensity && Middle.Intensity < Full.Intensity && Middle.Seconds > Scratch.Seconds);
	const GunfeelKick::FRumble Nothing = GunfeelKick::RumbleForHurt(0.f, Def::RumbleHurt);
	TestTrue(TEXT("No damage, no rumble"), Nothing.Intensity == 0.f && Nothing.Seconds == 0.f);

	// The gamepad: nothing rumbles unless the last input was one's (the watch notes it; a null player is safe).
	const bool bWas = GunfeelKick::IsGamepadLastInput();
	GunfeelKick::NoteInputDevice(true);
	TestTrue(TEXT("A gamepad press is noted"), GunfeelKick::IsGamepadLastInput());
	GunfeelKick::NoteInputDevice(false);
	TestFalse(TEXT("Then a key or the mouse takes it back"), GunfeelKick::IsGamepadLastInput());
	GunfeelKick::NoteInputDevice(true);
	GunfeelKick::RumbleShot(nullptr, EWeaponKind::Shotgun);
	GunfeelKick::RumbleHurt(nullptr, 0.5f);
	GunfeelKick::NoteInputDevice(bWas);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickRumbleSettingTest, "Looter.Gunfeel.Kick.RumbleSetting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickRumbleSettingTest::RunTest(const FString& Parameters)
{
	using Controls = UControlSettingsSubsystem;
	// A fresh install, and a save from before the setting, rumble as designed (100%).
	TestEqual(TEXT("Defaults to 100%"), Controls::DefaultControllerRumble, 1.f);
	TestEqual(TEXT("A fresh save starts at the default"), GetDefault<ULooterControlsSave>()->ControllerRumble, Controls::DefaultControllerRumble);
	TestEqual(TEXT("No save: the default"), Controls::ControllerRumbleOf(nullptr), Controls::DefaultControllerRumble);

	// 0% (off) to 100%, in the slider's 5% steps; anything outside (a hand-edited save) is pulled back in.
	TestEqual(TEXT("0% is allowed: no rumble"), Controls::ClampControllerRumble(0.f), 0.f);
	TestEqual(TEXT("Below 0 clamps to 0"), Controls::ClampControllerRumble(-1.f), 0.f);
	TestEqual(TEXT("Past 100% clamps to it"), Controls::ClampControllerRumble(4.f), 1.f);
	TestEqual(TEXT("Rounded to the slider's steps"), Controls::ClampControllerRumble(0.63f), 0.65f);
	TestEqual(TEXT("Not a number: the default"), Controls::ClampControllerRumble(std::numeric_limits<float>::quiet_NaN()), Controls::DefaultControllerRumble);

	// Saved and read back through a slot of the test's own, so the player's setting is never touched; the settings beside it
	// are left alone.
	const FString Slot = TEXT("ControlSettings_GunfeelKickTest");
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	ULooterControlsSave* Written = NewObject<ULooterControlsSave>();
	Written->ControllerRumble = 0.35f;
	if (TestTrue(TEXT("Saved"), UGameplayStatics::SaveGameToSlot(Written, Slot, 0)))
	{
		const ULooterControlsSave* Read = Controls::LoadControls(Slot, nullptr);
		TestEqual(TEXT("...read back"), Controls::ControllerRumbleOf(Read), 0.35f);
		TestEqual(TEXT("...the camera shake beside it untouched"), Controls::CameraShakeOf(Read), Controls::DefaultCameraShake);
		TestEqual(TEXT("...and the look sensitivity"), Controls::LookSensitivityOf(Read), Controls::DefaultLookSensitivity);
	}
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	return true;
}

#endif
