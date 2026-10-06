#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureUpdateRate.h"
#include "Creatures/HuntingGround.h"
#include "Creatures/ShroudChain.h"
#include "Creatures/UnpaidRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidPhaseStepTest, "Looter.Creatures.Unpaid.PhaseStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidPhaseStepTest::RunTest(const FString& Parameters)
{
	// Stuck, or fallen far behind while it chases, an Unpaid fades out and comes back 3 to 5 m nearer its target: never
	// into a wall, onto another level of ground or off its hunting ground, and never on top of the player.
	const FPhaseStepRules Rules;
	TestNearlyEqual(TEXT("A step is at least 3 m"), Rules.MinStep, 300.f, 0.01f);
	TestNearlyEqual(TEXT("and at most 5 m"), Rules.MaxStep, 500.f, 0.01f);

	// When.
	TestTrue(TEXT("Far behind, it steps"), UnpaidRules::WantsPhaseStep(Rules, Rules.FarBehind + 100.f, 0.f, 10.f));
	TestFalse(TEXT("Near and gaining, it doesn't"), UnpaidRules::WantsPhaseStep(Rules, 900.f, 0.2f, 10.f));
	TestTrue(TEXT("Stuck, it steps"), UnpaidRules::WantsPhaseStep(Rules, 900.f, Rules.StuckSeconds + 0.1f, 10.f));
	TestFalse(TEXT("Not before its last step's cooldown is over"), UnpaidRules::WantsPhaseStep(Rules, 3000.f, 5.f, Rules.Cooldown - 0.1f));
	TestFalse(TEXT("Not when a 3 m step would land on the player"),
		UnpaidRules::WantsPhaseStep(Rules, Rules.NearestToTarget + Rules.MinStep - 10.f, 5.f, 10.f));

	// How far: up to 5 m, never nearer the target than NearestToTarget, and no step under 3 m.
	TestNearlyEqual(TEXT("Far off: 5 m"), UnpaidRules::StepLength(Rules, 3000.f), 500.f, 0.01f);
	TestNearlyEqual(TEXT("6 m off: 3.5 m, landing 2.5 m away"), UnpaidRules::StepLength(Rules, 600.f), 350.f, 0.01f);
	TestNearlyEqual(TEXT("5 m off: no step"), UnpaidRules::StepLength(Rules, 500.f), 0.f, 0.01f);

	// Where: every spot is 3 to 5 m nearer the target, at its own height; straight at the target first.
	const FVector Here(0.0, 0.0, 0.0);
	const FVector Target(2000.0, 0.0, 90.0);
	const double Start = FVector::Dist2D(Here, Target);
	auto Closer = [&Target, Start](const FVector& Spot) { return Start - FVector::Dist2D(Spot, Target); };
	const TArray<FVector> Candidates = UnpaidRules::PhaseCandidates(Rules, Here, Target);
	TestTrue(TEXT("Spots to try"), Candidates.Num() > 10);
	if (Candidates.Num() > 0)
	{
		TestTrue(TEXT("Straight at the target first, 5 m on"), Candidates[0].Equals(FVector(500.0, 0.0, 0.0), 0.01));
	}
	for (const FVector& Spot : Candidates)
	{
		const double Gain = Closer(Spot);
		if (Gain < 299.5 || Gain > 500.5 || !FMath::IsNearlyEqual(Spot.Z, Here.Z))
		{
			AddError(FString::Printf(TEXT("%s is %.0f cm nearer, not 300-500, or off its height"), *Spot.ToString(), Gain));
		}
	}

	// Never into a wall: one across the straight way (where CanStand finds no room), so it comes back beside it.
	auto InWall = [](const FVector& Spot) { return Spot.X > 350.0 && Spot.X < 650.0 && FMath::Abs(Spot.Y) < 200.0; };
	auto OpenGround = [&InWall](const FVector& Candidate, FVector& OutFeet)
	{
		if (InWall(Candidate))
		{
			return false;
		}
		OutFeet = Candidate;
		return true;
	};
	const FHuntingGround Anywhere;
	FVector Chosen;
	if (TestTrue(TEXT("It finds a spot round the wall"), UnpaidRules::ChoosePhaseSpot(Rules, Here, Target, Anywhere, Here, OpenGround, Chosen)))
	{
		TestFalse(TEXT("Not in the wall"), InWall(Chosen));
		TestTrue(FString::Printf(TEXT("Still 3-5 m nearer (%.0f cm)"), Closer(Chosen)), Closer(Chosen) >= 299.5 && Closer(Chosen) <= 500.5);
	}

	// Never off its hunting ground: a yard of 4.6 m round its home takes the steps that stay inside it.
	FHuntingGround Yard;
	Yard.Radius = 460.f;
	auto Ground = [](const FVector& Candidate, FVector& OutFeet)
	{
		OutFeet = Candidate;
		return true;
	};
	if (TestTrue(TEXT("It finds a spot in its yard"), UnpaidRules::ChoosePhaseSpot(Rules, Here, Target, Yard, Here, Ground, Chosen)))
	{
		TestTrue(TEXT("On its hunting ground"), Yard.ContainsSpot(Chosen, Here));
		TestTrue(TEXT("Still at least 3 m nearer"), Closer(Chosen) >= 299.5);
	}
	FHuntingGround Pen;
	Pen.Radius = 200.f;
	TestFalse(TEXT("A yard too small for any step: it doesn't step"), UnpaidRules::ChoosePhaseSpot(Rules, Here, Target, Pen, Here, Ground, Chosen));

	// Never onto another level of ground: a ledge 2 m up straight ahead, so it comes back beside it, on its own level.
	auto Ledge = [](const FVector& Candidate, FVector& OutFeet)
	{
		OutFeet = Candidate;
		if (FMath::Abs(Candidate.Y) < 100.0)
		{
			OutFeet.Z += 200.0;
		}
		return true;
	};
	if (TestTrue(TEXT("It finds a spot off the ledge"), UnpaidRules::ChoosePhaseSpot(Rules, Here, Target, Anywhere, Here, Ledge, Chosen)))
	{
		TestTrue(TEXT("On its own level"), FMath::Abs(Chosen.Z - Here.Z) <= Rules.MaxRise);
	}
	auto Nowhere = [](const FVector&, FVector&) { return false; };
	TestFalse(TEXT("Nowhere to stand: no step"), UnpaidRules::ChoosePhaseSpot(Rules, Here, Target, Anywhere, Here, Nowhere, Chosen));
	TestTrue(TEXT("Too near the target: nowhere to step"), UnpaidRules::PhaseCandidates(Rules, Here, FVector(400.0, 0.0, 0.0)).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidShroudTest, "Looter.Creatures.Unpaid.Shroud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidShroudTest::RunTest(const FString& Parameters)
{
	// The shroud has no physics: a damped chain that trails the Unpaid's velocity, ripples more the faster it goes, settles
	// at rest, snaps straight on a lunge, and moves the same in a few long updates as in many short ones.
	const float RestAngles[] = { 5.f, 15.f, 30.f, 45.f, 60.f };
	const float Frame = 1.f / 60.f;
	FShroudChainSettings Still;
	Still.SwayDegrees = 0.f;
	auto Run = [Frame](FShroudChain& Chain, const FShroudChainSettings& Settings, const FVector& Velocity, float Snap, float Seconds)
	{
		for (float Elapsed = 0.f; Elapsed < Seconds - 0.5f * Frame; Elapsed += Frame)
		{
			Chain.Step(Settings, Velocity, 0.f, Snap, Frame);
		}
	};

	// Trails its velocity: carried forward it swings back, carried backward it swings forward, sideways it swings the other way.
	FShroudChain Chain;
	Chain.Init(MakeArrayView(RestAngles), 0.f);
	Run(Chain, Still, FVector(500.0, 0.0, 0.0), 0.f, 2.f);
	for (int32 Index = 0; Index < Chain.Links.Num(); ++Index)
	{
		TestTrue(FString::Printf(TEXT("Moving forward, link %d swings back (%.0f degrees)"), Index, Chain.Links[Index].Back), Chain.Links[Index].Back > 15.f);
	}
	TestTrue(TEXT("Its end trails higher behind than it hangs"), Chain.Links.Last().RestAngle + Chain.Links.Last().Back > Chain.Links.Last().RestAngle + 20.f);
	Run(Chain, Still, FVector(-300.0, 0.0, 0.0), 0.f, 2.f);
	TestTrue(TEXT("Moving backward, it swings forward"), Chain.Links[0].Back < -5.f);
	Chain.Settle();
	Run(Chain, Still, FVector(0.0, 400.0, 0.0), 0.f, 2.f);
	for (const FShroudChain::FLink& Link : Chain.Links)
	{
		TestTrue(TEXT("Moving right, it swings left"), Link.Side < -5.f);
	}

	// Settles at rest: still, every link back where it hangs.
	Run(Chain, Still, FVector::ZeroVector, 0.f, 4.f);
	for (const FShroudChain::FLink& Link : Chain.Links)
	{
		TestTrue(FString::Printf(TEXT("At rest it hangs still (%.2f, %.2f degrees)"), Link.Back, Link.Side),
			FMath::Abs(Link.Back) < 0.5f && FMath::Abs(Link.Side) < 0.5f && FMath::Abs(Link.BackSpeed) < 2.f && FMath::Abs(Link.SideSpeed) < 2.f);
	}
	// With its sway, at rest it never swings much more than the sway.
	const FShroudChainSettings Swaying;
	FShroudChain Swayer;
	Swayer.Init(MakeArrayView(RestAngles), 0.f);
	float Widest = 0.f;
	for (int32 Step = 0; Step < 600; ++Step)
	{
		Swayer.Step(Swaying, FVector::ZeroVector, 0.f, 0.f, Frame);
		Widest = FMath::Max(Widest, FMath::Abs(Swayer.Links.Last().Side));
	}
	TestTrue(FString::Printf(TEXT("It sways at rest, gently (%.1f degrees)"), Widest), Widest > 0.5f && Widest < Swaying.SwayDegrees + 2.f);

	// Snaps straight on a lunge: within a fifth of a second every link lies straight behind, and none swings aside.
	FShroudChain Lunging;
	Lunging.Init(MakeArrayView(RestAngles), 0.f);
	Run(Lunging, Still, FVector(1400.0, 0.0, 0.0), 1.f, 0.2f);
	for (const FShroudChain::FLink& Link : Lunging.Links)
	{
		const float Hang = Link.RestAngle + Link.Back;
		TestTrue(FString::Printf(TEXT("On a lunge, the link at %.0f degrees lies straight behind (%.1f)"), Link.RestAngle, Hang),
			FMath::Abs(Hang - Still.StraightAngle) < 3.f && FMath::Abs(Link.Side) < 1.f);
	}

	// The ripple runs down it, stronger the faster it goes.
	auto Flutter = [Frame, &RestAngles, &Still](float Speed)
	{
		FShroudChain Rippling;
		Rippling.Init(MakeArrayView(RestAngles), 0.f);
		float Low = 0.f;
		float High = 0.f;
		for (int32 Step = 0; Step < 240; ++Step)
		{
			Rippling.Step(Still, FVector(Speed, 0.0, 0.0), 0.f, 0.f, Frame);
			if (Step >= 120)
			{
				Low = FMath::Min(Low, Rippling.Links.Last().Side);
				High = FMath::Max(High, Rippling.Links.Last().Side);
			}
		}
		return High - Low;
	};
	const float Slow = Flutter(100.f);
	const float Fast = Flutter(500.f);
	TestTrue(FString::Printf(TEXT("It ripples more at speed (%.1f against %.1f degrees)"), Fast, Slow), Fast > Slow * 1.6f && Fast > 5.f);

	// The strips at its sides run out of phase: they don't swing in step.
	FShroudChain LeftStrip;
	FShroudChain RightStrip;
	const float Strip[] = { 10.f, 25.f };
	LeftStrip.Init(MakeArrayView(Strip), 0.f);
	RightStrip.Init(MakeArrayView(Strip), UE_PI);
	double Together = 0.0;
	for (int32 Step = 0; Step < 900; ++Step)
	{
		LeftStrip.Step(Swaying, FVector::ZeroVector, 0.f, 0.f, Frame);
		RightStrip.Step(Swaying, FVector::ZeroVector, 0.f, 0.f, Frame);
		if (Step >= 300)
		{
			Together += LeftStrip.Links.Last().Side * RightStrip.Links.Last().Side;
		}
	}
	TestTrue(TEXT("The side strips swing against each other"), Together < 0.0);

	// A distant one updates five times a second: its shroud moves as one updating every frame does.
	FShroudChain Near;
	FShroudChain Far;
	Near.Init(MakeArrayView(RestAngles), 0.f);
	Far.Init(MakeArrayView(RestAngles), 0.f);
	const float Long = FCreatureUpdateRate().VeryFarInterval;
	for (int32 Update = 0; Update < 10; ++Update)
	{
		const FVector Velocity(Update < 5 ? 450.0 : 0.0, 0.0, 0.0);
		Far.Step(Swaying, Velocity, 0.f, 0.f, Long);
		for (int32 Step = 0; Step < 12; ++Step)
		{
			Near.Step(Swaying, Velocity, 0.f, 0.f, Long / 12.f);
		}
		if (!TestTrue(TEXT("The chain stays finite"), FMath::IsFinite(Far.Links.Last().Back) && FMath::IsFinite(Far.Links.Last().Side)))
		{
			return false;
		}
		TestNearlyEqual(FString::Printf(TEXT("Update %d swings like twelve frames"), Update), Far.Links.Last().Back, Near.Links.Last().Back, 0.05f);
	}
	FShroudChain Hitch;
	Hitch.Init(MakeArrayView(RestAngles), 0.f);
	Hitch.Step(Swaying, FVector(450.0, 0.0, 0.0), 0.f, 0.f, 5.f);
	TestTrue(TEXT("A hitch leaves it sane"), FMath::IsFinite(Hitch.Links.Last().Back) && FMath::Abs(Hitch.Links.Last().Back) < 110.f);
	return true;
}

#endif
