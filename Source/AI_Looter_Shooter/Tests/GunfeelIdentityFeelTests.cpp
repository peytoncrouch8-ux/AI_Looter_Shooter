#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
#include "Weapons/GunFeelSettings.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/GunKindsTuning.h"
#include "Weapons/WeaponRecoil.h"
#include "Weapons/WeaponTypes.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/Package.h"

// Gun identity (GUNS.md lane A, 2026-10-28), the second half: the LMG's dig-in, the automatics' rhythm, each kind's ADS
// curve and sight-arrival settle, the heavy kinds' shakes, a held LMG belt's climb cap (2026-10-29), and the kinds' numbers
// in Project Settings > Game > Gun Feel with one kick layer (2026-11-03: Looter.Gunfeel.Identity no longer touches the kick).
// The kicks, the sight and the recovery are GunfeelIdentityTests.cpp. Driven with the shipped defaults; the few live reads
// set the variables or settings they read for their scope and put them back.

namespace
{
	namespace IdDef = GunfeelKick::Default;

	/** A console variable, set to Value for the scope and put back after. */
	struct FIdentityScopedVariable
	{
		FIdentityScopedVariable(const TCHAR* Name, float Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Was = Variable->GetString();
				Variable->Set(*FString::SanitizeFloat(Value), ECVF_SetByConsole);
			}
		}
		~FIdentityScopedVariable()
		{
			if (Variable)
			{
				Variable->Set(*Was, ECVF_SetByConsole);
			}
		}
		IConsoleVariable* Variable = nullptr;
		FString Was;
	};

	/** One kind's Gun Feel entry (Project Settings > Game > Gun Feel) set to Feel for the scope and put back after. */
	struct FScopedFeelSetting
	{
		FScopedFeelSetting(EWeaponKind InKind, const GunfeelKick::FKindFeel& Feel)
			: Kind(InKind)
			, Was(GetMutableDefault<UGunFeelSettings>()->For(InKind))
		{
			GetMutableDefault<UGunFeelSettings>()->For(Kind) = FGunKindFeelSettings(Feel);
		}
		~FScopedFeelSetting()
		{
			GetMutableDefault<UGunFeelSettings>()->For(Kind) = Was;
		}
		EWeaponKind Kind;
		FGunKindFeelSettings Was;
	};

	/** Whether two kinds' numbers match, every one. */
	bool SameFeel(const GunfeelKick::FKindFeel& A, const GunfeelKick::FKindFeel& B)
	{
		const float Tolerance = 0.0001f;
		return FMath::IsNearlyEqual(A.AimedTurn, B.AimedTurn, Tolerance) && FMath::IsNearlyEqual(A.AimedFieldOfView, B.AimedFieldOfView, Tolerance)
			&& FMath::IsNearlyEqual(A.AimedBack, B.AimedBack, Tolerance) && FMath::IsNearlyEqual(A.AimedFlip, B.AimedFlip, Tolerance)
			&& FMath::IsNearlyEqual(A.AimedRoll, B.AimedRoll, Tolerance) && FMath::IsNearlyEqual(A.AimKick, B.AimKick, Tolerance)
			&& FMath::IsNearlyEqual(A.Recovery, B.Recovery, Tolerance) && FMath::IsNearlyEqual(A.Cadence, B.Cadence, Tolerance)
			&& FMath::IsNearlyEqual(A.AimCurve, B.AimCurve, Tolerance) && FMath::IsNearlyEqual(A.AimSettle, B.AimSettle, Tolerance)
			&& FMath::IsNearlyEqual(A.SustainCap, B.SustainCap, Tolerance) && FMath::IsNearlyEqual(A.Shake, B.Shake, Tolerance)
			&& FMath::IsNearlyEqual(A.ShakeSeconds, B.ShakeSeconds, Tolerance);
	}

	constexpr EWeaponKind AllKinds[] = { EWeaponKind::SMG, EWeaponKind::Rifle, EWeaponKind::LMG, EWeaponKind::Revolver,
		EWeaponKind::Shotgun, EWeaponKind::Sniper, EWeaponKind::Launcher };

	bool IsAutomatic(EWeaponKind Kind)
	{
		return Kind == EWeaponKind::Rifle || Kind == EWeaponKind::SMG || Kind == EWeaponKind::LMG;
	}

	/** What a held LMG burst did to the aim: the climb at its end, the highest it reached, how far the sway took it each way. */
	struct FLMGBurst
	{
		float Climb = 0.f;
		float Highest = 0.f;
		float Left = 0.f;
		float Right = 0.f;
		int32 Shots = 0;
	};

	/**
	 * The trigger held Seconds on a cold LMG at Fps, as UPlayerViewComponent and AWeaponBase do it with the shipped numbers:
	 * DA_LMG's recoil (create_new_guns.py) through GunfeelKick::ShotProfile with the LMG's shipped numbers (its aim kick,
	 * recovery, climb, the recovery while held and the climb cap, or none with bCapped false), its spin-up rate, its dig-in
	 * and its sway pattern; Share the gun's KickShare (1 a base Common). The timer fires every shot due in a frame (several a
	 * frame when the game runs slow), then the view's recoil ticks. No hand on the mouse: nothing pulls down.
	 */
	FLMGBurst HoldLMG(float Share, float Seconds, float Fps, bool bCapped)
	{
		namespace KindsDef = GunKinds::Default;
		GunfeelKick::FProfileNumbers Numbers = GunfeelKick::ShippedProfileNumbers(EWeaponKind::LMG);
		Numbers.ClimbCap = bCapped ? KindsDef::LMGClimbCap : 0.f;
		FWeaponRecoilProfile Asset;
		Asset.KickBack = 2.6f;
		Asset.MuzzleFlip = 2.2f;
		Asset.MuzzleTwist = 0.9f;
		Asset.Roll = 1.6f;
		Asset.AimKick = 0.3f;
		Asset.AimKickSide = 0.18f;
		Asset.AimRecovery = 6.f;
		Asset.Snappiness = 0.85f;
		const GunfeelKick::FPatternNumbers Pattern{};
		const float FullInterval = 60.f / 650.f;
		const float Dt = 1.f / Fps;
		FWeaponRecoil Recoil;
		FRandomStream Random(0x1a6);
		FLMGBurst Burst;
		float Time = 0.f;
		float NextShot = 0.f;
		float Pitch = 0.f;
		float Yaw = 0.f;
		while (Time < Seconds - KINDA_SMALL_NUMBER)
		{
			Time += Dt;
			while (NextShot <= Time)
			{
				const float Spin = GunKinds::SpinAfter(0.f, NextShot, true, KindsDef::LMGSpinUp, KindsDef::LMGSpinDown);
				const float Kick = Share * GunfeelKick::DigIn(Spin, IdDef::LMGDigInFrom, IdDef::LMGDigInStart, IdDef::LMGDigInEnd);
				Recoil.AddShot(GunfeelKick::ShotProfile(Asset, Kick, Numbers), Random, 1.f);
				Recoil.AddAimYaw(GunfeelKick::PatternSide(EWeaponKind::LMG, Recoil.GetShotInBurst(), Pattern) * Kick);
				++Burst.Shots;
				NextShot += FullInterval / GunKinds::SpinRateShare(Spin, KindsDef::LMGSpinStart);
			}
			const FRotator Change = Recoil.Tick(Dt);
			Pitch += static_cast<float>(Change.Pitch);
			Yaw += static_cast<float>(Change.Yaw);
			Burst.Highest = FMath::Max(Burst.Highest, Pitch);
			Burst.Left = FMath::Min(Burst.Left, Yaw);
			Burst.Right = FMath::Max(Burst.Right, Yaw);
		}
		Burst.Climb = Pitch;
		return Burst;
	}
}

