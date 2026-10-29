#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/LootDropComponent.h"
#include "Loot/LootTable.h"
#include "Player/FirstPersonHandsGrips.h"
#include "Player/ViewKick.h"
#include "Player/WeaponSwapMotion.h"
#include "Tests/LocomotionTestWorld.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/BulletMarks.h"
#include "Weapons/GunKindsTuning.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponBrass.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponHipHold.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponNotches.h"
#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponRocket.h"
#include "Weapons/WeaponRocketBlast.h"
#include "Weapons/WeaponSounds.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

// The four frontier-tech guns (Phase 12.3, Saved/Loot/CONTRACT.md "GUNS-CODE"): the Rustler SMG, the Longhorn sniper, the
// Thresher LMG and the Powderkeg launcher. Their rules with explicit numbers or the shipped defaults (GunKinds::Default,
// GunfeelKick::Default), every switch on the gun's kind giving them their own (nothing falls to a wrong default), their
// reloads' choreography, their definitions and models once the editor scripts have made them, and the rocket's burst in
// a test level.

namespace
{
	namespace Def = GunKinds::Default;

	const EWeaponKind NewKinds[] = { EWeaponKind::SMG, EWeaponKind::Sniper, EWeaponKind::LMG, EWeaponKind::Launcher };
	const EWeaponReloadPart NewParts[] = { EWeaponReloadPart::Bolt, EWeaponReloadPart::Box, EWeaponReloadPart::Rocket, EWeaponReloadPart::Helix };

	const TCHAR* KindName(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::SMG: return TEXT("SMG");
		case EWeaponKind::Sniper: return TEXT("Sniper");
		case EWeaponKind::LMG: return TEXT("LMG");
		case EWeaponKind::Launcher: return TEXT("Launcher");
		default: return TEXT("old kind");
		}
	}

	/** The definitions Tools/Unreal/create_new_guns.py makes. */
	const TCHAR* DefinitionPath(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::SMG: return TEXT("/Game/Weapons/Data/DA_SMG.DA_SMG");
		case EWeaponKind::Sniper: return TEXT("/Game/Weapons/Data/DA_Sniper.DA_Sniper");
		case EWeaponKind::LMG: return TEXT("/Game/Weapons/Data/DA_LMG.DA_LMG");
		case EWeaponKind::Launcher: return TEXT("/Game/Weapons/Data/DA_Launcher.DA_Launcher");
		default: return TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle");
		}
	}

	bool SameKick(const FViewKick& A, const FViewKick& B)
	{
		return FMath::IsNearlyEqual(A.Pitch, B.Pitch) && FMath::IsNearlyEqual(A.FieldOfView, B.FieldOfView) && FMath::IsNearlyEqual(A.Frequency, B.Frequency);
	}

	bool SameSwap(const FWeaponSwapStyle& A, const FWeaponSwapStyle& B)
	{
		return FMath::IsNearlyEqual(A.HolsterSeconds, B.HolsterSeconds) && A.DownOffset.Equals(B.DownOffset) && A.DownRotation.Equals(B.DownRotation);
	}

	/** A spider standing on the ground at Feet, started as play starts it, dropping nothing (GrenadePlayTests' way). */
	ASpiderCreature* SpawnSpider(UWorld* World, const FVector& Feet)
	{
		ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(Feet + FVector(0.0, 0.0, 300.0), FRotator::ZeroRotator);
		if (!Spider)
		{
			return nullptr;
		}
		if (ULootDropComponent* Loot = Spider->FindComponentByClass<ULootDropComponent>())
		{
			Loot->bDropOnDeath = false;
		}
		Spider->DispatchBeginPlay();
		const float HalfHeight = Spider->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Spider->SetActorLocation(Feet + FVector(0.0, 0.0, HalfHeight + 1.0), false, nullptr, ETeleportType::TeleportPhysics);
		UCharacterMovementComponent* Movement = Spider->GetCharacterMovement();
		Movement->SetUpdatedComponent(Spider->GetCapsuleComponent());
		Movement->SetMovementMode(MOVE_Walking);
		Movement->Activate(true);
		return Spider;
	}
}

