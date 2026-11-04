#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/ViewKick.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponRecoil.h"
#include "Weapons/WeaponTypes.h"
#include "UObject/UObjectGlobals.h"

// A burst that climbs, one kick layer (RECOIL, audit change 3, 2026-11-03; Docs/Work/2026-11-03-Phase3/RECOIL.md). The trigger
// held on a base Common as UPlayerViewComponent does it (FWeaponRecoil fed the shot profile GunfeelKick::ShotProfile builds, the
// pattern's side kick, the view's stacking lift under the kind's cap), with the shipped numbers and the guns' own asset
// profiles, frame by frame at a real frame rate. No hand on the mouse: nothing pulls down.

namespace
{
	namespace BurstDef = GunfeelKick::Default;

	/** A gun's asset recoil (its DA_ data asset's Recoil, read off the .uasset files 2026-11-03; the rifle's are the struct's own). */
	FWeaponRecoilProfile AssetRecoil(float KickBack, float Flip, float Twist, float Roll, float AimKick, float Side, float Recovery, float Snap)
	{
		FWeaponRecoilProfile Profile;
		Profile.KickBack = KickBack;
		Profile.MuzzleFlip = Flip;
		Profile.MuzzleTwist = Twist;
		Profile.Roll = Roll;
		Profile.AimKick = AimKick;
		Profile.AimKickSide = Side;
		Profile.AimRecovery = Recovery;
		Profile.Snappiness = Snap;
		return Profile;
	}

	FWeaponRecoilProfile RifleAsset() { return AssetRecoil(2.2f, 2.5f, 0.8f, 1.5f, 0.35f, 0.12f, 7.f, 1.2f); }
	FWeaponRecoilProfile SMGAsset() { return AssetRecoil(1.6f, 1.6f, 0.6f, 1.f, 0.22f, 0.14f, 8.f, 1.6f); }
	FWeaponRecoilProfile ShotgunAsset() { return AssetRecoil(7.f, 11.f, 2.f, 4.f, 2.2f, 0.35f, 4.5f, 0.65f); }
	FWeaponRecoilProfile RevolverAsset() { return AssetRecoil(3.6f, 8.f, 1.f, 2.2f, 1.f, 0.15f, 9.f, 1.5f); }
	FWeaponRecoilProfile SniperAsset() { return AssetRecoil(5.f, 9.f, 1.2f, 2.5f, 1.6f, 0.2f, 5.f, 1.1f); }

	/** What a held burst did: the aim's climb, the view's lift, the drift, and how it settled after. */
	struct FBurst
	{
		/** The highest the aim and the view's lift together took the line the shots fly, the burst's last round included. */
		float Climb = 0.f;
		/** ...of which the aim alone (the part the player pulls down against). */
		float AimClimb = 0.f;
		/** The aim after ten rounds had gone in. */
		float AimAfterTen = 0.f;
		/** The most the stacked lifts held (FViewKickStack::GetShotPitch). */
		float LiftMost = 0.f;
		/** The aim's most to the right, and the most either way over the first five rounds. */
		float Right = 0.f;
		float EarlyYaw = 0.f;
		/** The aim's highest, and what was left of it and of the lift Settle seconds after the last round. */
		float AimPeak = 0.f;
		float AimLeft = 0.f;
		float LiftLeft = 0.f;
	};

