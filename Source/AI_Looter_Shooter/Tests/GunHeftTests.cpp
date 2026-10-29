#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Weapons/GunHeftTuning.h"
#include "Weapons/WeaponTypes.h"
#include "HAL/IConsoleManager.h"

// Gun identity, heft (GUNS.md lane B): the weight per kind, the look lag, the bob and breath, the jump and landing, the sight's
// arrival push, and the master switch that gives back today's feel. The rules are driven with explicit weights or the shipped
// defaults; the variables a test reads are set to their defaults for it and put back after.

namespace
{
	namespace Def = GunHeft::Default;

	/** A console variable, set to Value for the scope and put back after (a variable that doesn't exist is left alone). */
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

	/** Every weight and the master scale at its shipped value, and the identity switch on. */
	struct FShippedHeft
	{
		FShippedHeft()
			: Identity(TEXT("Looter.Gunfeel.Identity"), 1.f)
			, Scale(TEXT("Looter.Gunfeel.Heft.Scale"), Def::Scale)
			, Revolver(TEXT("Looter.Gunfeel.Heft.Revolver"), Def::WeightRevolver)
			, SMG(TEXT("Looter.Gunfeel.Heft.SMG"), Def::WeightSMG)
			, Rifle(TEXT("Looter.Gunfeel.Heft.Rifle"), Def::WeightRifle)
			, Shotgun(TEXT("Looter.Gunfeel.Heft.Shotgun"), Def::WeightShotgun)
			, Sniper(TEXT("Looter.Gunfeel.Heft.Sniper"), Def::WeightSniper)
			, Launcher(TEXT("Looter.Gunfeel.Heft.Launcher"), Def::WeightLauncher)
			, LMG(TEXT("Looter.Gunfeel.Heft.LMG"), Def::WeightLMG)
		{
		}
		FScopedVariable Identity, Scale, Revolver, SMG, Rifle, Shotgun, Sniper, Launcher, LMG;
	};

	/** The fraction of a lag still showing t seconds after the mouse stopped, at a return speed (the interp's exponential). */
	float LagLeft(float Interp, float Seconds)
	{
		return FMath::Exp(-Interp * Seconds);
	}
}