// ---------------------------------------------------------------------------
// The variables
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewGunsVariablesTest, "Looter.Weapons.NewGuns.Variables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNewGunsVariablesTest::RunTest(const FString& Parameters)
{
	// Every number that makes the four their own is a live Looter.Gunfeel.* variable at its shipped default.
	const TPair<const TCHAR*, float> Variables[] = {
		{ TEXT("Kick.SMG"), GunfeelKick::Default::KickSMG }, { TEXT("Kick.Sniper"), GunfeelKick::Default::KickSniper },
		{ TEXT("Kick.LMG"), GunfeelKick::Default::KickLMG }, { TEXT("Kick.Launcher"), GunfeelKick::Default::KickLauncher },
		{ TEXT("Recoil.SMG"), GunfeelKick::Default::RecoilSMG }, { TEXT("Recoil.Sniper"), GunfeelKick::Default::RecoilSniper },
		{ TEXT("Recoil.LMG"), GunfeelKick::Default::RecoilLMG }, { TEXT("Recoil.Launcher"), GunfeelKick::Default::RecoilLauncher },
		{ TEXT("Rumble.SMG"), GunfeelKick::Default::RumbleSMG }, { TEXT("Rumble.Launcher"), GunfeelKick::Default::RumbleLauncher },
		{ TEXT("Aim.Time.Sniper"), GunfeelKick::Default::AimTimeSniper }, { TEXT("Aim.Move.LMG"), GunfeelKick::Default::AimMoveLMG },
		{ TEXT("Swap.SMG"), GunfeelKick::Default::SwapSMG }, { TEXT("Crit.Sniper"), GunfeelKick::Default::CritSniper },
		{ TEXT("Recoil.SMGClimb"), GunfeelKick::Default::SMGClimb }, { TEXT("Spread.LMGBloom"), GunfeelKick::Default::LMGBloom },
		{ TEXT("SMG.Glow"), Def::SMGGlow }, { TEXT("SMG.GlowLowShare"), Def::SMGGlowLowShare }, { TEXT("SMG.GlowPulse"), Def::SMGGlowPulse },
		{ TEXT("Sniper.BoltDelay"), Def::SniperBoltDelay }, { TEXT("Sniper.BoltShare"), Def::SniperBoltShare },
		{ TEXT("Sniper.BoltMaxSeconds"), Def::SniperBoltMaxSeconds }, { TEXT("Sniper.AimedSpread"), Def::SniperAimedSpread },
		{ TEXT("Sniper.Sway"), Def::SniperSway }, { TEXT("Sniper.SwayRate"), Def::SniperSwayRate },
		{ TEXT("Sniper.BreathSeconds"), Def::SniperBreathSeconds }, { TEXT("Sniper.BreathSteady"), Def::SniperBreathSteady },
		{ TEXT("Sniper.WindedSeconds"), Def::SniperWindedSeconds }, { TEXT("Sniper.WindedSway"), Def::SniperWindedSway },
		{ TEXT("Sniper.BreathRefill"), Def::SniperBreathRefill },
		{ TEXT("LMG.SpinUp"), Def::LMGSpinUp }, { TEXT("LMG.SpinStart"), Def::LMGSpinStart }, { TEXT("LMG.SpinDown"), Def::LMGSpinDown },
		{ TEXT("LMG.HeatPerShot"), Def::LMGHeatPerShot }, { TEXT("LMG.HeatDelay"), Def::LMGHeatDelay }, { TEXT("LMG.HeatCool"), Def::LMGHeatCool },
		{ TEXT("LMG.HeatGlow"), Def::LMGHeatGlow },
		{ TEXT("Launcher.RocketSpeed"), Def::RocketSpeed }, { TEXT("Launcher.RocketGravity"), Def::RocketGravity },
		{ TEXT("Launcher.RocketLife"), Def::RocketLife }, { TEXT("Launcher.BlastRadius"), Def::BlastRadius },
		{ TEXT("Launcher.BlastCore"), Def::BlastCore }, { TEXT("Launcher.BlastEdgeShare"), Def::BlastEdgeShare },
		{ TEXT("Launcher.SelfScale"), Def::BlastSelfScale }, { TEXT("Launcher.KnockShare"), Def::BlastKnockShare },
		{ TEXT("Kill.SniperBurst"), Def::KillSniperBurst }, { TEXT("Kill.LauncherKnock"), Def::KillLauncherKnock },
		{ TEXT("Kill.LauncherLift"), Def::KillLauncherLift },
	};
	for (const TPair<const TCHAR*, float>& Variable : Variables)
	{
		const FString Name = FString(TEXT("Looter.Gunfeel.")) + Variable.Key;
		const IConsoleVariable* Found = IConsoleManager::Get().FindConsoleVariable(*Name);
		if (TestNotNull(*Name, Found))
		{
			TestEqual(*FString::Printf(TEXT("%s ships at its default"), *Name), Found->GetFloat(), Variable.Value);
		}
	}
	// The contract's start numbers.
	TestEqual(TEXT("The sniper's crits are 2x"), GunfeelKick::Default::CritSniper, 2.f);
	TestEqual(TEXT("The LMG spins up over 0.35 s"), Def::LMGSpinUp, 0.35f);
	TestEqual(TEXT("...from 60% of its rate"), Def::LMGSpinStart, 0.6f);
	TestEqual(TEXT("The LMG's aimed walk is 0.8x"), GunfeelKick::Default::AimMoveLMG, 0.8f);
	TestEqual(TEXT("The rocket flies at 4000 cm/s"), Def::RocketSpeed, 4000.f);
	TestEqual(TEXT("...and bursts 350 cm wide"), Def::BlastRadius, 350.f);
	TestEqual(TEXT("...its shooter taking half"), Def::BlastSelfScale, 0.5f);
	TestEqual(TEXT("A breath holds for 3 s"), Def::SniperBreathSeconds, 3.f);
	TestTrue(TEXT("The SMG swaps quickest of all"), GunfeelKick::Default::SwapSMG < GunfeelKick::Default::SwapRevolver
		&& GunfeelKick::Default::SwapSMG < GunfeelKick::Default::SwapRifle);
	return true;
}