	/** Shots of a base Common of Kind (Asset) held at Rpm and Fps, then Settle seconds let go. */
	FBurst Hold(EWeaponKind Kind, const FWeaponRecoilProfile& Asset, float Rpm, int32 Shots, float Fps, float Settle)
	{
		const FWeaponRecoilProfile Profile = GunfeelKick::ShotProfile(Asset, 1.f, GunfeelKick::ShippedProfileNumbers(Kind));
		const GunfeelKick::FPatternNumbers Pattern;
		const FShotKickShape Shape = FShotKickShape::For(Kind);
		FViewKick Lift = ViewKicks::ForLift(Kind, ViewKicks::BaseShotRecoil, 0.f, Shape) * BurstDef::LiftKick;
		Lift.SustainCap = GunfeelKick::KindFeelDefault(Kind).SustainCap;
		Lift.SustainFloor = BurstDef::SustainFloor;

		FWeaponRecoil Recoil;
		FViewKickStack View;
		FRandomStream Random(0x5eed);
		const float Interval = 60.f / Rpm;
		const float Dt = 1.f / Fps;
		FBurst Burst;
		float Time = 0.f;
		// The fire timer carries its overshoot (audit change 9): each round is due one interval after the last was due.
		float NextShot = 0.f;
		float LastShot = 0.f;
		int32 Fired = 0;
		float Pitch = 0.f;
		float Yaw = 0.f;
		bool bSettled = false;
		while (!bSettled)
		{
			Time += Dt;
			while (Fired < Shots && NextShot <= Time)
			{
				if (Fired == 10)
				{
					Burst.AimAfterTen = Pitch;
				}
				Recoil.AddShot(Profile, Random, 0.f);
				Recoil.AddAimYaw(GunfeelKick::PatternSide(Kind, Recoil.GetShotInBurst(), Pattern));
				if (!Lift.IsNone())
				{
					View.Add(Lift);
				}
				++Fired;
				NextShot += Interval;
				LastShot = Time;
			}
			const FRotator Change = Recoil.Tick(Dt);
			View.Tick(Dt);
			Pitch += static_cast<float>(Change.Pitch);
			Yaw += static_cast<float>(Change.Yaw);
			const float Held = View.GetShotPitch();
			Burst.AimPeak = FMath::Max(Burst.AimPeak, Pitch);
			Burst.LiftMost = FMath::Max(Burst.LiftMost, Held);
			Burst.Right = FMath::Max(Burst.Right, Yaw);
			if (Fired <= 5)
			{
				Burst.EarlyYaw = FMath::Max(Burst.EarlyYaw, FMath::Abs(Yaw));
			}
			// The burst's climb: until its last round has gone in (a tenth of a second after it, before the recovery starts).
			if (Fired < Shots || Time <= LastShot + 0.1f)
			{
				Burst.Climb = FMath::Max(Burst.Climb, Pitch + Held);
				Burst.AimClimb = FMath::Max(Burst.AimClimb, Pitch);
			}
			if (Fired >= Shots && Time >= LastShot + Settle)
			{
				Burst.AimLeft = Pitch;
				Burst.LiftLeft = Held;
				bSettled = true;
			}
		}
		return Burst;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelBurstRifleTest, "Looter.Gunfeel.Burst.RifleClimb",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelBurstRifleTest::RunTest(const FString& Parameters)
{
	// The numbers this test holds the rifle with are DA_AssaultRifle's own (the struct's defaults for aim kick, side and
	// recovery, which the asset doesn't change).
	if (const UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle")))
	{
		const FWeaponRecoilProfile Asset = RifleAsset();
		TestNearlyEqual(TEXT("DA_AssaultRifle's aim kick is the test's"), Rifle->Recoil.AimKick, Asset.AimKick, 0.001f);
		TestNearlyEqual(TEXT("...its side kick"), Rifle->Recoil.AimKickSide, Asset.AimKickSide, 0.001f);
		TestNearlyEqual(TEXT("...its recovery"), Rifle->Recoil.AimRecovery, Asset.AimRecovery, 0.001f);
		TestNearlyEqual(TEXT("...its flip"), Rifle->Recoil.MuzzleFlip, Asset.MuzzleFlip, 0.001f);
	}
	else
	{
		AddInfo(TEXT("DA_AssaultRifle didn't load: the asset's numbers weren't checked."));
	}

	// A round of a base Common kicks the aim about 0.8 degrees (0.35 x 1.4 = 0.49 before).
	const FWeaponRecoilProfile Shot = GunfeelKick::ShotProfile(RifleAsset(), 1.f, GunfeelKick::ShippedProfileNumbers(EWeaponKind::Rifle));
	TestTrue(FString::Printf(TEXT("A round kicks the aim %.2f degrees, about 0.8"), Shot.AimKick), Shot.AimKick > 0.75f && Shot.AimKick < 0.85f);
	TestNearlyEqual(TEXT("...barely recovering while held"), Shot.RecoveryWhileFiring, BurstDef::RecoveryWhileFiring, 0.0001f);
	TestTrue(TEXT("...past the round's interval before it recovers fully"), Shot.RecoveryDelay > 60.f / 650.f);

	// A held 30-round magazine, at 120, 60 and 30 frames a second: it climbs 5-8 degrees and wants pulling down (most of it
	// in the first second), drifts right past the hip cone after straight-up first rounds, and the view's lift stacks under
	// its cap.
	const float HipCone = 0.8f;
	for (const float Fps : { 120.f, 60.f, 30.f })
	{
		const FBurst Burst = Hold(EWeaponKind::Rifle, RifleAsset(), 650.f, 30, Fps, 0.5f);
		TestTrue(FString::Printf(TEXT("%.0f fps: 30 rounds climb %.2f degrees, 5-8"), Fps, Burst.Climb), Burst.Climb >= 5.f && Burst.Climb <= 8.f);
		TestTrue(FString::Printf(TEXT("%.0f fps: the aim's own part %.2f, 4.5-7.5"), Fps, Burst.AimClimb), Burst.AimClimb >= 4.5f && Burst.AimClimb <= 7.5f);
		TestTrue(FString::Printf(TEXT("%.0f fps: ten rounds in it is up %.2f and still climbing"), Fps, Burst.AimAfterTen),
			Burst.AimAfterTen > 3.f && Burst.AimAfterTen < Burst.AimClimb - 0.5f);
		TestTrue(FString::Printf(TEXT("%.0f fps: the first five rounds go straight up (%.2f sideways)"), Fps, Burst.EarlyYaw), Burst.EarlyYaw <= 0.6f);
		TestTrue(FString::Printf(TEXT("%.0f fps: then it drifts right %.2f degrees, outside the %.1f hip cone"), Fps, Burst.Right, HipCone),
			Burst.Right >= HipCone * 1.5f);
		const float Cap = GunfeelKick::KindFeelDefault(EWeaponKind::Rifle).SustainCap;
		TestTrue(FString::Printf(TEXT("%.0f fps: the view's lift stacks to %.2f, under its %.1f cap"), Fps, Burst.LiftMost, Cap),
			Burst.LiftMost >= 0.6f && Burst.LiftMost <= Cap + 0.01f);
		TestTrue(FString::Printf(TEXT("%.0f fps: let go, it comes back (%.2f left after 0.5 s)"), Fps, Burst.AimLeft), Burst.AimLeft < Burst.AimPeak * 0.1f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelBurstTapTest, "Looter.Gunfeel.Burst.Tap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelBurstTapTest::RunTest(const FString& Parameters)
{
	// A 3-round tap settles within about 0.3 s: the aim back to a tenth of its kick, the view's lift gone.
	for (const float Fps : { 120.f, 60.f })
	{
		const FBurst Tap = Hold(EWeaponKind::Rifle, RifleAsset(), 650.f, 3, Fps, 0.3f);
		TestTrue(FString::Printf(TEXT("%.0f fps: the tap kicks the aim %.2f degrees"), Fps, Tap.AimPeak), Tap.AimPeak > 1.5f);
		TestTrue(FString::Printf(TEXT("%.0f fps: 0.3 s after it %.0f%% is left, under 10%%"), Fps, 100.f * Tap.AimLeft / FMath::Max(Tap.AimPeak, KINDA_SMALL_NUMBER)),
			Tap.AimLeft <= Tap.AimPeak * 0.1f);
		TestTrue(FString::Printf(TEXT("%.0f fps: the view's lift all but gone (%.3f)"), Fps, Tap.LiftLeft),
			Tap.LiftLeft <= GunfeelKick::KindFeelDefault(EWeaponKind::Rifle).SustainCap * 0.1f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelBurstKindsTest, "Looter.Gunfeel.Burst.Kinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelBurstKindsTest::RunTest(const FString& Parameters)
{
	// The kinds stay apart: a round's aim kick runs shotgun, sniper, revolver, rifle, SMG (the single shots' as before: their
	// asset's x 1.4), and a hosed SMG magazine climbs about half what the rifle's does (its cone opens instead).
	auto AimKick = [](EWeaponKind Kind, const FWeaponRecoilProfile& Asset)
	{
		return GunfeelKick::ShotProfile(Asset, 1.f, GunfeelKick::ShippedProfileNumbers(Kind)).AimKick;
	};
	const float Shotgun = AimKick(EWeaponKind::Shotgun, ShotgunAsset());
	const float Sniper = AimKick(EWeaponKind::Sniper, SniperAsset());
	const float Revolver = AimKick(EWeaponKind::Revolver, RevolverAsset());
	const float Rifle = AimKick(EWeaponKind::Rifle, RifleAsset());
	const float SMG = AimKick(EWeaponKind::SMG, SMGAsset());
	TestTrue(FString::Printf(TEXT("Aim kicks: shotgun %.2f > sniper %.2f > revolver %.2f > rifle %.2f > SMG %.2f"), Shotgun, Sniper, Revolver, Rifle, SMG),
		Shotgun > Sniper && Sniper > Revolver && Revolver > Rifle && Rifle > SMG);
	TestNearlyEqual(TEXT("The shotgun's blast kicks as before (2.2 x 1.4)"), Shotgun, 2.2f * 1.4f, 0.001f);
	TestNearlyEqual(TEXT("...the six-gun's (1.0 x 1.4)"), Revolver, 1.4f, 0.001f);

	const FBurst SMGBurst = Hold(EWeaponKind::SMG, SMGAsset(), 900.f, 40, 120.f, 0.3f);
	const FBurst RifleBurst = Hold(EWeaponKind::Rifle, RifleAsset(), 650.f, 30, 120.f, 0.3f);
	TestTrue(FString::Printf(TEXT("A 40-round SMG magazine climbs %.2f degrees, 2-5"), SMGBurst.Climb), SMGBurst.Climb >= 2.f && SMGBurst.Climb <= 5.f);
	TestTrue(TEXT("...less than the rifle's"), SMGBurst.Climb < RifleBurst.Climb);
	TestTrue(FString::Printf(TEXT("...its hum's lift held under its own cap (%.2f)"), SMGBurst.LiftMost),
		SMGBurst.LiftMost <= GunfeelKick::KindFeelDefault(EWeaponKind::SMG).SustainCap + 0.01f);
	TestTrue(TEXT("A single-shot kind has no lift"), ViewKicks::ForLift(EWeaponKind::Shotgun, 1.4f, 0.f, FShotKickShape::For(EWeaponKind::Shotgun)).IsNone()
		&& ViewKicks::ForLift(EWeaponKind::Revolver, 1.4f, 0.f, FShotKickShape::For(EWeaponKind::Revolver)).IsNone()
		&& ViewKicks::ForLift(EWeaponKind::Sniper, 1.4f, 0.f, FShotKickShape::For(EWeaponKind::Sniper)).IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelBurstModelTest, "Looter.Gunfeel.Burst.Model",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelBurstModelTest::RunTest(const FString& Parameters)
{
	// The gun model: the six-gun's flip big at the hip, about 5 degrees through the irons; the shotgun's aimed shove stopped
	// short of the eye.
	auto Peaks = [](const FWeaponRecoilProfile& Profile, float AimAlpha, float& Flip, float& Back)
	{
		FRandomStream Random(4);
		FWeaponRecoil Recoil;
		Recoil.AddShot(Profile, Random, AimAlpha);
		Flip = 0.f;
		Back = 0.f;
		for (int32 Step = 0; Step < 300; ++Step)
		{
			Recoil.Tick(0.002f);
			Flip = FMath::Max(Flip, Recoil.GetKickRotation().Pitch);
			Back = FMath::Max(Back, Recoil.GetKickBack());
		}
	};
	const FWeaponRecoilProfile Revolver = GunfeelKick::ShotProfile(RevolverAsset(), 1.f, GunfeelKick::ShippedProfileNumbers(EWeaponKind::Revolver));
	float HipFlip, HipBack, SightFlip, SightBack;
	Peaks(Revolver, 0.f, HipFlip, HipBack);
	Peaks(Revolver, 1.f, SightFlip, SightBack);
	TestTrue(FString::Printf(TEXT("The six-gun flips %.1f degrees at the hip"), HipFlip), HipFlip > 9.f);
	TestTrue(FString::Printf(TEXT("...%.1f through the irons, about 5 (11 before)"), SightFlip), SightFlip > 4.4f && SightFlip < 5.6f);

	FWeaponRecoilProfile Shotgun = GunfeelKick::ShotProfile(ShotgunAsset(), 1.f, GunfeelKick::ShippedProfileNumbers(EWeaponKind::Shotgun));
	float Unused, FreeBack, HipShove;
	Peaks(Shotgun, 1.f, Unused, FreeBack);
	// The eye 22 cm behind the sight (ADS, SIGHTS round 3): the shove stops at AimedBackGap of it.
	const float Gap = 22.f;
	Shotgun.AimedBackMost = Gap * BurstDef::AimedBackGap;
	float LimitedBack;
	Peaks(Shotgun, 1.f, Unused, LimitedBack);
	Peaks(Shotgun, 0.f, Unused, HipShove);
	TestTrue(FString::Printf(TEXT("Unlimited, the aimed shove would be %.1f cm, most of the way to the eye"), FreeBack), FreeBack > Gap * BurstDef::AimedBackGap);
	TestTrue(FString::Printf(TEXT("Limited, %.1f cm: under %.0f%% of the %.0f cm gap"), LimitedBack, BurstDef::AimedBackGap * 100.f, Gap),
		LimitedBack <= Gap * BurstDef::AimedBackGap * 1.02f && LimitedBack < Gap);
	TestTrue(FString::Printf(TEXT("...the hip's shove untouched (%.1f cm)"), HipShove), FMath::IsNearlyEqual(HipShove, ShotgunAsset().KickBack * BurstDef::RecoilShotgun, 0.3f));
	TestEqual(TEXT("No sight measured: no limit"), GunfeelKick::AimedBackMost(0.f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelBurstMoreOnceTest, "Looter.Gunfeel.Burst.MoreOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelBurstMoreOnceTest::RunTest(const FString& Parameters)
{
	// Recoil.More is applied once: a base Common's share is 1, and a bigger More (or Recoil stat, or a looser roll) scales the
	// aim, the gun model, the pattern and the view alike.
	GunfeelKick::FAccuracyKick Shipped;
	TestNearlyEqual(TEXT("A base Common's share is 1"), GunfeelKick::KickShare(GunfeelKick::ShotRecoil(1.f, 0.8f, 0.8f, Shipped)), 1.f, 0.0001f);
	GunfeelKick::FAccuracyKick More = Shipped;
	More.More = 2.f;
	const float Share = GunfeelKick::KickShare(GunfeelKick::ShotRecoil(1.f, 0.8f, 0.8f, More));
	const float Ratio = 2.f / BurstDef::RecoilMore;
	TestNearlyEqual(TEXT("More 2: the share is 2 / 1.4"), Share, Ratio, 0.0001f);
	const GunfeelKick::FProfileNumbers Numbers = GunfeelKick::ShippedProfileNumbers(EWeaponKind::Rifle);
	const FWeaponRecoilProfile Base = GunfeelKick::ShotProfile(RifleAsset(), 1.f, Numbers);
	const FWeaponRecoilProfile Harder = GunfeelKick::ShotProfile(RifleAsset(), Share, Numbers);
	TestNearlyEqual(TEXT("...the aim's kick by it"), Harder.AimKick / Base.AimKick, Ratio, 0.0001f);
	TestNearlyEqual(TEXT("...the gun model's by it (not More twice)"), Harder.MuzzleFlip / Base.MuzzleFlip, Ratio, 0.0001f);
	TestNearlyEqual(TEXT("...the base model's flip is the asset's x Recoil.Rifle only"), Base.MuzzleFlip, RifleAsset().MuzzleFlip * BurstDef::RecoilRifle, 0.0001f);
	FRandomStream RandomA(2);
	FRandomStream RandomB(2);
	const float ViewBase = ViewKicks::ForShot(EWeaponKind::Rifle, BurstDef::RecoilMore, 0.f, RandomA).Pitch;
	const float ViewHarder = ViewKicks::ForShot(EWeaponKind::Rifle, 2.f, 0.f, RandomB).Pitch;
	TestNearlyEqual(TEXT("...the view's by it"), ViewHarder / ViewBase, Ratio, 0.0001f);
	const FShotKickShape Shape = FShotKickShape::For(EWeaponKind::Rifle);
	TestNearlyEqual(TEXT("...and its lift's"), ViewKicks::ForLift(EWeaponKind::Rifle, 2.f, 0.f, Shape).Pitch
		/ ViewKicks::ForLift(EWeaponKind::Rifle, BurstDef::RecoilMore, 0.f, Shape).Pitch, Ratio, 0.0001f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
