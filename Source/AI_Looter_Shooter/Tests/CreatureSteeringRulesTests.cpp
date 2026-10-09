#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureSteerPlanner.h"
#include "Creatures/CreatureUnstick.h"

// The creature steering's rules on their own: when its unstick acts (FCreatureUnstick) and how the planner picks and keeps
// the side it goes round a wall by. CreatureSteeringTests.cpp runs the planner through a flat world of fences and pens.

namespace
{
	using FLookResult = FCreatureSteerPlanner::FLookResult;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureUnstickRulesTest, "Looter.Creatures.Steering.UnstickRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureUnstickRulesTest::RunTest(const FString& Parameters)
{
	// When a creature is stuck: blocked a while, it goes round; pressing on against something solid without getting
	// anywhere, or hung in the air, it glides free; one moving, or one standing still for something else, is let be.
	constexpr float Frame = 1.f / 60.f;
	auto RunFrames = [Frame](FCreatureUnstick& Unstick, float Seconds, FCreatureUnstick::FFrame Step, bool bTouching, TArray<TPair<float, FCreatureUnstick::EAction>>& OutActions,
		const FVector& Velocity = FVector::ZeroVector)
	{
		for (float Clock = 0.f; Clock < Seconds - 0.5f * Frame; Clock += Frame)
		{
			Step.Here += Velocity * Frame;
			Step.DeltaSeconds = Frame;
			if (bTouching)
			{
				Unstick.NoteBlockedMove();
			}
			const FCreatureUnstick::EAction Action = Unstick.Update(Step);
			if (Action != FCreatureUnstick::EAction::None)
			{
				OutActions.Emplace(Clock + Frame, Action);
			}
		}
	};
	auto FirstOf = [](const TArray<TPair<float, FCreatureUnstick::EAction>>& Actions, FCreatureUnstick::EAction Wanted)
	{
		for (const TPair<float, FCreatureUnstick::EAction>& Each : Actions)
		{
			if (Each.Value == Wanted)
			{
				return Each.Key;
			}
		}
		return -1.f;
	};

	// Blocked: it goes round after BlockedSeconds, and again each BlockedSeconds while it's still blocked.
	{
		FCreatureUnstick Unstick;
		FCreatureUnstick::FFrame Step;
		Step.bBlocked = true;
		TArray<TPair<float, FCreatureUnstick::EAction>> Actions;
		RunFrames(Unstick, 1.2f, Step, false, Actions);
		const float First = FirstOf(Actions, FCreatureUnstick::EAction::GoRound);
		TestTrue(FString::Printf(TEXT("Blocked: it goes round after %.2f s"), First), FMath::Abs(First - FCreatureUnstick::BlockedSeconds) <= Frame * 1.5f);
		TestEqual(TEXT("...twice in 1.2 s"), Actions.Num(), 2);
		TestEqual(TEXT("...and isn't slid anywhere without something solid against it"), FirstOf(Actions, FCreatureUnstick::EAction::NudgeWedged), -1.f);
	}
	// Wedged: pressing on against something solid, getting nowhere.
	{
		FCreatureUnstick Unstick;
		FCreatureUnstick::FFrame Step;
		Step.bBlocked = true;
		Step.bPressing = true;
		TArray<TPair<float, FCreatureUnstick::EAction>> Actions;
		RunFrames(Unstick, 2.f, Step, true, Actions);
		const float Wedged = FirstOf(Actions, FCreatureUnstick::EAction::NudgeWedged);
		TestTrue(FString::Printf(TEXT("Wedged: it glides free after %.2f s"), Wedged), FMath::Abs(Wedged - FCreatureUnstick::WedgedSeconds) <= Frame * 1.5f);
		TestTrue(TEXT("...having tried going round first"), FirstOf(Actions, FCreatureUnstick::EAction::GoRound) > 0.f
			&& FirstOf(Actions, FCreatureUnstick::EAction::GoRound) < Wedged);
		// No free spot: it waits RetrySeconds before it tries again.
		FCreatureUnstick Waiting;
		TArray<TPair<float, FCreatureUnstick::EAction>> First;
		RunFrames(Waiting, FCreatureUnstick::WedgedSeconds + Frame, Step, true, First);
		Waiting.NudgeFailed();
		TArray<TPair<float, FCreatureUnstick::EAction>> Later;
		RunFrames(Waiting, FCreatureUnstick::RetrySeconds - 0.1f, Step, true, Later);
		TestEqual(TEXT("...and, with nowhere to go, waits before it looks again"), FirstOf(Later, FCreatureUnstick::EAction::NudgeWedged), -1.f);
		// Standing still for something else (its steering taken from it), or getting somewhere: not wedged.
		FCreatureUnstick Still;
		FCreatureUnstick::FFrame Held = Step;
		Held.bPressing = false;
		TArray<TPair<float, FCreatureUnstick::EAction>> HeldActions;
		RunFrames(Still, 2.f, Held, true, HeldActions);
		TestEqual(TEXT("A body its movement isn't pushing isn't wedged"), FirstOf(HeldActions, FCreatureUnstick::EAction::NudgeWedged), -1.f);
		FCreatureUnstick Sliding;
		TArray<TPair<float, FCreatureUnstick::EAction>> SlidingActions;
		RunFrames(Sliding, 2.f, Step, true, SlidingActions, FVector(60.0, 0.0, 0.0));
		TestEqual(TEXT("One sliding slowly along a wall isn't wedged"), FirstOf(SlidingActions, FCreatureUnstick::EAction::NudgeWedged), -1.f);
		FCreatureUnstick Untouched;
		TArray<TPair<float, FCreatureUnstick::EAction>> UntouchedActions;
		RunFrames(Untouched, 2.f, Step, false, UntouchedActions);
		TestEqual(TEXT("One nothing solid stops isn't wedged"), FirstOf(UntouchedActions, FCreatureUnstick::EAction::NudgeWedged), -1.f);
	}
	// Hung in the air without moving, it glides free; falling, it's let fall.
	{
		FCreatureUnstick Unstick;
		FCreatureUnstick::FFrame Step;
		Step.bFalling = true;
		TArray<TPair<float, FCreatureUnstick::EAction>> Actions;
		RunFrames(Unstick, 1.f, Step, false, Actions);
		const float Hung = FirstOf(Actions, FCreatureUnstick::EAction::NudgeHung);
		TestTrue(FString::Printf(TEXT("Hung: it glides free after %.2f s"), Hung), FMath::Abs(Hung - FCreatureUnstick::HungSeconds) <= Frame * 1.5f);
		FCreatureUnstick Falling;
		TArray<TPair<float, FCreatureUnstick::EAction>> FallingActions;
		RunFrames(Falling, 1.f, Step, false, FallingActions, FVector(0.0, 0.0, -400.0));
		TestEqual(TEXT("Falling: let fall"), FallingActions.Num(), 0);
	}
	// The glide: a quarter second, eased in and out, ending where it was sent; nothing else happens meanwhile.
	{
		FCreatureUnstick Unstick;
		const FVector From(0.0, 0.0, 100.0);
		const FVector To(60.0, 0.0, 64.0);
		Unstick.StartGlide(From, To);
		TestTrue(TEXT("Gliding"), Unstick.IsGliding());
		FCreatureUnstick::FFrame Step;
		Step.bFalling = true;
		Step.bBlocked = true;
		Step.DeltaSeconds = 1.f;
		TestTrue(TEXT("Nothing else while it glides"), Unstick.Update(Step) == FCreatureUnstick::EAction::None);
		FVector Was = From;
		double Biggest = 0.0;
		bool bDone = false;
		float Clock = 0.f;
		FVector Where = From;
		while (!bDone && Clock < 1.f)
		{
			Where = Unstick.StepGlide(Frame, bDone);
			Biggest = FMath::Max(Biggest, FVector::Dist(Where, Was));
			Was = Where;
			Clock += Frame;
		}
		TestTrue(FString::Printf(TEXT("Done after %.2f s"), Clock), bDone && FMath::Abs(Clock - FCreatureUnstick::GlideSeconds) <= Frame * 1.5f);
		TestTrue(TEXT("...where it was sent"), Where.Equals(To, 0.01));
		TestTrue(FString::Printf(TEXT("...smoothly (at most %.1f cm a frame of %.1f)"), Biggest, FVector::Dist(From, To)),
			Biggest <= 0.15 * FVector::Dist(From, To));
		TestFalse(TEXT("...and no longer gliding"), Unstick.IsGliding());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureSteeringSideMemoryTest, "Looter.Creatures.Steering.SideMemory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureSteeringSideMemoryTest::RunTest(const FString& Parameters)
{
	// Meeting a wall it picks the side whose way along it heads more toward its goal, and keeps to the side it took for a
	// few seconds when the next wall is met nearly head-on.
	auto Wall = [](const FVector& Normal)
	{
		return [Normal](const FVector& Direction, float Distance)
		{
			// A wall across the way 1 m out facing Normal: blocked going into it, clear along or away from it.
			return FVector::DotProduct(Direction, Normal) < -0.2 ? FLookResult::Blocked(100.f, Normal) : FLookResult::Clear(Distance);
		};
	};
	FCreatureSteerPlanner::FRequest Request;
	Request.Here = FVector::ZeroVector;
	Request.Goal = FVector(2000.0, 0.0, 0.0);
	Request.Desired = FVector::ForwardVector;
	Request.GoalDistance = 2000.f;
	Request.bNear = false;

	// A wall slanting away to its right (+Y): it goes round to the right, along the wall the way that heads more toward its
	// goal, whichever side it was seeded with (positive angles turn toward +Y: side +1).
	for (const float Seeded : { 1.f, -1.f })
	{
		FCreatureSteerPlanner Slanted;
		Slanted.SeedSide(Seeded);
		const FVector Way = Slanted.Plan(Request, Wall(FVector(-1.0, 0.5, 0.0).GetSafeNormal()));
		TestTrue(TEXT("Following the wall it met"), Slanted.IsFollowingWall());
		TestTrue(FString::Printf(TEXT("Seeded %+.0f: along it to the right, toward its goal (%.2f, %.2f)"), Seeded, Way.X, Way.Y),
			Slanted.GetSide() == 1.f && Way.Y > 0.0);
	}

	// The next wall a moment later leans a little toward the other side: it keeps the side it took (no flip-flop), where a
	// creature that took no side lately goes the other way.
	for (const float Taken : { 1.f, -1.f })
	{
		const FVector Leaning = FVector(-0.9887, -0.15 * Taken, 0.0).GetSafeNormal();
		FCreatureSteerPlanner Planner;
		Planner.SeedSide(Taken);
		Planner.Plan(Request, Wall(FVector(-1.0, 0.0, 0.0)));
		TestEqual(FString::Printf(TEXT("Seeded %+.0f: a wall head-on, its seeded side"), Taken), Planner.GetSide(), Taken);
		Planner.Forget(false);
		FCreatureSteerPlanner::FRequest Next = Request;
		Next.Here = FVector(10.0, 0.0, 0.0);
		Planner.Advance(1.f);
		Planner.Plan(Next, Wall(Leaning));
		TestEqual(TEXT("...the next one leaning the other way a second later: the same side"), Planner.GetSide(), Taken);
		TestEqual(TEXT("...without a change of side"), Planner.GetSideFlips(), 0);

		FCreatureSteerPlanner Fresh;
		Fresh.SeedSide(Taken);
		Fresh.Plan(Next, Wall(Leaning));
		TestEqual(TEXT("A creature that took no side lately goes the way the wall leans"), Fresh.GetSide(), -Taken);
	}
	return true;
}

#endif
