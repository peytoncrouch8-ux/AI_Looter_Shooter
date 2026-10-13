#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Scenes/OreLiftRide.h"
#include "Story/CaptionSubsystem.h"
#include "Tests/OreLiftTestWorld.h"
#include "World/OreLift.h"
#include "World/OreLiftBellPull.h"
#include "World/OreLiftMotion.h"

using namespace OreLiftTestWorld;

namespace
{
	/** The design's line as a run sees it going up: 57 m, bending over the tower 27.4 m up it (steep, then less so). */
	FOreLiftRunShape DesignRun()
	{
		FOreLiftRunShape Run;
		Run.Length = 5700.f;
		Run.Bends = { 2740.f };
		Run.Spans = { FVector2D(0.45, 0.05), FVector2D(0.69, 0.04) };
		return Run;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOreLiftMotionTest, "Looter.OreLift.Motion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOreLiftMotionTest::RunTest(const FString& Parameters)
{
	const FOreLiftRunShape Run = DesignRun();
	const FOreLiftSwing Swing;
	const float Travel = Run.TravelSeconds;
	const float Ease = Run.EaseSeconds;
	const float Cruise = Run.Length / (Travel - Ease);

	// Eased at both ends: still as it starts and stops, never faster than its cruise, all the way in its time.
	TestEqual(TEXT("Nowhere yet at the start"), OreLiftMotion::TravelDistance(0.f, Travel, Ease, Run.Length), 0.f);
	TestNearlyEqual(TEXT("The whole way by the end"), OreLiftMotion::TravelDistance(Travel, Travel, Ease, Run.Length), Run.Length, 0.5f);
	TestTrue(TEXT("Still as it starts and as it stops"), OreLiftMotion::TravelSpeed(0.01f, Travel, Ease, Run.Length) < 1.f
		&& OreLiftMotion::TravelSpeed(Travel - 0.01f, Travel, Ease, Run.Length) < 1.f);
	TestNearlyEqual(TEXT("At cruise in the middle"), OreLiftMotion::TravelSpeed(0.5f * Travel, Travel, Ease, Run.Length), Cruise, 0.5f);
	bool bSmooth = true;
	for (float Seconds = 0.f; Seconds < Travel; Seconds += 0.05f)
	{
		const float Step = OreLiftMotion::TravelDistance(Seconds + 0.05f, Travel, Ease, Run.Length) - OreLiftMotion::TravelDistance(Seconds, Travel, Ease, Run.Length);
		bSmooth = bSmooth && Step >= -0.001f && Step <= Cruise * 0.05f + 0.01f;
	}
	TestTrue(TEXT("Always on, never faster than its cruise"), bSmooth);
	for (const float Distance : { 500.f, 2740.f, 5000.f })
	{
		const float When = OreLiftMotion::TravelTimeAt(Distance, Travel, Ease, Run.Length);
		TestNearlyEqual(*FString::Printf(TEXT("Found again: %.0f cm along"), Distance), OreLiftMotion::TravelDistance(When, Travel, Ease, Run.Length), Distance, 1.f);
	}
	TestEqual(TEXT("On its landing through the take-up"), OreLiftMotion::DistanceAt(Run, Run.TakeUpSeconds - 0.01f), 0.f);
	TestNearlyEqual(TEXT("At the far landing from its arrival on"), OreLiftMotion::DistanceAt(Run, Run.GetArrival() + 0.5f), Run.Length, 0.5f);
	TestTrue(TEXT("A ride is about 24 s with its boarding"), FMath::IsWithinInclusive(OreLiftRide::DurationFor(Run), 22.f, 26.f));

	// Three clanks over the tower's sheaves, in order, a beat apart, the middle one where the cable bends.
	const TArray<float> Clanks = OreLiftMotion::ClankTimes(Run);
	if (TestEqual(TEXT("Three clanks"), Clanks.Num(), 3))
	{
		TestTrue(TEXT("...in order, a beat apart"), Clanks[0] < Clanks[1] && Clanks[1] < Clanks[2] && Clanks[2] - Clanks[0] < 1.f);
		TestNearlyEqual(TEXT("...the middle one at the bend"), Clanks[1],
			Run.TakeUpSeconds + OreLiftMotion::TravelTimeAt(2740.f, Travel, Ease, Run.Length), 0.01f);
	}

	// The sway: none at rest, the lift-off, the cage trailing as it gathers speed, kicked at the tower, bounded, set down
	// with a dip, and still at the end.
	const FOreLiftSway AtRest = OreLiftMotion::SwayAt(Run, Swing, 0.f);
	TestTrue(TEXT("At rest as it starts"), FMath::IsNearlyZero(AtRest.Along) && FMath::IsNearlyZero(AtRest.Side) && FMath::IsNearlyZero(AtRest.Bob, 0.01f));
	TestTrue(TEXT("Lifted off its landing by the take-up's end"),
		FMath::IsWithinInclusive(OreLiftMotion::SwayAt(Run, Swing, Run.TakeUpSeconds).Bob, 0.75f * Swing.TakeUpRise, 1.4f * Swing.TakeUpRise));
	TestTrue(TEXT("Trailing as it gathers speed"), OreLiftMotion::SwayAt(Run, Swing, Run.TakeUpSeconds + 0.5f * Ease).Along > 0.3f);
	TestTrue(TEXT("Dipping as it's set down"), OreLiftMotion::SwayAt(Run, Swing, Run.GetArrival() + 0.15f).Bob < -0.5f);
	const FOreLiftSway Stilled = OreLiftMotion::SwayAt(Run, Swing, Run.GetDuration());
	TestTrue(TEXT("Still at the end"), FMath::Abs(Stilled.Along) < 0.01f && FMath::Abs(Stilled.Side) < 0.01f && FMath::Abs(Stilled.Bob) < 0.01f
		&& FMath::Abs(Stilled.Jolt) < 0.01f);

	// Worked out frame by frame, or from nothing at any moment, it's the same swing.
	FOreLiftSwingState Carried;
	float WorstGap = 0.f;
	float WorstSwing = 0.f;
	for (float Seconds = 0.f; Seconds <= Run.GetDuration(); Seconds += Frame)
	{
		const FOreLiftSway Framed = OreLiftMotion::SwayAt(Run, Swing, Seconds, Carried);
		WorstSwing = FMath::Max(WorstSwing, FMath::Max(FMath::Abs(Framed.Along), FMath::Abs(Framed.Side)));
		if (FMath::Fmod(Seconds, 1.f) < Frame)
		{
			const FOreLiftSway Fresh = OreLiftMotion::SwayAt(Run, Swing, Seconds);
			WorstGap = FMath::Max(WorstGap, FMath::Max(FMath::Abs(Fresh.Along - Framed.Along), FMath::Abs(Fresh.Bob - Framed.Bob)));
		}
	}
	TestTrue(TEXT("The same swing in any frames"), WorstGap < 0.001f);
	TestTrue(TEXT("It swings, but never past its limit"), WorstSwing > 1.f && WorstSwing < Swing.MaxDegrees);

	// The bend over the tower is felt: a cable that didn't bend swings quite differently after the clanks.
	FOreLiftRunShape Straight = Run;
	Straight.Spans = { FVector2D(0.45, 0.05), FVector2D(0.45, 0.05) };
	float Kick = 0.f;
	for (int32 Tenth = 0; Tenth < 20; ++Tenth)
	{
		const float Seconds = Clanks.IsEmpty() ? 0.f : Clanks[1] + 0.1f * Tenth;
		Kick = FMath::Max(Kick, FMath::Abs(OreLiftMotion::SwayAt(Run, Swing, Seconds).Along - OreLiftMotion::SwayAt(Straight, Swing, Seconds).Along));
	}
	TestTrue(TEXT("The tower kicks its swing"), Kick > 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOreLiftRulesTest, "Looter.OreLift.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOreLiftRulesTest::RunTest(const FString& Parameters)
{
	const FName Flag(TEXT("Test.LiftOpen"));
	FCampaignRecord Campaign;
	TestTrue(TEXT("No mission and no flag: it always runs"), AOreLift::IsOpenFor(Campaign, nullptr, NAME_None, 0, NAME_None));
	TestFalse(TEXT("Before Dead Lift: it doesn't run"), AOreLift::IsOpenFor(Campaign, nullptr, DeadLiftId, RideStep, NAME_None));
	Campaign.ActiveMission = DeadLiftId;
	Campaign.ActiveMissionStep = 1;
	TestFalse(TEXT("Dead Lift under way before its ride: no steam yet"), AOreLift::IsOpenFor(Campaign, nullptr, DeadLiftId, RideStep, NAME_None));
	Campaign.ActiveMissionStep = RideStep;
	TestTrue(TEXT("At its ride: it runs"), AOreLift::IsOpenFor(Campaign, nullptr, DeadLiftId, RideStep, NAME_None));
	Campaign = FCampaignRecord();
	Campaign.MarkReady(DeadLiftId);
	TestTrue(TEXT("Ridden, waiting for its turn-in: it runs"), AOreLift::IsOpenFor(Campaign, nullptr, DeadLiftId, RideStep, NAME_None));
	Campaign = FCampaignRecord();
	Campaign.Complete(DeadLiftId);
	TestTrue(TEXT("After Dead Lift: it runs both ways for good"), AOreLift::IsOpenFor(Campaign, nullptr, DeadLiftId, RideStep, NAME_None));
	Campaign = FCampaignRecord();
	TestFalse(TEXT("A flag not set opens nothing"), AOreLift::IsOpenFor(Campaign, nullptr, NAME_None, 0, Flag));
	Campaign.SetFlag(Flag);
	TestTrue(TEXT("Its flag set: it runs"), AOreLift::IsOpenFor(Campaign, nullptr, DeadLiftId, RideStep, Flag));

	// In a level: the bell pulls say what a pull does, and the line runs from landing to landing.
	FLiftLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Build(RideStep)))
	{
		return false;
	}
	AOreLift* Lift = Level.Lift;
	TestTrue(TEXT("At the ride's step the bottom pull rides and the top one calls"),
		Lift->GetBellState(EOreLiftEnd::Bottom) == EOreLiftBell::Ride && Lift->GetBellState(EOreLiftEnd::Top) == EOreLiftBell::Call);
	TestTrue(TEXT("The cable runs from landing to landing over the tower, about 57 m"), FMath::IsWithinInclusive(Lift->GetCableLength(), 5000.f, 6500.f));
	TestTrue(TEXT("Its grip starts at the bottom landing and ends at the top's"),
		Lift->GetGripAt(EOreLiftEnd::Bottom, 0.f).Equals(Lift->GetGripAt(EOreLiftEnd::Top, Lift->GetCableLength()), 0.5)
		&& Lift->GetGripAt(EOreLiftEnd::Bottom, 0.f).Z < Lift->GetGripAt(EOreLiftEnd::Top, 0.f).Z);
	TestTrue(TEXT("The cage rests at the bottom as a level begins"), Level.IsCageAt(EOreLiftEnd::Bottom));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOreLiftBothWaysTest, "Looter.OreLift.BothWays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOreLiftBothWaysTest::RunTest(const FString& Parameters)
{
	FScenesOn Scenes;
	FLiftLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Build(RideStep)))
	{
		return false;
	}
	AOreLift* Lift = Level.Lift;
	TestFalse(TEXT("No ride from the top: the cage isn't there"), Lift->Ride(EOreLiftEnd::Top, Level.Rider));

	// Up: the first ride.
	TestTrue(TEXT("The ride up starts"), Lift->Ride(EOreLiftEnd::Bottom, Level.Rider));
	TestTrue(TEXT("...as the scene playing"), Level.Scenes->IsPlaying() && Level.Scenes->GetPlayingName() == OreLiftRide::SceneName());
	TestTrue(TEXT("...the missions hear Board.Lift"), Level.HasHeard(TEXT("Board.Lift")));
	TestTrue(TEXT("...which rides Dead Lift's last step"), Level.Runner->IsReadyToTurnIn(DeadLiftId) || Level.Runner->IsCompleted(DeadLiftId));
	TestTrue(TEXT("...the autosave waits"), Lift->IsHoldingSaves());
	TestTrue(TEXT("...both pulls are busy"),
		Lift->GetBellState(EOreLiftEnd::Bottom) == EOreLiftBell::Busy && Lift->GetBellState(EOreLiftEnd::Top) == EOreLiftBell::Busy);
	const UCaptionSubsystem* Captions = UCaptionSubsystem::Get(Level.World);
	const FCaptionEntry* Said = Captions ? Captions->GetCurrent() : nullptr;
	TestTrue(TEXT("...Hiram says to hold the rail"), Said && !Lift->FirstRideLines.IsEmpty() && Said->Line.Text.EqualTo(Lift->FirstRideLines[0].Text));

	Level.Tick(12.f);
	const float CageZ = Lift->GetCageTransform().GetLocation().Z;
	TestTrue(TEXT("12 s in, the cage hangs between the landings"),
		CageZ > Lift->GetCageRest(EOreLiftEnd::Bottom).GetLocation().Z + 500.0 && CageZ < Lift->GetCageRest(EOreLiftEnd::Top).GetLocation().Z - 500.0);
	TestTrue(TEXT("...while the rider waits on the top landing, where a save finds them"), Level.IsRiderOn(EOreLiftEnd::Top));
	const float Took = 12.f + Level.Settle(30.f);
	TestTrue(TEXT("The ride took about 24 s"), FMath::IsWithinInclusive(Took, 22.f, 26.f));
	TestFalse(TEXT("It's over"), Level.Scenes->IsPlaying());
	TestTrue(TEXT("The cage rests at the top"), Level.IsCageAt(EOreLiftEnd::Top));
	TestTrue(TEXT("...the rider on its landing, solid again"), Level.IsRiderOn(EOreLiftEnd::Top) && Level.Rider->GetActorEnableCollision());
	TestTrue(TEXT("...the missions hear Lift.Arrived.Top"), Level.HasHeard(TEXT("Lift.Arrived.Top")));
	TestFalse(TEXT("...the autosave goes again"), Lift->IsHoldingSaves());
	TestTrue(TEXT("...the first ride is remembered"), Level.Campaign.HasFlag(Lift->RiddenFlag) && !Lift->IsFirstRide());
	TestTrue(TEXT("...and it still runs, its mission ridden"), Lift->IsOpen());

	// Down.
	Level.Heard.Reset();
	TestTrue(TEXT("The ride down starts from the top"), Lift->Ride(EOreLiftEnd::Top, Level.Rider));
	TestTrue(TEXT("...the rider waiting at the bottom"), Level.IsRiderOn(EOreLiftEnd::Bottom));
	Level.Settle(30.f);
	TestTrue(TEXT("Down: the cage rests at the bottom"), Level.IsCageAt(EOreLiftEnd::Bottom) && !Level.Scenes->IsPlaying());
	TestTrue(TEXT("...Board.Lift, then Lift.Arrived.Bottom"), Level.HasHeard(TEXT("Board.Lift")) && Level.HasHeard(TEXT("Lift.Arrived.Bottom"))
		&& !Level.HasHeard(TEXT("Lift.Arrived.Top")));
	TestTrue(TEXT("...the rider on the bottom landing"), Level.IsRiderOn(EOreLiftEnd::Bottom));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOreLiftCallTest, "Looter.OreLift.Call",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOreLiftCallTest::RunTest(const FString& Parameters)
{
	FScenesOn Scenes;
	FLiftLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Build(RideStep)))
	{
		return false;
	}
	AOreLift* Lift = Level.Lift;
	// The bell pulls at both landings, as the build script places them.
	AOreLiftBellPull* BottomPull = Level.World->SpawnActor<AOreLiftBellPull>(Lift->GetRiderSpot(EOreLiftEnd::Bottom) + FVector(0.0, 180.0, 150.0), FRotator::ZeroRotator);
	AOreLiftBellPull* TopPull = Level.World->SpawnActor<AOreLiftBellPull>(Lift->GetRiderSpot(EOreLiftEnd::Top) + FVector(0.0, 180.0, 150.0), FRotator::ZeroRotator);
	if (!TestTrue(TEXT("Bell pulls placed"), BottomPull && TopPull))
	{
		return false;
	}
	BottomPull->Lift = Lift;
	BottomPull->End = EOreLiftEnd::Bottom;
	TopPull->Lift = Lift;
	TopPull->End = EOreLiftEnd::Top;
	BottomPull->DispatchBeginPlay();
	TopPull->DispatchBeginPlay();

