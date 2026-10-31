#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The spiders' ambush as plain rules (Creatures/SpiderAmbushRules; the user's pick, 2026-10-31: "spiders drop from
// walls/ledges"): which surfaces a lurker holds and how it hangs there, when it springs (under it, passed and walking away,
// stared at, lingering, shot, called by a mate; never on a player it can't see), how (a leap in reach, else down its wall),
// the leap's arc and its landing. No world: SpiderAmbushPlayTests.cpp has a spider on a wall in a test level.

#include "Creatures/SpiderAmbushRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderAmbushSurfacesTest, "Looter.Creatures.Spider.Ambush.Surfaces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderAmbushSurfacesTest::RunTest(const FString& Parameters)
{
	using namespace SpiderAmbushRules;
	TestTrue(TEXT("Flat ground or a rock's top is a ledge"), SurfaceOf(FVector::UpVector) == ESpiderSurface::Ledge);
	TestTrue(TEXT("A vertical face is a wall"), SurfaceOf(FVector(1.0, 0.0, 0.0)) == ESpiderSurface::Wall);
	TestTrue(TEXT("A steep slope (60 degrees) is a wall"), SurfaceOf(FVector(FMath::Sin(FMath::DegreesToRadians(60.f)), 0.f,
		FMath::Cos(FMath::DegreesToRadians(60.f)))) == ESpiderSurface::Wall);
	TestTrue(TEXT("A lip leaning out over the ground is an overhang"), SurfaceOf(FVector(0.8, 0.0, -0.5).GetSafeNormal()) == ESpiderSurface::Overhang);
	TestTrue(TEXT("A ceiling is nothing it holds"), SurfaceOf(FVector::DownVector) == ESpiderSurface::None);
	TestTrue(TEXT("No normal, nothing"), SurfaceOf(FVector::ZeroVector) == ESpiderSurface::None);

	// It hangs head down on a wall or a trunk; on a ledge it looks out the way it was set.
	const FVector Wall(-1.0, 0.0, 0.0);
	const FVector Down = ClingForward(Wall, ESpiderPerchKind::Wall, 0.f);
	TestTrue(FString::Printf(TEXT("Head down a wall (%s)"), *Down.ToString()), Down.Equals(FVector::DownVector, 1e-3));
	TestTrue(TEXT("...and down a trunk"), ClingForward(FVector(0.0, 1.0, 0.0), ESpiderPerchKind::Tree, 0.f).Equals(FVector::DownVector, 1e-3));
	const FVector Tilted = ClingForward(FVector(-0.8, 0.0, 0.6).GetSafeNormal(), ESpiderPerchKind::Wall, 0.f);
	TestTrue(TEXT("Down a leaning face it runs along the face"), FMath::Abs(FVector::DotProduct(Tilted, FVector(-0.8, 0.0, 0.6).GetSafeNormal())) < 1e-3
		&& Tilted.Z < -0.5);
	const FVector Out = ClingForward(FVector::UpVector, ESpiderPerchKind::Ledge, 90.f);
	TestTrue(FString::Printf(TEXT("On a ledge it looks out its way (%s)"), *Out.ToString()), Out.Equals(FVector(0.0, 1.0, 0.0), 1e-3));
	TestTrue(TEXT("FaceWay: 0 degrees is +X, 90 is +Y"), FaceWay(0.f).Equals(FVector(1.0, 0.0, 0.0), 1e-4) && FaceWay(90.f).Equals(FVector(0.0, 1.0, 0.0), 1e-4));
	const FQuat Hang = ClingRotation(Wall, Down);
	TestTrue(TEXT("Clinging, its back is out from the wall and its head along its way"),
		Hang.GetUpVector().Equals(Wall, 1e-3) && Hang.GetForwardVector().Equals(Down, 1e-3));

	// Watching: its head turns along the surface toward the player, never past its most, a step at a time.
	const FVector Toward(0.0, 1.0, -0.2);
	const FVector Step = TurnWatch(Down, Down, Toward, Wall, 70.f, 10.f);
	const float Turned = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(FVector::DotProduct(Step, Down)), -1.f, 1.f)));
	TestNearlyEqual(TEXT("It turns its head a step toward them"), Turned, 10.f, 0.1f);
	TestTrue(TEXT("...along the surface"), FMath::Abs(FVector::DotProduct(Step, Wall)) < 1e-3);
	FVector Watch = Down;
	for (int32 Index = 0; Index < 40; ++Index)
	{
		Watch = TurnWatch(Watch, Down, FVector(0.0, 1.0, 1.0), Wall, 70.f, 10.f);
	}
	const float Most = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(FVector::DotProduct(Watch, Down)), -1.f, 1.f)));
	TestNearlyEqual(TEXT("...never past its most (70 degrees off its hang)"), Most, 70.f, 0.2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderAmbushSpringTest, "Looter.Creatures.Spider.Ambush.Spring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderAmbushSpringTest::RunTest(const FString& Parameters)
{
	using namespace SpiderAmbushRules;
	const FSpiderLurkTuning Tuning;
	auto Seen = [](float Flat)
	{
		FSpiderLurkSense Sense;
		Sense.FlatDistance = Flat;
		Sense.Distance = FMath::Sqrt(Flat * Flat + 400.f * 400.f);
		Sense.bSees = true;
		return Sense;
	};
	FSpiderLurkSense Under = Seen(300.f);
	TestTrue(TEXT("A player it sees passing under it springs it"), WantsToSpring(Under, Tuning));
	Under.bSees = false;
	TestFalse(TEXT("...not one it can't see (behind a rock)"), WantsToSpring(Under, Tuning));
	FSpiderLurkSense Near = Seen(Tuning.SpringRadius + 200.f);
	TestFalse(TEXT("Near but not under, walking toward it, it waits"), WantsToSpring(Near, Tuning));
	Near.bMovingAway = true;
	TestTrue(TEXT("...walking on past it, it's on their back"), WantsToSpring(Near, Tuning));
	Near.bMovingAway = false;
	Near.StillSeconds = Tuning.LingerSeconds - 0.1f;
	TestFalse(TEXT("Standing about near it a moment, it waits"), WantsToSpring(Near, Tuning));
	Near.StillSeconds = Tuning.LingerSeconds;
	TestTrue(TEXT("...lingering, it goes"), WantsToSpring(Near, Tuning));
	Near.StillSeconds = 0.f;
	Near.LookedAtSeconds = Tuning.SpottedSeconds;
	TestTrue(TEXT("Stared at, it's found out and goes"), WantsToSpring(Near, Tuning));
	FSpiderLurkSense Far = Seen(Tuning.PassRadius + 50.f);
	Far.bMovingAway = true;
	Far.LookedAtSeconds = 10.f;
	TestFalse(TEXT("Farther than its pass radius nothing springs it"), WantsToSpring(Far, Tuning));
	Far.SpringRadius = Tuning.PassRadius + 100.f;
	TestTrue(TEXT("...unless its own spring radius reaches them"), WantsToSpring(Far, Tuning));
	FSpiderLurkSense Shot;
	Shot.bHurt = true;
	TestTrue(TEXT("Shot, it goes, seen or not"), WantsToSpring(Shot, Tuning));
	FSpiderLurkSense Called;
	Called.bCalled = true;
	TestTrue(TEXT("Called by a mate, it goes"), WantsToSpring(Called, Tuning));

	// How it springs.
	FSpiderLurkSense Reach = Seen(400.f);
	TestTrue(TEXT("In a leap's reach, seen: a leap"), ChooseSpring(Reach, ESpiderPerchKind::Wall, 1.f, Tuning) == ESpiderSpring::Leap);
	FSpiderLurkSense TooFar = Seen(2000.f);
	TooFar.Distance = 2000.f;
	TestTrue(TEXT("Out of reach on a wall: down the wall"), ChooseSpring(TooFar, ESpiderPerchKind::Wall, 1.f, Tuning) == ESpiderSpring::Climb);
	TestTrue(TEXT("...on a ledge it can only jump"), ChooseSpring(TooFar, ESpiderPerchKind::Ledge, 1.f, Tuning) == ESpiderSpring::Leap);
	FSpiderLurkSense Above = Seen(400.f);
	Above.Rise = Tuning.LeapHighest + 50.f;
	TestTrue(TEXT("A player well above its perch's foot: down the wall"), ChooseSpring(Above, ESpiderPerchKind::Tree, 1.f, Tuning) == ESpiderSpring::Climb);
	FSpiderLurkSense Blind = Seen(400.f);
	Blind.bSees = false;
	TestTrue(TEXT("Unseen (shot from cover): down the wall"), ChooseSpring(Blind, ESpiderPerchKind::Wall, 1.f, Tuning) == ESpiderSpring::Climb);
	TestTrue(TEXT("A bigger spider leaps farther"), CanLeap(Tuning.LeapReach * 1.5f, 0.f, true, 1.6f, Tuning) && !CanLeap(Tuning.LeapReach * 1.5f, 0.f, true, 1.f, Tuning));
	const float Delay = MateDelay(0.5f, Tuning);
	TestTrue(TEXT("A mate's call comes a moment later"), Delay >= Tuning.MateDelay[0] && Delay <= Tuning.MateDelay[1]
		&& MateDelay(0.f, Tuning) < MateDelay(1.f, Tuning));
	TestTrue(TEXT("Climbing down, it hops off once low"), ClimbDone(Tuning.ClimbHopHeight - 1.f, 1.f, Tuning) && !ClimbDone(400.f, 1.f, Tuning));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderAmbushLeapTest, "Looter.Creatures.Spider.Ambush.Leap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderAmbushLeapTest::RunTest(const FString& Parameters)
{
	using namespace SpiderAmbushRules;
	const FSpiderLurkTuning Tuning;
	constexpr float Gravity = -980.f;
	struct FCase
	{
		FVector From;
		FVector To;
	};
	const FCase Cases[] = {
		{ FVector(0.0, 0.0, 400.0), FVector(600.0, 0.0, 62.0) },     // off a wall, down onto the path
		{ FVector(0.0, 0.0, 62.0), FVector(500.0, 300.0, 62.0) },    // across flat ground
		{ FVector(0.0, 0.0, 900.0), FVector(300.0, -200.0, 62.0) },  // a long drop
		{ FVector(0.0, 0.0, 62.0), FVector(400.0, 0.0, 150.0) },     // up a little
	};
	for (const FCase& Case : Cases)
	{
		const float Seconds = LeapSeconds(Case.From, Case.To);
		const FVector Launch = LeapVelocity(Case.From, Case.To, Seconds, Gravity);
		const FVector Landed = LeapPositionAt(Case.From, Launch, Seconds, Gravity);
		TestTrue(FString::Printf(TEXT("A leap from %s lands where it was aimed (%s, %.1f cm off)"), *Case.From.ToString(), *Case.To.ToString(),
			FVector::Dist(Landed, Case.To)), FVector::Dist(Landed, Case.To) < 0.5);
		TestTrue(FString::Printf(TEXT("...in a believable time (%.2f s)"), Seconds), Seconds >= 0.45f && Seconds <= 0.85f);
	}
	TestTrue(TEXT("A longer leap flies longer"), LeapSeconds(FVector::ZeroVector, FVector(1400.0, 0.0, 0.0)) > LeapSeconds(FVector::ZeroVector, FVector(300.0, 0.0, 0.0)));

	// Where it comes down: short of a still player, along its way; ahead of a moving one, never past them.
	const FVector From(0.0, 0.0, 400.0);
	const FVector Prey(800.0, 0.0, 0.0);
	const FVector Still = LeapLanding(From, Prey, FVector::ZeroVector, 1.f, Tuning);
	TestNearlyEqual(TEXT("It lands short of a still player by LandShort"), static_cast<float>(FVector::Dist2D(Still, Prey)), Tuning.LandShort, 0.5f);
	TestTrue(TEXT("...on their side of them, on their ground"), Still.X < Prey.X && FMath::IsNearlyEqual(Still.Z, Prey.Z));
	const FVector Running = LeapLanding(From, Prey, FVector(0.0, 400.0, 0.0), 1.f, Tuning);
	TestTrue(FString::Printf(TEXT("A running player is led (landing at Y %.0f)"), Running.Y), Running.Y > 60.f);
	const FVector Close = LeapLanding(FVector(0.0, 0.0, 300.0), FVector(100.0, 0.0, 0.0), FVector::ZeroVector, 1.f, Tuning);
	TestTrue(TEXT("A player right under it: it lands between, not past"), Close.X > 0.0 && Close.X < 100.0);

	// Its landing bites a player it came down on, not one it missed.
	TestTrue(TEXT("Landing beside them bites"), LandingBites(Still, Prey, 1.f, Tuning));
	TestFalse(TEXT("Landing 3 m off doesn't"), LandingBites(Prey - FVector(300.0, 0.0, 0.0), Prey, 1.f, Tuning));
	TestFalse(TEXT("...nor on a ledge 3 m over them"), LandingBites(Prey + FVector(-50.0, 0.0, 300.0), Prey, 1.f, Tuning));
	return true;
}

#endif
