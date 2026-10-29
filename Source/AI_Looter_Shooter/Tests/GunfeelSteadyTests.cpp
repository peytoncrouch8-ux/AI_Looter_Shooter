#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Weapons/GunKindsTuning.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponRecoil.h"
#include "Weapons/WeaponTypes.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

// Playtest round 2 (the user, 2026-10-27): more kick on every gun, calmed a lot by a high accuracy stat (note 28), and every
// gun's idle sway through its sights, smaller than the sniper's, steadied by a crouch (note 29). The rules are driven with
// explicit numbers or the shipped defaults; the variables a test reads are set to their defaults for it and put back after.

namespace
{
	namespace KickDef = GunfeelKick::Default;
	namespace KindDef = GunKinds::Default;

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

	/** The accuracy-recoil variables at their shipped numbers for a test's scope. */
	struct FShippedAccuracyKick
	{
		FScopedVariable More{ TEXT("Looter.Gunfeel.Recoil.More"), KickDef::RecoilMore };
		FScopedVariable Calm{ TEXT("Looter.Gunfeel.Recoil.AccuracyCalm"), KickDef::AccuracyCalm };
		FScopedVariable Least{ TEXT("Looter.Gunfeel.Recoil.AccuracyCalmLeast"), KickDef::AccuracyCalmLeast };
		FScopedVariable Most{ TEXT("Looter.Gunfeel.Recoil.AccuracyCalmMost"), KickDef::AccuracyCalmMost };
	};

	/** A rolled rifle of Rarity from a fixed seed, parts as the seed picks them. */
	FWeaponInstanceData MakeRifle(UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Seed)
	{
		FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Definition, Rarity, 1);
		Gun.Seed = Seed;
		Gun.Parts.Reset();
		return Gun;
	}
}

