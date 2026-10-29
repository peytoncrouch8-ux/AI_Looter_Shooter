#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/ViewKick.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/WeaponRecoil.h"
#include "Weapons/WeaponTypes.h"
#include "HAL/IConsoleManager.h"

// Gun identity (GUNS.md lane A, 2026-10-28), the first half: every kind its own view kick (none clipping the caps, every pair
// apart), its own sustain cap, its sight shares, its recovery (the six-gun's quick, the sniper's held through the bolt) and the
// launcher's heave. The rhythm, the ADS curve, the settle, the shakes and the master switch are GunfeelIdentityFeelTests.cpp.
// The rules are driven with the shipped defaults (GunfeelKick::Default, KindFeelDefault, the view table); the few live reads
// set the variables they read for their scope and put them back.

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

	/** A base Common's hip kick of Kind (Recoil 1.4, its shipped shape): the numbers GUNS.md's table gives. */
	FViewKick HipKick(EWeaponKind Kind, FRandomStream& Random)
	{
		return ViewKicks::ForShot(Kind, ViewKicks::BaseShotRecoil, 0.f, Random);
	}

	/** The most a stack holds (turn, widening, push) while Kicks play out over Seconds, in millisecond steps. */
	void StackPeaks(TConstArrayView<FViewKick> Kicks, float Seconds, float& Pitch, float& FieldOfView, float& Push)
	{
		FViewKickStack Stack;
		for (const FViewKick& Kick : Kicks)
		{
			Stack.Add(Kick);
		}
		Pitch = FieldOfView = Push = 0.f;
		for (float Time = 0.f; Time < Seconds; Time += 0.001f)
		{
			Stack.Tick(0.001f);
			Pitch = FMath::Max(Pitch, FMath::Abs(Stack.GetRotation().Pitch));
			FieldOfView = FMath::Max(FieldOfView, FMath::Abs(Stack.GetFieldOfView()));
			Push = FMath::Max(Push, FMath::Abs(Stack.GetPush()));
		}
	}

	/** The most the shot kicks of a stack hold (GetShotPitch) while Kick is added every Interval seconds for Seconds. */
	float BurstPeak(const FViewKick& Kick, float Interval, float Seconds)
	{
		FViewKickStack Stack;
		float Peak = 0.f;
		float NextShot = 0.f;
		for (float Time = 0.f; Time < Seconds; Time += 0.002f)
		{
			if (Time >= NextShot)
			{
				Stack.Add(Kick);
				NextShot += Interval;
			}
			Stack.Tick(0.002f);
			Peak = FMath::Max(Peak, Stack.GetShotPitch());
		}
		return Peak;
	}

	/** Whether A and B differ by at least a fifth of the larger (both nothing: no difference). */
	bool Differs(float A, float B)
	{
		const float Larger = FMath::Max(FMath::Abs(A), FMath::Abs(B));
		return Larger > KINDA_SMALL_NUMBER && FMath::Abs(A - B) / Larger >= 0.2f - 0.0005f;
	}

	/** The aim a recoil has put on (degrees of pitch) as it plays Seconds after a shot of Profile, held Hold seconds. */
	TArray<float> AimTrace(const FWeaponRecoilProfile& Profile, float Hold, float Seconds)
	{
		FRandomStream Random(3);
		FWeaponRecoil Recoil;
		Recoil.AddShot(Profile, Random);
		Recoil.HoldRecovery(Hold);
		TArray<float> Trace;
		float Aim = 0.f;
		for (float Time = 0.f; Time < Seconds; Time += 0.001f)
		{
			Aim += Recoil.Tick(0.001f).Pitch;
			Trace.Add(Aim);
		}
		return Trace;
	}

	/** An aim-only profile (no model kick, no side, no random spread on the climb to speak of). */
	FWeaponRecoilProfile AimOnly(float Recovery)
	{
		FWeaponRecoilProfile Profile;
		Profile.KickBack = 0.f;
		Profile.MuzzleFlip = 0.f;
		Profile.MuzzleTwist = 0.f;
		Profile.Roll = 0.f;
		Profile.AimKick = 1.f;
		Profile.AimKickSide = 0.f;
		Profile.AimRecovery = Recovery;
		return Profile;
	}
}

