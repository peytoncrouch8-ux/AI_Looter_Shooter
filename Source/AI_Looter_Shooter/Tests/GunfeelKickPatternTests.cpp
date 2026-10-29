#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/ViewKick.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/WeaponRecoil.h"
#include "Weapons/WeaponTypes.h"
#include "HAL/IConsoleManager.h"

// The gun feel pass's kick rules (GF-KICK, 2026-10-22): the recoil pattern, the first shot's kick and spread, the sustained
// cap, the sight's model kick, the shake budget and the pump's rack. Each rule is driven with explicit numbers or the shipped
// defaults (GunfeelKick::Default), never a value the player may have set in play; the few live reads set a variable for the
// scope and put it back.

namespace
{
	namespace PatDef = GunfeelKick::Default;

	/** A console variable, set for the scope and put back after. */
	struct FPatternScopedVariable
	{
		FPatternScopedVariable(const TCHAR* Name, float Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Was = Variable->GetFloat();
				Variable->Set(*FString::SanitizeFloat(Value), ECVF_SetByConsole);
			}
		}
		~FPatternScopedVariable()
		{
			if (Variable)
			{
				Variable->Set(*FString::SanitizeFloat(Was), ECVF_SetByConsole);
			}
		}
		IConsoleVariable* Variable = nullptr;
		float Was = 0.f;
	};

	/** The most the shot kicks of a stack hold (GetShotPitch) while Kick is added every Interval seconds for Seconds. */
	float PeakShotPitch(const FViewKick& Kick, float Interval, float Seconds)
	{
		FViewKickStack Stack;
		const float Step = 0.002f;
		float Peak = 0.f;
		float NextShot = 0.f;
		for (float Time = 0.f; Time < Seconds; Time += Step)
		{
			if (Time >= NextShot)
			{
				Stack.Add(Kick);
				NextShot += Interval;
			}
			Stack.Tick(Step);
			Peak = FMath::Max(Peak, Stack.GetShotPitch());
		}
		return Peak;
	}
}

