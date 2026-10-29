#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/WeaponTypes.h"
#include "HAL/IConsoleManager.h"
#include "UObject/Package.h"

// Gun identity (GUNS.md lane A, 2026-10-28), the second half: the LMG's dig-in, the automatics' rhythm, each kind's ADS
// curve and sight-arrival settle, the heavy kinds' shakes, and the master switch Looter.Gunfeel.Identity giving today's
// numbers back (the kicks, the sight and the recovery are GunfeelIdentityTests.cpp). Driven with the shipped defaults; the
// few live reads set the variables they read for their scope and put them back.

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

	constexpr EWeaponKind AllKinds[] = { EWeaponKind::SMG, EWeaponKind::Rifle, EWeaponKind::LMG, EWeaponKind::Revolver,
		EWeaponKind::Shotgun, EWeaponKind::Sniper, EWeaponKind::Launcher };

	bool IsAutomatic(EWeaponKind Kind)
	{
		return Kind == EWeaponKind::Rifle || Kind == EWeaponKind::SMG || Kind == EWeaponKind::LMG;
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
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 1.f);
		FIdentityScopedVariable StartVar(TEXT("Looter.Gunfeel.LMG.DigInStart"), Start);
		FIdentityScopedVariable EndVar(TEXT("Looter.Gunfeel.LMG.DigInEnd"), End);
		FIdentityScopedVariable FromVar(TEXT("Looter.Gunfeel.LMG.DigInFrom"), From);
		TestEqual(TEXT("The live one reads its variables"), GunfeelKick::LMGDigIn(0.f), Start, 0.0001f);
	}
	{
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 0.f);
		TestEqual(TEXT("Identity 0: no dig-in"), GunfeelKick::LMGDigIn(0.f), 1.f);
	}
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
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 1.f);
		FIdentityScopedVariable LMG(TEXT("Looter.Gunfeel.Cadence.LMG"), GunfeelKick::KindFeelDefault(EWeaponKind::LMG).Cadence);
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
	TestEqual(TEXT("The rifle keeps today's ease"), GunfeelKick::KindFeelDefault(EWeaponKind::Rifle).AimCurve, IdDef::Today::AimCurve);
	{
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 0.f);
		for (const EWeaponKind Kind : AllKinds)
		{
			TestEqual(TEXT("Identity 0: one ease for all"), GunfeelKick::KindFeel(Kind).AimCurve, IdDef::Today::AimCurve);
		}
	}
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
	{
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 0.f);
		TestEqual(TEXT("Identity 0: no settle"), GunfeelKick::KindFeel(EWeaponKind::LMG).AimSettle, 0.f);
	}
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
	{
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 0.f);
		TestEqual(TEXT("Identity 0: no shot shakes"), GunfeelKick::ShotShake(EWeaponKind::Launcher, 0.f, 0.f).Strength, 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentitySwitchTest, "Looter.Gunfeel.Identity.Switch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentitySwitchTest::RunTest(const FString& Parameters)
{
	// Looter.Gunfeel.Identity 0 gives back today's numbers everywhere; 1 (the default) every kind's own.
	FIdentityScopedVariable Pattern(TEXT("Looter.Gunfeel.Recoil.Pattern"), IdDef::RecoilPattern);
	FIdentityScopedVariable Drift(TEXT("Looter.Gunfeel.Recoil.RifleDrift"), IdDef::RifleDrift);
	FIdentityScopedVariable DriftFrom(TEXT("Looter.Gunfeel.Recoil.RifleDriftFrom"), IdDef::RifleDriftFrom);
	FIdentityScopedVariable DriftBack(TEXT("Looter.Gunfeel.Recoil.RifleDriftBack"), IdDef::RifleDriftBack);
	FIdentityScopedVariable DriftBackFrom(TEXT("Looter.Gunfeel.Recoil.RifleDriftBackFrom"), IdDef::RifleDriftBackFrom);
	FIdentityScopedVariable Shimmy(TEXT("Looter.Gunfeel.Recoil.SMGShimmy"), IdDef::SMGShimmy);
	FIdentityScopedVariable Side(TEXT("Looter.Gunfeel.Recoil.SideRandom"), IdDef::SideRandom);
	FIdentityScopedVariable SideSMG(TEXT("Looter.Gunfeel.Recoil.SideRandom.SMG"), IdDef::SideRandomSMG);
	FIdentityScopedVariable Cap(TEXT("Looter.Gunfeel.Kick.SustainCap"), IdDef::SustainCap);
	FIdentityScopedVariable Bloom(TEXT("Looter.Gunfeel.Spread.SMGBloom"), IdDef::SMGBloom);
	FIdentityScopedVariable BloomMax(TEXT("Looter.Gunfeel.Spread.BloomMax"), IdDef::BloomMax);
	FIdentityScopedVariable KickRifle(TEXT("Looter.Gunfeel.Kick.Rifle"), IdDef::KickRifle);
	FIdentityScopedVariable KickLauncher(TEXT("Looter.Gunfeel.Kick.Launcher"), IdDef::KickLauncher);
	TestNotNull(TEXT("The switch exists"), IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Gunfeel.Identity")));
	// Every new number is a live variable: the single ones, and each kind's own at its shipped default.
	for (const TCHAR* Name : { TEXT("LMG.DigInStart"), TEXT("LMG.DigInEnd"), TEXT("LMG.DigInFrom"), TEXT("Shake.LMGMax"),
		TEXT("Recoil.SniperHold"), TEXT("Kick.Heave"), TEXT("Kick.BoltHome"), TEXT("Recoil.SideRandom.SMG"), TEXT("Recoil.RifleDriftBack"),
		TEXT("Recoil.RifleDriftBackFrom") })
	{
		TestNotNull(*FString::Printf(TEXT("Looter.Gunfeel.%s exists"), Name), IConsoleManager::Get().FindConsoleVariable(*(FString(TEXT("Looter.Gunfeel.")) + Name)));
	}
	const TCHAR* KindNames[] = { TEXT("SMG"), TEXT("Rifle"), TEXT("LMG"), TEXT("Revolver"), TEXT("Shotgun"), TEXT("Sniper"), TEXT("Launcher") };
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(AllKinds)); ++Index)
	{
		const EWeaponKind Kind = AllKinds[Index];
		const GunfeelKick::FKindFeel Feel = GunfeelKick::KindFeelDefault(Kind);
		TArray<TPair<FString, float>> Own = {
			{ TEXT("Kick.AimedTurn."), Feel.AimedTurn }, { TEXT("Kick.AimedFov."), Feel.AimedFieldOfView },
			{ TEXT("Recoil.AimedBack."), Feel.AimedBack }, { TEXT("Recoil.AimedFlip."), Feel.AimedFlip }, { TEXT("Recoil.AimedRoll."), Feel.AimedRoll },
			{ TEXT("Recoil.Recovery."), Feel.Recovery }, { TEXT("Aim.Curve."), Feel.AimCurve }, { TEXT("Aim.Settle."), Feel.AimSettle } };
		if (IsAutomatic(Kind))
		{
			Own.Add({ TEXT("Cadence."), Feel.Cadence });
			Own.Add({ TEXT("Kick.SustainCap."), Feel.SustainCap });
		}
		if (Feel.Shake > 0.f)
		{
			Own.Add({ TEXT("Shake."), Feel.Shake });
		}
		for (const TPair<FString, float>& Each : Own)
		{
			const FString Name = FString(TEXT("Looter.Gunfeel.")) + Each.Key + KindNames[Index];
			const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*Name);
			if (TestNotNull(*FString::Printf(TEXT("%s exists"), *Name), Variable))
			{
				TestNearlyEqual(*FString::Printf(TEXT("%s ships at its default"), *Name), Variable->GetFloat(), Each.Value, 0.0001f);
			}
		}
		if (Feel.Shake > 0.f)
		{
			TestNotNull(TEXT("...and its shake's seconds"), IConsoleManager::Get().FindConsoleVariable(
				*FString::Printf(TEXT("Looter.Gunfeel.Shake.%sSeconds"), KindNames[Index])));
		}
	}
	{
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 0.f);
		TestFalse(TEXT("0 is off"), GunfeelKick::IdentityOn());
		TestEqual(TEXT("Today's rifle weight on today's table"), GunfeelKick::ViewKickScale(EWeaponKind::Rifle), IdDef::Today::KickRifle, 0.0001f);
		TestEqual(TEXT("...and the launcher's"), GunfeelKick::ViewKickScale(EWeaponKind::Launcher), IdDef::Today::KickLauncher, 0.0001f);
		FRandomStream Random(6);
		TestEqual(TEXT("Today's table: a rifle 0.32 at Recoil 1"), ViewKicks::ForShotToday(EWeaponKind::Rifle, 1.f, 0.f, Random).Pitch, 0.32f, 0.0001f);
		TestEqual(TEXT("...held to 1.5x"), ViewKicks::ForShotToday(EWeaponKind::Shotgun, 9.f, 0.f, Random).Pitch, 1.15f * 1.5f, 0.0001f);
		TestEqual(TEXT("...half the turn through the sight"), ViewKicks::ForShotToday(EWeaponKind::Sniper, 1.f, 1.f, Random).Pitch, 1.3f * 0.5f, 0.0001f);
		TestEqual(TEXT("...and no push"), ViewKicks::ForShotToday(EWeaponKind::Launcher, 1.f, 0.f, Random).Push, 0.f);
		for (const EWeaponKind Kind : AllKinds)
		{
			const GunfeelKick::FKindFeel Feel = GunfeelKick::KindFeel(Kind);
			TestTrue(TEXT("One sight share, ease, recovery; no rhythm, settle or shake"),
				FMath::IsNearlyEqual(Feel.AimedTurn, 0.5f) && FMath::IsNearlyEqual(Feel.AimedFieldOfView, 0.6f) && FMath::IsNearlyEqual(Feel.AimCurve, 2.f)
				&& FMath::IsNearlyEqual(Feel.Recovery, 1.f) && Feel.Cadence == 0.f && Feel.AimSettle == 0.f && Feel.Shake == 0.f);
			TestEqual(TEXT("...one cap for the automatics"), GunfeelKick::SustainCapFor(Kind), IsAutomatic(Kind) ? IdDef::SustainCap : 0.f, 0.0001f);
			TestEqual(TEXT("...no cadence"), GunfeelKick::CadenceScale(Kind, 3, 1.f), 1.f);
		}
		TestEqual(TEXT("The SMG keeps the shared side random"), GunfeelKick::SideRandomScale(EWeaponKind::SMG), IdDef::SideRandom, 0.0001f);
		TestEqual(TEXT("...today's shimmy"), GunfeelKick::PatternSide(EWeaponKind::SMG, 0), IdDef::Today::SMGShimmy, 0.0001f);
		TestEqual(TEXT("...and today's bloom"), GunfeelKick::BloomAfterShot(EWeaponKind::SMG, 0.f, 0.f), IdDef::Today::SMGBloom, 0.0001f);
		TestEqual(TEXT("The rifle drifts right and stays"), GunfeelKick::PatternSide(EWeaponKind::Rifle, 30), IdDef::RifleDrift, 0.0001f);
		TestEqual(TEXT("No dig-in"), GunfeelKick::LMGDigIn(0.f), 1.f);
		TestEqual(TEXT("No held recovery"), GunfeelKick::SniperRecoilHold(0.8f), 0.f);
		TestEqual(TEXT("No heave"), GunfeelKick::HeaveScale(), 0.f);
		TestEqual(TEXT("No bolt-home beat"), GunfeelKick::BoltHomeScale(), 0.f);
		TestEqual(TEXT("No shot shake"), GunfeelKick::ShotShake(EWeaponKind::Shotgun, 0.f, 0.f).Strength, 0.f);
	}
	{
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 1.f);
		TestTrue(TEXT("1 is on"), GunfeelKick::IdentityOn());
		TestEqual(TEXT("The table holds the weight: the scale is 1"), GunfeelKick::ViewKickScale(EWeaponKind::Rifle), 1.f, 0.0001f);
		TestEqual(TEXT("The SMG's own side random"), GunfeelKick::SideRandomScale(EWeaponKind::SMG), IdDef::SideRandomSMG, 0.0001f);
		TestEqual(TEXT("...its own shimmy"), GunfeelKick::PatternSide(EWeaponKind::SMG, 0), IdDef::SMGShimmy, 0.0001f);
		TestEqual(TEXT("...its own bloom"), GunfeelKick::BloomAfterShot(EWeaponKind::SMG, 0.f, 0.f), IdDef::SMGBloom, 0.0001f);
		TestTrue(TEXT("The rifle's S comes back left"), GunfeelKick::PatternSide(EWeaponKind::Rifle, 30) < 0.f);
		TestTrue(TEXT("The heave and the bolt-home beat are on"), GunfeelKick::HeaveScale() > 0.f && GunfeelKick::BoltHomeScale() > 0.f);
		TestTrue(TEXT("A sniper's recovery holds"), GunfeelKick::SniperRecoilHold(0.5f) > 0.f);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
