#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/ReloadBeats.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/ReloadTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponSounds.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"
#include "TimerManager.h"

// The reload side of the gun feel pass (Docs/Work/2026-10-22-Gunfeel/PLAN.md, GF-RELOAD; Looter.Gunfeel.Reload.*): empty
// slower than tactical, the rounds in at the seat, ready a beat early, the shotgun shell by shell with its interrupt, and
// the pump racked between shots. The rules run on the shipped numbers (ReloadTuning::Default; the variables a rule reads
// live are set to them for the test and put back), then two real guns in a test world.

namespace
{
	namespace Def = ReloadTuning::Default;

	/** A console variable, set to Value for the scope and put back after. */
	struct FScopedReloadVariable
	{
		FScopedReloadVariable(const TCHAR* Name, float Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Was = Variable->GetString();
				Variable->Set(*FString::SanitizeFloat(Value), ECVF_SetByConsole);
			}
		}
		~FScopedReloadVariable()
		{
			if (Variable)
			{
				Variable->Set(*Was, ECVF_SetByConsole);
			}
		}
		IConsoleVariable* Variable = nullptr;
		FString Was;
	};

	/** The shotgun's beats at their shipped numbers for the scope (StepsFor and the pump pose read them live). */
	struct FShippedShells
	{
		FScopedReloadVariable Start{ TEXT("Looter.Gunfeel.Reload.Shotgun.Start"), Def::ShotgunStart };
		FScopedReloadVariable Shell{ TEXT("Looter.Gunfeel.Reload.Shotgun.Shell"), Def::ShotgunShell };
		FScopedReloadVariable Pump{ TEXT("Looter.Gunfeel.Reload.Shotgun.Pump"), Def::ShotgunPump };
	};

	FReloadPlan MakePlan(bool bEmpty, int32 Shells = 0)
	{
		FReloadPlan Plan;
		Plan.bEmpty = bEmpty;
		Plan.Shells = Shells;
		return Plan;
	}

	bool HasStep(const TArray<FReloadStepAt>& Steps, EReloadStep Step)
	{
		return Steps.ContainsByPredicate([Step](const FReloadStepAt& At) { return At.Step == Step; });
	}

	int32 CountStep(const TArray<FReloadStepAt>& Steps, EReloadStep Step)
	{
		return Steps.FilterByPredicate([Step](const FReloadStepAt& At) { return At.Step == Step; }).Num();
	}

	/** A holder in a test world with a gun of Definition in hand, Magazine rounds in it and Reserve in the pool. */
	AWeaponBase* GiveHeldGun(UWorld* World, UWeaponDefinition* Definition, int32 Magazine, int32 Reserve, UWeaponManagerComponent*& OutWeapons)
	{
		OutWeapons = nullptr;
		APawn* Holder = World->SpawnActor<APawn>();
		if (!Holder || !Definition)
		{
			return nullptr;
		}
		UHealthComponent* Health = NewObject<UHealthComponent>(Holder, TEXT("Health"));
		Health->bShowDamageNumbers = false;
		Holder->AddInstanceComponent(Health);
		Health->RegisterComponent();
		UWeaponManagerComponent* Weapons = NewObject<UWeaponManagerComponent>(Holder, TEXT("Weapons"));
		Holder->AddInstanceComponent(Weapons);
		Weapons->RegisterComponent();
		Weapons->MaxWeapons = 3;
		Holder->DispatchBeginPlay();
		FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Definition, EWeaponRarity::Rare, 1);
		Instance.SavedMagazine = Magazine;
		AWeaponBase* Gun = Weapons->GiveWeapon(Instance);
		if (Gun)
		{
			Weapons->AddAmmo(Gun->GetAmmoType(), Reserve);
		}
		OutWeapons = Weapons;
		return Gun;
	}

	/** Moves a reload's timer on by Seconds: a timer set this frame counts from the manager's next tick (see FX.RevolverEmpties). */
	void RunReloadClock(UWorld* World, float Seconds)
	{
		FTimerManager& Timers = World->GetTimerManager();
		Timers.Tick(0.f);
		++GFrameCounter;
		Timers.Tick(Seconds);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReloadPlanTest, "Looter.Gunfeel.Reload.Plan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReloadPlanTest::RunTest(const FString& Parameters)
{
	// Every number is a live variable.
	for (const TCHAR* Name : { TEXT("Looter.Gunfeel.Reload.Tactical.Rifle"), TEXT("Looter.Gunfeel.Reload.Empty.Rifle"),
			TEXT("Looter.Gunfeel.Reload.Tactical.SMG"), TEXT("Looter.Gunfeel.Reload.Empty.Sniper"), TEXT("Looter.Gunfeel.Reload.Tactical.LMG"),
			TEXT("Looter.Gunfeel.Reload.Empty.Revolver"), TEXT("Looter.Gunfeel.Reload.Tactical.Launcher"), TEXT("Looter.Gunfeel.Reload.CommitAtSeat"),
			TEXT("Looter.Gunfeel.Reload.ReadyEarly"), TEXT("Looter.Gunfeel.Reload.Shotgun.Start"), TEXT("Looter.Gunfeel.Reload.Shotgun.Shell"),
			TEXT("Looter.Gunfeel.Reload.Shotgun.Pump"), TEXT("Looter.Gunfeel.Reload.Shotgun.Interrupt"), TEXT("Looter.Gunfeel.Shotgun.RackShare"),
			TEXT("Looter.Gunfeel.Shotgun.RackMin"), TEXT("Looter.Gunfeel.Shotgun.RackMax"), TEXT("Looter.Gunfeel.Reload.Debug"),
			TEXT("Looter.Gunfeel.Reload.Beats") })
	{
		TestNotNull(*FString::Printf(TEXT("%s exists"), Name), IConsoleManager::Get().FindConsoleVariable(Name));
	}

	// Empty slower than tactical where a handle or bolt is worked; one rhythm for the box, the cylinder and the rocket.
	TestTrue(TEXT("Rifle: tactical 0.9, empty 1.1"), FMath::IsNearlyEqual(Def::TimeScale(EWeaponKind::Rifle, false), 0.9f)
		&& FMath::IsNearlyEqual(Def::TimeScale(EWeaponKind::Rifle, true), 1.1f));
	TestTrue(TEXT("SMG: empty slower"), Def::TimeScale(EWeaponKind::SMG, true) > Def::TimeScale(EWeaponKind::SMG, false));
	TestTrue(TEXT("Sniper: empty slower"), Def::TimeScale(EWeaponKind::Sniper, true) > Def::TimeScale(EWeaponKind::Sniper, false));
	for (const EWeaponKind Kind : { EWeaponKind::LMG, EWeaponKind::Revolver, EWeaponKind::Launcher, EWeaponKind::Shotgun })
	{
		TestEqual(*FString::Printf(TEXT("%s: one rhythm"), *UEnum::GetValueAsString(Kind)), Def::TimeScale(Kind, true), Def::TimeScale(Kind, false));
	}

	// The plan: empty when nothing's chambered; a pump gun's shells are what its tube lacks, as many as the pool has.
	const FReloadPlan Rifle = ReloadTuning::PlanFor(EWeaponReloadPart::Magazine, 0, 30, 90);
	TestTrue(TEXT("A dry magazine: empty, no shells"), Rifle.bEmpty && Rifle.Shells == 0);
	TestFalse(TEXT("Rounds left: tactical"), ReloadTuning::PlanFor(EWeaponReloadPart::Magazine, 3, 30, 90).bEmpty);
	TestEqual(TEXT("Shells: what the tube lacks"), ReloadTuning::PlanFor(EWeaponReloadPart::Pump, 2, 6, 40).Shells, 4);
	TestEqual(TEXT("...as many as the pool has"), ReloadTuning::PlanFor(EWeaponReloadPart::Pump, 2, 6, 3).Shells, 3);
	TestEqual(TEXT("...never fewer than none"), ReloadTuning::ShellsNeeded(6, 6, 10), 0);

	// The times: a magazine's from its reload time and kind; a pump gun's from its shells, scaled by its parts and maker.
	const ReloadTuning::FShellTiming Shipped;
	const float Tactical = ReloadTuning::MotionSeconds(EWeaponReloadPart::Magazine, MakePlan(false), 2.1f, 1.f, Def::TimeScale(EWeaponKind::Rifle, false), Shipped);
	const float Empty = ReloadTuning::MotionSeconds(EWeaponReloadPart::Magazine, MakePlan(true), 2.1f, 1.f, Def::TimeScale(EWeaponKind::Rifle, true), Shipped);
	TestTrue(FString::Printf(TEXT("Rifle: tactical %.2f s quicker than empty %.2f s"), Tactical, Empty), Tactical < Empty);
	const float ThreeShells = ReloadTuning::MotionSeconds(EWeaponReloadPart::Pump, MakePlan(false, 3), 2.f, 1.f, 1.f, Shipped);
	TestTrue(FString::Printf(TEXT("Three shells: start + 3 x shell (%.2f s)"), ThreeShells),
		FMath::IsNearlyEqual(ThreeShells, Def::ShotgunStart + 3.f * Def::ShotgunShell, 0.001f));
	TestTrue(TEXT("...the pump only from empty"), FMath::IsNearlyEqual(ReloadTuning::MotionSeconds(EWeaponReloadPart::Pump, MakePlan(true, 3), 2.f, 1.f, 1.f,
		Shipped), ThreeShells + Def::ShotgunPump, 0.001f));
	TestTrue(TEXT("...a slower roll's shells slower"), FMath::IsNearlyEqual(ReloadTuning::MotionSeconds(EWeaponReloadPart::Pump, MakePlan(false, 3), 2.f,
		ReloadTuning::GunScale(2.4f, 2.f), 1.f, Shipped), ThreeShells * 1.2f, 0.001f));
	TestEqual(TEXT("No base, no scale"), ReloadTuning::GunScale(2.f, 0.f), 1.f);

	// Ready a beat early, never before half the motion.
	TestTrue(TEXT("Ready 0.08 s early"), FMath::IsNearlyEqual(ReloadTuning::ReadySeconds(2.1f, Def::ReadyEarly), 2.1f - Def::ReadyEarly, 0.0001f));
	TestTrue(TEXT("...a tiny reload at half"), FMath::IsNearlyEqual(ReloadTuning::ReadySeconds(0.1f, Def::ReadyEarly), 0.05f, 0.0001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReloadStepsTest, "Looter.Gunfeel.Reload.Steps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReloadStepsTest::RunTest(const FString& Parameters)
{
	const FShippedShells Shells;
	TArray<FReloadStepAt> Steps;

	// A tactical magazine or helix reload skips its handle; from empty it's worked.
	for (const EWeaponReloadPart Part : { EWeaponReloadPart::Magazine, EWeaponReloadPart::Helix })
	{
		const FString Name = UEnum::GetValueAsString(Part);
		LooterReload::StepsFor(Part, MakePlan(true), Steps);
		TestTrue(*FString::Printf(TEXT("%s: from empty, the handle"), *Name), HasStep(Steps, EReloadStep::Bolt));
		LooterReload::StepsFor(Part, MakePlan(false), Steps);
		TestFalse(*FString::Printf(TEXT("%s: tactical, no handle"), *Name), HasStep(Steps, EReloadStep::Bolt));
		TestEqual(*FString::Printf(TEXT("%s: tactical, the magazine out and in"), *Name), Steps.Num(), 2);
	}
	// The sniper's tactical reload is the box only.
	LooterReload::StepsFor(EWeaponReloadPart::Bolt, MakePlan(false), Steps);
	TestTrue(TEXT("Sniper tactical: the box, the bolt home"), HasStep(Steps, EReloadStep::MagIn) && !HasStep(Steps, EReloadStep::BoltOpen)
		&& !HasStep(Steps, EReloadStep::BoltClose));
	LooterReload::StepsFor(EWeaponReloadPart::Bolt, MakePlan(true), Steps);
	TestTrue(TEXT("Sniper empty: the bolt opened and run home"), HasStep(Steps, EReloadStep::BoltOpen) && HasStep(Steps, EReloadStep::BoltClose));
	TestEqual(TEXT("Sniper tactical: the bolt stays shut"), LooterReload::BoltOpenInReload(0.5f, MakePlan(false)), 0.f);
	TestEqual(TEXT("...and the hand off its knob"), LooterReload::BoltHandInReload(0.05f, MakePlan(false)), 0.f);

	// A pump gun: one ShellIn per planned shell, in order, then the pump from empty only.
	for (const bool bEmpty : { false, true })
	{
		for (int32 Count = 1; Count <= 6; ++Count)
		{
			const FString Name = FString::Printf(TEXT("%d shells %s"), Count, bEmpty ? TEXT("from empty") : TEXT("tactical"));
			LooterReload::StepsFor(EWeaponReloadPart::Pump, MakePlan(bEmpty, Count), Steps);
			TestEqual(*FString::Printf(TEXT("%s: a step a shell"), *Name), CountStep(Steps, EReloadStep::ShellIn), Count);
			TestEqual(*FString::Printf(TEXT("%s: the pump"), *Name), CountStep(Steps, EReloadStep::Pump), bEmpty ? 1 : 0);
			float Last = 0.f;
			bool bInOrder = true;
			for (const FReloadStepAt& Step : Steps)
			{
				bInOrder &= Step.Progress > Last && Step.Progress < 1.f;
				Last = Step.Progress;
			}
			TestTrue(*FString::Printf(TEXT("%s: in order, inside the reload"), *Name), bInOrder);
			TestTrue(*FString::Printf(TEXT("%s: the pump after the last shell"), *Name), !bEmpty || Steps.Last().Step == EReloadStep::Pump);
		}
	}

	// Each kind hears its own: the sniper's box, the SMG's charge; the rest as before.
	TestEqual(TEXT("Sniper box out"), WeaponSounds::ReloadStep(EWeaponKind::Sniper, EReloadStep::MagOut), FName(TEXT("Weapon.Sniper.MagOut")));
	TestEqual(TEXT("Sniper box in"), WeaponSounds::ReloadStep(EWeaponKind::Sniper, EReloadStep::MagIn), FName(TEXT("Weapon.Sniper.MagIn")));
	TestEqual(TEXT("SMG charge"), WeaponSounds::ReloadStep(EWeaponKind::SMG, EReloadStep::Bolt), FName(TEXT("Weapon.SMG.Charge")));
	TestEqual(TEXT("Rifle handle"), WeaponSounds::ReloadStep(EWeaponKind::Rifle, EReloadStep::Bolt), FName(LooterSoundCue::RifleBolt));
	TestEqual(TEXT("Rifle magazine"), WeaponSounds::ReloadStep(EWeaponKind::Rifle, EReloadStep::MagIn), FName(LooterSoundCue::RifleMagIn));
	TestEqual(TEXT("Every reload's cloth"), WeaponSounds::ReloadStart(), FName(TEXT("Weapon.Reload.Cloth")));
	TestEqual(TEXT("The SMG taken up light"), WeaponSounds::Equip(EWeaponKind::SMG), FName(TEXT("Weapon.Equip.Light")));
	for (const EWeaponKind Kind : { EWeaponKind::Sniper, EWeaponKind::LMG, EWeaponKind::Launcher })
	{
		const FString Name = UEnum::GetValueAsString(Kind);
		TestEqual(*FString::Printf(TEXT("%s: taken up heavy"), *Name), WeaponSounds::Equip(Kind), FName(TEXT("Weapon.Equip.Heavy")));
		TestEqual(*FString::Printf(TEXT("%s: put away heavy"), *Name), WeaponSounds::Holster(Kind), FName(TEXT("Weapon.Holster.Heavy")));
	}
	TestEqual(TEXT("The rifle swung off the back as before"), WeaponSounds::Equip(EWeaponKind::Rifle), FName(LooterSoundCue::Equip));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReloadCommitTest, "Looter.Gunfeel.Reload.Commit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReloadCommitTest::RunTest(const FString& Parameters)
{
	const FShippedShells Shells;
	// Each reload has its seat, before the gun is ready again, in either plan.
	TArray<FReloadStepAt> Steps;
	for (const EWeaponReloadPart Part : { EWeaponReloadPart::Magazine, EWeaponReloadPart::Helix, EWeaponReloadPart::Box, EWeaponReloadPart::Bolt,
			EWeaponReloadPart::Rocket, EWeaponReloadPart::Cylinder, EWeaponReloadPart::Pump })
	{
		for (const bool bEmpty : { false, true })
		{
			const FReloadPlan Plan = MakePlan(bEmpty, Part == EWeaponReloadPart::Pump ? 3 : 0);
			LooterReload::StepsFor(Part, Plan, Steps);
			const FReloadStepAt* Seat = Steps.FindByPredicate([Part](const FReloadStepAt& At) { return LooterReload::CommitsAt(Part, At.Step); });
			const FString Name = FString::Printf(TEXT("%s %s"), *UEnum::GetValueAsString(Part), bEmpty ? TEXT("empty") : TEXT("tactical"));
			if (!TestNotNull(*FString::Printf(TEXT("%s: a seat"), *Name), Seat))
			{
				continue;
			}
			// The steps are in the choreography; the gun is ready on the motion's clock (a 2 s reload, 0.08 s early).
			const float ReadyRaw = ReloadTuning::ReadySeconds(2.f, Def::ReadyEarly) / 2.f;
			TestTrue(*FString::Printf(TEXT("%s: seated (%.2f) before ready (%.2f)"), *Name, Seat->Progress,
				LooterReload::ChoreographyProgress(Part, Plan, ReadyRaw)), Seat->Progress < LooterReload::ChoreographyProgress(Part, Plan, ReadyRaw));
		}
	}
	TestFalse(TEXT("No reload part, no seat: rounds at the end"), LooterReload::CommitsAt(EWeaponReloadPart::None, EReloadStep::MagIn));

	// The tactical magazine jumps its handle with nothing seen to jump: the pose either side of the jump is the same.
	const FReloadPlan Tactical = MakePlan(false);
	const float Skipped = LooterReload::HandleTo - LooterReload::HandleFrom;
	const float JumpRaw = LooterReload::HandleFrom / (1.f - Skipped);
	TestEqual(TEXT("Tactical starts at the start"), LooterReload::ChoreographyProgress(EWeaponReloadPart::Magazine, Tactical, 0.f), 0.f);
	TestTrue(TEXT("...ends at the end"), FMath::IsNearlyEqual(LooterReload::ChoreographyProgress(EWeaponReloadPart::Magazine, Tactical, 1.f), 1.f));
	TestTrue(TEXT("...past the handle at the jump"), LooterReload::ChoreographyProgress(EWeaponReloadPart::Magazine, Tactical, JumpRaw + 0.001f)
		>= LooterReload::HandleTo);
	FVector Before;
	FRotator BeforeTurn;
	FVector After;
	FRotator AfterTurn;
	LooterReload::ViewModelPose(EWeaponReloadPart::Magazine, LooterReload::HandleFrom - 0.0005f, Before, BeforeTurn);
	LooterReload::ViewModelPose(EWeaponReloadPart::Magazine, LooterReload::HandleTo + 0.0005f, After, AfterTurn);
	TestTrue(*FString::Printf(TEXT("...the gun where it was (%s / %s)"), *Before.ToCompactString(), *After.ToCompactString()),
		Before.Equals(After, 0.1) && BeforeTurn.Equals(AfterTurn, 0.3f));
	bool bShown = true;
	TestTrue(TEXT("...the magazine home both sides"), LooterReload::MagazineTravel(LooterReload::HandleFrom, bShown) == 0.f
		&& LooterReload::MagazineTravel(LooterReload::HandleTo, bShown) == 0.f);
	TestEqual(TEXT("From empty, no jump"), LooterReload::ChoreographyProgress(EWeaponReloadPart::Magazine, MakePlan(true), 0.75f), 0.75f);
	TestEqual(TEXT("The LMG's box: one rhythm"), LooterReload::ChoreographyProgress(EWeaponReloadPart::Box, Tactical, 0.75f), 0.75f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReloadInterruptTest, "Looter.Gunfeel.Reload.Interrupt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReloadInterruptTest::RunTest(const FString& Parameters)
{
	const FShippedShells Shells;
	const ReloadTuning::FShellTiming Timing;
	const FReloadPlan Five = MakePlan(true, 5);

	// The press keeps the shells seated and the one in hand; the first one if the hand is still on its way.
	TestEqual(TEXT("Pressed at once: the first shell"), ReloadTuning::ShellsKept(Five, 0.f, Timing), 1);
	TestEqual(TEXT("Pressed mid second shell: two"), ReloadTuning::ShellsKept(Five, Timing.Start + 1.5f * Timing.Shell, Timing), 2);
	TestEqual(TEXT("Pressed during the pump: all"), ReloadTuning::ShellsKept(Five, ReloadTuning::ShellReloadSeconds(Five, Timing) - 0.01f, Timing), 5);
	TestEqual(TEXT("No shells planned, none kept"), ReloadTuning::ShellsKept(MakePlan(false, 0), 1.f, Timing), 0);

	// Cut short, the motion carries on from where it was: the shells already seated keep their seconds, the gun its pose.
	const float PressedAt = Timing.Start + 1.3f * Timing.Shell;
	const FReloadPlan Two = MakePlan(true, ReloadTuning::ShellsKept(Five, PressedAt, Timing));
	const float FiveSeconds = ReloadTuning::ShellReloadSeconds(Five, Timing);
	const float TwoSeconds = ReloadTuning::ShellReloadSeconds(Two, Timing);
	TArray<FReloadStepAt> Long;
	TArray<FReloadStepAt> Short;
	LooterReload::StepsFor(EWeaponReloadPart::Pump, Five, Long);
	LooterReload::StepsFor(EWeaponReloadPart::Pump, Two, Short);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		TestTrue(*FString::Printf(TEXT("Shell %d seats at the same second"), Index + 1),
			FMath::IsNearlyEqual(Long[Index].Progress * FiveSeconds, Short[Index].Progress * TwoSeconds, 0.001f));
	}
	TestTrue(TEXT("...then the pump (it was empty)"), Short.Last().Step == EReloadStep::Pump && Short.Num() == 3);
	FVector LongOffset;
	FRotator LongTurn;
	FVector ShortOffset;
	FRotator ShortTurn;
	LooterReload::ViewModelPose(EWeaponReloadPart::Pump, PressedAt / FiveSeconds, Five, LongOffset, LongTurn);
	LooterReload::ViewModelPose(EWeaponReloadPart::Pump, PressedAt / TwoSeconds, Two, ShortOffset, ShortTurn);
	TestTrue(TEXT("...the gun where it was"), LongOffset.Equals(ShortOffset, 0.05) && LongTurn.Equals(ShortTurn, 0.1f));

	// The planned pump reload comes up from the hold and goes back to it; its pump racks from empty only.
	for (const bool bEmpty : { false, true })
	{
		const FReloadPlan Plan = MakePlan(bEmpty, 3);
		FVector Offset;
		FRotator Turn;
		LooterReload::ViewModelPose(EWeaponReloadPart::Pump, 0.f, Plan, Offset, Turn);
		TestTrue(TEXT("From the hold"), Offset.IsNearlyZero(0.01) && Turn.IsNearlyZero(0.01f));
		LooterReload::ViewModelPose(EWeaponReloadPart::Pump, 0.5f, Plan, Offset, Turn);
		TestFalse(TEXT("Worked on"), Offset.IsNearlyZero(0.5) && Turn.IsNearlyZero(1.f));
		LooterReload::ViewModelPose(EWeaponReloadPart::Pump, 1.f, Plan, Offset, Turn);
		TestTrue(TEXT("Back on the hold"), Offset.IsNearlyZero(0.05) && Turn.IsNearlyZero(0.1f));
		float Most = 0.f;
		for (int32 Step = 0; Step <= 200; ++Step)
		{
			Most = FMath::Max(Most, LooterReload::PumpTravel(Step / 200.f, Plan));
		}
		TestTrue(FString::Printf(TEXT("%s: pumped %.1f cm"), bEmpty ? TEXT("Empty") : TEXT("Tactical"), Most),
			bEmpty ? Most > LooterReload::PumpStroke * 0.9f : Most == 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReloadRackTest, "Looter.Gunfeel.Reload.Rack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReloadRackTest::RunTest(const FString& Parameters)
{
	const ReloadTuning::FRackRules Rules;
	// A share of the time between shots, within its bounds, home before the gun can fire again.
	for (const float Interval : { 0.5f, 0.75f, 1.f, 1.5f })
	{
		const ReloadTuning::FRackTiming Rack = ReloadTuning::RackTiming(Interval, Rules);
		TestTrue(*FString::Printf(TEXT("%.2f s: %.2f s long, within %.2f-%.2f"), Interval, Rack.Seconds, Rules.Min, Rules.Max),
			Rack.Seconds >= Rules.Min - 0.0001f && Rack.Seconds <= Rules.Max + 0.0001f
			&& FMath::IsNearlyEqual(Rack.Seconds, FMath::Clamp(Interval * Rules.Share, Rules.Min, Rules.Max), 0.0001f));
		TestTrue(*FString::Printf(TEXT("%.2f s: home (%.2f) before the next shot"), Interval, Rack.End()), Rack.End() <= Interval + 0.0001f);
		TestTrue(*FString::Printf(TEXT("%.2f s: after the kick"), Interval), Rack.Start <= Rules.Delay + 0.0001f && Rack.Start >= 0.f);
	}
	const ReloadTuning::FRackTiming Fast = ReloadTuning::RackTiming(0.2f, Rules);
	TestTrue(TEXT("A fast roll racks at once"), Fast.Start == 0.f);

	// The fore-end: home before and after, fully back at the back point (the hull flies there).
	TestEqual(TEXT("Still before"), LooterReload::PumpCycleTravel(-1.f), 0.f);
	TestEqual(TEXT("Home at the start"), LooterReload::PumpCycleTravel(0.f), 0.f);
	TestTrue(TEXT("Home at the end"), LooterReload::PumpCycleTravel(0.999f) < 0.1f);
	TestTrue(TEXT("Back at the back point"), FMath::IsNearlyEqual(LooterReload::PumpCycleTravel(LooterReload::PumpCycleBackAt), LooterReload::PumpStroke, 0.01f));
	float Most = 0.f;
	for (int32 Step = 0; Step < 100; ++Step)
	{
		Most = FMath::Max(Most, LooterReload::PumpCycleTravel(Step / 100.f));
	}
	TestTrue(TEXT("Never past its stroke"), Most <= LooterReload::PumpStroke + 0.001f);
	FVector Offset;
	FRotator Turn;
	LooterReload::PumpCycleViewPose(-1.f, Offset, Turn);
	TestTrue(TEXT("The gun still between racks"), Offset.IsZero() && Turn.IsZero());
	LooterReload::PumpCycleViewPose(LooterReload::PumpCycleBackAt, Offset, Turn);
	TestTrue(TEXT("...rocked back as it's racked"), Offset.X < -1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReloadGunsTest, "Looter.Gunfeel.Reload.Guns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReloadGunsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun"));
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Shotgun loads"), Shotgun)
		|| !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const FScopedReloadVariable Commit(TEXT("Looter.Gunfeel.Reload.CommitAtSeat"), 1.f);
	const FScopedReloadVariable Interrupt(TEXT("Looter.Gunfeel.Reload.Shotgun.Interrupt"), 1.f);

	// The rifle: its rounds are in once the magazine seats, so a reload cut after that keeps them.
	UWeaponManagerComponent* Weapons = nullptr;
	AWeaponBase* RifleGun = GiveHeldGun(World, Rifle, 5, 200, Weapons);
	if (TestNotNull(TEXT("A rifle in hand"), RifleGun) && RifleGun->GetReloadPart() == EWeaponReloadPart::Magazine)
	{
		const int32 Full = RifleGun->GetStats().MagazineSize;
		RifleGun->Reload();
		TestTrue(TEXT("Rifle: a tactical reload"), RifleGun->IsReloading() && !RifleGun->IsReloadEmpty());
		const float Motion = RifleGun->GetStats().ReloadTime * ReloadTuning::TimeScale(EWeaponKind::Rifle, false);
		// Past the seat (the choreography's 0.62 is 0.70 of a tactical reload's time), short of ready.
		RunReloadClock(World, Motion * 0.75f);
		RifleGun->Tick(0.f);
		TestEqual(TEXT("Rifle: seated, its rounds in"), RifleGun->GetCurrentMagazine(), Full);
		RifleGun->BeginMeleeSwing();
		RifleGun->EndMeleeSwing(false);
		TestTrue(TEXT("Rifle: cut short after the seat, it keeps them"), !RifleGun->IsReloading() && RifleGun->GetCurrentMagazine() == Full);
	}

	// The shotgun: a shell at a time; a trigger press keeps the shell in hand, then fires.
	AWeaponBase* ShotgunGun = GiveHeldGun(World, Shotgun, 2, 30, Weapons);
	if (TestNotNull(TEXT("A shotgun in hand"), ShotgunGun) && TestEqual(TEXT("...a pump gun"), ShotgunGun->GetReloadPart(), EWeaponReloadPart::Pump))
	{
		const int32 Size = ShotgunGun->GetStats().MagazineSize;
		const int32 Pool = Weapons->GetAmmo(ShotgunGun->GetAmmoType());
		ShotgunGun->Reload();
		TestTrue(TEXT("Shotgun: reloading, tactical"), ShotgunGun->IsReloading() && !ShotgunGun->IsReloadEmpty());
		TestEqual(TEXT("Shotgun: a shell for each round it lacks"), ShotgunGun->GetReloadPlan().Shells, FMath::Min(Size - 2, Pool));
		TestEqual(TEXT("...a step each, no pump"), ShotgunGun->GetReloadSteps().Num(), ShotgunGun->GetReloadPlan().Shells);
		TestTrue(TEXT("Shotgun: a press mid-reload will fire"), ShotgunGun->InterruptReload());
		TestEqual(TEXT("...after the shell in hand"), ShotgunGun->GetReloadPlan().Shells, 1);
		TestTrue(TEXT("...nothing fired yet"), ShotgunGun->GetCurrentMagazine() == 2 && ShotgunGun->IsReloading());
		RunReloadClock(World, 5.f);
		TestFalse(TEXT("Shotgun: the shell in, ready"), ShotgunGun->IsReloading());
		TestEqual(TEXT("...one shell from the pool"), Weapons->GetAmmo(ShotgunGun->GetAmmoType()), Pool - 1);
		// The shot it then fires waits on the gun's draw, which a test world's clock never gets past: in play it fires
		// (the play-check's to see); here the round is in, or in and fired.
		const int32 After = ShotgunGun->GetCurrentMagazine();
		TestTrue(FString::Printf(TEXT("...its round in (%d)"), After), After == 3 || After == 2);
	}
	return true;
}


// ---------------------------------------------------------------------------
// The felt beats of a reload (gun identity, 2026-10-28; ReloadBeats.h)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunfeelReloadBeatsTest, "Looter.Gunfeel.Reload.Beats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunfeelReloadBeatsTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("The beats' strength is a live variable"), IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Gunfeel.Reload.Beats")));

	// The table (GUNS.md): each kind's seat, slam or snap, and the steps that carry none.
	const FReloadBeat HelixIn = ReloadBeats::For(EWeaponKind::SMG, EReloadStep::HelixIn);
	TestTrue(TEXT("SMG helix in: a tiny dip, a little roll"), FMath::IsNearlyEqual(HelixIn.Kick.Pitch, -0.08f) && FMath::IsNearlyEqual(HelixIn.Kick.Roll, 0.1f));
	TestTrue(TEXT("...rumble 0.15 for 0.04 s"), FMath::IsNearlyEqual(HelixIn.Rumble, 0.15f) && FMath::IsNearlyEqual(HelixIn.RumbleSeconds, 0.04f));
	TestTrue(TEXT("SMG charge: up a tenth"), FMath::IsNearlyEqual(ReloadBeats::For(EWeaponKind::SMG, EReloadStep::Bolt).Kick.Pitch, 0.1f));
	const FReloadBeat MagIn = ReloadBeats::For(EWeaponKind::Rifle, EReloadStep::MagIn);
	TestTrue(TEXT("Rifle mag in: -0.12 deg, view 0.3% narrower"), FMath::IsNearlyEqual(MagIn.Kick.Pitch, -0.12f) && FMath::IsNearlyEqual(MagIn.Kick.FieldOfView, -0.003f));
	const FReloadBeat Slap = ReloadBeats::For(EWeaponKind::Rifle, EReloadStep::Bolt);
	TestTrue(TEXT("Rifle handle slap: +0.18, roll 0.2, rumble 0.3/0.06"), FMath::IsNearlyEqual(Slap.Kick.Pitch, 0.18f) && FMath::IsNearlyEqual(Slap.Kick.Roll, 0.2f)
		&& FMath::IsNearlyEqual(Slap.Rumble, 0.3f) && FMath::IsNearlyEqual(Slap.RumbleSeconds, 0.06f));
	const FReloadBeat BoxIn = ReloadBeats::For(EWeaponKind::LMG, EReloadStep::BoxIn);
	TestTrue(TEXT("LMG box in: the biggest of its three"), BoxIn.Kick.Pitch < ReloadBeats::For(EWeaponKind::LMG, EReloadStep::BoxOut).Kick.Pitch
		&& BoxIn.Kick.Pitch < ReloadBeats::For(EWeaponKind::LMG, EReloadStep::BeltLay).Kick.Pitch);
	TestTrue(TEXT("...-0.25, view 0.5% narrower, rumble 0.45/0.08"), FMath::IsNearlyEqual(BoxIn.Kick.Pitch, -0.25f) && FMath::IsNearlyEqual(BoxIn.Kick.FieldOfView, -0.005f)
		&& FMath::IsNearlyEqual(BoxIn.Rumble, 0.45f) && FMath::IsNearlyEqual(BoxIn.RumbleSeconds, 0.08f));
	TestTrue(TEXT("Revolver cylinder out rolls the wrist back"), ReloadBeats::For(EWeaponKind::Revolver, EReloadStep::CylinderOut).Kick.Roll < -0.4f);
	const FReloadBeat Snap = ReloadBeats::For(EWeaponKind::Revolver, EReloadStep::CylinderIn);
	TestTrue(TEXT("Revolver cylinder in snaps: roll +0.6, pitch -0.1, 0.3/0.05"), FMath::IsNearlyEqual(Snap.Kick.Roll, 0.6f) && FMath::IsNearlyEqual(Snap.Kick.Pitch, -0.1f)
		&& FMath::IsNearlyEqual(Snap.Rumble, 0.3f) && FMath::IsNearlyEqual(Snap.RumbleSeconds, 0.05f));
	TestTrue(TEXT("Shotgun: a shell dips, the pump lifts"), ReloadBeats::For(EWeaponKind::Shotgun, EReloadStep::ShellIn).Kick.Pitch < 0.f
		&& FMath::IsNearlyEqual(ReloadBeats::For(EWeaponKind::Shotgun, EReloadStep::Pump).Kick.Pitch, 0.15f));
	TestTrue(TEXT("Sniper: bolt open lifts, bolt closed slams"), ReloadBeats::For(EWeaponKind::Sniper, EReloadStep::BoltOpen).Kick.Pitch > 0.f
		&& FMath::IsNearlyEqual(ReloadBeats::For(EWeaponKind::Sniper, EReloadStep::BoltClose).Kick.Pitch, -0.2f));
	const FReloadBeat Rocket = ReloadBeats::For(EWeaponKind::Launcher, EReloadStep::RocketIn);
	TestTrue(TEXT("Launcher rocket in: -0.3, view 0.6% narrower, 0.5/0.1"), FMath::IsNearlyEqual(Rocket.Kick.Pitch, -0.3f) && FMath::IsNearlyEqual(Rocket.Kick.FieldOfView, -0.006f)
		&& FMath::IsNearlyEqual(Rocket.Rumble, 0.5f) && FMath::IsNearlyEqual(Rocket.RumbleSeconds, 0.1f));
	TestTrue(TEXT("A step with no beat is none"), ReloadBeats::For(EWeaponKind::Rifle, EReloadStep::MagOut).IsNone());
	TestTrue(TEXT("...a kind's step on another kind is none"), ReloadBeats::For(EWeaponKind::Rifle, EReloadStep::BoxIn).IsNone());

	// Every beat is a fraction of a degree and sits well under the view's caps: a reload is hands at work, not a second recoil.
	const EWeaponKind Kinds[] = { EWeaponKind::Rifle, EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::SMG, EWeaponKind::Sniper,
		EWeaponKind::LMG, EWeaponKind::Launcher };
	int32 Beats = 0;
	for (const EWeaponKind Kind : Kinds)
	{
		for (int32 StepIndex = 0; StepIndex <= static_cast<int32>(EReloadStep::HelixIn); ++StepIndex)
		{
			const FReloadBeat Beat = ReloadBeats::For(Kind, static_cast<EReloadStep>(StepIndex));
			if (Beat.IsNone())
			{
				continue;
			}
			++Beats;
			TestTrue(FString::Printf(TEXT("Kind %d step %d: under a degree each way"), static_cast<int32>(Kind), StepIndex),
				FMath::Abs(Beat.Kick.Pitch) <= 1.f && FMath::Abs(Beat.Kick.Roll) <= 1.f && FMath::Abs(Beat.Kick.FieldOfView) <= 0.01f);
			TestTrue(FString::Printf(TEXT("Kind %d step %d: a real spring"), static_cast<int32>(Kind), StepIndex),
				Beat.Kick.Frequency > 0.f && Beat.Kick.Damping > 0.f && Beat.Kick.Damping < 1.f);
			TestTrue(FString::Printf(TEXT("Kind %d step %d: rumble short and strong enough or none"), static_cast<int32>(Kind), StepIndex),
				(Beat.Rumble <= 0.f && Beat.RumbleSeconds <= 0.f) || (Beat.Rumble > 0.f && Beat.Rumble <= 1.f && Beat.RumbleSeconds > 0.f && Beat.RumbleSeconds <= 0.2f));
		}
	}
	TestEqual(TEXT("Seventeen beats in the table"), Beats, 17);

	// The rule: identity on, a strength above 0, the sight down. Aimed, none; Identity 0, none; Beats 0, none.
	TestTrue(TEXT("Hip, identity on: plays"), ReloadBeats::Plays(true, 1.f, 0.f));
	TestFalse(TEXT("Aimed: none"), ReloadBeats::Plays(true, 1.f, 1.f));
	TestFalse(TEXT("The sight half up: none"), ReloadBeats::Plays(true, 1.f, 0.5f));
	TestFalse(TEXT("Identity 0: none"), ReloadBeats::Plays(false, 1.f, 0.f));
	TestFalse(TEXT("Strength 0: none"), ReloadBeats::Plays(true, 0.f, 0.f));
	{
		FScopedReloadVariable Off(TEXT("Looter.Gunfeel.Reload.Beats"), -3.f);
		TestEqual(TEXT("A negative strength reads as none"), ReloadBeats::Scale(), 0.f);
	}
	{
		FScopedReloadVariable Half(TEXT("Looter.Gunfeel.Reload.Beats"), 0.5f);
		TestEqual(TEXT("A changed strength is read at once"), ReloadBeats::Scale(), 0.5f);
	}
	return true;
}

#endif
