#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/FirstPersonHandsMotion.h"
#include "Player/FirstPersonHandsTypes.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/ReloadTuning.h"
#include "Weapons/WeaponModelComponent.h"
#include "HAL/IConsoleManager.h"
#include "UObject/Class.h"

// The reload's snap (Phase 3 change 12; Docs/Work/2026-11-03-Phase3/RELOAD.md): the spring curves as plain math (they start
// and end at rest, wind up, overshoot, settle), the master switch (0 gives the eased curves before; the steps, commits and
// times never move), the seat's and the slam's jolt on the recoil's spring, and the support hand's sprung handle and its
// letting the thrown magazine go. The thrown magazine itself is ReloadDropTests.cpp.

namespace
{
	/** A console variable set to Value for the scope and put back after. */
	struct FSnapTestVariable
	{
		FSnapTestVariable(const TCHAR* Name, float Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Was = Variable->GetString();
				Variable->Set(*FString::SanitizeFloat(Value), ECVF_SetByConsole);
			}
		}
		~FSnapTestVariable()
		{
			if (Variable)
			{
				Variable->Set(*Was, ECVF_SetByConsole);
			}
		}
		IConsoleVariable* Variable = nullptr;
		FString Was;
	};

	/** A curve sampled over 0-1: its lowest and highest and where, the lowest after its highest, the biggest step between samples. */
	struct FSnapSamples
	{
		float Min = 0.f;
		float MinAt = 0.f;
		float Max = 0.f;
		float MaxAt = 0.f;
		float LowestAfterMax = 1000.f;
		float BiggestStep = 0.f;
	};

	template <typename CurveType>
	FSnapSamples SnapSample(CurveType Curve, int32 Samples = 2000)
	{
		FSnapSamples Out;
		float Last = Curve(0.f);
		Out.Min = Out.Max = Last;
		for (int32 Step = 1; Step <= Samples; ++Step)
		{
			const float T = static_cast<float>(Step) / Samples;
			const float Value = Curve(T);
			Out.BiggestStep = FMath::Max(Out.BiggestStep, FMath::Abs(Value - Last));
			if (Value < Out.Min)
			{
				Out.Min = Value;
				Out.MinAt = T;
			}
			if (Value > Out.Max)
			{
				Out.Max = Value;
				Out.MaxAt = T;
				Out.LowestAfterMax = Value;
			}
			Out.LowestAfterMax = FMath::Min(Out.LowestAfterMax, Value);
			Last = Value;
		}
		return Out;
	}

	/** Every reload part with its plans: from empty and tactical (a pump gun's three shells). */
	const EWeaponReloadPart SnapTestParts[] = { EWeaponReloadPart::Magazine, EWeaponReloadPart::Helix, EWeaponReloadPart::Box, EWeaponReloadPart::Bolt,
		EWeaponReloadPart::Rocket, EWeaponReloadPart::Cylinder, EWeaponReloadPart::Pump };

	FReloadPlan SnapTestPlan(EWeaponReloadPart Part, bool bEmpty)
	{
		FReloadPlan Plan;
		Plan.bEmpty = bEmpty;
		Plan.Shells = Part == EWeaponReloadPart::Pump ? 3 : 0;
		return Plan;
	}

	/** The rifle held as a view model holds one (as the hands' own tests build it), its reload from empty. */
	FHandsGun SnapTestRifleHands()
	{
		FHandsGun Gun;
		Gun.ToCamera = FTransform(FRotator(4.f, -3.f, 10.f), FVector(28.f, 18.f, -20.f));
		Gun.Kind = EWeaponKind::Rifle;
		Gun.ReloadPart = EWeaponReloadPart::Magazine;
		Gun.Scale = UWeaponModelComponent::ScaleFor(EWeaponKind::Rifle);
		Gun.Grip = FVector(22.5f, 0.f, -10.f) * Gun.Scale;
		Gun.Foregrip = FVector(44.f, 0.f, -4.5f) * Gun.Scale;
		Gun.MagazineSeat = FVector(14.f, 0.f, -5.6f) * Gun.Scale;
		Gun.bHasMagazineSeat = true;
		return Gun;
	}

	FHandPlacement SnapTestHandAt(FHandsGun Gun, float Progress)
	{
		Gun.ReloadProgress = Progress;
		return FirstPersonHandsMotion::SupportInReload(Gun);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReloadSnapCurvesTest, "Looter.Gunfeel.Reload.Snap.Curves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FReloadSnapCurvesTest::RunTest(const FString& Parameters)
{
	// The shipped shape: the move goes 20-30% the other way first and 10-15% past.
	const ReloadTuning::FSnapShape Shape;
	TestTrue(FString::Printf(TEXT("Anticipation %.2f of the move (0.2-0.3)"), Shape.Anticipate), Shape.Anticipate >= 0.2f && Shape.Anticipate <= 0.3f);
	TestTrue(FString::Printf(TEXT("Overshoot %.2f of the move (0.1-0.15)"), Shape.Overshoot), Shape.Overshoot >= 0.1f && Shape.Overshoot <= 0.15f);

	// A move: at rest before and after, wound up in its lead, swung past, settled without a second swing, never a jump.
	TestEqual(TEXT("Snap: 0 before"), ReloadCurves::Snap(-0.3f, Shape), 0.f);
	TestEqual(TEXT("...0 at its start"), ReloadCurves::Snap(0.f, Shape), 0.f);
	TestEqual(TEXT("...1 at its end"), ReloadCurves::Snap(1.f, Shape), 1.f);
	TestEqual(TEXT("...1 after"), ReloadCurves::Snap(1.4f, Shape), 1.f);
	const FSnapSamples Move = SnapSample([&Shape](float T) { return ReloadCurves::Snap(T, Shape); });
	TestTrue(FString::Printf(TEXT("Snap winds up to %.3f by %.3f (its lead %.2f)"), Move.Min, Move.MinAt, Shape.Lead),
		FMath::IsNearlyEqual(Move.Min, -Shape.Anticipate, 0.005f) && Move.MinAt <= Shape.Lead + 0.002f);
	TestTrue(FString::Printf(TEXT("...swings past to %.3f"), Move.Max), FMath::IsNearlyEqual(Move.Max, 1.f + Shape.Overshoot, 0.005f));
	TestTrue(FString::Printf(TEXT("...settles onto 1 critically damped (lowest after the swing %.4f)"), Move.LowestAfterMax), Move.LowestAfterMax >= 1.f - 0.0001f);
	TestTrue(FString::Printf(TEXT("...never a jump (%.4f between samples)"), Move.BiggestStep), Move.BiggestStep < 0.01f);
	TestTrue(TEXT("...leaves and arrives at rest"), FMath::Abs(ReloadCurves::Snap(0.002f, Shape)) < 0.001f
		&& FMath::Abs(ReloadCurves::Snap(0.998f, Shape) - 1.f) < 0.001f);

	// Into a stop: wound up, then fastest as it hits, never past it; from a stop, no wind-up at all.
	TestEqual(TEXT("Into: 0 at its start"), ReloadCurves::SnapInto(0.f, Shape), 0.f);
	TestEqual(TEXT("...1 at the stop"), ReloadCurves::SnapInto(1.f, Shape), 1.f);
	const FSnapSamples Into = SnapSample([&Shape](float T) { return ReloadCurves::SnapInto(T, Shape); });
	TestTrue(FString::Printf(TEXT("Into winds up to %.3f"), Into.Min), FMath::IsNearlyEqual(Into.Min, -Shape.Anticipate, 0.005f));
	TestTrue(FString::Printf(TEXT("...never past the stop (%.4f)"), Into.Max), Into.Max <= 1.f);
	const float Arrival = (1.f - ReloadCurves::SnapInto(0.99f, Shape)) / 0.01f;
	TestTrue(FString::Printf(TEXT("...hits it at speed (%.2f a window, eased would be 0)"), Arrival), Arrival > 2.f);
	const FSnapSamples FromStop = SnapSample([&Shape](float T) { return ReloadCurves::SnapInto(T, Shape.NoWindUp()); });
	TestTrue(TEXT("From a stop: never the other way"), FromStop.Min >= 0.f && ReloadCurves::SnapInto(0.01f, Shape.NoWindUp()) < 0.001f);

	// A knock: out and back, wound up before the hit, peaking at 1, rebounding past rest, settled at the end.
	TestEqual(TEXT("Beat: 0 at its start"), ReloadCurves::SnapBeat(0.f, Shape), 0.f);
	TestEqual(TEXT("...0 at its end"), ReloadCurves::SnapBeat(1.f, Shape), 0.f);
	TestTrue(TEXT("...the hit at the wind-up's bottom"), FMath::IsNearlyEqual(ReloadCurves::SnapBeat(Shape.Lead, Shape), -Shape.Anticipate, 0.001f));
	const FSnapSamples Knock = SnapSample([&Shape](float T) { return ReloadCurves::SnapBeat(T, Shape); });
	TestTrue(FString::Printf(TEXT("...peaks at %.3f at %.3f"), Knock.Max, Knock.MaxAt), FMath::IsNearlyEqual(Knock.Max, 1.f, 0.005f)
		&& FMath::IsNearlyEqual(Knock.MaxAt, ReloadCurves::BeatPeakAt(Shape), 0.005f));
	TestTrue(FString::Printf(TEXT("...rebounds past rest to %.3f"), Knock.LowestAfterMax), FMath::IsNearlyEqual(Knock.LowestAfterMax, -Shape.Overshoot, 0.005f));
	TestTrue(FString::Printf(TEXT("...never a jump (%.4f)"), Knock.BiggestStep), Knock.BiggestStep < 0.02f);
	TestTrue(TEXT("...at rest as it ends"), FMath::Abs(ReloadCurves::SnapBeat(0.998f, Shape)) < 0.001f);

	// No overshoot or no lead: none of it.
	ReloadTuning::FSnapShape Plain = Shape;
	Plain.Overshoot = 0.f;
	TestTrue(TEXT("No overshoot: never past 1"), SnapSample([&Plain](float T) { return ReloadCurves::Snap(T, Plain); }).Max <= 1.0001f);
	ReloadTuning::FSnapShape NoLead = Shape;
	NoLead.Lead = 0.f;
	TestTrue(TEXT("No lead: no wind-up"), SnapSample([&NoLead](float T) { return ReloadCurves::Snap(T, NoLead); }).Min >= -0.0001f);

	// The jolt: the gun recoil's spring (9 Hz, damping 0.55) knocked from rest: a quick peak, a 10-15% rebound, rung out
	// in 120-150 ms (under 5%), nothing from JoltSeconds.
	const float Hz = ReloadTuning::Default::SnapJoltHz;
	const float Damping = ReloadTuning::Default::SnapJoltDamping;
	const float RingsFor = ReloadCurves::JoltSeconds(Hz, Damping);
	TestTrue(FString::Printf(TEXT("Jolt: cut at %.3f s"), RingsFor), RingsFor >= 0.15f && RingsFor <= 0.2f);
	float Peak = 0.f;
	float PeakAt = 0.f;
	float Rebound = 0.f;
	float LateMost = 0.f;
	for (float Seconds = 0.f; Seconds <= 0.3f; Seconds += 0.0005f)
	{
		const float Value = ReloadCurves::Jolt(Seconds, Hz, Damping);
		if (Value > Peak)
		{
			Peak = Value;
			PeakAt = Seconds;
		}
		Rebound = FMath::Min(Rebound, Value);
		LateMost = Seconds >= 0.13f ? FMath::Max(LateMost, FMath::Abs(Value)) : LateMost;
	}
	TestEqual(TEXT("...0 as it's knocked"), ReloadCurves::Jolt(0.f, Hz, Damping), 0.f);
	TestTrue(FString::Printf(TEXT("...peaks at 1 (%.3f) in %.0f ms"), Peak, PeakAt * 1000.f), FMath::IsNearlyEqual(Peak, 1.f, 0.01f) && PeakAt > 0.015f && PeakAt < 0.03f);
	TestTrue(FString::Printf(TEXT("...rebounds %.0f%% past rest"), -Rebound * 100.f), Rebound <= -0.1f && Rebound >= -0.15f);
	TestTrue(FString::Printf(TEXT("...under 5%% from 0.13 s (%.3f)"), LateMost), LateMost < 0.05f);
	TestEqual(TEXT("...nothing once rung out"), ReloadCurves::Jolt(RingsFor + 0.001f, Hz, Damping), 0.f);
	TestTrue(TEXT("A heavier gun's (a slower spring) rings longer"), ReloadCurves::JoltSeconds(Hz * 0.6f, Damping) > RingsFor);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReloadSnapSwitchTest, "Looter.Gunfeel.Reload.Snap.Switch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FReloadSnapSwitchTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* Name : { TEXT("Looter.Gunfeel.Reload.Snap"), TEXT("Looter.Gunfeel.Reload.Snap.Anticipate"), TEXT("Looter.Gunfeel.Reload.Snap.Overshoot"),
			TEXT("Looter.Gunfeel.Reload.Snap.Lead"), TEXT("Looter.Gunfeel.Reload.Snap.Settle"), TEXT("Looter.Gunfeel.Reload.Snap.Jolt"),
			TEXT("Looter.Gunfeel.Reload.Snap.JoltSeat"), TEXT("Looter.Gunfeel.Reload.Snap.JoltSlam"), TEXT("Looter.Gunfeel.Reload.Snap.JoltPitch"),
			TEXT("Looter.Gunfeel.Reload.Snap.JoltHz"), TEXT("Looter.Gunfeel.Reload.Snap.JoltDamping"), TEXT("Looter.Gunfeel.Reload.DropMag"),
			TEXT("Looter.Gunfeel.Reload.DropMag.Rest") })
	{
		TestNotNull(*FString::Printf(TEXT("%s exists"), Name), IConsoleManager::Get().FindConsoleVariable(Name));
	}
	TestTrue(TEXT("Shipped on"), ReloadTuning::Default::Snap && ReloadTuning::Default::DropMag);

	// The steps (sounds, commits) and so the reload's times never move with the switch.
	TMap<int32, TArray<FReloadStepAt>> Eased;
	FVector Offset;
	FRotator Turn;
	float SlapEased = 0.f;
	const float MagSeated = LooterReload::Steps(EWeaponReloadPart::Magazine)[1].Progress;
	{
		const FSnapTestVariable Off(TEXT("Looter.Gunfeel.Reload.Snap"), 0.f);
		TestFalse(TEXT("Off reads off"), ReloadCurves::On());
		bool bSmooth = true;
		for (float P = 0.f; P <= 1.f; P += 0.01f)
		{
			bSmooth &= FMath::IsNearlyEqual(ReloadCurves::Rise(P, 0.3f, 0.5f), FMath::SmoothStep(0.3f, 0.5f, P), 1e-6f)
				&& FMath::IsNearlyEqual(ReloadCurves::Into(P, 0.3f, 0.5f, true), FMath::SmoothStep(0.3f, 0.5f, P), 1e-6f)
				&& FMath::IsNearlyEqual(ReloadCurves::Beat(P, 0.5f, 0.05f),
					FMath::SmoothStep(0.45f, 0.5f, P) * (1.f - FMath::SmoothStep(0.5f, 0.55f, P)), 1e-5f);
		}
		TestTrue(TEXT("Off: every curve the SmoothStep before"), bSmooth);
		bool bShown = true;
		const float FreshFrom = LooterReload::MagazineTravel(LooterReload::FreshMagazineAt, bShown);
		const float Quarter = LooterReload::FreshMagazineAt + (MagSeated - LooterReload::FreshMagazineAt) * 0.25f;
		TestTrue(TEXT("Off: the fresh magazine slows as it seats, as before"),
			FMath::IsNearlyEqual(LooterReload::MagazineTravel(Quarter, bShown), FreshFrom * 0.5625f, 0.01f));
		TestTrue(TEXT("Off: the rack's fore-end home late, as before"), LooterReload::PumpCycleTravel(0.9f) > 0.f);
		LooterReload::JoltPose(EWeaponReloadPart::Magazine, SnapTestPlan(EWeaponReloadPart::Magazine, true), MagSeated + 0.01f, 2.f, 1.f,
			ReloadTuning::FJoltRules(), Offset, Turn);
		TestTrue(TEXT("Off: no jolt"), Offset.IsZero() && Turn.IsZero());
		LooterReload::ViewModelPose(EWeaponReloadPart::Magazine, MagSeated - 0.005f, Offset, Turn);
		SlapEased = static_cast<float>(Offset.Z);
		for (const EWeaponReloadPart Part : SnapTestParts)
		{
			for (const bool bEmpty : { false, true })
			{
				LooterReload::StepsFor(Part, SnapTestPlan(Part, bEmpty), Eased.FindOrAdd(static_cast<int32>(Part) * 2 + (bEmpty ? 1 : 0)));
			}
		}
	}
	{
		const FSnapTestVariable On(TEXT("Looter.Gunfeel.Reload.Snap"), 1.f);
		const ReloadTuning::FSnapShape Shape = ReloadTuning::SnapShape();
		TestTrue(TEXT("On: a rise is a Snap"), FMath::IsNearlyEqual(ReloadCurves::Rise(0.42f, 0.3f, 0.5f), ReloadCurves::Snap(0.6f, Shape), 0.0001f));
		TestTrue(TEXT("On: a knock's hit is its wind-up's bottom"), FMath::IsNearlyEqual(ReloadCurves::Beat(0.5f, 0.5f, 0.05f), -Shape.Anticipate, 0.001f));
		// The fresh magazine: cocked down under the well, then rammed home, still well out a breath before it seats.
		bool bShown = true;
		const float FreshFrom = LooterReload::MagazineTravel(LooterReload::FreshMagazineAt, bShown);
		float Deepest = 0.f;
		for (float P = LooterReload::FreshMagazineAt; P < MagSeated; P += 0.002f)
		{
			Deepest = FMath::Max(Deepest, LooterReload::MagazineTravel(P, bShown));
		}
		TestTrue(FString::Printf(TEXT("On: the fresh magazine cocked down to %.1f cm (from %.1f)"), Deepest, FreshFrom), Deepest > FreshFrom * 1.2f);
		const float Breath = LooterReload::MagazineTravel(MagSeated - 0.004f, bShown);
		// About 2 cm (1.98 on the first build): the ram's last 8 ms of a 2 s reload still cover a finger's width.
		TestTrue(FString::Printf(TEXT("...rammed home: %.2f cm out 0.004 before the seat"), Breath), Breath > 1.5f);
		TestEqual(TEXT("...seated on its step"), LooterReload::MagazineTravel(MagSeated, bShown), 0.f);
		TestTrue(TEXT("On: the rack's fore-end home sooner"), LooterReload::PumpCycleTravel(0.85f) == 0.f
			&& FMath::IsNearlyEqual(LooterReload::PumpCycleTravel(LooterReload::PumpCycleBackAt), LooterReload::PumpStroke, 0.01f));
		LooterReload::ViewModelPose(EWeaponReloadPart::Magazine, MagSeated - 0.005f, Offset, Turn);
		TestTrue(FString::Printf(TEXT("On: the gun winds down before the slap (%.2f, eased %.2f)"), Offset.Z, SlapEased), Offset.Z < SlapEased);
		for (const EWeaponReloadPart Part : SnapTestParts)
		{
			for (const bool bEmpty : { false, true })
			{
				const FString Name = FString::Printf(TEXT("%s %s"), *UEnum::GetValueAsString(Part), bEmpty ? TEXT("empty") : TEXT("tactical"));
				const FReloadPlan Plan = SnapTestPlan(Part, bEmpty);
				TArray<FReloadStepAt> Steps;
				LooterReload::StepsFor(Part, Plan, Steps);
				const TArray<FReloadStepAt>& Before = Eased.FindChecked(static_cast<int32>(Part) * 2 + (bEmpty ? 1 : 0));
				bool bSame = Steps.Num() == Before.Num();
				for (int32 Index = 0; bSame && Index < Steps.Num(); ++Index)
				{
					bSame = Steps[Index].Step == Before[Index].Step && Steps[Index].Progress == Before[Index].Progress;
				}
				TestTrue(*FString::Printf(TEXT("%s: the same steps, commits and times either way"), *Name), bSame);
				LooterReload::ViewModelPose(Part, 0.f, Plan, Offset, Turn);
				TestTrue(*FString::Printf(TEXT("%s: from the hold"), *Name), Offset.IsNearlyZero(0.01) && Turn.IsNearlyZero(0.01f));
				LooterReload::ViewModelPose(Part, 1.f, Plan, Offset, Turn);
				TestTrue(*FString::Printf(TEXT("%s: back on the hold (%s)"), *Name, *Offset.ToCompactString()), Offset.IsNearlyZero(0.05) && Turn.IsNearlyZero(0.1f));
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReloadSnapJoltTest, "Looter.Gunfeel.Reload.Snap.Jolt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FReloadSnapJoltTest::RunTest(const FString& Parameters)
{
	const FSnapTestVariable On(TEXT("Looter.Gunfeel.Reload.Snap"), 1.f);
	const ReloadTuning::FJoltRules Rules;
	TestTrue(FString::Printf(TEXT("A seat %.1f cm, a slam %.1f cm (1.5-3)"), Rules.Seat, Rules.Slam), Rules.Seat >= 1.5f && Rules.Seat <= 3.f
		&& Rules.Slam >= 1.5f && Rules.Slam <= 3.f && Rules.Slam > Rules.Seat);

	// Where each reload knocks: every seat; from empty the handle (the belt cover), the sniper's bolt, the pump home.
	auto Count = [](EWeaponReloadPart Part, bool bEmpty, int32& OutSeats, int32& OutSlams)
	{
		TArray<FReloadJoltAt> Jolts;
		LooterReload::JoltsFor(Part, SnapTestPlan(Part, bEmpty), Jolts);
		OutSeats = Jolts.FilterByPredicate([](const FReloadJoltAt& At) { return !At.bSlam; }).Num();
		OutSlams = Jolts.Num() - OutSeats;
	};
	struct FJoltCase
	{
		EWeaponReloadPart Part;
		int32 EmptySeats;
		int32 EmptySlams;
		int32 TacticalSeats;
		int32 TacticalSlams;
	};
	const FJoltCase Cases[] = {
		{ EWeaponReloadPart::Magazine, 1, 1, 1, 0 },
		{ EWeaponReloadPart::Helix, 1, 1, 1, 0 },
		{ EWeaponReloadPart::Box, 1, 1, 1, 1 },
		{ EWeaponReloadPart::Bolt, 1, 1, 1, 0 },
		{ EWeaponReloadPart::Rocket, 1, 0, 1, 0 },
		{ EWeaponReloadPart::Cylinder, 1, 0, 1, 0 },
		{ EWeaponReloadPart::Pump, 0, 1, 0, 0 },
	};
	for (const FJoltCase& Case : Cases)
	{
		int32 Seats = 0;
		int32 Slams = 0;
		Count(Case.Part, true, Seats, Slams);
		TestTrue(*FString::Printf(TEXT("%s from empty: %d seats, %d slams"), *UEnum::GetValueAsString(Case.Part), Seats, Slams),
			Seats == Case.EmptySeats && Slams == Case.EmptySlams);
		Count(Case.Part, false, Seats, Slams);
		TestTrue(*FString::Printf(TEXT("%s tactical: %d seats, %d slams"), *UEnum::GetValueAsString(Case.Part), Seats, Slams),
			Seats == Case.TacticalSeats && Slams == Case.TacticalSlams);
	}

	// A tactical magazine's raw clock and its choreography (the handle folded away) agree both ways.
	const FReloadPlan Tactical = SnapTestPlan(EWeaponReloadPart::Magazine, false);
	for (const float Progress : { 0.1f, 0.5f, 0.62f, 0.9f })
	{
		const float Back = LooterReload::ChoreographyProgress(EWeaponReloadPart::Magazine, Tactical,
			LooterReload::RawProgressOf(EWeaponReloadPart::Magazine, Tactical, Progress));
		TestTrue(*FString::Printf(TEXT("Raw and back at %.2f (%.4f)"), Progress, Back), FMath::IsNearlyEqual(Back, Progress, 0.0001f));
	}

	// The rifle from empty, 2.31 s: nothing before the seat, back and down by the seat's size at the spring's peak, the
	// muzzle dipped with it, gone once rung out; the handle's slam bigger.
	const FReloadPlan Empty = SnapTestPlan(EWeaponReloadPart::Magazine, true);
	TArray<FReloadJoltAt> Jolts;
	LooterReload::JoltsFor(EWeaponReloadPart::Magazine, Empty, Jolts);
	if (!TestEqual(TEXT("The rifle: a seat and a slam"), Jolts.Num(), 2))
	{
		return false;
	}
	const float Motion = 2.31f;
	const float PeakSeconds = [&Rules]
	{
		float Best = 0.f;
		float BestAt = 0.f;
		for (float Seconds = 0.f; Seconds < 0.1f; Seconds += 0.0002f)
		{
			const float Value = ReloadCurves::Jolt(Seconds, Rules.Hz, Rules.Damping);
			BestAt = Value > Best ? Seconds : BestAt;
			Best = FMath::Max(Best, Value);
		}
		return BestAt;
	}();
	FVector Offset;
	FRotator Turn;
	LooterReload::JoltPose(EWeaponReloadPart::Magazine, Empty, Jolts[0].Progress - 0.01f, Motion, 1.f, Rules, Offset, Turn);
	TestTrue(TEXT("Nothing before the seat"), Offset.IsZero() && Turn.IsZero());
	LooterReload::JoltPose(EWeaponReloadPart::Magazine, Empty, Jolts[0].Progress + PeakSeconds / Motion, Motion, 1.f, Rules, Offset, Turn);
	TestTrue(FString::Printf(TEXT("The seat: %.2f cm back, %.2f down (%.2f cm)"), -Offset.X, -Offset.Z, Offset.Size()),
		Offset.X < 0.0 && Offset.Z < 0.0 && FMath::IsNearlyEqual(static_cast<float>(Offset.Size()), Rules.Seat, 0.05f));
	TestTrue(FString::Printf(TEXT("...the muzzle dipped %.2f degrees"), -Turn.Pitch), FMath::IsNearlyEqual(Turn.Pitch, -Rules.Pitch * Rules.Seat, 0.05f));
	LooterReload::JoltPose(EWeaponReloadPart::Magazine, Empty, Jolts[0].Progress + 0.2f / Motion, Motion, 1.f, Rules, Offset, Turn);
	TestTrue(TEXT("...rung out 0.2 s on"), Offset.IsZero() && Turn.IsZero());
	LooterReload::JoltPose(EWeaponReloadPart::Magazine, Empty, Jolts[1].Progress + PeakSeconds / Motion, Motion, 1.f, Rules, Offset, Turn);
	TestTrue(FString::Printf(TEXT("The handle's slam: %.2f cm"), Offset.Size()), FMath::IsNearlyEqual(static_cast<float>(Offset.Size()), Rules.Slam, 0.05f));
	ReloadTuning::FJoltRules Silent = Rules;
	Silent.Scale = 0.f;
	LooterReload::JoltPose(EWeaponReloadPart::Magazine, Empty, Jolts[0].Progress + PeakSeconds / Motion, Motion, 1.f, Silent, Offset, Turn);
	TestTrue(TEXT("Scale 0: none"), Offset.IsZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReloadSnapHandsTest, "Looter.Gunfeel.Reload.Snap.Hands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FReloadSnapHandsTest::RunTest(const FString& Parameters)
{
	using namespace FirstPersonHandsMotion;
	const FHandsGun Gun = SnapTestRifleHands();

	// The charging handle: snapped, nudged forward first, yanked past its pull and settled on it; eased, never outside it.
	const FVector Rest = OnHandle(Gun, 0.f).Hold;
	const FVector Pull = OnHandle(Gun, 1.f).Hold - Rest;
	float HandleStep = 0.72f;
	for (const FReloadStepAt& At : LooterReload::Steps(EWeaponReloadPart::Magazine))
	{
		HandleStep = At.Step == EReloadStep::Bolt ? At.Progress : HandleStep;
	}
	// The hand takes the handle this long before its step (FirstPersonHandsReload.cpp HandleBefore) and yanks it over 0.04.
	const float HandleOn = HandleStep - 0.02f;
	for (const float Snap : { 0.f, 1.f })
	{
		const FSnapTestVariable Snapped(TEXT("Looter.Gunfeel.Reload.Snap"), Snap);
		float Least = 0.f;
		float Most = 0.f;
		for (int32 Step = 0; Step <= 80; ++Step)
		{
			const FVector Hand = SnapTestHandAt(Gun, HandleOn + 0.04f * Step / 80.f).Hold;
			const float Along = static_cast<float>(FVector::DotProduct(Hand - Rest, Pull) / FMath::Max(Pull.SizeSquared(), 0.0001));
			Least = FMath::Min(Least, Along);
			Most = FMath::Max(Most, Along);
		}
		if (Snap > 0.f)
		{
			TestTrue(FString::Printf(TEXT("Snapped: the hand cocks forward before the yank (%.2f of the pull)"), Least), Least < -0.1f);
			TestTrue(FString::Printf(TEXT("...and is yanked past the pull (%.2f)"), Most), Most > 1.05f && Most < 1.25f);
		}
		else
		{
			TestTrue(FString::Printf(TEXT("Eased: the yank stays inside the pull (%.2f to %.2f)"), Least, Most), Least > -0.001f && Most < 1.001f);
		}
	}

	// The thrown magazine: the hand lets go as the old one clears the well (Looter.Gunfeel.Reload.DropMag 1); off, it rides
	// the old one's fall a little way as before.
	auto OffMagazine = [&Gun](float Progress)
	{
		bool bShown = true;
		return static_cast<float>(FVector::Dist(SnapTestHandAt(Gun, Progress).Hold, OnMagazine(Gun, LooterReload::MagazineTravel(Progress, bShown)).Hold));
	};
	{
		const FSnapTestVariable Thrown(TEXT("Looter.Gunfeel.Reload.DropMag"), 1.f);
		const float Off = OffMagazine(LooterReload::MagazineClearAt);
		TestTrue(FString::Printf(TEXT("Thrown: the hand on the magazine as it lets it go (%.2f cm off)"), Off), Off <= 0.5f);
		const float Gone = OffMagazine(LooterReload::MagazineClearAt + 0.06f);
		TestTrue(FString::Printf(TEXT("...and off after (%.1f cm from where it would fall)"), Gone), Gone > 2.f);
	}
	{
		const FSnapTestVariable Kept(TEXT("Looter.Gunfeel.Reload.DropMag"), 0.f);
		const float Off = OffMagazine(LooterReload::MagazineClearAt + 0.01f);
		TestTrue(FString::Printf(TEXT("Not thrown: the hand still on it past the well (%.2f cm off)"), Off), Off <= 0.5f);
	}
	return true;
}

#endif