// ---------------------------------------------------------------------------
// The rules
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewGunsRulesTest, "Looter.Weapons.NewGuns.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNewGunsRulesTest::RunTest(const FString& Parameters)
{
	// The SMG's helix: the rounds left lit, steady while there's plenty, pulsing in the last quarter, dark when empty.
	TestEqual(TEXT("Full helix"), GunKinds::AmmoGlowFill(40, 40), 1.f);
	TestEqual(TEXT("Half"), GunKinds::AmmoGlowFill(20, 40), 0.5f);
	TestEqual(TEXT("Empty"), GunKinds::AmmoGlowFill(0, 40), 0.f);
	TestEqual(TEXT("Plenty: steady at its strength"), GunKinds::AmmoGlowStrength(0.6f, 4.f, 0.25f, 5.f, 0.37f), 4.f);
	TestEqual(TEXT("Empty: dark"), GunKinds::AmmoGlowStrength(0.f, 4.f, 0.25f, 5.f, 0.37f), 0.f);
	const float PulseA = GunKinds::AmmoGlowStrength(0.1f, 4.f, 0.25f, 5.f, 0.05f);
	const float PulseB = GunKinds::AmmoGlowStrength(0.1f, 4.f, 0.25f, 5.f, 0.15f);
	TestTrue(TEXT("The last rounds pulse"), !FMath::IsNearlyEqual(PulseA, PulseB) && PulseA >= 2.f - 0.01f && PulseA <= 6.f + 0.01f);

	// The LMG: its rate from 60% when cold to full once spun up, up over 0.35 s of firing, down off the trigger.
	TestEqual(TEXT("Cold: 60% of its rate"), GunKinds::SpinRateShare(0.f, 0.6f), 0.6f);
	TestEqual(TEXT("Spun up: full"), GunKinds::SpinRateShare(1.f, 0.6f), 1.f);
	TestTrue(TEXT("Half way: between"), GunKinds::SpinRateShare(0.5f, 0.6f) > 0.6f && GunKinds::SpinRateShare(0.5f, 0.6f) < 1.f);
	TestEqual(TEXT("Spun up after 0.35 s of firing"), GunKinds::SpinAfter(0.f, 0.35f, true, 0.35f, 0.5f), 1.f);
	TestTrue(TEXT("...half way at half that"), FMath::IsNearlyEqual(GunKinds::SpinAfter(0.f, 0.175f, true, 0.35f, 0.5f), 0.5f, 0.01f));
	TestEqual(TEXT("Spun down off the trigger"), GunKinds::SpinAfter(1.f, 0.5f, false, 0.35f, 0.5f), 0.f);
	TestTrue(TEXT("A quick let-go keeps most of it"), GunKinds::SpinAfter(1.f, 0.1f, false, 0.35f, 0.5f) > 0.7f);

	// Its heat: each shot adds, it's held a moment, then cools; never past its hottest.
	float Heat = 0.f;
	for (int32 Shot = 0; Shot < 50; ++Shot)
	{
		Heat = GunKinds::HeatAfterShot(Heat, 0.012f);
	}
	TestTrue(TEXT("50 shots heat it well"), FMath::IsNearlyEqual(Heat, 0.6f, 0.001f));
	TestEqual(TEXT("Held just after"), GunKinds::HeatCooled(Heat, 0.3f, 0.5f, 0.3f), Heat);
	TestTrue(TEXT("Cooling later"), GunKinds::HeatCooled(Heat, 1.5f, 0.5f, 0.3f) < Heat - 0.29f);
	TestEqual(TEXT("Cold in the end"), GunKinds::HeatCooled(Heat, 10.f, 0.5f, 0.3f), 0.f);
	TestEqual(TEXT("Never past its hottest"), GunKinds::HeatAfterShot(0.995f, 0.012f), 1.f);

	// The sniper's bolt: after the kick, inside the time between shots, so it's always home before the next can go.
	const float Interval = 60.f / 45.f;
	const GunKinds::FBoltTiming Timing = GunKinds::BoltTiming(Interval, Def::SniperBoltDelay, Def::SniperBoltShare, Def::SniperBoltMaxSeconds);
	TestEqual(TEXT("It waits out the kick"), Timing.Start, Def::SniperBoltDelay);
	TestTrue(TEXT("Home before the next shot"), Timing.Start + Timing.Seconds < Interval);
	TestTrue(TEXT("...never longer than its most"), Timing.Seconds <= Def::SniperBoltMaxSeconds + 0.0001f);
	TestEqual(TEXT("Not yet worked during the kick"), GunKinds::BoltCycleProgress(0.05f, Timing), -1.f);
	TestTrue(TEXT("Being worked"), GunKinds::BoltCycleProgress(Timing.Start + Timing.Seconds * 0.5f, Timing) > 0.4f);
	TestEqual(TEXT("Home after"), GunKinds::BoltCycleProgress(Timing.Start + Timing.Seconds + 0.01f, Timing), -1.f);
	const GunKinds::FBoltTiming Fast = GunKinds::BoltTiming(0.3f, 0.12f, 0.7f, 0.85f);
	TestTrue(TEXT("A fast-firing bolt gun still fits its cycle"), Fast.Start + Fast.Seconds <= 0.3f + 0.0001f);
	TestEqual(TEXT("The bolt is home as the cycle starts"), LooterReload::BoltCycleOpen(0.f), 0.f);
	TestTrue(TEXT("...fully back as the case flies"), LooterReload::BoltCycleOpen(LooterReload::BoltCaseOutAt) > 0.99f);
	TestTrue(TEXT("...home again at its end"), LooterReload::BoltCycleOpen(0.999f) < 0.01f);
	float Lift = 0.f;
	float Back = 0.f;
	LooterReload::BoltPose(0.5f, Lift, Back);
	TestTrue(TEXT("Half open: turned up, not yet drawn"), FMath::IsNearlyEqual(Lift, LooterReload::BoltLiftDegrees) && FMath::IsNearlyZero(Back));
	LooterReload::BoltPose(1.f, Lift, Back);
	TestTrue(TEXT("Open: turned up and drawn back"), FMath::IsNearlyEqual(Lift, LooterReload::BoltLiftDegrees)
		&& FMath::IsNearlyEqual(Back, LooterReload::BoltBack));
	TestEqual(TEXT("The trigger hand is on the knob through the cycle"), LooterReload::BoltHandInCycle(0.5f), 1.f);
	TestEqual(TEXT("...and on the grip otherwise"), LooterReload::BoltHandInCycle(-1.f), 0.f);

	// The scope: the sway within its amplitude; the breath steady for its hold, then winded (wider), no holding meanwhile,
	// the breath back after.
	for (float Time = 0.f; Time < 6.f; Time += 0.1f)
	{
		const FRotator Sway = GunKinds::SwayAt(Time, 0.3f, 0.35f);
		if (FMath::Abs(Sway.Yaw) > 0.3001f || FMath::Abs(Sway.Pitch) > 0.3001f)
		{
			AddError(FString::Printf(TEXT("The sway past its amplitude at %.1f s: %s"), Time, *Sway.ToCompactString()));
			break;
		}
	}
	TestFalse(TEXT("The sway moves"), GunKinds::SwayAt(0.f, 0.3f, 0.35f).Equals(GunKinds::SwayAt(0.7f, 0.3f, 0.35f), 0.01f));
	GunKinds::FBreathRules Rules;
	GunKinds::FScopeBreath Breath;
	TestEqual(TEXT("Breathing: the sway whole"), Breath.Step(false, 0.1f, Rules), 1.f);
	TestEqual(TEXT("Held: steadied"), Breath.Step(true, 0.1f, Rules), Rules.Steady);
	TestTrue(TEXT("Holding"), Breath.bHolding);
	float Scale = Rules.Steady;
	for (int32 Tick = 0; Tick < 100 && Breath.Winded <= 0.f; ++Tick)
	{
		Scale = Breath.Step(true, 0.1f, Rules);
	}
	TestEqual(TEXT("Held to the end: winded, the sway wider"), Scale, Rules.WindedSway);
	TestTrue(TEXT("...and no holding while winded"), Breath.Step(true, 0.1f, Rules) > 1.f && !Breath.bHolding);
	for (float Time = 0.f; Time < Rules.WindedSeconds + Rules.HoldSeconds / Rules.Refill + 0.5f; Time += 0.1f)
	{
		Breath.Step(false, 0.1f, Rules);
	}
	TestTrue(TEXT("The breath comes back"), FMath::IsNearlyEqual(Breath.Left(Rules), 1.f) && FMath::IsNearlyZero(Breath.Winded));
	TestEqual(TEXT("...and can be held again"), Breath.Step(true, 0.1f, Rules), Rules.Steady);

	// The sniper's spread: all of its own at the hip (no halving), all but none through the scope; the others' as before.
	TestEqual(TEXT("Sniper at the hip: its whole spread"), GunKinds::AimSpreadScale(EWeaponKind::Sniper, 0.f, 0.5f, 0.04f), 1.f);
	TestEqual(TEXT("Sniper scoped: all but none"), GunKinds::AimSpreadScale(EWeaponKind::Sniper, 1.f, 0.5f, 0.04f), 0.04f);
	TestEqual(TEXT("A rifle aimed: halved as ever"), GunKinds::AimSpreadScale(EWeaponKind::Rifle, 1.f, 0.5f, 0.04f), 0.5f);

	// Crits: the sniper's 2x (its job), the revolver's own, 1.5x for the rest; a cursed iron's own wins when larger.
	TestEqual(TEXT("Sniper crit"), GunfeelKick::CritMultiplier(EWeaponKind::Sniper, 1.f, 1.75f, 2.f), 2.f);
	TestEqual(TEXT("SMG crit: the game's"), GunfeelKick::CritMultiplier(EWeaponKind::SMG, 1.f, 1.75f, 2.f), 1.5f);
	TestEqual(TEXT("A cursed sniper's 3x wins"), GunfeelKick::CritMultiplier(EWeaponKind::Sniper, 3.f, 1.75f, 2.f), 3.f);
	TestEqual(TEXT("The three-number rule gives the sniper its default"), GunfeelKick::CritMultiplier(EWeaponKind::Sniper, 1.f, 1.75f),
		GunfeelKick::Default::CritSniper);

	// Bloom: the automatic guns open up, the single-shot ones never do.
	TestTrue(TEXT("The SMG blooms"), GunfeelKick::BloomAfterShot(EWeaponKind::SMG, 0.f, 0.f, 0.09f, 2.f) > 0.f);
	TestTrue(TEXT("The LMG blooms"), GunfeelKick::BloomAfterShot(EWeaponKind::LMG, 0.f, 0.f, 0.12f, 2.f) > 0.f);
	TestEqual(TEXT("The sniper doesn't"), GunfeelKick::BloomAfterShot(EWeaponKind::Sniper, 0.f, 0.f, 0.12f, 2.f), 0.f);
	TestEqual(TEXT("Nor the launcher"), GunfeelKick::BloomAfterShot(EWeaponKind::Launcher, 0.f, 0.f, 0.12f, 2.f), 0.f);

	// The rocket's burst: full in its heart, down to its edge's share at its reach, nothing past; its shooter half.
	GunKinds::FBlastRules Blast;
	TestEqual(TEXT("Full in the heart"), GunKinds::BlastFalloff(50.f, 80.f, 350.f, 0.2f), 1.f);
	TestTrue(TEXT("Its edge's share at its reach"), FMath::IsNearlyEqual(GunKinds::BlastFalloff(350.f, 80.f, 350.f, 0.2f), 0.2f, 0.001f));
	TestEqual(TEXT("Nothing past it"), GunKinds::BlastFalloff(351.f, 80.f, 350.f, 0.2f), 0.f);
	TestTrue(TEXT("Less the farther"), GunKinds::BlastFalloff(150.f, 80.f, 350.f, 0.2f) > GunKinds::BlastFalloff(250.f, 80.f, 350.f, 0.2f));
	TestEqual(TEXT("A body in the heart takes the rocket's damage"), WeaponRocketBlast::DamageAt(300.f, 0.f, Blast, false), 300.f);
	TestEqual(TEXT("...its shooter half of it"), WeaponRocketBlast::DamageAt(300.f, 0.f, Blast, true), 150.f);

	// Rockets come few: one to three a pickup, a chest's three.
	TestEqual(TEXT("A small kill pickup: one rocket"), LooterLoot::RocketsFor(LooterLoot::KillAmmoAmountMin), 1);
	TestEqual(TEXT("...a big one: three"), LooterLoot::RocketsFor(LooterLoot::KillAmmoAmountMax), 3);
	TestEqual(TEXT("A chest's: three"), LooterLoot::AmmoAmountFor(EAmmoType::Rocket, LooterLoot::ChestAmmoAmount), 3);
	TestEqual(TEXT("Other ammo: the rounds themselves"), LooterLoot::AmmoAmountFor(EAmmoType::SMG, 27), 27);
	for (int32 Rounds = LooterLoot::KillAmmoAmountMin; Rounds <= LooterLoot::KillAmmoAmountMax; ++Rounds)
	{
		const int32 Rockets = LooterLoot::RocketsFor(Rounds);
		if (Rockets < 1 || Rockets > 3)
		{
			AddError(FString::Printf(TEXT("%d rounds make %d rockets"), Rounds, Rockets));
		}
	}
	const LooterAmmo::FInfo& RocketInfo = LooterAmmo::GetInfo(EAmmoType::Rocket);
	TestTrue(TEXT("Rockets are an ammo class of their own, carried few"), LooterAmmo::IsValid(EAmmoType::Rocket) && RocketInfo.MaxCarried > 0
		&& RocketInfo.MaxCarried < LooterAmmo::GetInfo(EAmmoType::Sniper).MaxCarried);
	TestTrue(TEXT("Every class iterated, rockets last"), LooterAmmo::AllTypes().Last() == EAmmoType::Rocket);
	const ULootTable* Table = GetDefault<ULootTable>();
	const FAmmoLootEntry* RocketEntry = Table->AmmoTypes.FindByPredicate([](const FAmmoLootEntry& Entry) { return Entry.Type == EAmmoType::Rocket; });
	TestTrue(TEXT("A new loot table drops rockets, less often than the rest"), RocketEntry && RocketEntry->Weight > 0.f && RocketEntry->Weight < 1.f);
	return true;
}