// ---------------------------------------------------------------------------
// The table
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunHeftTableTest, "Looter.Gunfeel.Heft.Table",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunHeftTableTest::RunTest(const FString& Parameters)
{
	// The shipped order, light to heavy: revolver < SMG < rifle < shotgun < sniper < launcher < LMG.
	TestTrue(TEXT("revolver lighter than SMG"), Def::WeightRevolver < Def::WeightSMG);
	TestTrue(TEXT("SMG lighter than rifle"), Def::WeightSMG < Def::WeightRifle);
	TestTrue(TEXT("rifle lighter than shotgun"), Def::WeightRifle < Def::WeightShotgun);
	TestTrue(TEXT("shotgun lighter than sniper"), Def::WeightShotgun < Def::WeightSniper);
	TestTrue(TEXT("sniper lighter than launcher"), Def::WeightSniper < Def::WeightLauncher);
	TestTrue(TEXT("launcher lighter than LMG"), Def::WeightLauncher < Def::WeightLMG);
	TestNearlyEqual(TEXT("the rifle is the reference, 1"), Def::WeightRifle, 1.f, 0.0001f);

	// The variables ship at those numbers and are what the rules read.
	const FShippedHeft Shipped;
	const TPair<const TCHAR*, float> Variables[] = {
		{ TEXT("Heft.Scale"), Def::Scale }, { TEXT("Heft.Revolver"), Def::WeightRevolver }, { TEXT("Heft.SMG"), Def::WeightSMG },
		{ TEXT("Heft.Rifle"), Def::WeightRifle }, { TEXT("Heft.Shotgun"), Def::WeightShotgun }, { TEXT("Heft.Sniper"), Def::WeightSniper },
		{ TEXT("Heft.Launcher"), Def::WeightLauncher }, { TEXT("Heft.LMG"), Def::WeightLMG },
		{ TEXT("Move.Revolver"), Def::MoveRevolver }, { TEXT("Move.SMG"), Def::MoveSMG }, { TEXT("Move.Sniper"), Def::MoveSniper },
		{ TEXT("Move.LMG"), Def::MoveLMG }, { TEXT("Move.Launcher"), Def::MoveLauncher },
	};
	for (const TPair<const TCHAR*, float>& Each : Variables)
	{
		const FString Name = FString(TEXT("Looter.Gunfeel.")) + Each.Key;
		IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*Name);
		if (TestNotNull(*FString::Printf(TEXT("%s exists"), *Name), Variable))
		{
			TestNearlyEqual(*FString::Printf(TEXT("%s ships at its default"), *Name), Variable->GetFloat(), Each.Value, 0.0001f);
		}
	}

	const EWeaponKind LightToHeavy[] = { EWeaponKind::Revolver, EWeaponKind::SMG, EWeaponKind::Rifle, EWeaponKind::Shotgun,
		EWeaponKind::Sniper, EWeaponKind::Launcher, EWeaponKind::LMG };
	float Previous = 0.f;
	for (const EWeaponKind Kind : LightToHeavy)
	{
		const float Weight = GunHeft::Weight(Kind);
		TestTrue(*FString::Printf(TEXT("kind %d is heavier than the one before it"), static_cast<int32>(Kind)), Weight > Previous);
		Previous = Weight;
	}
	TestNearlyEqual(TEXT("the live LMG weight"), GunHeft::For(EWeaponKind::LMG).Weight, Def::WeightLMG, 0.0001f);
	TestNearlyEqual(TEXT("the live revolver weight"), GunHeft::For(EWeaponKind::Revolver).Weight, Def::WeightRevolver, 0.0001f);
	TestNearlyEqual(TEXT("no kind: weight 1"), GunHeft::Weight(EWeaponKind::None), 1.f, 0.0001f);

	// The scale pulls every weight toward 1 (0: none) or pushes it away.
	{
		FScopedVariable Half(TEXT("Looter.Gunfeel.Heft.Scale"), 0.5f);
		TestNearlyEqual(TEXT("half scale: the LMG halfway to 1"), GunHeft::Weight(EWeaponKind::LMG), 1.f + (Def::WeightLMG - 1.f) * 0.5f, 0.0001f);
		TestNearlyEqual(TEXT("half scale: the revolver halfway to 1"), GunHeft::Weight(EWeaponKind::Revolver), 1.f + (Def::WeightRevolver - 1.f) * 0.5f, 0.0001f);
	}
	{
		FScopedVariable None(TEXT("Looter.Gunfeel.Heft.Scale"), 0.f);
		TestNearlyEqual(TEXT("scale 0: the LMG weighs 1"), GunHeft::Weight(EWeaponKind::LMG), 1.f, 0.0001f);
		TestNearlyEqual(TEXT("scale 0: no arrival push"), GunHeft::For(EWeaponKind::LMG).ArrivalPush, 0.f, 0.0001f);
	}

	// Walking speed by kind (Main's call): SMG and revolver quicker, LMG, launcher and sniper slower, the rest as they were.
	TestNearlyEqual(TEXT("SMG +5%"), GunHeft::MoveScale(EWeaponKind::SMG), 1.05f, 0.0001f);
	TestNearlyEqual(TEXT("revolver +5%"), GunHeft::MoveScale(EWeaponKind::Revolver), 1.05f, 0.0001f);
	TestNearlyEqual(TEXT("LMG -5%"), GunHeft::MoveScale(EWeaponKind::LMG), 0.95f, 0.0001f);
	TestNearlyEqual(TEXT("launcher -5%"), GunHeft::MoveScale(EWeaponKind::Launcher), 0.95f, 0.0001f);
	TestNearlyEqual(TEXT("sniper -3%"), GunHeft::MoveScale(EWeaponKind::Sniper), 0.97f, 0.0001f);
	TestNearlyEqual(TEXT("rifle unchanged"), GunHeft::MoveScale(EWeaponKind::Rifle), 1.f, 0.0001f);
	TestNearlyEqual(TEXT("shotgun unchanged"), GunHeft::MoveScale(EWeaponKind::Shotgun), 1.f, 0.0001f);
	return true;
}