// ---------------------------------------------------------------------------
// The view kicks: no clipping, every kind apart
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentityCapsTest, "Looter.Gunfeel.Identity.Caps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentityCapsTest::RunTest(const FString& Parameters)
{
	// The user's caps stay (3 degrees, 5% wider, 4 cm back): at a base gun no kind's hip shot reaches them, so the heavy three
	// read apart by shape, not by all clipping to the same peak as before. Played out on the stack, with every extra a shot
	// can carry: the rifle's and the SMG's first shot, a cold LMG's dig-in, the launcher's heave.
	FRandomStream Random(21);
	for (const EWeaponKind Kind : AllKinds)
	{
		const float Extra = FMath::Max(GunfeelKick::FirstShotKick(Kind, 0, IdDef::KickFirstShot), 1.f)
			* (Kind == EWeaponKind::LMG ? IdDef::LMGDigInStart : 1.f);
		const FShotKickShape Shape = FShotKickShape::For(Kind);
		const FViewKick Shot = HipKick(Kind, Random) * Extra;
		const FViewKick Heave = ViewKicks::ForHeave(Kind, ViewKicks::BaseShotRecoil, 0.f, Shape) * Extra;
		TestTrue(FString::Printf(TEXT("Kind %d: its kick lifts (%.2f degrees)"), static_cast<int32>(Kind), Shot.Pitch), Shot.Pitch > 0.f);
		float Pitch, FieldOfView, Push;
		StackPeaks(TArray<FViewKick>{ Shot, Heave }, 0.6f, Pitch, FieldOfView, Push);
		TestTrue(FString::Printf(TEXT("Kind %d: the hip turn %.2f stays under 2.9 degrees"), static_cast<int32>(Kind), Pitch), Pitch <= 2.9f);
		TestTrue(FString::Printf(TEXT("Kind %d: the widening %.3f under 4.8%%"), static_cast<int32>(Kind), FieldOfView), FieldOfView <= 0.048f);
		TestTrue(FString::Printf(TEXT("Kind %d: the push %.2f cm under 3.8"), static_cast<int32>(Kind), Push), Push <= 3.8f);
	}
	TestEqual(TEXT("The caps are the user's: 3 degrees"), FViewKickStack::MaxTurn, 3.f);
	TestEqual(TEXT("...5% wider"), FViewKickStack::MaxFieldOfView, 0.05f);
	TestEqual(TEXT("...4 cm back"), FViewKickStack::MaxPush, 4.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentityDistinctTest, "Looter.Gunfeel.Identity.Distinct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentityDistinctTest::RunTest(const FString& Parameters)
{
	// Every pair of kinds differs by a fifth or more in at least two of: the lift, the widening, the spring's speed, the push.
	FRandomStream Random(8);
	TArray<FViewKick> Kicks;
	for (const EWeaponKind Kind : AllKinds)
	{
		Kicks.Add(HipKick(Kind, Random));
	}
	for (int32 A = 0; A < Kicks.Num(); ++A)
	{
		for (int32 B = A + 1; B < Kicks.Num(); ++B)
		{
			const FViewKick& One = Kicks[A];
			const FViewKick& Two = Kicks[B];
			const int32 Apart = (Differs(One.Pitch, Two.Pitch) ? 1 : 0) + (Differs(One.FieldOfView, Two.FieldOfView) ? 1 : 0)
				+ (Differs(One.Frequency, Two.Frequency) ? 1 : 0) + (Differs(One.Push, Two.Push) ? 1 : 0);
			TestTrue(FString::Printf(TEXT("Kinds %d and %d differ in %d of four"), static_cast<int32>(AllKinds[A]), static_cast<int32>(AllKinds[B]), Apart),
				Apart >= 2);
		}
	}
	// The heavy three are apart by shape: the shotgun lifts most, the launcher widens most and pushes deepest, the sniper
	// rings longest of the three springs' middle.
	const FViewKick& Shotgun = Kicks[4];
	const FViewKick& Sniper = Kicks[5];
	const FViewKick& Launcher = Kicks[6];
	TestTrue(TEXT("The shotgun lifts the most"), Shotgun.Pitch > Sniper.Pitch && Shotgun.Pitch > Launcher.Pitch);
	TestTrue(TEXT("The launcher widens and pushes the most, slowest"), Launcher.FieldOfView > Shotgun.FieldOfView && Launcher.Push > Shotgun.Push
		&& Launcher.Frequency < Shotgun.Frequency);
	// The revolver's wrist rolls clockwise and a little right every pull, not a random way.
	for (int32 Pull = 0; Pull < 6; ++Pull)
	{
		const FViewKick Revolver = HipKick(EWeaponKind::Revolver, Random);
		TestTrue(TEXT("The six-gun rolls the wrist clockwise, every pull"), Revolver.Roll > 0.f && Revolver.Yaw > 0.f);
	}
	// The LMG's roll follows its sway's side.
	FShotKickShape Left = FShotKickShape::For(EWeaponKind::LMG);
	Left.Lean = -1.f;
	FShotKickShape Right = Left;
	Right.Lean = 1.f;
	TestTrue(TEXT("The LMG rolls with its sway"), ViewKicks::ForShot(EWeaponKind::LMG, 1.4f, 0.f, Random, Left).Roll < 0.f
		&& ViewKicks::ForShot(EWeaponKind::LMG, 1.4f, 0.f, Random, Right).Roll > 0.f);
	TestTrue(TEXT("...the sway's side"), GunfeelKick::PatternLean(EWeaponKind::LMG, 2, IdDef::LMGSwayShots) > 0.f
		&& GunfeelKick::PatternLean(EWeaponKind::LMG, 5, IdDef::LMGSwayShots) < 0.f);
	TestEqual(TEXT("No other kind leans"), GunfeelKick::PatternLean(EWeaponKind::Rifle, 2, IdDef::LMGSwayShots), 0.f);
	return true;
}

// ---------------------------------------------------------------------------
// Bursts and the sight
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentitySustainTest, "Looter.Gunfeel.Identity.SustainCaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentitySustainTest::RunTest(const FString& Parameters)
{
	// Each automatic its own cap: the SMG's hum held low, the rifle's in between, the LMG's allowed to dig in.
	const float SMG = GunfeelKick::KindFeelDefault(EWeaponKind::SMG).SustainCap;
	const float Rifle = GunfeelKick::KindFeelDefault(EWeaponKind::Rifle).SustainCap;
	const float LMG = GunfeelKick::KindFeelDefault(EWeaponKind::LMG).SustainCap;
	TestTrue(TEXT("SMG 0.5 < rifle 1.0 < LMG 1.6"), FMath::IsNearlyEqual(SMG, 0.5f) && FMath::IsNearlyEqual(Rifle, 1.f) && FMath::IsNearlyEqual(LMG, 1.6f));
	{
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 1.f);
		FIdentityScopedVariable CapSMG(TEXT("Looter.Gunfeel.Kick.SustainCap.SMG"), SMG);
		FIdentityScopedVariable CapRifle(TEXT("Looter.Gunfeel.Kick.SustainCap.Rifle"), Rifle);
		FIdentityScopedVariable CapLMG(TEXT("Looter.Gunfeel.Kick.SustainCap.LMG"), LMG);
		TestEqual(TEXT("The live cap is the SMG's own"), GunfeelKick::SustainCapFor(EWeaponKind::SMG), SMG, 0.0001f);
		TestEqual(TEXT("...the rifle's"), GunfeelKick::SustainCapFor(EWeaponKind::Rifle), Rifle, 0.0001f);
		TestEqual(TEXT("...the LMG's"), GunfeelKick::SustainCapFor(EWeaponKind::LMG), LMG, 0.0001f);
		TestEqual(TEXT("A gun with no kind has none (KindOf makes it a rifle first)"), GunfeelKick::SustainCapFor(EWeaponKind::None), 0.f);
		for (const EWeaponKind Single : { EWeaponKind::Revolver, EWeaponKind::Shotgun, EWeaponKind::Sniper, EWeaponKind::Launcher })
		{
			TestEqual(TEXT("A single shot has no cap"), GunfeelKick::SustainCapFor(Single), 0.f);
		}
	}
	// Three seconds held at the real rates with the wildest parts (Recoil 2) and every shot given the first shot's extra (and
	// the LMG its cold dig-in): never past the cap, or a lone kick's own size where that is bigger.
	struct FRate { EWeaponKind Kind; float ShotsPerSecond; };
	FRandomStream Random(31);
	for (const FRate& Each : { FRate{ EWeaponKind::SMG, 15.f }, FRate{ EWeaponKind::Rifle, 14.f }, FRate{ EWeaponKind::LMG, 11.f } })
	{
		const GunfeelKick::FKindFeel Feel = GunfeelKick::KindFeelDefault(Each.Kind);
		FViewKick Burst = ViewKicks::ForShot(Each.Kind, 2.f, 0.f, Random)
			* (IdDef::KickFirstShot * (Each.Kind == EWeaponKind::LMG ? IdDef::LMGDigInStart : 1.f));
		Burst.SustainCap = Feel.SustainCap;
		Burst.SustainFloor = IdDef::SustainFloor;
		const float Peak = BurstPeak(Burst, 1.f / Each.ShotsPerSecond, 3.f);
		const float Most = FMath::Max(Feel.SustainCap, Burst.Pitch);
		TestTrue(FString::Printf(TEXT("Kind %d held 3 s: %.2f degrees, at most %.2f"), static_cast<int32>(Each.Kind), Peak, Most), Peak <= Most + 0.01f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentityAimedTest, "Looter.Gunfeel.Identity.Aimed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentityAimedTest::RunTest(const FString& Parameters)
{
	// Through the sight each kind keeps its own share of the view kick, the sniper's scope the least (and the least widening,
	// so it stays readable); the gun model's sight shares are the kind's, the six-gun's flip its character through the irons.
	for (const EWeaponKind Kind : AllKinds)
	{
		const GunfeelKick::FKindFeel Feel = GunfeelKick::KindFeelDefault(Kind);
		FRandomStream SameA(4);
		FRandomStream SameB(4);
		const FViewKick Hip = ViewKicks::ForShot(Kind, 1.4f, 0.f, SameA);
		const FViewKick Sight = ViewKicks::ForShot(Kind, 1.4f, 1.f, SameB);
		TestEqual(FString::Printf(TEXT("Kind %d: the sight keeps its turn share"), static_cast<int32>(Kind)), Sight.Pitch, Hip.Pitch * Feel.AimedTurn, 0.0001f);
		TestEqual(TEXT("...its widening share"), Sight.FieldOfView, Hip.FieldOfView * Feel.AimedFieldOfView, 0.00001f);
		TestEqual(TEXT("...and the same share of its push"), Sight.Push, Hip.Push * Feel.AimedFieldOfView, 0.0001f);
	}
	const GunfeelKick::FKindFeel Sniper = GunfeelKick::KindFeelDefault(EWeaponKind::Sniper);
	for (const EWeaponKind Kind : AllKinds)
	{
		if (Kind != EWeaponKind::Sniper)
		{
			TestTrue(TEXT("The scope keeps the least of the turn and the widening"),
				Sniper.AimedTurn < GunfeelKick::KindFeelDefault(Kind).AimedTurn && Sniper.AimedFieldOfView < GunfeelKick::KindFeelDefault(Kind).AimedFieldOfView);
		}
	}
	const GunfeelKick::FKindFeel Revolver = GunfeelKick::KindFeelDefault(EWeaponKind::Revolver);
	const GunfeelKick::FKindFeel Rifle = GunfeelKick::KindFeelDefault(EWeaponKind::Rifle);
	const GunfeelKick::FKindFeel SMG = GunfeelKick::KindFeelDefault(EWeaponKind::SMG);
	const GunfeelKick::FKindFeel Shotgun = GunfeelKick::KindFeelDefault(EWeaponKind::Shotgun);
	TestTrue(TEXT("The rifle keeps today's sight shares"), FMath::IsNearlyEqual(Rifle.AimedBack, IdDef::AimedBack)
		&& FMath::IsNearlyEqual(Rifle.AimedFlip, IdDef::AimedFlip) && FMath::IsNearlyEqual(Rifle.AimedRoll, IdDef::AimedRoll));
	TestTrue(TEXT("The six-gun flips most through the irons"), Revolver.AimedFlip > Rifle.AimedFlip && Revolver.AimedFlip > Shotgun.AimedFlip);
	TestTrue(TEXT("The SMG flips least"), SMG.AimedFlip < Rifle.AimedFlip);
	TestTrue(TEXT("The shotgun shoves back most"), Shotgun.AimedBack > Rifle.AimedBack && Shotgun.AimedBack > Revolver.AimedBack);
	{
		// The live numbers read their variables.
		FIdentityScopedVariable Identity(TEXT("Looter.Gunfeel.Identity"), 1.f);
		FIdentityScopedVariable Flip(TEXT("Looter.Gunfeel.Recoil.AimedFlip.Revolver"), 0.9f);
		TestEqual(TEXT("A kind's sight flip is its variable"), GunfeelKick::KindFeel(EWeaponKind::Revolver).AimedFlip, 0.9f, 0.0001f);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Recovery, heave, dig-in, rhythm
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentityRecoveryTest, "Looter.Gunfeel.Identity.Recovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentityRecoveryTest::RunTest(const FString& Parameters)
{
	// The revolver (its asset's recovery 9, x its kind's): the sight is back on target within 0.35 s, before the next pull
	// (0.43 s at 140 a minute), and quicker than before.
	const float RevolverAsset = 9.f;
	const float Faster = GunfeelKick::KindFeelDefault(EWeaponKind::Revolver).Recovery;
	TestTrue(TEXT("The six-gun recovers faster than its asset"), Faster > 1.f);
	const TArray<float> Now = AimTrace(AimOnly(RevolverAsset * Faster), 0.f, 0.5f);
	const TArray<float> Before = AimTrace(AimOnly(RevolverAsset), 0.f, 0.5f);
	float Peak = 0.f;
	for (const float Each : Now)
	{
		Peak = FMath::Max(Peak, Each);
	}
	const float Left = Now[349] / FMath::Max(Peak, KINDA_SMALL_NUMBER);
	TestTrue(FString::Printf(TEXT("Six-gun: %.0f%% of the kick left at 0.35 s, under 10%%"), Left * 100.f), Left < 0.1f);
	TestTrue(TEXT("...less than before"), Now[349] < Before[349]);

	// The sniper: held while the bolt is worked (no recovery at all), then the world settles at its own pace.
	TestEqual(TEXT("The hold is the bolt's time"), GunfeelKick::RecoilHold(0.6f, 0.85f, IdDef::SniperHold), 0.6f, 0.0001f);
	TestEqual(TEXT("...at most the bolt's longest"), GunfeelKick::RecoilHold(0.97f, 0.85f, IdDef::SniperHold), 0.85f, 0.0001f);
	TestEqual(TEXT("...none at share 0"), GunfeelKick::RecoilHold(0.6f, 0.85f, 0.f), 0.f);
	const float Hold = 0.85f;
	const TArray<float> Sniper = AimTrace(AimOnly(5.f), Hold, 1.4f);
	const float SniperPeak = Sniper[120];
	// The kick is 0.85-1.15 of the profile's (its random spread).
	TestTrue(TEXT("The kick went in"), SniperPeak > 0.8f);
	TestTrue(FString::Printf(TEXT("Held through the bolt (%.3f at 0.84 s against %.3f)"), Sniper[839], SniperPeak), Sniper[839] >= SniperPeak - 0.001f);
	TestTrue(FString::Printf(TEXT("...then settling (%.2f at 1.35 s)"), Sniper[1349]), Sniper[1349] < SniperPeak * 0.25f);
	const TArray<float> Unheld = AimTrace(AimOnly(5.f), 0.f, 1.4f);
	TestTrue(TEXT("Without the hold it was recovering already"), Unheld[839] < SniperPeak * 0.1f);
	// The next shot clears a hold: after it the aim comes back down past where the second kick took it (the first's held
	// kick recovers too), where a hold kept would leave it up.
	{
		FRandomStream Random(1);
		FWeaponRecoil Recoil;
		Recoil.AddShot(AimOnly(5.f), Random);
		Recoil.HoldRecovery(5.f);
		for (int32 Step = 0; Step < 50; ++Step)
		{
			Recoil.Tick(0.001f);
		}
		Recoil.AddShot(AimOnly(5.f), Random);
		float Aim = 0.f;
		for (int32 Step = 0; Step < 1000; ++Step)
		{
			Aim += Recoil.Tick(0.001f).Pitch;
		}
		TestTrue(FString::Printf(TEXT("A new shot's kick recovers unless held again (%.2f since it)"), Aim), Aim < 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelIdentityHeaveTest, "Looter.Gunfeel.Identity.Heave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelIdentityHeaveTest::RunTest(const FString& Parameters)
{
	// The launcher's tube sags back after its shove: a second kick down, late and slower; no other kind heaves.
	const FShotKickShape Shape = FShotKickShape::For(EWeaponKind::Launcher);
	const FViewKick Heave = ViewKicks::ForHeave(EWeaponKind::Launcher, ViewKicks::BaseShotRecoil, 0.f, Shape);
	TestEqual(TEXT("Half a degree down at a base gun"), Heave.Pitch, -0.5f, 0.0001f);
	TestEqual(TEXT("...0.16 s after the shot"), Heave.Delay, 0.16f, 0.0001f);
	TestEqual(TEXT("...at 6 Hz"), Heave.Frequency, 6.f, 0.0001f);
	for (const EWeaponKind Kind : AllKinds)
	{
		if (Kind != EWeaponKind::Launcher)
		{
			TestTrue(TEXT("No other kind heaves"), ViewKicks::ForHeave(Kind, 1.4f, 0.f, FShotKickShape::For(Kind)).IsNone());
		}
	}
	TestEqual(TEXT("Through the sight it keeps the launcher's turn share"),
		ViewKicks::ForHeave(EWeaponKind::Launcher, 1.4f, 1.f, Shape).Pitch, Heave.Pitch * Shape.AimedTurn, 0.0001f);

	// Played on the stack: nothing until its delay, then down, then settled.
	FViewKickStack Stack;
	Stack.Add(Heave);
	float Before = 0.f;
	float Lowest = 0.f;
	for (float Time = 0.f; Time < 0.6f; Time += 0.001f)
	{
		Stack.Tick(0.001f);
		const float Pitch = Stack.GetRotation().Pitch;
		if (Time < 0.155f)
		{
			Before = FMath::Max(Before, FMath::Abs(Pitch));
		}
		Lowest = FMath::Min(Lowest, Pitch);
	}
	TestTrue(TEXT("Still before its delay"), Before < 0.0001f);
	TestEqual(TEXT("...then half a degree down"), Lowest, -0.5f, 0.02f);
	TestTrue(TEXT("...and settled within 0.6 s"), Stack.IsSettled());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