// ---------------------------------------------------------------------------
// Every switch on the kind
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewGunsKindsTest, "Looter.Weapons.NewGuns.Kinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNewGunsKindsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Saved by name, the old kinds keep their numbers"), static_cast<int32>(EWeaponKind::Revolver), 3);
	TestEqual(TEXT("...the new ones after them"), static_cast<int32>(EWeaponKind::SMG), 4);
	TestEqual(TEXT("Every gun kind listed"), static_cast<int32>(UE_ARRAY_COUNT(AllGunKinds)), 7);

	FRandomStream Random(3);
	const FViewKick RifleKick = ViewKicks::ForShot(EWeaponKind::Rifle, 1.f, 0.f, Random);
	TArray<FName> Icons = { LoadoutParts::GunIconName(EWeaponKind::Rifle), LoadoutParts::GunIconName(EWeaponKind::Shotgun),
		LoadoutParts::GunIconName(EWeaponKind::Revolver) };
	TArray<FString> Nicknames;
	for (const EWeaponKind Kind : NewKinds)
	{
		const FString Name = KindName(Kind);
		// Its own kick, swap, hold, hands, rumble, nicknames and icon, none of them the rifle's by default.
		FRandomStream Same(3);
		TestFalse(*FString::Printf(TEXT("%s: its own view kick"), *Name), SameKick(ViewKicks::ForShot(Kind, 1.f, 0.f, Same), RifleKick));
		TestFalse(*FString::Printf(TEXT("%s: its own swap"), *Name), SameSwap(WeaponSwapMotion::StyleOf(Kind), WeaponSwapMotion::StyleOf(EWeaponKind::Rifle)));
		FVector Grip;
		FRotator Turn;
		TestTrue(*FString::Printf(TEXT("%s: a hip hold"), *Name), WeaponHipHold::For(Kind, Grip, Turn));
		TestEqual(*FString::Printf(TEXT("%s: drawn at the long guns' size"), *Name), UWeaponModelComponent::ScaleFor(Kind), 1.1f);
		TestTrue(*FString::Printf(TEXT("%s: a rumble"), *Name), GunfeelKick::RumbleForShot(Kind).Intensity > 0.f);
		const TConstArrayView<const TCHAR*> Names = WeaponNotches::Nicknames(Kind);
		TestTrue(*FString::Printf(TEXT("%s: nicknames of its own"), *Name), Names.Num() >= 10
			&& Names.GetData() != WeaponNotches::Nicknames(EWeaponKind::Rifle).GetData());
		for (const TCHAR* Nickname : Names)
		{
			Nicknames.Add(Nickname);
		}
		const FName Icon = LoadoutParts::GunIconName(Kind);
		TestFalse(*FString::Printf(TEXT("%s: an icon of its own"), *Name), Icons.Contains(Icon));
		Icons.Add(Icon);
		TestTrue(*FString::Printf(TEXT("%s: its icon drawn"), *Name), LoadoutParts::GunIcon(Kind).ViewBox.X > 0.f
			&& &LoadoutParts::GunIcon(Kind) != &LoadoutParts::GunIcon(EWeaponKind::Rifle));
	}
	TSet<FString> Distinct;
	for (const FString& Nickname : Nicknames)
	{
		Distinct.Add(Nickname);
	}
	TestEqual(TEXT("No nickname twice"), Distinct.Num(), Nicknames.Num());
	TestTrue(TEXT("The rocket's icon"), LoadoutParts::AmmoIcon(EAmmoType::Rocket).ViewBox.X > 0.f
		&& &LoadoutParts::AmmoIcon(EAmmoType::Rocket) != &LoadoutParts::AmmoIcon(EAmmoType::Sniper));

	// The hands: the launcher's support hand round its upright grip, the sniper's bolt, each gun's own reload spots.
	TestTrue(TEXT("The launcher's front grip is wrapped"), FirstPersonHandsGrips::For(EWeaponKind::Launcher).ForeGrip == EHandGrip::Wrap);
	TestFalse(TEXT("The sniper's bolt known"), FirstPersonHandsGrips::For(EWeaponKind::Sniper).BoltPivot.IsZero());
	const FVector Knob = FirstPersonHandsGrips::BoltKnobAt(FirstPersonHandsGrips::For(EWeaponKind::Sniper), 0.f);
	const FVector Drawn = FirstPersonHandsGrips::BoltKnobAt(FirstPersonHandsGrips::For(EWeaponKind::Sniper), 1.f);
	TestTrue(TEXT("Its knob rises and comes back as the bolt opens"), Drawn.Z > Knob.Z + 1.f && Drawn.X < Knob.X - LooterReload::BoltBack + 0.1f);
	TestFalse(TEXT("The launcher holds its next rocket"), FirstPersonHandsGrips::For(EWeaponKind::Launcher).RocketGrab.IsZero());

	// The shot's leavings: the SMG's small holes, the sniper's big ones; bottlenecked cases from the three that throw one.
	TestTrue(TEXT("The SMG's marks are small"), FBulletMarks::GunScale(EWeaponKind::SMG) < FBulletMarks::GunScale(EWeaponKind::Rifle));
	TestTrue(TEXT("The sniper's big"), FBulletMarks::GunScale(EWeaponKind::Sniper) > FBulletMarks::GunScale(EWeaponKind::Revolver));
	// Gun identity (2026-10-28): the SMG, LMG and sniper each throw their own case (the rifle's with the switch off).
	TestTrue(TEXT("The SMG throws small cases"), FWeaponBrass::KindFor(EWeaponKind::SMG, true) == ECasingKind::Small);
	TestTrue(TEXT("The LMG throws belt cases"), FWeaponBrass::KindFor(EWeaponKind::LMG, true) == ECasingKind::Belt);
	TestTrue(TEXT("The sniper throws magnum cases"), FWeaponBrass::KindFor(EWeaponKind::Sniper, true) == ECasingKind::Magnum);
	TestTrue(TEXT("Identity 0: the SMG and LMG throw the rifle's"), FWeaponBrass::KindFor(EWeaponKind::SMG, false) == ECasingKind::Rifle
		&& FWeaponBrass::KindFor(EWeaponKind::LMG, false) == ECasingKind::Rifle);

	// Its gunfeel numbers by kind, the live ones at their defaults (the Variables test checks they ship so).
	TestTrue(TEXT("The LMG's aimed walk the slowest"), GunfeelKick::AimMoveScale(EWeaponKind::LMG) < GunfeelKick::AimMoveScale(EWeaponKind::Rifle));
	TestTrue(TEXT("The SMG's swap the quickest"), GunfeelKick::SwapScale(EWeaponKind::SMG) < GunfeelKick::SwapScale(EWeaponKind::Revolver));
	// Since gun identity (2026-10-28) every Kick.<Kind> is 1 and the kinds differ by their own tables; with the switch off the
	// old scales come back, the launcher's the biggest.
	TestTrue(TEXT("The launcher kicked the view hardest (Identity 0)"), GunfeelKick::Default::Today::KickLauncher > GunfeelKick::Default::Today::KickShotgun);
	TestTrue(TEXT("The SMG and the LMG climb; the sniper doesn't"), GunfeelKick::ClimbPerShot(EWeaponKind::SMG) > 0.f
		&& GunfeelKick::ClimbPerShot(EWeaponKind::LMG) > 0.f && GunfeelKick::ClimbPerShot(EWeaponKind::Sniper) == 0.f);
	return true;
}