	TestFalse(TEXT("Not called to where it waits"), Lift->CallTo(EOreLiftEnd::Bottom));
	TestTrue(TEXT("The top pull calls it"), TopPull->GetPromptNow().EqualTo(TopPull->CallPrompt));
	TestFalse(TEXT("...not with a tap"), TopPull->Pull(Level.Rider, /*bHeld*/ false));
	TestTrue(TEXT("...with a hold"), TopPull->Pull(Level.Rider, /*bHeld*/ true) && Lift->IsCalled());
	TestTrue(TEXT("On its way: nothing to pull at either landing"), TopPull->GetPromptNow().IsEmpty() && BottomPull->GetPromptNow().IsEmpty());
	TestFalse(TEXT("...no ride"), Lift->Ride(EOreLiftEnd::Bottom, Level.Rider));
	TestFalse(TEXT("...no second call"), Lift->CallTo(EOreLiftEnd::Bottom));
	TestFalse(TEXT("...no scene: the empty cage comes on its own"), Level.Scenes->IsPlaying());

	Level.Tick(4.f);
	const float CageZ = Lift->GetCageTransform().GetLocation().Z;
	TestTrue(TEXT("4 s in, the empty cage is on its way up"),
		CageZ > Lift->GetCageRest(EOreLiftEnd::Bottom).GetLocation().Z + 500.0 && CageZ < Lift->GetCageRest(EOreLiftEnd::Top).GetLocation().Z - 100.0);
	const float Took = 4.f + Level.Settle(12.f);
	TestNearlyEqual(TEXT("It came in 8 s"), Took, 8.f, 0.1f);
	TestTrue(TEXT("...and waits at the top"), Level.IsCageAt(EOreLiftEnd::Top));
	TestFalse(TEXT("...nobody rode it"), Level.HasHeard(TEXT("Board.Lift")) || Level.HasHeard(TEXT("Lift.Arrived.Top")));
	TestTrue(TEXT("Now the top pull rides it down"), TopPull->GetPromptNow().EqualTo(TopPull->RideDownPrompt));
	TestTrue(TEXT("...and the bottom pull calls it back"), BottomPull->GetPromptNow().EqualTo(BottomPull->CallPrompt));
	TestTrue(TEXT("Called back down"), BottomPull->Pull(Level.Rider, /*bHeld*/ true));
	Level.Settle(12.f);
	TestTrue(TEXT("...waiting at the bottom again"), Level.IsCageAt(EOreLiftEnd::Bottom));
	return true;
}

#endif
