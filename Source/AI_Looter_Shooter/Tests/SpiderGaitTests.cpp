#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The brown spider's weighty gait as plain rules (Creatures/SpiderGaitRules; the user, 2026-10-31: "their animations need
// improved upon"): steps that lift first and plant firmly, legs that never straighten into spikes, the body riding its steps,
// the bite's coil, threat display, tell and lunge, a landing's dip. No world: the numbers only. The pose in a level is
// Looter.Creatures.Anim.Spider* (CreatureAnimTests.cpp, CreatureAnimMotionTests.cpp).

#include "Creatures/SpiderGaitRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderGaitStepsTest, "Looter.Creatures.Spider.Gait.Steps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderGaitStepsTest::RunTest(const FString& Parameters)
{
	// SK_Spider's legs reach about 205 cm (femur 90, tibia 115).
	constexpr float Reach = 205.f;
	float LastSeconds = TNumericLimits<float>::Max();
	float LastStray = 0.f;
	for (float Speed = 0.f; Speed <= 1200.f; Speed += 50.f)
	{
		const float Seconds = SpiderGait::StepSeconds(Speed);
		const float Stray = SpiderGait::StepStray(Speed, Reach);
		TestTrue(FString::Printf(TEXT("At %.0f cm/s a step takes 0.11-0.27 s, no longer than slower (%.3f)"), Speed, Seconds),
			Seconds >= 0.11f - KINDA_SMALL_NUMBER && Seconds <= 0.27f + KINDA_SMALL_NUMBER && Seconds <= LastSeconds + KINDA_SMALL_NUMBER);
		TestTrue(FString::Printf(TEXT("At %.0f cm/s a foot strays at most a quarter of its reach before it steps (%.0f cm), no less than slower"),
			Speed, Stray), Stray <= SpiderGait::MaxStrayShare * Reach + KINDA_SMALL_NUMBER && Stray >= LastStray - KINDA_SMALL_NUMBER);
		LastSeconds = Seconds;
		LastStray = Stray;
	}
	TestTrue(TEXT("A trailing leg steps before the solve would have to draw it short, and no leg is posed past 0.9 of its reach (it read as a spike at 0.92, 0.98 and 0.999)"),
		SpiderGait::OverreachShare < SpiderGait::SevereOverreachShare && SpiderGait::SevereOverreachShare < SpiderGait::MaxReachShare
		&& SpiderGait::MaxReachShare <= 0.9f + KINDA_SMALL_NUMBER);
	TestTrue(TEXT("A faster step lifts higher"), SpiderGait::StepHeight(700.f) > SpiderGait::StepHeight(0.f));

	// The step's path: up first, across, down firmly onto its spot.
	TestNearlyEqual(TEXT("A step starts on its spot"), SpiderGait::StepAlong(0.f), 0.f, 1e-4f);
	TestNearlyEqual(TEXT("...and ends on the new one"), SpiderGait::StepAlong(1.f), 1.f, 1e-4f);
	TestTrue(TEXT("...lifting before it moves across (still on its spot as it leaves the ground)"),
		SpiderGait::StepAlong(0.05f) < 0.01f && SpiderGait::StepLift(0.05f) > 0.2f);
	float Previous = 0.f;
	float PeakAt = 0.f;
	float Peak = 0.f;
	bool bMonotonic = true;
	for (int32 Index = 0; Index <= 100; ++Index)
	{
		const float Alpha = Index / 100.f;
		const float Along = SpiderGait::StepAlong(Alpha);
		bMonotonic &= Along >= Previous - 1e-5f;
		Previous = Along;
		const float Lift = SpiderGait::StepLift(Alpha);
		if (Lift > Peak)
		{
			Peak = Lift;
			PeakAt = Alpha;
		}
	}
	TestTrue(TEXT("It never goes back along its way"), bMonotonic);
	TestTrue(FString::Printf(TEXT("Its lift peaks early (at %.2f of the step)"), PeakAt), PeakAt > 0.25f && PeakAt < 0.5f);
	TestTrue(FString::Printf(TEXT("...and comes down firmly at the end (%.2f of its height a tenth from the end)"), SpiderGait::StepLift(0.9f)),
		SpiderGait::StepLift(0.9f) < 0.5f && SpiderGait::StepLift(1.f) < 0.02f && SpiderGait::StepLift(0.f) < 0.02f);

	// A turn on the spot steps its feet as a walk of their speed.
	TestNearlyEqual(TEXT("A quarter turn a second moves a foot 1.5 m out at about 2.4 m/s"), SpiderGait::TurnSpeed(90.f, 150.f), 235.6f, 0.5f);
	TestNearlyEqual(TEXT("...either way"), SpiderGait::TurnSpeed(-90.f, 150.f), 235.6f, 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderGaitBodyTest, "Looter.Creatures.Spider.Gait.Body",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderGaitBodyTest::RunTest(const FString& Parameters)
{
	const SpiderGait::FBodyMotion Still = SpiderGait::BodyMotion(-1.f, 1.f, 0.f, 0.f, 0.f, 0.f);
	TestTrue(TEXT("Standing, its steps move its body not at all"),
		FMath::IsNearlyZero(Still.Rise) && FMath::IsNearlyZero(Still.Yaw) && FMath::IsNearlyZero(Still.Roll) && FMath::IsNearlyZero(Still.Pitch));
	const SpiderGait::FBodyMotion Left = SpiderGait::BodyMotion(0.5f, -1.f, 1.f, 0.f, 0.f, 600.f);
	const SpiderGait::FBodyMotion Right = SpiderGait::BodyMotion(0.5f, 1.f, 1.f, 0.f, 0.f, 600.f);
	TestTrue(FString::Printf(TEXT("Mid-swing at a run the body rides up a little (%.1f cm)"), Left.Rise), Left.Rise > 1.f && Left.Rise < 5.f);
	TestTrue(TEXT("...turned toward the reaching front leg (left, then right)"), Left.Yaw < 0.f && Right.Yaw > 0.f && FMath::Abs(Right.Yaw) <= 4.f);
	TestTrue(TEXT("...and slower, less"), SpiderGait::BodyMotion(0.5f, 1.f, 0.3f, 0.f, 0.f, 180.f).Rise < Right.Rise);
	const SpiderGait::FBodyMotion TurnRight = SpiderGait::BodyMotion(-1.f, 1.f, 1.f, 120.f, 0.f, 600.f);
	const SpiderGait::FBodyMotion Hard = SpiderGait::BodyMotion(-1.f, 1.f, 1.f, 600.f, 0.f, 900.f);
	TestTrue(FString::Printf(TEXT("Turning right at a run it banks into the turn (%.1f degrees)"), TurnRight.Roll), TurnRight.Roll > 1.f);
	TestTrue(TEXT("...never past 6 degrees"), Hard.Roll <= 6.f + KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Speeding up it dips its nose; stopping, it lifts it; never past 5 degrees"),
		SpiderGait::BodyMotion(-1.f, 1.f, 0.f, 0.f, 1500.f, 300.f).Pitch < 0.f && SpiderGait::BodyMotion(-1.f, 1.f, 0.f, 0.f, -1500.f, 300.f).Pitch > 0.f
		&& SpiderGait::BodyMotion(-1.f, 1.f, 0.f, 0.f, 1e6f, 300.f).Pitch >= -5.f - KINDA_SMALL_NUMBER);
	TestTrue(TEXT("A group planting at a run jolts the body more than walking"), SpiderGait::PlantDip(1.f) > SpiderGait::PlantDip(0.2f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderGaitBiteTest, "Looter.Creatures.Spider.Gait.Bite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderGaitBiteTest::RunTest(const FString& Parameters)
{
	const SpiderGait::FBitePose Start = SpiderGait::BitePose(0.f, 0.f, -1.f);
	TestTrue(TEXT("A bite starts from its stance"), FMath::IsNearlyZero(Start.Rise) && FMath::IsNearlyZero(Start.Pitch) && FMath::IsNearlyZero(Start.Lunge));
	const SpiderGait::FBitePose Coiled = SpiderGait::BitePose(SpiderGait::CoilShare, 0.f, -1.f);
	TestTrue(FString::Printf(TEXT("It coils first: down (%.0f cm), nose down (%.0f degrees), drawn back"), Coiled.Rise, Coiled.Pitch),
		Coiled.Rise < -5.f && Coiled.Pitch < -3.f && Coiled.Lunge < 0.f);
	const SpiderGait::FBitePose Reared = SpiderGait::BitePose(1.f, 0.f, -1.f);
	TestTrue(FString::Printf(TEXT("...then rears into its threat display: up (%.0f cm), nose high (%.0f degrees), front legs up and spread, fangs wide"),
		Reared.Rise, Reared.Pitch), Reared.Rise > 15.f && Reared.Pitch > 25.f && Reared.FrontRaise > 0.95f && Reared.FrontSpread > 1.3f
		&& Reared.Fang >= 30.f);
	// The tell: a quick shiver through it just before it goes, small.
	const SpiderGait::FBitePose Shiver = SpiderGait::BitePose(1.f, 0.f, 0.06f);
	TestTrue(FString::Printf(TEXT("Its tell shivers it a little (%.2f degrees off)"), FMath::Abs(Shiver.Pitch - Reared.Pitch)),
		FMath::Abs(Shiver.Pitch - Reared.Pitch) > 0.1f && FMath::Abs(Shiver.Pitch - Reared.Pitch) <= 2.f);
	// The wind-up hands over to the strike without a jump.
	const SpiderGait::FBitePose Struck = SpiderGait::BitePose(1.f, 0.001f, -1.f);
	TestTrue(TEXT("The strike starts where the wind-up ended (no pop)"),
		FMath::Abs(Struck.Rise - Reared.Rise) < 1.f && FMath::Abs(Struck.Pitch - Reared.Pitch) < 1.f && FMath::Abs(Struck.Lunge - Reared.Lunge) < 1.f);
	const SpiderGait::FBitePose Lunged = SpiderGait::BitePose(1.f, 0.3f, -1.f);
	TestTrue(FString::Printf(TEXT("The lunge throws it forward (%.0f cm), nose down, fangs shut"), Lunged.Lunge),
		Lunged.Lunge > 35.f && Lunged.Pitch < 0.f && Lunged.Fang < 1.f && Lunged.FrontRaise < 0.1f);
	const SpiderGait::FBitePose Done = SpiderGait::BitePose(1.f, 1.f, -1.f);
	TestTrue(TEXT("...and it's back in its stance by the recovery's end"),
		FMath::Abs(Done.Rise) < 0.5f && FMath::Abs(Done.Pitch) < 0.5f && FMath::Abs(Done.Lunge) < 0.5f);
	// The wind-up is smooth (no frame's change a jump, 60 frames a second through a 0.55 s wind-up); the strike is meant to be
	// sudden (the body's smoothing softens it on screen).
	float Worst = 0.f;
	SpiderGait::FBitePose Last = Start;
	for (int32 Frame = 1; Frame <= 33; ++Frame)
	{
		const float Time = Frame / 60.f;
		const SpiderGait::FBitePose Now = SpiderGait::BitePose(Time / 0.55f, (Time - 0.55f) / 0.45f, -1.f);
		Worst = FMath::Max3(Worst, FMath::Abs(Now.Rise - Last.Rise), FMath::Abs(Now.Pitch - Last.Pitch) * 0.5f);
		Last = Now;
	}
	TestTrue(FString::Printf(TEXT("No frame of the wind-up jumps (worst %.1f cm or 2 degrees)"), Worst), Worst < 4.f);
	TestTrue(TEXT("The strike is sudden: most of its reach within 0.15 s of a 0.45 s recovery"), SpiderGait::BitePose(1.f, 0.15f / 0.45f, -1.f).Lunge > 35.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderGaitLandingTest, "Looter.Creatures.Spider.Gait.Landing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderGaitLandingTest::RunTest(const FString& Parameters)
{
	TestNearlyEqual(TEXT("Stepping down a kerb, no dip"), SpiderGait::LandingDip(100.f), 0.f, 1e-4f);
	TestTrue(TEXT("A leap's landing dips it, a big fall more"), SpiderGait::LandingDip(600.f) > 5.f && SpiderGait::LandingDip(1200.f) > SpiderGait::LandingDip(600.f));
	TestTrue(TEXT("...never past its most"), SpiderGait::LandingDip(1e6f) <= SpiderGait::LandingDipMost + KINDA_SMALL_NUMBER);
	return true;
}

#endif