// ---------------------------------------------------------------------------
// The recoil pattern
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickPatternTest, "Looter.Gunfeel.Kick.Pattern",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickPatternTest::RunTest(const FString& Parameters)
{
	const GunfeelKick::FPatternNumbers Numbers;
	auto Side = [&Numbers](EWeaponKind Kind, int32 Shot) { return GunfeelKick::PatternSide(Kind, Shot, Numbers); };

	// The rifle: straight up for six shots, then a steady drift right, then (gun identity) back left: an S to learn.
	for (int32 Shot = 0; Shot < 6; ++Shot)
	{
		TestEqual(FString::Printf(TEXT("Rifle shot %d: straight up"), Shot), Side(EWeaponKind::Rifle, Shot), 0.f);
	}
	TestEqual(TEXT("Rifle shot 6: the drift starts"), Side(EWeaponKind::Rifle, 6), PatDef::RifleDrift);
	TestEqual(TEXT("...and holds to shot 15"), Side(EWeaponKind::Rifle, 15), PatDef::RifleDrift);
	TestTrue(TEXT("The drift is to the right"), Side(EWeaponKind::Rifle, 12) > 0.f);
	TestEqual(TEXT("Rifle shot 16: back left"), Side(EWeaponKind::Rifle, 16), PatDef::RifleDriftBack);
	TestTrue(TEXT("...the S's other way"), Side(EWeaponKind::Rifle, 30) < 0.f);
	{
		GunfeelKick::FPatternNumbers NoS;
		NoS.RifleDriftBack = 0.f;
		TestEqual(TEXT("No drift back: it holds right (today's)"), GunfeelKick::PatternSide(EWeaponKind::Rifle, 30, NoS), PatDef::RifleDrift);
	}

	// The SMG: a shimmy, one way then the other, so it hums and does not walk.
	TestEqual(TEXT("SMG shot 0: right"), Side(EWeaponKind::SMG, 0), PatDef::SMGShimmy);
	TestEqual(TEXT("SMG shot 1: left"), Side(EWeaponKind::SMG, 1), -PatDef::SMGShimmy);
	float ShimmySum = 0.f;
	for (int32 Shot = 0; Shot < 40; ++Shot)
	{
		ShimmySum += Side(EWeaponKind::SMG, Shot);
	}
	TestEqual(TEXT("...and it nets to nothing over an even burst"), ShimmySum, 0.f, 0.0001f);

	// The LMG: a slow sway that reaches both ways, never past its amplitude, and comes round in a period.
	float MostRight = 0.f;
	float MostLeft = 0.f;
	float SwaySum = 0.f;
	const int32 Period = FMath::RoundToInt(PatDef::LMGSwayShots);
	for (int32 Shot = 0; Shot < Period; ++Shot)
	{
		const float Each = Side(EWeaponKind::LMG, Shot);
		MostRight = FMath::Max(MostRight, Each);
		MostLeft = FMath::Min(MostLeft, Each);
		SwaySum += Each;
	}
	TestTrue(TEXT("The LMG sways right"), MostRight > PatDef::LMGSway * 0.7f);
	TestTrue(TEXT("...and left"), MostLeft < -PatDef::LMGSway * 0.7f);
	TestTrue(TEXT("...never past its amplitude"), MostRight <= PatDef::LMGSway + KINDA_SMALL_NUMBER && MostLeft >= -PatDef::LMGSway - KINDA_SMALL_NUMBER);
	TestEqual(TEXT("A whole sway nets to nothing"), SwaySum, 0.f, 0.001f);
	TestEqual(TEXT("It comes round after a period"), Side(EWeaponKind::LMG, 3), Side(EWeaponKind::LMG, 3 + Period), 0.0001f);

	// The single shots have no pattern, and a shot index below zero is the first.
	for (const EWeaponKind Kind : { EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::Sniper, EWeaponKind::Launcher, EWeaponKind::None })
	{
		TestEqual(TEXT("A single-shot kind has no pattern"), Side(Kind, 8), 0.f);
	}
	TestEqual(TEXT("A shot below zero is the first"), Side(EWeaponKind::SMG, -4), Side(EWeaponKind::SMG, 0));

	// Numbers given are the numbers used.
	GunfeelKick::FPatternNumbers Own;
	Own.RifleDrift = 0.1f;
	Own.RifleDriftFrom = 2.f;
	TestEqual(TEXT("Its own drift"), GunfeelKick::PatternSide(EWeaponKind::Rifle, 2, Own), 0.1f);
	TestEqual(TEXT("...from its own shot"), GunfeelKick::PatternSide(EWeaponKind::Rifle, 1, Own), 0.f);

	// The live ones: the pattern's strength scales it (0 off, 2 double); the random sideways kick keeps its share.
	{
		FPatternScopedVariable Pattern(TEXT("Looter.Gunfeel.Recoil.Pattern"), 0.f);
		TestEqual(TEXT("Pattern 0: no drift"), GunfeelKick::PatternSide(EWeaponKind::Rifle, 12), 0.f);
		TestEqual(TEXT("...and the old random sideways kick whole"), GunfeelKick::SideRandomScale(EWeaponKind::Rifle), 1.f);
	}
	{
		FPatternScopedVariable Pattern(TEXT("Looter.Gunfeel.Recoil.Pattern"), 2.f);
		FPatternScopedVariable Drift(TEXT("Looter.Gunfeel.Recoil.RifleDrift"), PatDef::RifleDrift);
		FPatternScopedVariable From(TEXT("Looter.Gunfeel.Recoil.RifleDriftFrom"), PatDef::RifleDriftFrom);
		TestEqual(TEXT("Pattern 2: double the drift"), GunfeelKick::PatternSide(EWeaponKind::Rifle, 12), PatDef::RifleDrift * 2.f, 0.0001f);
	}
	{
		FPatternScopedVariable Pattern(TEXT("Looter.Gunfeel.Recoil.Pattern"), 1.f);
		FPatternScopedVariable SideVar(TEXT("Looter.Gunfeel.Recoil.SideRandom"), PatDef::SideRandom);
		FPatternScopedVariable SideSMG(TEXT("Looter.Gunfeel.Recoil.SideRandom.SMG"), PatDef::SideRandomSMG);
		FPatternScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 1.f);
		TestEqual(TEXT("The rifle keeps 40% of its random sideways kick"), GunfeelKick::SideRandomScale(EWeaponKind::Rifle), PatDef::SideRandom, 0.0001f);
		TestEqual(TEXT("...the LMG too"), GunfeelKick::SideRandomScale(EWeaponKind::LMG), PatDef::SideRandom, 0.0001f);
		TestEqual(TEXT("...the SMG its own (gun identity: it buzzes about)"), GunfeelKick::SideRandomScale(EWeaponKind::SMG), PatDef::SideRandomSMG, 0.0001f);
		TestEqual(TEXT("The shotgun keeps all of it"), GunfeelKick::SideRandomScale(EWeaponKind::Shotgun), 1.f);
		TestEqual(TEXT("...and the revolver"), GunfeelKick::SideRandomScale(EWeaponKind::Revolver), 1.f);
	}
	return true;
}