// ---------------------------------------------------------------------------
// The reloads
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewGunsReloadTest, "Looter.Weapons.NewGuns.Reload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNewGunsReloadTest::RunTest(const FString& Parameters)
{
	for (const EWeaponReloadPart Part : NewParts)
	{
		const FString Name = UEnum::GetValueAsString(Part);
		// Its steps in order inside the reload, each one heard (a cue named for its kind, or the rifle's).
		const TConstArrayView<FReloadStepAt> Steps = LooterReload::Steps(Part);
		TestTrue(*FString::Printf(TEXT("%s: a choreography"), *Name), Steps.Num() >= 2);
		float Last = 0.f;
		for (const FReloadStepAt& Step : Steps)
		{
			TestTrue(*FString::Printf(TEXT("%s: its steps in order, inside the reload"), *Name), Step.Progress > Last && Step.Progress < 1.f);
			TestFalse(*FString::Printf(TEXT("%s: every step heard"), *Name), WeaponSounds::ReloadStep(Step.Step).IsNone());
			Last = Step.Progress;
		}
		// The gun comes up to be worked and back to its hold.
		FVector Offset;
		FRotator Rotation;
		LooterReload::ViewModelPose(Part, 0.f, Offset, Rotation);
		TestTrue(*FString::Printf(TEXT("%s: from the hold"), *Name), Offset.IsNearlyZero(0.01) && Rotation.IsNearlyZero(0.01f));
		LooterReload::ViewModelPose(Part, 0.5f, Offset, Rotation);
		TestFalse(*FString::Printf(TEXT("%s: worked on"), *Name), Offset.IsNearlyZero(0.5) && Rotation.IsNearlyZero(1.f));
		LooterReload::ViewModelPose(Part, 1.f, Offset, Rotation);
		TestTrue(*FString::Printf(TEXT("%s: back on the hold"), *Name), Offset.IsNearlyZero(0.05) && Rotation.IsNearlyZero(0.1f));
	}

	// The contract's cue names.
	TestEqual(TEXT("Bolt open"), WeaponSounds::ReloadStep(EReloadStep::BoltOpen), FName(TEXT("Weapon.Sniper.BoltOpen")));
	TestEqual(TEXT("Belt laid"), WeaponSounds::ReloadStep(EReloadStep::BeltLay), FName(TEXT("Weapon.LMG.BeltLay")));
	TestEqual(TEXT("Rocket in"), WeaponSounds::ReloadStep(EReloadStep::RocketIn), FName(TEXT("Weapon.Launcher.RocketIn")));
	TestEqual(TEXT("Helix out"), WeaponSounds::ReloadStep(EReloadStep::HelixOut), FName(TEXT("Weapon.SMG.HelixOut")));

	// The helix and the box come out and go in as a magazine does; the sniper's box a little later, after its bolt opens.
	for (float Progress = 0.f; Progress <= 1.f; Progress += 0.02f)
	{
		bool bMagazine = true;
		bool bHelix = true;
		const float Magazine = LooterReload::MagazineTravel(Progress, bMagazine);
		const float Helix = LooterReload::WellTravel(EWeaponReloadPart::Helix, Progress, bHelix);
		if (!FMath::IsNearlyEqual(Magazine, Helix) || bMagazine != bHelix)
		{
			AddError(FString::Printf(TEXT("The helix isn't on the magazine's beats at %.2f"), Progress));
			break;
		}
	}
	bool bShown = true;
	TestEqual(TEXT("The sniper's box is still seated as its bolt opens"), LooterReload::WellTravel(EWeaponReloadPart::Bolt, 0.18f, bShown), 0.f);
	TestTrue(TEXT("The bolt is open by the box's swap"), LooterReload::BoltOpenInReload(0.4f) > 0.99f);
	TestEqual(TEXT("...and home at the end"), LooterReload::BoltOpenInReload(1.f), 0.f);
	TestTrue(TEXT("The trigger hand works the bolt as it opens"), LooterReload::BoltHandInReload(0.1f) > 0.5f);
	TestEqual(TEXT("...not while the box is swapped"), LooterReload::BoltHandInReload(0.45f), 0.f);

	// The launcher's: drawn from the back (not yet seen), in front of the mouth, slid in, seated.
	bool bSeated = false;
	LooterReload::RocketTravel(0.1f, bShown, bSeated);
	TestTrue(TEXT("Not in view while it's drawn"), !bShown && !bSeated);
	const float Out = LooterReload::RocketTravel(0.4f, bShown, bSeated);
	TestTrue(TEXT("In front of the mouth"), bShown && !bSeated && Out > 0.f && Out <= LooterReload::RocketReach);
	TestTrue(TEXT("Going in"), LooterReload::RocketTravel(0.6f, bShown, bSeated) < Out);
	LooterReload::RocketTravel(0.8f, bShown, bSeated);
	TestTrue(TEXT("Seated"), bSeated && !bShown);
	return true;
}

