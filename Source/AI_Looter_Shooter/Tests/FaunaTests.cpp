#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/FaunaRules.h"
#include "World/FaunaTypes.h"

namespace
{
	FFaunaThreat Player(const FVector& Where, float Fear = 1.f)
	{
		FFaunaThreat Threat;
		Threat.Location = Where;
		Threat.Fear = Fear;
		Threat.bPlayer = true;
		return Threat;
	}

	FFaunaNoise Noise(EFaunaNoise Kind, const FVector& Where, double Time, float Radius = 8000.f)
	{
		FFaunaNoise Made;
		Made.Kind = Kind;
		Made.Location = Where;
		Made.Time = Time;
		Made.Radius = Radius;
		return Made;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaFearTest, "Looter.World.Fauna.Rules.Fear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaFearTest::RunTest(const FString& Parameters)
{
	// How near a bird lets the player come: nearer creeping, farther running; creatures startle it less than a man does.
	TestEqual(TEXT("A walking player scares at the flee radius"), FaunaRules::FearOf(true, false, FaunaRules::WalkSpeed), 1.f);
	TestEqual(TEXT("...standing still, at 0.8 of it"), FaunaRules::FearOf(true, false, 0.f), 0.8f);
	TestEqual(TEXT("...sprinting, at 1.35"), FaunaRules::FearOf(true, false, FaunaRules::SprintSpeed), 1.35f);
	TestEqual(TEXT("...faster than a sprint, no farther"), FaunaRules::FearOf(true, false, 2000.f), 1.35f);
	TestEqual(TEXT("...crouch-walking, at 0.6"), FaunaRules::FearOf(true, true, FaunaRules::WalkSpeed), 0.6f);
	TestEqual(TEXT("A creature, at 0.7 whatever it does"), FaunaRules::FearOf(false, false, 900.f), 0.7f);

	// The flee radius (12 m, a crow's).
	const FVector Bird(0.0, 0.0, 100.0);
	const float Radius = 1200.f;
	TestTrue(TEXT("A walking player 11 m off startles it"), FaunaRules::IsThreatened(Bird, Player(FVector(1100.0, 0.0, 100.0)), Radius));
	TestFalse(TEXT("...13 m off doesn't"), FaunaRules::IsThreatened(Bird, Player(FVector(1300.0, 0.0, 100.0)), Radius));
	TestFalse(TEXT("A crouching one 8 m off doesn't"), FaunaRules::IsThreatened(Bird, Player(FVector(800.0, 0.0, 100.0), 0.6f), Radius));
	TestTrue(TEXT("...but 7 m off does"), FaunaRules::IsThreatened(Bird, Player(FVector(700.0, 0.0, 100.0), 0.6f), Radius));
	TestTrue(TEXT("Walking right under a bird on a 6 m roof startles it"),
		FaunaRules::IsThreatened(FVector(0.0, 0.0, 700.0), Player(FVector(0.0, 0.0, 100.0)), Radius));
	TestFalse(TEXT("...under one 25 m up doesn't"), FaunaRules::IsThreatened(FVector(0.0, 0.0, 2600.0), Player(FVector(0.0, 0.0, 100.0)), Radius));

	TArray<FFaunaThreat> Threats = { Player(FVector(5000.0, 0.0, 0.0)), Player(FVector(0.0, 900.0, 100.0)) };
	TestTrue(TEXT("Any one threat near is enough"), FaunaRules::IsThreatenedByAny(Bird, Threats, Radius));
	float Distance = 0.f;
	TestEqual(TEXT("The nearest threat is the one 9 m off"), FaunaRules::NearestThreat(Bird, Threats, Distance), 1);
	TestEqual(TEXT("...9 m off"), Distance, 900.f, 1.f);
	TestFalse(TEXT("No threats, no fear"), FaunaRules::IsThreatenedByAny(Bird, TArray<FFaunaThreat>(), Radius));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaNoiseTest, "Looter.World.Fauna.Rules.Gunfire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaNoiseTest::RunTest(const FString& Parameters)
{
	// A gunshot within the flock's gunfire radius startles it; a bullet's strike only nearer; a noise is heard once.
	const FVector Flock(0.0, 0.0, 0.0);
	const float Gunfire = 5000.f;
	const float Impact = 1500.f;
	const double Since = 10.0;
	TestTrue(TEXT("A shot 40 m off is heard"), FaunaRules::Hears(Flock, Noise(EFaunaNoise::Gunshot, FVector(4000.0, 0.0, 0.0), 10.5), Since, Gunfire, Impact));
	TestFalse(TEXT("...60 m off isn't"), FaunaRules::Hears(Flock, Noise(EFaunaNoise::Gunshot, FVector(6000.0, 0.0, 0.0), 10.5), Since, Gunfire, Impact));
	TestFalse(TEXT("...nor one from before the last update (heard already)"),
		FaunaRules::Hears(Flock, Noise(EFaunaNoise::Gunshot, FVector(1000.0, 0.0, 0.0), 9.5), Since, Gunfire, Impact));
	TestTrue(TEXT("A bullet striking 12 m off is heard"), FaunaRules::Hears(Flock, Noise(EFaunaNoise::Impact, FVector(0.0, 1200.0, 0.0), 10.5), Since, Gunfire, Impact));
	TestFalse(TEXT("...20 m off isn't"), FaunaRules::Hears(Flock, Noise(EFaunaNoise::Impact, FVector(0.0, 2000.0, 0.0), 10.5), Since, Gunfire, Impact));
	TestTrue(TEXT("Another noise carries its own radius"),
		FaunaRules::Hears(Flock, Noise(EFaunaNoise::Other, FVector(7000.0, 0.0, 0.0), 10.5, 8000.f), Since, Gunfire, Impact));

	const TArray<FFaunaNoise> Noises = { Noise(EFaunaNoise::Gunshot, FVector(9000.0, 0.0, 0.0), 10.5),
		Noise(EFaunaNoise::Impact, FVector(500.0, 0.0, 0.0), 10.6) };
	TestEqual(TEXT("The first heard is the near strike, not the far shot"), FaunaRules::FirstHeard(Flock, Noises, Since, Gunfire, Impact), 1);
	TestEqual(TEXT("After both, nothing new"), FaunaRules::FirstHeard(Flock, Noises, 11.0, Gunfire, Impact), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaRatesTest, "Looter.World.Fauna.Rules.Culling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaRatesTest::RunTest(const FString& Parameters)
{
	// How often fauna updates by the view, when it hides, and how many insects a swarm may have out.
	TestEqual(TEXT("On screen 10 m off: every frame"), FaunaRules::UpdateInterval(1000.f, true, false), 0.f);
	TestEqual(TEXT("On screen 45 m off at rest: 30 a second"), FaunaRules::UpdateInterval(4500.f, true, false), 1.f / 30.f, 1e-4f);
	TestEqual(TEXT("...busy (birds up): every frame"), FaunaRules::UpdateInterval(4500.f, true, true), 0.f);
	TestEqual(TEXT("On screen 80 m off at rest: 15 a second"), FaunaRules::UpdateInterval(8000.f, true, false), 1.f / 15.f, 1e-4f);
	TestEqual(TEXT("...busy: 30 a second"), FaunaRules::UpdateInterval(8000.f, true, true), 1.f / 30.f, 1e-4f);
	TestEqual(TEXT("Off screen at rest: 4 a second, however near"), FaunaRules::UpdateInterval(500.f, false, false), 0.25f, 1e-4f);
	TestEqual(TEXT("Off screen busy: 10 a second"), FaunaRules::UpdateInterval(500.f, false, true), 0.1f, 1e-4f);

	const float Cull = 8000.f;
	TestTrue(TEXT("Shown 79 m off"), FaunaRules::ShouldShow(7900.f, Cull, false));
	TestFalse(TEXT("Not shown 85 m off when hidden"), FaunaRules::ShouldShow(8500.f, Cull, false));
	TestTrue(TEXT("...but kept shown at 85 m once shown (no flicker at the edge)"), FaunaRules::ShouldShow(8500.f, Cull, true));
	TestFalse(TEXT("...hidden past the hysteresis"), FaunaRules::ShouldShow(Cull * FaunaRules::CullHysteresis + 10.f, Cull, true));
	TestTrue(TEXT("A cull distance of 0 never hides it (the hawks)"), FaunaRules::ShouldShow(1.0e6f, 0.f, false));

	TestEqual(TEXT("Cap 12 with 10 out: room for 2 of 5 wanted"), FaunaRules::SwarmRoom(12, 10, 5), 2);
	TestEqual(TEXT("...with 12 out, none"), FaunaRules::SwarmRoom(12, 12, 5), 0);
	TestEqual(TEXT("...over the cap, none (never negative)"), FaunaRules::SwarmRoom(12, 14, 5), 0);
	TestEqual(TEXT("...wanting fewer than there are, none"), FaunaRules::SwarmRoom(12, 3, -2), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaMathTest, "Looter.World.Fauna.Rules.Motion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaMathTest::RunTest(const FString& Parameters)
{
	// The seeded randomness: the same on every run, spread over [0, 1), different for different seeds.
	TestEqual(TEXT("The same seed and index give the same number"), FaunaRules::Random01(7, 3), FaunaRules::Random01(7, 3));
	double Sum = 0.0;
	int32 Differ = 0;
	bool bInRange = true;
	for (uint32 Index = 0; Index < 400; ++Index)
	{
		const float Value = FaunaRules::Random01(20261008, Index);
		bInRange &= Value >= 0.f && Value < 1.f;
		Sum += Value;
		Differ += FaunaRules::Random01(1, Index) != FaunaRules::Random01(2, Index) ? 1 : 0;
	}
	TestTrue(TEXT("Every number in [0, 1)"), bInRange);
	TestEqual(TEXT("...averaging a half"), Sum / 400.0, 0.5, 0.06);
	TestTrue(TEXT("Two seeds give different numbers"), Differ > 390);
	const float Ranged = FaunaRules::RandomRange(5, 9, 10.f, 20.f);
	TestTrue(TEXT("A range's number lies in it"), Ranged >= 10.f && Ranged < 20.f);

	// A wing's turn: both tips rise alike for the same angle, and sweep back alike.
	const FVector LeftTip(0.0, -100.0, 0.0);
	const FVector RightTip(0.0, 100.0, 0.0);
	const FVector LeftUp = FaunaRules::WingTurn(-1, 30.f, 0.f).RotateVector(LeftTip);
	const FVector RightUp = FaunaRules::WingTurn(1, 30.f, 0.f).RotateVector(RightTip);
	TestTrue(TEXT("Up 30 degrees raises the left wing's tip"), LeftUp.Z > 45.0);
	TestTrue(TEXT("...and the right one's"), RightUp.Z > 45.0);
	TestEqual(TEXT("...the same height"), LeftUp.Z, RightUp.Z, 0.01);
	TestTrue(TEXT("...each still out on its own side"), LeftUp.Y < -80.0 && RightUp.Y > 80.0);
	const FVector LeftBack = FaunaRules::WingTurn(-1, 0.f, 20.f).RotateVector(LeftTip);
	const FVector RightBack = FaunaRules::WingTurn(1, 0.f, 20.f).RotateVector(RightTip);
	TestTrue(TEXT("Back 20 degrees sweeps the left tip back"), LeftBack.X < -30.0);
	TestTrue(TEXT("...and the right one"), RightBack.X < -30.0);
	TestTrue(TEXT("Down (a negative angle) lowers a tip"), FaunaRules::WingTurn(1, -20.f, 0.f).RotateVector(RightTip).Z < -30.0);

	// The flight curves end where they're meant to, at the speeds asked.
	const FVector P0(0.0, 0.0, 100.0);
	const FVector V0(0.0, 0.0, 600.0);
	const FVector P1(1500.0, 300.0, 1200.0);
	const FVector V1(900.0, 0.0, 0.0);
	FVector Velocity;
	TestTrue(TEXT("A curve starts at its start"), FaunaRules::Hermite(P0, V0, P1, V1, 2.f, 0.f, &Velocity).Equals(P0, 0.01));
	TestTrue(TEXT("...leaving at its starting velocity"), Velocity.Equals(V0, 0.5));
	TestTrue(TEXT("...and ends at its end"), FaunaRules::Hermite(P0, V0, P1, V1, 2.f, 1.f, &Velocity).Equals(P1, 0.01));
	TestTrue(TEXT("...arriving at its velocity"), Velocity.Equals(V1, 0.5));
	TestTrue(TEXT("Past its end it stays there"), FaunaRules::Hermite(P0, V0, P1, V1, 2.f, 1.5f).Equals(P1, 0.01));
	TestEqual(TEXT("A flight's time is its length at its speed"), FaunaRules::FlightSeconds(1800.f, 900.f, 1.f, 5.f), 2.f, 1e-4f);
	TestEqual(TEXT("...no shorter than its least"), FaunaRules::FlightSeconds(100.f, 900.f, 1.f, 5.f), 1.f);
	TestEqual(TEXT("...no longer than its most"), FaunaRules::FlightSeconds(90000.f, 900.f, 1.f, 5.f), 5.f);

	// Banking into a turn: right (yaw growing) is a positive roll, the right wing down.
	TestTrue(TEXT("Turning right banks right"), FaunaRules::BankDegrees(900.f, 0.5f, 50.f) > 10.f);
	TestTrue(TEXT("Turning left banks left"), FaunaRules::BankDegrees(900.f, -0.5f, 50.f) < -10.f);
	TestEqual(TEXT("...never past the widest bank"), FaunaRules::BankDegrees(5000.f, 5.f, 50.f), 50.f);
	TestEqual(TEXT("Turning the short way round"), FaunaRules::StepAngle(170.f, -170.f, 5.f), 175.f, 1e-3f);

	// A slope under a ground perch: 10 degrees up toward +X.
	FFaunaPerch Slope;
	Slope.Kind = EFaunaPerchKind::Ground;
	Slope.Location = FVector(0.0, 0.0, 50.0);
	Slope.Normal = FVector(-FMath::Sin(FMath::DegreesToRadians(10.f)), 0.0, FMath::Cos(FMath::DegreesToRadians(10.f)));
	TestEqual(TEXT("A metre uphill on a 10 degree slope is about 17.6 cm higher"),
		FaunaRules::GroundNear(Slope, FVector2D(100.0, 0.0)).Z, 50.0 + 17.63, 0.1);
	TestEqual(TEXT("...and across it level"), FaunaRules::GroundNear(Slope, FVector2D(0.0, 100.0)).Z, 50.0, 0.01);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaBounceTest, "Looter.World.Fauna.Rules.Bounce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaBounceTest::RunTest(const FString& Parameters)
{
	// A tumbleweed glancing off a fence: what goes into it comes back at the restitution, what runs along it stays.
	const FVector Wall(-1.0, 0.0, 0.0);
	TestTrue(TEXT("Straight in at 100, back at 50 (restitution 0.5)"), FaunaRules::Bounce(FVector(100.0, 0.0, 0.0), Wall, 0.5f).Equals(FVector(-50.0, 0.0, 0.0), 0.01));
	TestTrue(TEXT("Running along it, unchanged"), FaunaRules::Bounce(FVector(0.0, 100.0, 0.0), Wall, 0.5f).Equals(FVector(0.0, 100.0, 0.0), 0.01));
	TestTrue(TEXT("Already leaving it, unchanged"), FaunaRules::Bounce(FVector(-100.0, 0.0, 0.0), Wall, 0.5f).Equals(FVector(-100.0, 0.0, 0.0), 0.01));
	const FVector Slanted = FaunaRules::Bounce(FVector(100.0, 100.0, 0.0), Wall, 0.35f);
	TestTrue(TEXT("At a slant it leaves the wall"), FVector::DotProduct(Slanted, Wall) >= 0.0);
	TestTrue(TEXT("...keeping its run along it"), FMath::IsNearlyEqual(Slanted.Y, 100.0, 0.01));
	TestTrue(TEXT("...and slower than it came"), Slanted.Size() < FVector(100.0, 100.0, 0.0).Size());
	TestTrue(TEXT("Restitution 0 stops it dead against the wall"), FMath::IsNearlyZero(FaunaRules::Bounce(FVector(100.0, 0.0, 0.0), Wall, 0.f).X, 0.01));
	return true;
}

#endif