// ---------------------------------------------------------------------------
// The first shot
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickFirstShotTest, "Looter.Gunfeel.Kick.FirstShot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickFirstShotTest::RunTest(const FString& Parameters)
{
	// The kick: the rifle's and the SMG's first pull of a burst is felt harder, then the burst settles to the ordinary.
	TestEqual(TEXT("Rifle first shot: 1.3x"), GunfeelKick::FirstShotKick(EWeaponKind::Rifle, 0, PatDef::KickFirstShot), 1.3f);
	TestEqual(TEXT("SMG first shot: 1.3x"), GunfeelKick::FirstShotKick(EWeaponKind::SMG, 0, PatDef::KickFirstShot), 1.3f);
	TestEqual(TEXT("The second shot: ordinary"), GunfeelKick::FirstShotKick(EWeaponKind::Rifle, 1, PatDef::KickFirstShot), 1.f);
	TestEqual(TEXT("The LMG has no first-shot kick"), GunfeelKick::FirstShotKick(EWeaponKind::LMG, 0, PatDef::KickFirstShot), 1.f);
	TestEqual(TEXT("...nor a shotgun"), GunfeelKick::FirstShotKick(EWeaponKind::Shotgun, 0, PatDef::KickFirstShot), 1.f);

	// The spread: a rested automatic aims 40% tighter for its first shot, eased in; holding the trigger blooms as before.
	const float Rest = PatDef::SpreadFirstShotRest;
	const float Ease = PatDef::SpreadFirstShotEase;
	const float First = PatDef::SpreadFirstShot;
	auto Bloom = [&](EWeaponKind Kind, float Raw, float Since) { return GunfeelKick::FirstShotBloom(Kind, Raw, Since, Rest, Ease, First); };
	TestEqual(TEXT("Mid-burst nothing changes"), Bloom(EWeaponKind::Rifle, 0.3f, 0.07f), 0.3f);
	TestEqual(TEXT("Just before the rest is over: as it was"), Bloom(EWeaponKind::Rifle, 0.05f, Rest - 0.01f), 0.05f);
	TestEqual(TEXT("Rested: 40% less spread (1 + bloom = 0.6)"), 1.f + Bloom(EWeaponKind::Rifle, 0.f, Rest + Ease + 0.1f), First, 0.0001f);
	TestEqual(TEXT("...for any leftover bloom"), 1.f + Bloom(EWeaponKind::Rifle, 0.2f, 5.f), First, 0.0001f);
	const float Halfway = Bloom(EWeaponKind::SMG, 0.f, Rest + Ease * 0.5f);
	TestTrue(FString::Printf(TEXT("Half way into the ease it is between (%.2f)"), Halfway), Halfway < 0.f && Halfway > First - 1.f);
	float Last = 1.f;
	for (float Since = 0.f; Since < 1.5f; Since += 0.02f)
	{
		const float Value = Bloom(EWeaponKind::LMG, 0.1f * FMath::Exp(-3.f * Since), Since);
		TestTrue(TEXT("The cone only tightens as it rests (no jump)"), Value <= Last + 0.02f);
		Last = Value;
	}
	TestEqual(TEXT("A shotgun never gets it"), Bloom(EWeaponKind::Shotgun, 0.f, 9.f), 0.f);
	TestEqual(TEXT("...nor a revolver"), Bloom(EWeaponKind::Revolver, 0.f, 9.f), 0.f);
	TestEqual(TEXT("A first-shot share of 1 is no reward"), GunfeelKick::FirstShotBloom(EWeaponKind::Rifle, 0.f, 9.f, Rest, Ease, 1.f), 0.f);
	return true;
}