// ---------------------------------------------------------------------------
// The variables
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSteadyVariablesTest, "Looter.Gunfeel.Steady.Variables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSteadyVariablesTest::RunTest(const FString& Parameters)
{
	const TPair<const TCHAR*, float> Variables[] = {
		{ TEXT("Recoil.More"), KickDef::RecoilMore }, { TEXT("Recoil.AccuracyCalm"), KickDef::AccuracyCalm },
		{ TEXT("Recoil.AccuracyCalmLeast"), KickDef::AccuracyCalmLeast }, { TEXT("Recoil.AccuracyCalmMost"), KickDef::AccuracyCalmMost },
		{ TEXT("Sway.Rifle"), KindDef::SwayRifle }, { TEXT("Sway.Shotgun"), KindDef::SwayShotgun },
		{ TEXT("Sway.Revolver"), KindDef::SwayRevolver }, { TEXT("Sway.SMG"), KindDef::SwaySMG }, { TEXT("Sway.LMG"), KindDef::SwayLMG },
		{ TEXT("Sway.Launcher"), KindDef::SwayLauncher }, { TEXT("Sway.Rate"), KindDef::SwayRate }, { TEXT("Sway.Crouch"), KindDef::SwayCrouch },
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

	// Read live: a change in play shows on the next shot / the next frame.
	{
		FScopedVariable More(TEXT("Looter.Gunfeel.Recoil.More"), 2.f);
		TestNearlyEqual(TEXT("Recoil.More is read live"), GunfeelKick::AccuracyKickNumbers().More, 2.f, 0.0001f);
	}
	{
		FScopedVariable Sway(TEXT("Looter.Gunfeel.Sway.LMG"), 0.5f);
		TestNearlyEqual(TEXT("Sway.LMG is read live"), GunKinds::AimSway(EWeaponKind::LMG), 0.5f, 0.0001f);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Recoil by accuracy
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSteadyAccuracyTest, "Looter.Gunfeel.Steady.AccuracyCalm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSteadyAccuracyTest::RunTest(const FString& Parameters)
{
	const GunfeelKick::FAccuracyKick Shipped;
	// A gun at its kind's base accuracy kicks as its Recoil stat says, times the "more kick" for all.
	TestNearlyEqual(TEXT("Base accuracy: no calm"), GunfeelKick::AccuracyCalm(2.f, 2.f, Shipped), 1.f, 0.0001f);
	TestNearlyEqual(TEXT("...and the kick is the stat x More"), GunfeelKick::ShotRecoil(1.f, 2.f, 2.f, Shipped), KickDef::RecoilMore, 0.0001f);
	TestTrue(TEXT("Every gun kicks harder than before"), KickDef::RecoilMore > 1.f);

	// The stat clearly matters: +30% accuracy about two thirds the kick, twice as tight about a third.
	TestNearlyEqual(TEXT("+30% accuracy: 1.3^-1.5"), GunfeelKick::AccuracyCalm(1.3f, 1.f, Shipped), FMath::Pow(1.3f, -1.5f), 0.001f);
	TestNearlyEqual(TEXT("Twice as tight: 2^-1.5"), GunfeelKick::AccuracyCalm(2.f, 1.f, Shipped), FMath::Pow(2.f, -1.5f), 0.001f);
	TestTrue(TEXT("A high accuracy calms the kick a lot (under half at 1.6x)"), GunfeelKick::AccuracyCalm(1.6f, 1.f, Shipped) < 0.5f);
	TestTrue(TEXT("A sloppy roll kicks harder than base"), GunfeelKick::AccuracyCalm(1.f, 1.25f, Shipped) > 1.f);

	// Always calmer as the gun tightens, both ends held.
	float Last = TNumericLimits<float>::Max();
	for (float Ratio = 0.4f; Ratio <= 4.f; Ratio += 0.1f)
	{
		const float Calm = GunfeelKick::AccuracyCalm(Ratio, 1.f, Shipped);
		if (Calm > Last + 0.0001f || Calm < Shipped.Least - 0.0001f || Calm > Shipped.Most + 0.0001f)
		{
			AddError(FString::Printf(TEXT("The calm at %.1fx accuracy is %.3f (last %.3f)"), Ratio, Calm, Last));
			break;
		}
		Last = Calm;
	}
	TestNearlyEqual(TEXT("A pinpoint gun still kicks the floor's share"), GunfeelKick::AccuracyCalm(10.f, 1.f, Shipped), Shipped.Least, 0.0001f);
	TestNearlyEqual(TEXT("The worst roll is held to the ceiling"), GunfeelKick::AccuracyCalm(1.f, 10.f, Shipped), Shipped.Most, 0.0001f);
	TestNearlyEqual(TEXT("No spread known: no calm"), GunfeelKick::AccuracyCalm(0.f, 1.f, Shipped), 1.f, 0.0001f);
	TestNearlyEqual(TEXT("...either way"), GunfeelKick::AccuracyCalm(1.f, 0.f, Shipped), 1.f, 0.0001f);
	GunfeelKick::FAccuracyKick Off = Shipped;
	Off.Exponent = 0.f;
	TestNearlyEqual(TEXT("AccuracyCalm 0: accuracy leaves the kick alone"), GunfeelKick::AccuracyCalm(2.f, 1.f, Off), 1.f, 0.0001f);
	TestNearlyEqual(TEXT("The Recoil stat still scales it"), GunfeelKick::ShotRecoil(0.5f, 2.f, 1.f, Shipped),
		0.5f * KickDef::RecoilMore * FMath::Pow(2.f, -1.5f), 0.001f);

	// The live ones, by a definition's base spread; the card's number leaves out the "more" so a plain gun reads its stat.
	FShippedAccuracyKick Live;
	UWeaponDefinition* Definition = NewObject<UWeaponDefinition>();
	Definition->BaseStats.Spread = 2.f;
	FWeaponStats Tight;
	Tight.Spread = 1.f;
	Tight.Recoil = 1.f;
	TestNearlyEqual(TEXT("Live: the definition's spread is the base"), GunfeelKick::ShotRecoil(Definition, Tight),
		KickDef::RecoilMore * FMath::Pow(2.f, -1.5f), 0.001f);
	TestNearlyEqual(TEXT("Live: no definition, no calm"), GunfeelKick::ShotRecoil(nullptr, Tight), KickDef::RecoilMore, 0.001f);
	TestNearlyEqual(TEXT("The card's recoil: the stat x the calm"), GunfeelKick::FeltRecoil(Definition, Tight), FMath::Pow(2.f, -1.5f), 0.001f);
	FWeaponStats Plain = Tight;
	Plain.Spread = 2.f;
	TestNearlyEqual(TEXT("...a plain gun reads its stat"), GunfeelKick::FeltRecoil(Definition, Plain), 1.f, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSteadyProfileTest, "Looter.Gunfeel.Steady.Profile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSteadyProfileTest::RunTest(const FString& Parameters)
{
	// On real guns: the kick in the hand and on the aim is the definition's profile x ShotRecoil, so a more accurate rifle of
	// the same make kicks less for the same Recoil stat.
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	FTestWorldWrapper WorldWrapper;
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	FShippedAccuracyKick Live;
	AWeaponBase* Common = UWeaponRollLibrary::SpawnWeapon(World, MakeRifle(Rifle, EWeaponRarity::Common, 4242), FTransform::Identity);
	AWeaponBase* Legendary = UWeaponRollLibrary::SpawnWeapon(World, MakeRifle(Rifle, EWeaponRarity::Legendary, 4242), FTransform::Identity);
	if (!TestNotNull(TEXT("A Common rifle"), Common) || !TestNotNull(TEXT("A Legendary rifle"), Legendary)
		|| !TestTrue(TEXT("The rifle's profile kicks the aim"), Rifle->Recoil.AimKick > 0.f))
	{
		return false;
	}
	for (const AWeaponBase* Gun : { Common, Legendary })
	{
		const float Shot = GunfeelKick::ShotRecoil(Gun);
		TestNearlyEqual(TEXT("The aim's kick is the profile's x ShotRecoil"), Gun->GetRecoilProfile().AimKick, Rifle->Recoil.AimKick * Shot, 0.0001f);
		TestNearlyEqual(TEXT("ShotRecoil is the stat x More x the calm"), Shot,
			Gun->GetStats().Recoil * KickDef::RecoilMore * GunfeelKick::AccuracyCalm(Rifle->BaseStats.Spread, Gun->GetStats().Spread,
				GunfeelKick::FAccuracyKick()), 0.0001f);
		// Gun identity (2026-10-28): the aim comes back at the kind's pace on the asset's, the model's sight shares are the
		// kind's (the rifle's are today's), and nothing but an LMG digs in.
		const GunfeelKick::FKindFeel Feel = GunfeelKick::KindFeel(EWeaponKind::Rifle);
		const FWeaponRecoilProfile Profile = Gun->GetRecoilProfile();
		TestNearlyEqual(TEXT("The aim's recovery is the asset's x the kind's"), Profile.AimRecovery, Rifle->Recoil.AimRecovery * Feel.Recovery, 0.0001f);
		TestNearlyEqual(TEXT("...its sight flip the kind's"), Profile.AimedFlip, Feel.AimedFlip, 0.0001f);
		TestNearlyEqual(TEXT("...its sight push back the kind's"), Profile.AimedBack, Feel.AimedBack, 0.0001f);
		TestNearlyEqual(TEXT("A rifle doesn't dig in"), Gun->GetKickDigIn(), 1.f, 0.0001f);
		TestNearlyEqual(TEXT("...nor hold its recovery"), Gun->GetRecoilHoldSeconds(), 0.f, 0.0001f);
	}
	// Per point of Recoil stat: the tighter of the two kicks less.
	auto PerStat = [Rifle](const AWeaponBase* Gun) { return Gun->GetRecoilProfile().AimKick / FMath::Max(Gun->GetStats().Recoil * Rifle->Recoil.AimKick, 0.0001f); };
	const AWeaponBase* Tighter = Common->GetStats().Spread < Legendary->GetStats().Spread ? Common : Legendary;
	const AWeaponBase* Looser = Tighter == Common ? Legendary : Common;
	if (Looser->GetStats().Spread > Tighter->GetStats().Spread * 1.05f)
	{
		TestTrue(TEXT("The more accurate rifle kicks less"), PerStat(Tighter) < PerStat(Looser));
	}
	return true;
}

// ---------------------------------------------------------------------------
// Every gun's sway
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelSteadySwayTest, "Looter.Gunfeel.Steady.Sway",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelSteadySwayTest::RunTest(const FString& Parameters)
{
	// Each kind sways, every other gun less than the scope; the scope keeps its own numbers.
	const float Defaults[] = { KindDef::SwayRifle, KindDef::SwayShotgun, KindDef::SwayRevolver, KindDef::SwaySMG, KindDef::SwayLMG,
		KindDef::SwayLauncher };
	for (const float Each : Defaults)
	{
		TestTrue(TEXT("A gun's sway is there, and smaller than the scope's"), Each > 0.f && Each < KindDef::SniperSway);
	}
	{
		FScopedVariable Scope(TEXT("Looter.Gunfeel.Sniper.Sway"), KindDef::SniperSway);
		FScopedVariable ScopeRate(TEXT("Looter.Gunfeel.Sniper.SwayRate"), KindDef::SniperSwayRate);
		FScopedVariable Rate(TEXT("Looter.Gunfeel.Sway.Rate"), KindDef::SwayRate);
		FScopedVariable Rifle(TEXT("Looter.Gunfeel.Sway.Rifle"), KindDef::SwayRifle);
		TestNearlyEqual(TEXT("The sniper sways as its scope always has"), GunKinds::AimSway(EWeaponKind::Sniper), KindDef::SniperSway, 0.0001f);
		TestNearlyEqual(TEXT("...at its own pace"), GunKinds::AimSwayRate(EWeaponKind::Sniper), KindDef::SniperSwayRate, 0.0001f);
		TestNearlyEqual(TEXT("The others at theirs"), GunKinds::AimSwayRate(EWeaponKind::Rifle), KindDef::SwayRate, 0.0001f);
		TestNearlyEqual(TEXT("A gun with no kind sways as the rifle"), GunKinds::AimSway(EWeaponKind::None), KindDef::SwayRifle, 0.0001f);
		TestTrue(TEXT("The rifle sways less than the scope"), GunKinds::AimSway(EWeaponKind::Rifle) < GunKinds::AimSway(EWeaponKind::Sniper));
	}

	// A rifle's sway stays within its own amplitude and moves.
	for (float Time = 0.f; Time < 6.f; Time += 0.1f)
	{
		const FRotator Sway = GunKinds::SwayAt(Time, KindDef::SwayRifle, KindDef::SwayRate);
		if (FMath::Abs(Sway.Yaw) > KindDef::SwayRifle + 0.0001f || FMath::Abs(Sway.Pitch) > KindDef::SwayRifle + 0.0001f)
		{
			AddError(FString::Printf(TEXT("The rifle's sway past its amplitude at %.1f s: %s"), Time, *Sway.ToCompactString()));
			break;
		}
	}
	TestFalse(TEXT("The rifle's sway moves"), GunKinds::SwayAt(0.f, KindDef::SwayRifle, KindDef::SwayRate)
		.Equals(GunKinds::SwayAt(0.7f, KindDef::SwayRifle, KindDef::SwayRate), 0.01f));

	// A crouch steadies it; the scope's held breath and a crouch stack.
	TestNearlyEqual(TEXT("Standing: the sway whole"), GunKinds::SwayShare(1.f, false, KindDef::SwayCrouch), 1.f, 0.0001f);
	TestNearlyEqual(TEXT("Crouched: steadier"), GunKinds::SwayShare(1.f, true, KindDef::SwayCrouch), KindDef::SwayCrouch, 0.0001f);
	TestTrue(TEXT("...a real difference"), KindDef::SwayCrouch < 1.f);
	TestNearlyEqual(TEXT("Held breath and a crouch together"), GunKinds::SwayShare(KindDef::SniperBreathSteady, true, KindDef::SwayCrouch),
		KindDef::SniperBreathSteady * KindDef::SwayCrouch, 0.0001f);
	TestNearlyEqual(TEXT("Never below nothing"), GunKinds::SwayShare(-1.f, false, KindDef::SwayCrouch), 0.f, 0.0001f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