// ---------------------------------------------------------------------------
// The look lag
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunHeftLagTest, "Looter.Gunfeel.Heft.Lag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunHeftLagTest::RunTest(const FString& Parameters)
{
	// Weight 1 is today's lag, to the bit.
	const GunHeft::FGunHeft Today = GunHeft::FromWeight(1.f, 1.f);
	TestEqual(TEXT("gain at weight 1 is today's"), Today.LagGain, 0.012f);
	TestEqual(TEXT("yaw clamp at weight 1 is today's"), Today.LagClampYaw, 4.f);
	TestEqual(TEXT("pitch clamp at weight 1 is today's"), Today.LagClampPitch, 3.f);
	TestEqual(TEXT("return speed at weight 1 is today's"), Today.LagInterp, 9.f);

	// Gain and clamps scale with the weight; the return slows as W^-0.8.
	const GunHeft::FGunHeft Heavy = GunHeft::FromWeight(Def::WeightLMG, 1.f);
	const GunHeft::FGunHeft Light = GunHeft::FromWeight(Def::WeightSMG, 1.f);
	TestNearlyEqual(TEXT("LMG gain = 0.012 x 1.6"), Heavy.LagGain, 0.012f * 1.6f, 0.00001f);
	TestNearlyEqual(TEXT("LMG yaw clamp = 4 x 1.6"), Heavy.LagClampYaw, 4.f * 1.6f, 0.0001f);
	TestNearlyEqual(TEXT("LMG pitch clamp = 3 x 1.6"), Heavy.LagClampPitch, 3.f * 1.6f, 0.0001f);
	TestNearlyEqual(TEXT("LMG return = 9 / 1.6^0.8"), Heavy.LagInterp, 9.f / FMath::Pow(1.6f, 0.8f), 0.001f);
	TestTrue(TEXT("a heavy gun returns slower than a rifle"), Heavy.LagInterp < Today.LagInterp);
	TestTrue(TEXT("a light gun returns quicker than a rifle"), Light.LagInterp > Today.LagInterp);

	// A hard flick (60 deg/s at 60 fps, over any clamp): the LMG trails further than the SMG, and 0.2 s after the mouse stops
	// still shows more of it.
	const float Dt = 1.f / 60.f;
	const float FlickTurn = 60.f * Dt;
	const float LmgTarget = FMath::Clamp(FlickTurn * Heavy.LagGain / Dt, -Heavy.LagClampYaw, Heavy.LagClampYaw);
	const float SmgTarget = FMath::Clamp(FlickTurn * Light.LagGain / Dt, -Light.LagClampYaw, Light.LagClampYaw);
	TestTrue(TEXT("the LMG's lag reaches further than the SMG's"), LmgTarget > SmgTarget);
	TestTrue(TEXT("the LMG still shows more lag 0.2 s after the stop"),
		LmgTarget * LagLeft(Heavy.LagInterp, 0.2f) > SmgTarget * LagLeft(Light.LagInterp, 0.2f));

	// Every heavier weight lags further and returns slower (monotonic).
	float PreviousGain = 0.f;
	float PreviousInterp = 1000.f;
	for (const float W : { 0.6f, 0.7f, 1.f, 1.25f, 1.3f, 1.5f, 1.6f })
	{
		const GunHeft::FGunHeft Each = GunHeft::FromWeight(W, 1.f);
		TestTrue(*FString::Printf(TEXT("weight %.2f lags further than the one before"), W), Each.LagGain > PreviousGain);
		TestTrue(*FString::Printf(TEXT("weight %.2f returns slower than the one before"), W), Each.LagInterp < PreviousInterp);
		PreviousGain = Each.LagGain;
		PreviousInterp = Each.LagInterp;
	}

	// Jump and landing: times the weight.
	TestNearlyEqual(TEXT("jump lag at weight 1 is today's 18"), Today.JumpLag, 18.f, 0.0001f);
	TestNearlyEqual(TEXT("LMG jump lag = 18 x 1.6"), Heavy.JumpLag, 18.f * 1.6f, 0.0001f);
	TestNearlyEqual(TEXT("landing kick at weight 1 is today's"), Today.LandKick, 1.f, 0.0001f);
	TestNearlyEqual(TEXT("LMG landing kick = 1.6"), Heavy.LandKick, 1.6f, 0.0001f);
	TestTrue(TEXT("a light gun sinks less at a landing"), Light.LandKick < 1.f);
	return true;
}