// ---------------------------------------------------------------------------
// The LMG's dig-in, the rhythm
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentityDigInTest, "Looter.Gunfeel.Identity.DigIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentityDigInTest::RunTest(const FString& Parameters)
{
	// The LMG's first rounds bully the shooter; a held belt, spun up, settles.
	const float From = IdDef::LMGDigInFrom;
	const float Start = IdDef::LMGDigInStart;
	const float End = IdDef::LMGDigInEnd;
	TestEqual(TEXT("Cold: the start's"), GunfeelKick::DigIn(0.f, From, Start, End), Start, 0.0001f);
	TestEqual(TEXT("...until spin 0.6"), GunfeelKick::DigIn(From, From, Start, End), Start, 0.0001f);
	TestEqual(TEXT("Half way: between"), GunfeelKick::DigIn(0.8f, From, Start, End), (Start + End) * 0.5f, 0.0001f);
	TestEqual(TEXT("Spun up: steadier"), GunfeelKick::DigIn(1.f, From, Start, End), End, 0.0001f);
	TestTrue(TEXT("Harder cold than spun"), Start > 1.f && End < 1.f);
	float Last = Start + 1.f;
	for (float Spin = 0.f; Spin <= 1.f; Spin += 0.05f)
	{
		const float Each = GunfeelKick::DigIn(Spin, From, Start, End);
		TestTrue(TEXT("It only eases as it spins"), Each <= Last + 0.0001f);
		Last = Each;
	}
	{
		FIdentityScopedVariable StartVar(TEXT("Looter.Gunfeel.LMG.DigInStart"), Start);
		FIdentityScopedVariable EndVar(TEXT("Looter.Gunfeel.LMG.DigInEnd"), End);
		FIdentityScopedVariable FromVar(TEXT("Looter.Gunfeel.LMG.DigInFrom"), From);
		TestEqual(TEXT("The live one reads its variables"), GunfeelKick::LMGDigIn(0.f), Start, 0.0001f);
		// One kick layer: the old switch no longer takes it away.
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 0.f);
		TestEqual(TEXT("Identity 0 keeps the dig-in"), GunfeelKick::LMGDigIn(0.f), Start, 0.0001f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentityLMGBurstTest, "Looter.Gunfeel.Identity.LMGBurst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentityLMGBurstTest::RunTest(const FString& Parameters)
{
	// A held belt climbs a few degrees and then rides its cap, swaying side to side (the 2026-10-29 play-check saw a 2 s
	// burst walk the view into the sky); held, it barely recovers now (2026-11-03), so the cap does all the holding. The base
	// gun (KickShare 1) and a sloppy one (past the accuracy's worst).
	const float Cap = GunKinds::Default::LMGClimbCap;
	const float Base = 1.f;
	const float Sloppy = IdDef::AccuracyCalmMost * 1.15f;
	const float Margin = 0.3f;

	const FLMGBurst Two = HoldLMG(Base, 2.f, 60.f, true);
	TestTrue(FString::Printf(TEXT("2 s at 60 fps: %d shots, a belt's worth"), Two.Shots), Two.Shots >= 18);
	TestTrue(FString::Printf(TEXT("2 s: it climbs a few degrees (%.2f)"), Two.Climb), Two.Climb > 1.f);
	TestTrue(FString::Printf(TEXT("2 s: held under the cap (%.2f, cap %.1f) and far from the sky"), Two.Highest, Cap),
		Two.Highest <= Cap + Margin && Two.Highest < 8.f);

	const FLMGBurst Hard = HoldLMG(Sloppy, 2.f, 60.f, true);
	TestTrue(FString::Printf(TEXT("A sloppy gun's 2 s: held too (%.2f)"), Hard.Highest), Hard.Highest <= Cap + Margin);
	const FLMGBurst Loose = HoldLMG(Sloppy, 2.f, 60.f, false);
	TestTrue(FString::Printf(TEXT("...the cap is what holds it (%.2f without)"), Loose.Highest), Loose.Highest > Cap + Margin);

	const FLMGBurst Box = HoldLMG(Sloppy, 9.f, 60.f, true);
	TestTrue(FString::Printf(TEXT("A whole box (9 s): never past the cap (%.2f)"), Box.Highest), Box.Highest <= Cap + Margin);
	TestTrue(FString::Printf(TEXT("...the sway carries it both ways (%.2f..%.2f degrees)"), Box.Left, Box.Right),
		Box.Left < -0.1f && Box.Right > 0.1f);

	// A slow frame (a background editor runs at 3 fps) fires several shots a frame: still held.
	const FLMGBurst Slow = HoldLMG(Sloppy, 2.f, 3.f, true);
	TestTrue(FString::Printf(TEXT("3 fps: held (%.2f)"), Slow.Highest), Slow.Highest <= Cap + Margin);

	// A profile with no cap (the rifle's, the SMG's, every other kind's) kicks exactly as before.
	FWeaponRecoilProfile Plain;
	Plain.ClimbPerShot = IdDef::RifleClimb;
	Plain.ClimbMax = IdDef::RifleClimbMax;
	FWeaponRecoilProfile Roomy = Plain;
	Roomy.ClimbCap = 1000.f;
	FWeaponRecoil A;
	FWeaponRecoil B;
	FRandomStream RandomA(5);
	FRandomStream RandomB(5);
	float PitchA = 0.f;
	float PitchB = 0.f;
	for (int32 Shot = 0; Shot < 30; ++Shot)
	{
		A.AddShot(Plain, RandomA);
		B.AddShot(Roomy, RandomB);
		PitchA += static_cast<float>(A.Tick(0.09f).Pitch);
		PitchB += static_cast<float>(B.Tick(0.09f).Pitch);
	}
	TestEqual(TEXT("No cap (or one never reached): the same climb"), PitchA, PitchB, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentityCadenceTest, "Looter.Gunfeel.Identity.Cadence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentityCadenceTest::RunTest(const FString& Parameters)
{
	// An automatic's rhythm: each interval a touch long or short, the same for every read of one shot, the mean kept (the
	// damage per second unchanged).
	for (const EWeaponKind Kind : { EWeaponKind::SMG, EWeaponKind::Rifle, EWeaponKind::LMG })
	{
		const float Amount = GunfeelKick::KindFeelDefault(Kind).Cadence;
		TestTrue(TEXT("An automatic has a rhythm"), Amount > 0.f && Amount <= 0.05f);
		double Sum = 0.0;
		float Least = 2.f;
		float Most = 0.f;
		for (int32 Shot = 0; Shot < 1000; ++Shot)
		{
			const float Each = GunfeelKick::CadenceJitter(Shot, Amount);
			Sum += Each;
			Least = FMath::Min(Least, Each);
			Most = FMath::Max(Most, Each);
		}
		const float Mean = static_cast<float>(Sum / 1000.0);
		TestTrue(FString::Printf(TEXT("Kind %d: the mean over 1000 shots is %.4f, within 0.5%%"), static_cast<int32>(Kind), Mean), FMath::Abs(Mean - 1.f) <= 0.005f);
		TestTrue(TEXT("...never past its share"), Least >= 1.f - Amount - 0.0001f && Most <= 1.f + Amount + 0.0001f);
		TestTrue(TEXT("...and reaching both ways"), Least < 1.f - Amount * 0.8f && Most > 1.f + Amount * 0.8f);
	}
	TestEqual(TEXT("One shot always reads the same"), GunfeelKick::CadenceJitter(417, 0.05f), GunfeelKick::CadenceJitter(417, 0.05f));
	TestEqual(TEXT("No share: no change"), GunfeelKick::CadenceJitter(9, 0.f), 1.f);
	for (const EWeaponKind Single : { EWeaponKind::Revolver, EWeaponKind::Shotgun, EWeaponKind::Sniper, EWeaponKind::Launcher })
	{
		TestEqual(TEXT("A single shot's rhythm is the pull's own"), GunfeelKick::KindFeelDefault(Single).Cadence, 0.f);
	}
	{
		FScopedFeelSetting LMG(EWeaponKind::LMG, GunfeelKick::KindFeelDefault(EWeaponKind::LMG));
		FScopedFeelSetting Revolver(EWeaponKind::Revolver, GunfeelKick::KindFeelDefault(EWeaponKind::Revolver));
		TestEqual(TEXT("An LMG still spinning up keeps its climb"), GunfeelKick::CadenceScale(EWeaponKind::LMG, 5, 0.5f), 1.f);
		TestEqual(TEXT("...spun up it has its rhythm"), GunfeelKick::CadenceScale(EWeaponKind::LMG, 5, 1.f),
			GunfeelKick::CadenceJitter(5, GunfeelKick::KindFeelDefault(EWeaponKind::LMG).Cadence), 0.0001f);
		TestEqual(TEXT("A six-gun none"), GunfeelKick::CadenceScale(EWeaponKind::Revolver, 5, 1.f), 1.f);
	}
	return true;
}

// ---------------------------------------------------------------------------
// The sights, the shakes, the switch
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentityAimCurveTest, "Looter.Gunfeel.Identity.AimCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentityAimCurveTest::RunTest(const FString& Parameters)
{
	// Each kind comes up its own way, the ends the same for all (the spread, the look's speed and the crosshair's fade read
	// it): a light gun snaps to the eye, a heavy one is hefted.
	for (const EWeaponKind Kind : AllKinds)
	{
		const float Curve = GunfeelKick::KindFeelDefault(Kind).AimCurve;
		TestEqual(TEXT("Hip is 0"), FMath::InterpEaseInOut(0.f, 1.f, 0.f, Curve), 0.f, 0.0001f);
		TestEqual(TEXT("...the sight 1"), FMath::InterpEaseInOut(0.f, 1.f, 1.f, Curve), 1.f, 0.0001f);
		TestEqual(TEXT("...half way half"), FMath::InterpEaseInOut(0.f, 1.f, 0.5f, Curve), 0.5f, 0.0001f);
	}
	auto Early = [](EWeaponKind Kind) { return FMath::InterpEaseInOut(0.f, 1.f, 0.25f, GunfeelKick::KindFeelDefault(Kind).AimCurve); };
	TestTrue(TEXT("The SMG and the six-gun come up quickest, the LMG and the scope slowest"),
		Early(EWeaponKind::SMG) > Early(EWeaponKind::Rifle) && Early(EWeaponKind::Revolver) > Early(EWeaponKind::Rifle)
		&& Early(EWeaponKind::Rifle) > Early(EWeaponKind::LMG) && Early(EWeaponKind::Rifle) > Early(EWeaponKind::Sniper));
	TestEqual(TEXT("The rifle keeps the old shared ease (2)"), GunfeelKick::KindFeelDefault(EWeaponKind::Rifle).AimCurve, 2.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentitySettleTest, "Looter.Gunfeel.Identity.Settle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentitySettleTest::RunTest(const FString& Parameters)
{
	// The sights arriving at the eye: a small dip and squeeze, a heavy gun's more, gone in a moment.
	const FViewKick Settle = ViewKicks::ForAimSettle(1.f);
	TestEqual(TEXT("0.12 degrees down"), Settle.Pitch, -0.12f, 0.0001f);
	TestEqual(TEXT("...0.3% narrower"), Settle.FieldOfView, -0.003f, 0.00001f);
	TestEqual(TEXT("...at 9 Hz"), Settle.Frequency, 9.f);
	TestEqual(TEXT("It scales"), ViewKicks::ForAimSettle(1.6f).Pitch, Settle.Pitch * 1.6f, 0.0001f);
	TestTrue(TEXT("Scale 0: none"), ViewKicks::ForAimSettle(0.f).IsNone());
	auto Of = [](EWeaponKind Kind) { return GunfeelKick::KindFeelDefault(Kind).AimSettle; };
	TestTrue(TEXT("The LMG settles heaviest, the SMG lightest"), Of(EWeaponKind::LMG) > Of(EWeaponKind::Launcher)
		&& Of(EWeaponKind::Launcher) > Of(EWeaponKind::Shotgun) && Of(EWeaponKind::Shotgun) > Of(EWeaponKind::Sniper)
		&& Of(EWeaponKind::Sniper) > Of(EWeaponKind::Rifle) && Of(EWeaponKind::Rifle) > Of(EWeaponKind::Revolver)
		&& Of(EWeaponKind::Revolver) > Of(EWeaponKind::SMG) && Of(EWeaponKind::SMG) > 0.f);
	FViewKickStack Stack;
	Stack.Add(ViewKicks::ForAimSettle(Of(EWeaponKind::LMG)));
	for (int32 Step = 0; Step < 300; ++Step)
	{
		Stack.Tick(0.001f);
	}
	TestTrue(TEXT("Even the heaviest is gone in 0.3 s"), Stack.IsSettled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentityShakesTest, "Looter.Gunfeel.Identity.Shakes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentityShakesTest::RunTest(const FString& Parameters)
{
	// The heavy kinds' shot shakes the camera a moment; the light ones never; the LMG's held small under its cap.
	const float LMGMax = IdDef::ShakeLMGMax;
	auto Shake = [LMGMax](EWeaponKind Kind, float Aim, float Amount)
	{
		return GunfeelKick::ShotShake(Kind, GunfeelKick::KindFeelDefault(Kind), Aim, Amount, LMGMax);
	};
	for (const EWeaponKind Light : { EWeaponKind::SMG, EWeaponKind::Rifle, EWeaponKind::Revolver })
	{
		TestEqual(TEXT("A light gun doesn't shake"), Shake(Light, 0.f, 0.f).Strength, 0.f);
	}
	TestEqual(TEXT("The shotgun: 0.10"), Shake(EWeaponKind::Shotgun, 0.f, 0.f).Strength, 0.1f, 0.0001f);
	TestEqual(TEXT("...for 0.18 s"), Shake(EWeaponKind::Shotgun, 0.f, 0.f).Seconds, 0.18f, 0.0001f);
	TestEqual(TEXT("The sniper: 0.12 for 0.3 s"), Shake(EWeaponKind::Sniper, 0.f, 0.f).Strength, 0.12f, 0.0001f);
	TestEqual(TEXT("The launcher: 0.3 for 0.45 s"), Shake(EWeaponKind::Launcher, 0.f, 0.f).Seconds, 0.45f, 0.0001f);
	TestTrue(TEXT("The launcher shakes hardest"), Shake(EWeaponKind::Launcher, 0.f, 0.f).Strength > Shake(EWeaponKind::Sniper, 0.f, 0.f).Strength
		&& Shake(EWeaponKind::Sniper, 0.f, 0.f).Strength > Shake(EWeaponKind::Shotgun, 0.f, 0.f).Strength);
	TestEqual(TEXT("Through the sight, the kind's turn share of it"), Shake(EWeaponKind::Sniper, 1.f, 0.f).Strength,
		0.12f * GunfeelKick::KindFeelDefault(EWeaponKind::Sniper).AimedTurn, 0.0001f);
	TestEqual(TEXT("The LMG: 0.035 a shot"), Shake(EWeaponKind::LMG, 0.f, 0.f).Strength, 0.035f, 0.0001f);
	TestEqual(TEXT("...only the room left under its cap"), Shake(EWeaponKind::LMG, 0.f, LMGMax - 0.01f).Strength, 0.01f, 0.0001f);
	TestEqual(TEXT("...none once it's there"), Shake(EWeaponKind::LMG, 0.f, LMGMax).Strength, 0.f);
	TestEqual(TEXT("Other kinds aren't held by the LMG's cap"), Shake(EWeaponKind::Shotgun, 0.f, 1.f).Strength, 0.1f, 0.0001f);

	// A belt held at 11 shots a second never shakes the camera past the cap.
	UCameraShakeModifier* Modifier = NewObject<UCameraShakeModifier>(GetTransientPackage());
	FMinimalViewInfo View;
	float Most = 0.f;
	for (int32 Step = 0; Step < 3000; ++Step)
	{
		if (Step % 91 == 0)
		{
			const GunfeelKick::FShotShake Each = Shake(EWeaponKind::LMG, 0.f, Modifier->GetAmount());
			Modifier->AddShake(Each.Strength, Each.Seconds);
		}
		Modifier->ModifyCamera(0.001f, View);
		Most = FMath::Max(Most, Modifier->GetAmount());
	}
	TestTrue(FString::Printf(TEXT("A held belt's shake stays under %.2f (%.3f)"), LMGMax, Most), Most <= LMGMax + 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentitySwitchTest, "Looter.Gunfeel.Identity.Switch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentitySwitchTest::RunTest(const FString& Parameters)
{
	// Each kind's numbers live in Project Settings > Game > Gun Feel (UGunFeelSettings, 2026-11-03), starting at
	// KindFeelDefault; the kick has one layer: Looter.Gunfeel.Identity still switches the sound, heft, brass and reload beats
	// for an A/B, but nothing in the kick, the sight or the rhythm asks it.
	FIdentityScopedVariable Pattern(TEXT("Looter.Gunfeel.Recoil.Pattern"), IdDef::RecoilPattern);
	FIdentityScopedVariable Drift(TEXT("Looter.Gunfeel.Recoil.RifleDrift"), IdDef::RifleDrift);
	FIdentityScopedVariable DriftFrom(TEXT("Looter.Gunfeel.Recoil.RifleDriftFrom"), IdDef::RifleDriftFrom);
	FIdentityScopedVariable DriftBack(TEXT("Looter.Gunfeel.Recoil.RifleDriftBack"), IdDef::RifleDriftBack);
	FIdentityScopedVariable DriftBackFrom(TEXT("Looter.Gunfeel.Recoil.RifleDriftBackFrom"), IdDef::RifleDriftBackFrom);
	FIdentityScopedVariable Shimmy(TEXT("Looter.Gunfeel.Recoil.SMGShimmy"), IdDef::SMGShimmy);
	FIdentityScopedVariable SideSMG(TEXT("Looter.Gunfeel.Recoil.SideRandom.SMG"), IdDef::SideRandomSMG);
	FIdentityScopedVariable Heave(TEXT("Looter.Gunfeel.Kick.Heave"), IdDef::HeaveKick);
	FIdentityScopedVariable BoltHome(TEXT("Looter.Gunfeel.Kick.BoltHome"), IdDef::BoltHomeKick);
	FIdentityScopedVariable Hold(TEXT("Looter.Gunfeel.Recoil.SniperHold"), IdDef::SniperHold);
	TestNotNull(TEXT("The switch exists (sound, heft, brass, reload beats)"), IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Gunfeel.Identity")));
	// The single numbers are live variables.
	for (const TCHAR* Name : { TEXT("LMG.DigInStart"), TEXT("LMG.DigInEnd"), TEXT("LMG.DigInFrom"), TEXT("Shake.LMGMax"),
		TEXT("Recoil.SniperHold"), TEXT("Kick.Heave"), TEXT("Kick.BoltHome"), TEXT("Kick.Lift"), TEXT("Recoil.SideRandom.SMG"),
		TEXT("Recoil.RifleDriftBack"), TEXT("Recoil.RifleDriftBackFrom") })
	{
		TestNotNull(*FString::Printf(TEXT("Looter.Gunfeel.%s exists"), Name), IConsoleManager::Get().FindConsoleVariable(*(FString(TEXT("Looter.Gunfeel.")) + Name)));
	}

	// The kinds' numbers are settings now, not a variable each.
	const TCHAR* KindNames[] = { TEXT("SMG"), TEXT("Rifle"), TEXT("LMG"), TEXT("Revolver"), TEXT("Shotgun"), TEXT("Sniper"), TEXT("Launcher") };
	for (const TCHAR* Old : { TEXT("Kick.AimedTurn."), TEXT("Recoil.Recovery."), TEXT("Aim.Curve."), TEXT("Kick.SustainCap.") })
	{
		TestNull(*FString::Printf(TEXT("Looter.Gunfeel.%sRifle is gone (Gun Feel settings)"), Old),
			IConsoleManager::Get().FindConsoleVariable(*(FString(TEXT("Looter.Gunfeel.")) + Old + KindNames[1])));
	}
	// Each kind's entry starts at its shipped numbers (unless a designer has saved their own to DefaultGame.ini), round-trips
	// to the rules' numbers, and is what the live rule reads.
	const bool bSaved = GConfig && GConfig->DoesSectionExist(TEXT("/Script/AI_Looter_Shooter.GunFeelSettings"), GGameIni);
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(AllKinds)); ++Index)
	{
		const EWeaponKind Kind = AllKinds[Index];
		const GunfeelKick::FKindFeel Shipped = GunfeelKick::KindFeelDefault(Kind);
		TestTrue(FString::Printf(TEXT("%s: its entry round-trips"), KindNames[Index]), SameFeel(FGunKindFeelSettings(Shipped).ToFeel(), Shipped));
		if (!bSaved)
		{
			TestTrue(FString::Printf(TEXT("%s: Gun Feel ships at its defaults"), KindNames[Index]), SameFeel(UGunFeelSettings::Get().For(Kind).ToFeel(), Shipped));
		}
		GunfeelKick::FKindFeel Own = Shipped;
		Own.AimKick = Shipped.AimKick + 0.5f;
		Own.SustainCap = IsAutomatic(Kind) ? Shipped.SustainCap + 0.25f : 0.f;
		FScopedFeelSetting Setting(Kind, Own);
		TestEqual(FString::Printf(TEXT("%s: the live aim kick is its setting"), KindNames[Index]), GunfeelKick::KindFeel(Kind).AimKick, Own.AimKick, 0.0001f);
		TestEqual(FString::Printf(TEXT("%s: ...and its cap"), KindNames[Index]), GunfeelKick::SustainCapFor(Kind), Own.SustainCap, 0.0001f);
	}
	if (bSaved)
	{
		AddInfo(TEXT("DefaultGame.ini has saved Gun Feel numbers: the shipped-defaults check was skipped."));
	}
	{
		GunfeelKick::FKindFeel Wild = GunfeelKick::KindFeelDefault(EWeaponKind::Rifle);
		Wild.Cadence = 0.9f;
		Wild.AimCurve = 0.1f;
		Wild.Recovery = -1.f;
		FScopedFeelSetting Setting(EWeaponKind::Rifle, Wild);
		const GunfeelKick::FKindFeel Held = GunfeelKick::KindFeel(EWeaponKind::Rifle);
		TestTrue(TEXT("A wild setting is held to sane ranges"), Held.Cadence <= 0.2f && Held.AimCurve >= 1.f && Held.Recovery >= 0.1f);
	}

	// One kick layer: with the switch at 0 or 1 the kick reads the same (the kinds at their shipped numbers for the check).
	FScopedFeelSetting ShippedSMG(EWeaponKind::SMG, GunfeelKick::KindFeelDefault(EWeaponKind::SMG));
	FScopedFeelSetting ShippedRifle(EWeaponKind::Rifle, GunfeelKick::KindFeelDefault(EWeaponKind::Rifle));
	FScopedFeelSetting ShippedLMG(EWeaponKind::LMG, GunfeelKick::KindFeelDefault(EWeaponKind::LMG));
	FScopedFeelSetting ShippedShotgun(EWeaponKind::Shotgun, GunfeelKick::KindFeelDefault(EWeaponKind::Shotgun));
	auto Read = [](TArray<float>& Out)
	{
		for (const EWeaponKind Kind : AllKinds)
		{
			const GunfeelKick::FKindFeel Feel = GunfeelKick::KindFeel(Kind);
			Out.Append({ Feel.AimedTurn, Feel.AimKick, Feel.Recovery, Feel.AimCurve, Feel.AimSettle, GunfeelKick::SustainCapFor(Kind),
				GunfeelKick::CadenceScale(Kind, 3, 1.f), GunfeelKick::ViewKickScale(Kind), GunfeelKick::SideRandomScale(Kind),
				GunfeelKick::PatternSide(Kind, 9), GunfeelKick::ShotShake(Kind, 0.f, 0.f).Strength });
		}
		Out.Append({ GunfeelKick::BloomAfterShot(EWeaponKind::SMG, 0.f, 0.f), GunfeelKick::LMGDigIn(0.f), GunfeelKick::SniperRecoilHold(0.5f),
			GunfeelKick::HeaveScale(), GunfeelKick::BoltHomeScale(), GunfeelKick::LiftScale() });
	};
	TArray<float> On;
	TArray<float> Off;
	{
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 1.f);
		TestTrue(TEXT("1 is on"), GunfeelKick::IdentityOn());
		Read(On);
	}
	{
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 0.f);
		TestFalse(TEXT("0 is off"), GunfeelKick::IdentityOn());
		Read(Off);
	}
	bool bSame = On.Num() == Off.Num();
	for (int32 Index = 0; bSame && Index < On.Num(); ++Index)
	{
		bSame = FMath::IsNearlyEqual(On[Index], Off[Index], 0.0001f);
	}
	TestTrue(TEXT("Identity 0 leaves the kick, the sight and the rhythm as they are"), bSame);
	TestTrue(TEXT("...the SMG its own shimmy and side random"), FMath::IsNearlyEqual(GunfeelKick::PatternSide(EWeaponKind::SMG, 0), IdDef::SMGShimmy)
		&& FMath::IsNearlyEqual(GunfeelKick::SideRandomScale(EWeaponKind::SMG), IdDef::SideRandomSMG));
	TestTrue(TEXT("...the rifle's hook back left"), GunfeelKick::PatternSide(EWeaponKind::Rifle, 29) < 0.f);
	TestTrue(TEXT("...the heave, the bolt-home beat and a sniper's held recovery on"), GunfeelKick::HeaveScale() > 0.f && GunfeelKick::BoltHomeScale() > 0.f
		&& GunfeelKick::SniperRecoilHold(0.5f) > 0.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