// ---------------------------------------------------------------------------
// The sustained cap and the shake budget
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickSustainTest, "Looter.Gunfeel.Kick.SustainCap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickSustainTest::RunTest(const FString& Parameters)
{
	{
		// Today's shared cap (Identity 0); each automatic's own since gun identity is Looter.Gunfeel.Identity.SustainCaps.
		FPatternScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 0.f);
		FPatternScopedVariable Cap(TEXT("Looter.Gunfeel.Kick.SustainCap"), PatDef::SustainCap);
		TestEqual(TEXT("A rifle's shot kicks are capped at 1.2 degrees"), GunfeelKick::SustainCapFor(EWeaponKind::Rifle), PatDef::SustainCap);
		TestEqual(TEXT("...the SMG's and LMG's too"), GunfeelKick::SustainCapFor(EWeaponKind::LMG), GunfeelKick::SustainCapFor(EWeaponKind::SMG));
		TestEqual(TEXT("A shotgun has none"), GunfeelKick::SustainCapFor(EWeaponKind::Shotgun), 0.f);
		TestEqual(TEXT("...nor the launcher"), GunfeelKick::SustainCapFor(EWeaponKind::Launcher), 0.f);
	}

	// A fast, ringing kick piled up 50 times a second: uncapped it stacks well past the cap, capped it is held near it.
	FViewKick Fast;
	Fast.Pitch = 1.f;
	Fast.Frequency = 10.f;
	Fast.Damping = 0.2f;
	FViewKick Capped = Fast;
	Capped.SustainCap = PatDef::SustainCap;
	Capped.SustainFloor = PatDef::SustainFloor;
	const float Uncapped = PeakShotPitch(Fast, 0.02f, 1.5f);
	const float Held = PeakShotPitch(Capped, 0.02f, 1.5f);
	TestEqual(TEXT("An uncapped kick counts as no shot kick"), Uncapped, 0.f);
	// The uncapped run needs its kicks marked to be measured against: same kicks, a cap so far up it never bites.
	FViewKick Free = Fast;
	Free.SustainCap = 99.f;
	const float Piled = PeakShotPitch(Free, 0.02f, 1.5f);
	TestTrue(FString::Printf(TEXT("Piled up uncapped it passes the cap (%.2f)"), Piled), Piled > PatDef::SustainCap);
	TestTrue(FString::Printf(TEXT("Capped it is held lower (%.2f against %.2f)"), Held, Piled), Held < Piled * 0.9f);
	TestTrue(FString::Printf(TEXT("...and near the cap (%.2f)"), Held), Held <= PatDef::SustainCap * 1.5f);

	// A lone kick bigger than the cap (a launcher's) is never trimmed, and a kick marked with no pitch is left alone.
	FViewKick Heavy = Fast;
	Heavy.Pitch = 2.8f;
	Heavy.SustainCap = PatDef::SustainCap;
	Heavy.SustainFloor = PatDef::SustainFloor;
	const float Lone = PeakShotPitch(Heavy, 5.f, 0.2f);
	TestEqual(TEXT("A lone 2.8 degree kick keeps its size"), Lone, 2.8f, 0.15f);

	// Kicks with no cap (a hurt, a kill) don't count among the shot kicks, so an enemy's blow never trims a burst.
	FViewKickStack Mixed;
	Mixed.Add(ViewKicks::ForKill(true));
	Mixed.Tick(0.02f);
	TestEqual(TEXT("A kill's punch is no shot kick"), Mixed.GetShotPitch(), 0.f);

	// Today's shake budget (Identity 0: the old table at the old weights; gun identity's is Looter.Gunfeel.Identity.Caps): no
	// kind's shot kick is past 2.8 degrees at the hip or 1.4 through the sight, and the automatics' bursts held at the real
	// shot rates never get past the cap.
	struct FKindRate { EWeaponKind Kind; float Weight; float ShotsPerSecond; };
	namespace Today = PatDef::Today;
	const FKindRate Kinds[] = {
		{ EWeaponKind::Rifle, Today::KickRifle, 14.f }, { EWeaponKind::Shotgun, Today::KickShotgun, 1.f },
		{ EWeaponKind::Revolver, Today::KickRevolver, 2.f }, { EWeaponKind::SMG, Today::KickSMG, 15.f },
		{ EWeaponKind::Sniper, Today::KickSniper, 1.f }, { EWeaponKind::LMG, Today::KickLMG, 11.f },
		{ EWeaponKind::Launcher, Today::KickLauncher, 1.f } };
	FRandomStream Random(12);
	for (const FKindRate& Each : Kinds)
	{
		const FViewKick Hip = ViewKicks::ForShotToday(Each.Kind, 1.f, 0.f, Random) * Each.Weight;
		const FViewKick Sight = ViewKicks::ForShotToday(Each.Kind, 1.f, 1.f, Random) * Each.Weight;
		TestTrue(FString::Printf(TEXT("Kind %d: a hip shot turns the view %.2f degrees, 2.8 at most"), static_cast<int32>(Each.Kind), Hip.Pitch), Hip.Pitch <= 2.8f + 0.01f);
		TestEqual(TEXT("...and half through the sight"), Sight.Pitch, Hip.Pitch * 0.5f, 0.001f);

		if (Each.Kind == EWeaponKind::Rifle || Each.Kind == EWeaponKind::SMG || Each.Kind == EWeaponKind::LMG)
		{
			// A 3 second burst of the heaviest parts (Recoil stat 1.5), every shot given the first shot's extra: held under the cap.
			FViewKick Burst = ViewKicks::ForShotToday(Each.Kind, 1.5f, 0.f, Random)
				* (Each.Weight * FMath::Max(GunfeelKick::FirstShotKick(EWeaponKind::Rifle, 0, PatDef::KickFirstShot), 1.f));
			Burst.SustainCap = PatDef::SustainCap;
			Burst.SustainFloor = PatDef::SustainFloor;
			const float Peak = PeakShotPitch(Burst, 1.f / Each.ShotsPerSecond, 3.f);
			TestTrue(FString::Printf(TEXT("Kind %d held 3 seconds at heavy parts: %.2f degrees, under the cap"), static_cast<int32>(Each.Kind), Peak),
				Peak <= PatDef::SustainCap + 0.01f);
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// The sight's model kick and the shot count
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickAimedTest, "Looter.Gunfeel.Kick.AimedModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickAimedTest::RunTest(const FString& Parameters)
{
	FVector Shares;
	GunfeelKick::AimedModelKick(0.f, PatDef::AimedBack, PatDef::AimedFlip, PatDef::AimedRoll, Shares);
	TestTrue(TEXT("At the hip the model kicks as before"), Shares.Equals(FVector(1.f, 1.f, 1.f), 0.0001));
	GunfeelKick::AimedModelKick(1.f, PatDef::AimedBack, PatDef::AimedFlip, PatDef::AimedRoll, Shares);
	TestEqual(TEXT("Through the sight: back x1.2"), static_cast<float>(Shares.X), 1.2f, 0.0001f);
	TestEqual(TEXT("...flip x0.45"), static_cast<float>(Shares.Y), 0.45f, 0.0001f);
	TestEqual(TEXT("...roll x0.4"), static_cast<float>(Shares.Z), 0.4f, 0.0001f);
	GunfeelKick::AimedModelKick(0.5f, PatDef::AimedBack, PatDef::AimedFlip, PatDef::AimedRoll, Shares);
	TestEqual(TEXT("Half way up the sight: half way between"), static_cast<float>(Shares.Y), 0.725f, 0.0001f);
	GunfeelKick::AimedModelKick(7.f, PatDef::AimedBack, PatDef::AimedFlip, PatDef::AimedRoll, Shares);
	TestEqual(TEXT("An alpha past 1 is 1"), static_cast<float>(Shares.Y), 0.45f, 0.0001f);
	{
		FPatternScopedVariable Flip(TEXT("Looter.Gunfeel.Recoil.AimedFlip"), 0.3f);
		GunfeelKick::AimedModelKick(1.f, Shares);
		TestEqual(TEXT("The live one reads its variable"), static_cast<float>(Shares.Y), 0.3f, 0.0001f);
	}

	// The revolver's flip really shrinks: 8 degrees x 1.4 = 11.2 at the hip, about 5 through the sight; the back goes further.
	FWeaponRecoilProfile Hip;
	Hip.KickBack = 4.f;
	Hip.MuzzleFlip = 11.2f;
	Hip.MuzzleTwist = 0.f;
	Hip.Roll = 0.f;
	Hip.AimedBack = PatDef::AimedBack;
	Hip.AimedFlip = PatDef::AimedFlip;
	Hip.AimedRoll = PatDef::AimedRoll;
	auto Peaks = [&Hip](float AimAlpha, float& Flip, float& Back)
	{
		FRandomStream Random(4);
		FWeaponRecoil Recoil;
		Recoil.AddShot(Hip, Random, AimAlpha);
		Flip = 0.f;
		Back = 0.f;
		for (int32 Step = 0; Step < 300; ++Step)
		{
			Recoil.Tick(0.002f);
			Flip = FMath::Max(Flip, Recoil.GetKickRotation().Pitch);
			Back = FMath::Max(Back, Recoil.GetKickBack());
		}
	};
	float HipFlip, HipBack, SightFlip, SightBack;
	Peaks(0.f, HipFlip, HipBack);
	Peaks(1.f, SightFlip, SightBack);
	TestTrue(FString::Printf(TEXT("The hip flip is big (%.1f degrees)"), HipFlip), HipFlip > 8.f);
	TestEqual(TEXT("Through the sight the flip is 45% of it"), SightFlip / HipFlip, 0.45f, 0.02f);
	TestEqual(TEXT("...the push back 120%"), SightBack / HipBack, 1.2f, 0.03f);

	// The old overload is the hip.
	{
		FRandomStream RandomA(4);
		FRandomStream RandomB(4);
		FWeaponRecoil Old;
		FWeaponRecoil New;
		Old.AddShot(Hip, RandomA);
		New.AddShot(Hip, RandomB, 0.f);
		Old.Tick(0.01f);
		New.Tick(0.01f);
		TestEqual(TEXT("AddShot without an AimAlpha is the hip"), static_cast<double>(Old.GetKickRotation().Pitch), static_cast<double>(New.GetKickRotation().Pitch), 0.0001);
	}

	// The shot count and the pattern's sideways kick.
	FWeaponRecoilProfile Plain;
	Plain.AimKickSide = 0.f;
	Plain.MuzzleTwist = 0.f;
	Plain.Roll = 0.f;
	FRandomStream Random(9);
	FWeaponRecoil Recoil;
	TestEqual(TEXT("Before any shot: the first"), Recoil.GetShotInBurst(), 0);
	Recoil.AddShot(Plain, Random);
	TestEqual(TEXT("The first shot is shot 0"), Recoil.GetShotInBurst(), 0);
	Recoil.Tick(0.05f);
	Recoil.AddShot(Plain, Random);
	TestEqual(TEXT("...the next is 1"), Recoil.GetShotInBurst(), 1);
	for (int32 Pause = 0; Pause < 6; ++Pause)
	{
		Recoil.Tick(0.1f);
	}
	Recoil.AddShot(Plain, Random);
	TestEqual(TEXT("After a pause it starts over"), Recoil.GetShotInBurst(), 0);

	float Yaw = 0.f;
	Recoil.AddAimYaw(0.5f);
	for (int32 Step = 0; Step < 20; ++Step)
	{
		Yaw += Recoil.Tick(0.005f).Yaw;
	}
	TestTrue(FString::Printf(TEXT("A sideways kick moves the aim right (%.2f degrees)"), Yaw), Yaw > 0.2f);
	return true;
}

// ---------------------------------------------------------------------------
// The pump's rack
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelKickRackTest, "Looter.Gunfeel.Kick.Rack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelKickRackTest::RunTest(const FString& Parameters)
{
	const FViewKick Hip = ViewKicks::ForRack(0.f);
	const FViewKick Sight = ViewKicks::ForRack(1.f);
	TestTrue(TEXT("The rack dips the view a little"), Hip.Pitch < 0.f && Hip.Pitch > -0.2f);
	TestTrue(TEXT("...and squeezes it"), Hip.FieldOfView < 0.f);
	TestEqual(TEXT("Through the sight: half the dip"), Sight.Pitch, Hip.Pitch * 0.5f, 0.0001f);
	FRandomStream Random(1);
	TestTrue(TEXT("It is a beat, not a second shot: far less than the shotgun's shove"),
		FMath::Abs(Hip.Pitch) < ViewKicks::ForShot(EWeaponKind::Shotgun, 1.f, 0.f, Random).Pitch * 0.2f);
	FViewKickStack Stack;
	Stack.Add(Hip);
	for (int32 Step = 0; Step < 50; ++Step)
	{
		Stack.Tick(0.01f);
	}
	TestTrue(TEXT("It has settled within half a second"), Stack.IsSettled());
	{
		FPatternScopedVariable Rack(TEXT("Looter.Gunfeel.Kick.Rack"), 0.5f);
		TestEqual(TEXT("The live scale reads its variable"), GunfeelKick::RackKickScale(), 0.5f);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