// ---------------------------------------------------------------------------
// Bob, breath and the sight's arrival
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunHeftBobTest, "Looter.Gunfeel.Heft.Bob",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunHeftBobTest::RunTest(const FString& Parameters)
{
	const GunHeft::FGunHeft Today = GunHeft::FromWeight(1.f, 1.f);
	TestEqual(TEXT("bob at weight 1 is today's"), Today.Bob, 1.f);
	TestEqual(TEXT("breath at weight 1 is today's"), Today.Breath, 1.f);

	// Bob x (1 + 0.35 (W - 1)), breath x (1 + 0.4 (W - 1)).
	const GunHeft::FGunHeft Heavy = GunHeft::FromWeight(Def::WeightLMG, 1.f);
	TestNearlyEqual(TEXT("LMG bob = 1 + 0.35 x 0.6"), Heavy.Bob, 1.21f, 0.0001f);
	TestNearlyEqual(TEXT("LMG breath = 1 + 0.4 x 0.6"), Heavy.Breath, 1.24f, 0.0001f);
	const GunHeft::FGunHeft Light = GunHeft::FromWeight(Def::WeightRevolver, 1.f);
	TestNearlyEqual(TEXT("revolver bob = 1 - 0.35 x 0.4"), Light.Bob, 0.86f, 0.0001f);
	TestNearlyEqual(TEXT("revolver breath = 1 - 0.4 x 0.4"), Light.Breath, 0.84f, 0.0001f);
	TestTrue(TEXT("bob stays well under the lag's change (a gentler rule)"), Heavy.Bob < Heavy.Weight);

	// The sight's arrival push: 0.3 cm x W at its peak, 0 before and after, a smooth rise then an ease out, no pop.
	TestNearlyEqual(TEXT("rifle push is 0.3 cm"), Today.ArrivalPush, 0.3f, 0.0001f);
	TestNearlyEqual(TEXT("LMG push is 0.3 x 1.6 cm"), Heavy.ArrivalPush, 0.48f, 0.0001f);
	TestNearlyEqual(TEXT("no push before the sight lands"), GunHeft::ArrivalAt(Heavy, -0.01f), 0.f, 0.00001f);
	TestNearlyEqual(TEXT("no push at the very instant (no pop)"), GunHeft::ArrivalAt(Heavy, 0.f), 0.f, 0.00001f);
	TestNearlyEqual(TEXT("no push once 0.15 s have gone"), GunHeft::ArrivalAt(Heavy, Def::ArrivalSeconds), 0.f, 0.00001f);
	TestNearlyEqual(TEXT("no push long after"), GunHeft::ArrivalAt(Heavy, 2.f), 0.f, 0.00001f);

	float Peak = 0.f;
	float PeakAt = 0.f;
	float Last = 0.f;
	bool bFalling = false;
	bool bFallOnly = true;
	for (int32 Step = 0; Step <= 150; ++Step)
	{
		const float T = Def::ArrivalSeconds * static_cast<float>(Step) / 150.f;
		const float Push = GunHeft::ArrivalAt(Heavy, T);
		TestTrue(TEXT("the push is never negative"), Push >= 0.f);
		TestTrue(TEXT("the push never passes its peak value"), Push <= Heavy.ArrivalPush + 0.00001f);
		if (Push > Peak)
		{
			Peak = Push;
			PeakAt = T;
		}
		if (T > PeakAt + 0.0001f && Push > Last + 0.00001f)
		{
			bFallOnly = false;
		}
		bFalling = bFalling || T > PeakAt;
		Last = Push;
	}
	TestNearlyEqual(TEXT("the push reaches its full value"), Peak, Heavy.ArrivalPush, 0.002f);
	TestTrue(TEXT("it peaks early, in the first third"), PeakAt < Def::ArrivalSeconds * 0.34f);
	TestTrue(TEXT("after the peak it only eases out"), bFalling && bFallOnly);

	// Heavier pushes further.
	TestTrue(TEXT("a light gun pushes less than a heavy one"), Light.ArrivalPush < Heavy.ArrivalPush);
	return true;
}