// ---------------------------------------------------------------------------
// The definitions and models (after Tools/Unreal/create_new_guns.py and setup_gun_parts.py)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewGunsDefinitionsTest, "Looter.Weapons.NewGuns.Definitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNewGunsDefinitionsTest::RunTest(const FString& Parameters)
{
	const UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, DefinitionPath(EWeaponKind::Rifle));
	const UWeaponDefinition* SMG = LoadObject<UWeaponDefinition>(nullptr, DefinitionPath(EWeaponKind::SMG));
	const UWeaponDefinition* Sniper = LoadObject<UWeaponDefinition>(nullptr, DefinitionPath(EWeaponKind::Sniper));
	const UWeaponDefinition* LMG = LoadObject<UWeaponDefinition>(nullptr, DefinitionPath(EWeaponKind::LMG));
	const UWeaponDefinition* Launcher = LoadObject<UWeaponDefinition>(nullptr, DefinitionPath(EWeaponKind::Launcher));
	if (!TestNotNull(TEXT("The rifle"), Rifle) || !TestNotNull(TEXT("DA_SMG (create_new_guns.py)"), SMG) || !TestNotNull(TEXT("DA_Sniper"), Sniper)
		|| !TestNotNull(TEXT("DA_LMG"), LMG) || !TestNotNull(TEXT("DA_Launcher"), Launcher))
	{
		return false;
	}
	const TPair<const UWeaponDefinition*, EWeaponKind> Guns[] = { { SMG, EWeaponKind::SMG }, { Sniper, EWeaponKind::Sniper },
		{ LMG, EWeaponKind::LMG }, { Launcher, EWeaponKind::Launcher } };
	const ULootTable* Loot = LoadObject<ULootTable>(nullptr, TEXT("/Game/Weapons/Data/DA_LootTable_Default.DA_LootTable_Default"));
	for (const TPair<const UWeaponDefinition*, EWeaponKind>& Gun : Guns)
	{
		const FString Name = KindName(Gun.Value);
		TestEqual(*FString::Printf(TEXT("%s: its kind"), *Name), Gun.Key->Kind, Gun.Value);
		TestFalse(*FString::Printf(TEXT("%s: built from parts (setup_gun_parts.py)"), *Name), Gun.Key->Parts.IsEmpty());
		TestEqual(*FString::Printf(TEXT("%s: its rarity glow"), *Name), Gun.Key->RarityGlowSlot, FName(TEXT("GunAccentGlow")));
		TestTrue(*FString::Printf(TEXT("%s: in the default loot table"), *Name), Loot && Loot->Entries.ContainsByPredicate(
			[&Gun](const FLootTableEntry& Entry) { return Entry.Weapon == Gun.Key && Entry.Weight > 0.f; }));
	}
	// Ammo, fire modes and reload parts as the contract has them.
	TestEqual(TEXT("SMG on SMG ammo"), SMG->AmmoType, EAmmoType::SMG);
	TestEqual(TEXT("Sniper on sniper rounds"), Sniper->AmmoType, EAmmoType::Sniper);
	TestEqual(TEXT("LMG on rifle ammo"), LMG->AmmoType, EAmmoType::AssaultRifle);
	TestEqual(TEXT("Launcher on rockets"), Launcher->AmmoType, EAmmoType::Rocket);
	TestEqual(TEXT("SMG full auto"), SMG->FireMode, EWeaponFireMode::FullAuto);
	TestEqual(TEXT("LMG full auto"), LMG->FireMode, EWeaponFireMode::FullAuto);
	TestEqual(TEXT("Sniper one shot a pull"), Sniper->FireMode, EWeaponFireMode::SemiAuto);
	TestEqual(TEXT("Launcher one rocket a pull"), Launcher->FireMode, EWeaponFireMode::SemiAuto);
	TestEqual(TEXT("SMG's helix"), SMG->ReloadPart, EWeaponReloadPart::Helix);
	TestEqual(TEXT("Sniper's bolt"), Sniper->ReloadPart, EWeaponReloadPart::Bolt);
	TestEqual(TEXT("LMG's box"), LMG->ReloadPart, EWeaponReloadPart::Box);
	TestEqual(TEXT("Launcher's rocket"), Launcher->ReloadPart, EWeaponReloadPart::Rocket);
	TestNotNull(TEXT("The launcher's rocket model (SM_PowderkegRocket)"), Launcher->ProjectileMesh.Get());
	TestTrue(TEXT("The sniper has its bolt slot"), Sniper->Parts.ContainsByPredicate([](const FWeaponPartSlot& Slot) { return Slot.Name == FName(TEXT("Bolt")); }));
	const FWeaponPartSlot* Rocket = Launcher->Parts.FindByPredicate([](const FWeaponPartSlot& Slot) { return Slot.Name == FName(TEXT("Magazine")); });
	TestTrue(TEXT("The launcher's rocket hangs in its barrel's mouth"), Rocket && Rocket->Socket == FName(TEXT("Rocket")));

	// Their jobs against the rifle: the SMG fastest, the sniper's one heavy shot, the LMG's long belt, the launcher's few.
	TestTrue(TEXT("The SMG fires fastest"), SMG->BaseStats.FireRate > Rifle->BaseStats.FireRate);
	TestTrue(TEXT("...hits lightest"), SMG->BaseStats.Damage < Rifle->BaseStats.Damage);
	TestTrue(TEXT("The sniper hits hardest of the bullets"), Sniper->BaseStats.Damage > Rifle->BaseStats.Damage * 4.f);
	TestTrue(TEXT("...slowly"), Sniper->BaseStats.FireRate < 60.f);
	TestTrue(TEXT("...reaching farthest"), Sniper->BaseStats.Range > Rifle->BaseStats.Range * 1.5f);
	TestTrue(TEXT("The LMG's rate between"), LMG->BaseStats.FireRate > Sniper->BaseStats.FireRate && LMG->BaseStats.FireRate <= SMG->BaseStats.FireRate);
	TestTrue(TEXT("The launcher's rocket outhits a sniper round"), Launcher->BaseStats.Damage > Sniper->BaseStats.Damage);
	TestTrue(TEXT("...on a long reload"), Launcher->BaseStats.ReloadTime > Rifle->BaseStats.ReloadTime);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewGunsModelsTest, "Looter.Weapons.NewGuns.Models",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNewGunsModelsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	for (const EWeaponKind Kind : NewKinds)
	{
		const FString Name = KindName(Kind);
		UWeaponDefinition* Definition = LoadObject<UWeaponDefinition>(nullptr, DefinitionPath(Kind));
		if (!TestNotNull(*FString::Printf(TEXT("%s: its definition"), *Name), Definition))
		{
			continue;
		}
		// A spread of rolls: each builds, with a muzzle ahead, both hands' sockets, a sight to aim along.
		for (int32 Roll = 0; Roll < 12; ++Roll)
		{
			const EWeaponRarity Rarity = static_cast<EWeaponRarity>(Roll % 5);
			FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Definition, Rarity, 1);
			AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(World, Gun, FTransform::Identity);
			UWeaponModelComponent* Model = Weapon ? Weapon->FindComponentByClass<UWeaponModelComponent>() : nullptr;
			if (!TestTrue(*FString::Printf(TEXT("%s roll %d built"), *Name, Roll), Model && Model->IsAssembled()))
			{
				continue;
			}
			TestTrue(*FString::Printf(TEXT("%s: muzzle ahead (%s)"), *Name, *Model->GetMuzzle().ToCompactString()), Model->GetMuzzle().X > 30.0);
			TestFalse(*FString::Printf(TEXT("%s: a grip"), *Name), Model->GetGrip().IsZero());
			TestFalse(*FString::Printf(TEXT("%s: a foregrip"), *Name), Model->GetForegrip().IsZero());
			TestTrue(*FString::Printf(TEXT("%s: aims over its middle"), *Name), FMath::Abs(Model->GetAimPoint().Y) < 0.6 && Model->GetAimPoint().Z > 1.0);
			TestEqual(*FString::Printf(TEXT("%s: its reload part"), *Name), Model->GetReloadPart(), Definition->ReloadPart);
			switch (Kind)
			{
			case EWeaponKind::SMG:
				TestTrue(TEXT("The SMG's helix glows with its rounds"), Model->HasAmmoGauge());
				break;
			case EWeaponKind::LMG:
				TestTrue(TEXT("The LMG's barrel glows with its heat"), Model->HasHeatGauge());
				break;
			case EWeaponKind::Sniper:
			{
				TestTrue(TEXT("The sniper has its bolt"), Model->HasBolt());
				FVector Port;
				TestTrue(TEXT("...and a port its cases fly from"), Model->GetEject(Port));
				break;
			}
			case EWeaponKind::Launcher:
			{
				FVector Exhaust;
				FVector Way;
				TestTrue(TEXT("The launcher's back-blast comes out behind"), Model->GetBackblast(Exhaust, Way) && Way.X < -0.5);
				// Loaded, its rocket's nose shows; fired, the mouth is empty; mid-reload the next one comes in.
				Model->SetRocketLoad(0.f, false, true);
				const UStaticMeshComponent* Loaded = nullptr;
				for (const UStaticMeshComponent* Part : Model->GetParts())
				{
					if (Part && Part->GetAttachSocketName() == FName(TEXT("Rocket")))
					{
						Loaded = Part;
					}
				}
				if (TestNotNull(TEXT("The loaded rocket"), Loaded))
				{
					TestTrue(TEXT("Its nose shows while loaded"), Loaded->IsVisible());
					Model->SetRocketLoad(0.f, false, false);
					TestFalse(TEXT("...and not once it's gone"), Loaded->IsVisible());
				}
				break;
			}
			default:
				break;
			}
			Weapon->Destroy();
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// The rocket in a test level
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewGunsRocketTest, "Looter.Weapons.NewGuns.Rocket",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNewGunsRocketTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	// A wall west of the burst: what stands behind it is shielded.
	LocomotionTestWorld::SpawnBlock(World, FVector(-150.0, 0.0, 200.0), FVector(40.0, 400.0, 400.0));
	const FVector Center(0.0, 0.0, 20.0);
	const GunKinds::FBlastRules Rules;

	ASpiderCreature* Near = SpawnSpider(World, FVector(120.0, 0.0, 0.0));
	ASpiderCreature* Edge = SpawnSpider(World, FVector(0.0, 300.0, 0.0));
	ASpiderCreature* Beyond = SpawnSpider(World, FVector(0.0, -900.0, 0.0));
	ASpiderCreature* Shielded = SpawnSpider(World, FVector(-300.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Spiders"), Near) || !TestNotNull(TEXT("..."), Edge) || !TestNotNull(TEXT("..."), Beyond) || !TestNotNull(TEXT("..."), Shielded))
	{
		return false;
	}
	const TArray<FRocketBlastTarget> Targets = WeaponRocketBlast::FindTargets(World, Center, nullptr, nullptr, 300.f, Rules);
	auto Find = [&Targets](const AActor* Actor) -> const FRocketBlastTarget*
	{
		return Targets.FindByPredicate([Actor](const FRocketBlastTarget& Target) { return Target.Actor.Get() == Actor; });
	};
	const FRocketBlastTarget* NearHit = Find(Near);
	const FRocketBlastTarget* EdgeHit = Find(Edge);
	TestNotNull(TEXT("The spider close by is caught"), NearHit);
	TestNotNull(TEXT("...and the one near the edge"), EdgeHit);
	TestNull(TEXT("Nothing past its reach"), Find(Beyond));
	TestNull(TEXT("Nothing through a wall"), Find(Shielded));
	if (NearHit && EdgeHit)
	{
		TestTrue(TEXT("Nearer takes more"), NearHit->Damage > EdgeHit->Damage);
		TestEqual(TEXT("Its damage is the falloff's"), NearHit->Damage, WeaponRocketBlast::DamageAt(300.f, NearHit->Distance, Rules, false));
	}
	// What the rocket struck takes it whole, whatever's between; its shooter takes half.
	const TArray<FRocketBlastTarget> Struck = WeaponRocketBlast::FindTargets(World, Center, Near, Shielded, 300.f, Rules);
	const FRocketBlastTarget* StruckHit = Struck.FindByPredicate([Shielded](const FRocketBlastTarget& Target) { return Target.Actor.Get() == Shielded; });
	const FRocketBlastTarget* SelfHit = Struck.FindByPredicate([Near](const FRocketBlastTarget& Target) { return Target.Actor.Get() == Near; });
	TestTrue(TEXT("What it struck takes it whole"), StruckHit && FMath::IsNearlyEqual(StruckHit->Damage, 300.f));
	TestTrue(TEXT("Its shooter takes its share"), SelfHit && SelfHit->bShooter && SelfHit->Damage < 300.f * Rules.SelfScale + 0.01f);

	// The burst itself reaches them (a test level has no game mode, so pawns take no damage here: the hits are counted).
	const FRocketBlastResult Result = WeaponRocketBlast::Detonate(World, Center, FVector::UpVector, nullptr, nullptr, nullptr, 50.f, 0.f, nullptr, nullptr);
	TestTrue(TEXT("The burst hurt what it reached"), Result.Hits >= 2);

	// A rocket's flight: straight at a wall, it bursts there; one fired at nothing bursts at the end of its life.
	// (Clear of the spiders, along the wall's middle.)
	AWeaponRocket* AtWall = AWeaponRocket::Launch(World, FVector(400.0, 150.0, 150.0), FVector(-1.0, 0.0, 0.0), FVector(400.0, 150.0, 150.0), nullptr);
	if (TestNotNull(TEXT("A rocket"), AtWall))
	{
		for (int32 Frame = 0; Frame < 120 && !AtWall->HasBurst(); ++Frame)
		{
			AtWall->Step(1.f / 60.f);
		}
		TestTrue(TEXT("It bursts on the wall"), AtWall->HasBurst());
		TestTrue(FString::Printf(TEXT("...at the wall (%s)"), *AtWall->GetFlightLocation().ToCompactString()),
			FMath::Abs(AtWall->GetFlightLocation().X - (-130.0)) < 20.0);
	}
	AWeaponRocket* Skyward = AWeaponRocket::Launch(World, FVector(0.0, 0.0, 500.0), FVector(0.0, 0.0, 1.0), FVector(0.0, 0.0, 500.0), nullptr);
	if (TestNotNull(TEXT("Another"), Skyward))
	{
		float Time = 0.f;
		while (Time < Def::RocketLife + 1.f && !Skyward->HasBurst())
		{
			Skyward->Step(1.f / 30.f);
			Time += 1.f / 30.f;
		}
		TestTrue(TEXT("At nothing it bursts at the end of its life"), Skyward->HasBurst() && Time >= Def::RocketLife - 0.1f);
	}
	return true;
}

#endif