// ---------------------------------------------------------------------------
// The master switch
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunHeftSwitchTest, "Looter.Gunfeel.Heft.Switch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunHeftSwitchTest::RunTest(const FString& Parameters)
{
	const EWeaponKind Kinds[] = { EWeaponKind::Rifle, EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::SMG,
		EWeaponKind::Sniper, EWeaponKind::LMG, EWeaponKind::Launcher };
	const FShippedHeft Shipped;

	// A share of 0 is today's feel to the bit, whatever the weight asked for.
	{
		const GunHeft::FGunHeft Off = GunHeft::FromWeight(Def::WeightLMG, 0.f);
		TestEqual(TEXT("off: weight 1"), Off.Weight, 1.f);
		TestEqual(TEXT("off: lag gain is today's"), Off.LagGain, 0.012f);
		TestEqual(TEXT("off: yaw clamp is today's"), Off.LagClampYaw, 4.f);
		TestEqual(TEXT("off: pitch clamp is today's"), Off.LagClampPitch, 3.f);
		TestEqual(TEXT("off: return speed is today's"), Off.LagInterp, 9.f);
		TestEqual(TEXT("off: bob is today's"), Off.Bob, 1.f);
		TestEqual(TEXT("off: breath is today's"), Off.Breath, 1.f);
		TestEqual(TEXT("off: jump lag is today's"), Off.JumpLag, 18.f);
		TestEqual(TEXT("off: landing kick is today's"), Off.LandKick, 1.f);
		TestEqual(TEXT("off: no arrival push"), Off.ArrivalPush, 0.f);
		TestEqual(TEXT("off: the push is 0 at any time"), GunHeft::ArrivalAt(Off, 0.03f), 0.f);
	}

	// Looter.Gunfeel.Identity 0 gives that for every kind, and the walking speed as it was.
	{
		FScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 0.f);
		if (Identity.Variable)
		{
			for (const EWeaponKind Kind : Kinds)
			{
				const GunHeft::FGunHeft Heft = GunHeft::For(Kind);
				const FString Which = FString::Printf(TEXT("identity 0, kind %d"), static_cast<int32>(Kind));
				TestEqual(*(Which + TEXT(": weight 1")), Heft.Weight, 1.f);
				TestEqual(*(Which + TEXT(": lag gain is today's")), Heft.LagGain, 0.012f);
				TestEqual(*(Which + TEXT(": return speed is today's")), Heft.LagInterp, 9.f);
				TestEqual(*(Which + TEXT(": bob is today's")), Heft.Bob, 1.f);
				TestEqual(*(Which + TEXT(": breath is today's")), Heft.Breath, 1.f);
				TestEqual(*(Which + TEXT(": jump lag is today's")), Heft.JumpLag, 18.f);
				TestEqual(*(Which + TEXT(": landing kick is today's")), Heft.LandKick, 1.f);
				TestEqual(*(Which + TEXT(": no arrival push")), Heft.ArrivalPush, 0.f);
				TestEqual(*(Which + TEXT(": walking speed is today's")), GunHeft::MoveScale(Kind), 1.f);
			}
		}
		else
		{
			AddInfo(TEXT("Looter.Gunfeel.Identity isn't defined in this build: the identity-0 half was skipped."));
		}
	}

	// Heft.Scale 0 does the same for the weight and the push (the walking speed is Move.* and stays).
	{
		FScopedVariable Scale(TEXT("Looter.Gunfeel.Heft.Scale"), 0.f);
		for (const EWeaponKind Kind : Kinds)
		{
			const GunHeft::FGunHeft Heft = GunHeft::For(Kind);
			TestEqual(TEXT("scale 0: weight 1"), Heft.Weight, 1.f);
			TestEqual(TEXT("scale 0: lag gain is today's"), Heft.LagGain, 0.012f);
			TestEqual(TEXT("scale 0: no arrival push"), Heft.ArrivalPush, 0.f);
		}
	}

	// And with the switch on again, the kinds differ.
	TestTrue(TEXT("on: the LMG lags further than the revolver"),
		GunHeft::For(EWeaponKind::LMG).LagGain > GunHeft::For(EWeaponKind::Revolver).LagGain);
	TestTrue(TEXT("on: the LMG pushes the sight further than the revolver"),
		GunHeft::For(EWeaponKind::LMG).ArrivalPush > GunHeft::For(EWeaponKind::Revolver).ArrivalPush);
	return true;
}

#endif
