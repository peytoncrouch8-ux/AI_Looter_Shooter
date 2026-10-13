#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Session/SessionSaveGate.h"
#include "Story/CaptionSubsystem.h"
#include "Tests/BossTestWorld.h"
#include "Tests/OreLiftTestWorld.h"
#include "World/OreLift.h"
#include "World/OreLiftBellPull.h"

using namespace OreLiftTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOreLiftClosedTest, "Looter.OreLift.Closed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOreLiftClosedTest::RunTest(const FString& Parameters)
{
	FScenesOn Scenes;
	FLiftLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Build(INDEX_NONE)))
	{
		return false;
	}
	AOreLift* Lift = Level.Lift;
	AOreLiftBellPull* Pull = Level.World->SpawnActor<AOreLiftBellPull>(Lift->GetRiderSpot(EOreLiftEnd::Bottom) + FVector(0.0, 180.0, 150.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A bell pull"), Pull))
	{
		return false;
	}
	Pull->Lift = Lift;
	Pull->End = EOreLiftEnd::Bottom;
	Pull->DispatchBeginPlay();

	// Before Dead Lift: a dead lift.
	TestFalse(TEXT("It doesn't run before Dead Lift"), Lift->IsOpen());
	TestTrue(TEXT("...both pulls say so"),
		Lift->GetBellState(EOreLiftEnd::Bottom) == EOreLiftBell::Closed && Lift->GetBellState(EOreLiftEnd::Top) == EOreLiftBell::Closed);
	TestFalse(TEXT("...no ride"), Lift->Ride(EOreLiftEnd::Bottom, Level.Rider));
	TestFalse(TEXT("...no call"), Lift->CallTo(EOreLiftEnd::Top));
	TestTrue(TEXT("...the reason: the boiler's been cold for years"), Lift->GetClosedReason().EqualTo(Lift->ClosedColdLine));
	TestTrue(TEXT("The pull offers a tap"), Pull->GetPromptNow().EqualTo(Pull->ClosedPrompt));
	TestFalse(TEXT("...nothing to hold for"), Pull->Pull(Level.Rider, /*bHeld*/ true));
	TestTrue(TEXT("...a tap clunks"), Pull->Pull(Level.Rider, /*bHeld*/ false));
	const UCaptionSubsystem* Captions = UCaptionSubsystem::Get(Level.World);
	const FCaptionEntry* Said = Captions ? Captions->GetCurrent() : nullptr;
	TestTrue(TEXT("...and the caption says why"), Said && Said->Line.Text.EqualTo(Lift->ClosedColdLine));
	TestFalse(TEXT("Nothing moved, nothing played"), Lift->IsMoving() || Level.Scenes->IsPlaying());

	// Dead Lift under way, before the ride: no steam.
	Level.Runner->StartMission(DeadLiftId, 1, /*bForce*/ true);
	TestFalse(TEXT("Dead Lift's fittings: it still doesn't run"), Lift->IsOpen());
	TestTrue(TEXT("...the reason now: no steam yet"), Lift->GetClosedReason().EqualTo(Lift->ClosedSteamLine));

	// Its ride step: it runs.
	Level.Runner->SetStep(DeadLiftId, RideStep);
	TestTrue(TEXT("At the ride's step it runs"), Lift->IsOpen() && Lift->GetClosedReason().IsEmpty());
	TestTrue(TEXT("...the pull rides it up"), Pull->GetPromptNow().EqualTo(Pull->RideUpPrompt));

	// Forced open from the console, whatever the story.
	FLiftLevel Other;
	if (TestTrue(TEXT("A second test level"), Other.Build(INDEX_NONE)))
	{
		Other.Lift->SetForcedOpen(true);
		TestTrue(TEXT("Forced open, it runs before its mission"), Other.Lift->IsOpen());
		Other.Lift->SetForcedOpen(false);
		TestFalse(TEXT("...and back to the story"), Other.Lift->IsOpen());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOreLiftSavesHeldTest, "Looter.OreLift.SavesHeld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOreLiftSavesHeldTest::RunTest(const FString& Parameters)
{
	// The session's gate as a ride holds it: the autosave and save-soons wait, a save asked for and the level's end still go
	// (they find the rider on the far landing), and the last hold released lets the save that waited go.
	FSessionSaveGate Gate;
	Gate.Hold(TEXT("OreLift"));
	Gate.Hold(TEXT("Scene"));
	TestFalse(TEXT("Riding: no autosave, no save-soon"), Gate.Allows(ESessionSaveReason::Autosave) || Gate.Allows(ESessionSaveReason::Soon));
	TestTrue(TEXT("...a save asked for, and the level's end, still save"), Gate.Allows(ESessionSaveReason::Asked) && Gate.Allows(ESessionSaveReason::LevelEnd));
	TestFalse(TEXT("The scene's hold goes first: the lift's still holds"), Gate.Release(TEXT("Scene")));
	TestTrue(TEXT("The lift's is the last: what waited saves now"), Gate.Release(TEXT("OreLift")));
	TestTrue(TEXT("...and the autosave goes again"), Gate.Allows(ESessionSaveReason::Autosave));

	// A ride holds it from its first moment to its last, played or skipped; mid-ride the rider is already on the far landing.
	FScenesOn Scenes;
	FLiftLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Build(RideStep)))
	{
		return false;
	}
	AOreLift* Lift = Level.Lift;
	TestFalse(TEXT("Before the ride the autosave goes"), Lift->IsHoldingSaves());
	TestTrue(TEXT("The ride starts"), Lift->Ride(EOreLiftEnd::Bottom, Level.Rider));
	TestTrue(TEXT("...holding the autosave"), Lift->IsHoldingSaves());
	Level.Tick(6.f);
	TestTrue(TEXT("6 s in, still held"), Lift->IsHoldingSaves());
	TestTrue(TEXT("...and a save now would find the rider on the top landing"), Level.IsRiderOn(EOreLiftEnd::Top));
	Level.Scenes->SkipScene();
	TestFalse(TEXT("Skipped: it's over at once"), Level.Scenes->IsPlaying() || Lift->IsMoving());
	TestFalse(TEXT("...the autosave goes again"), Lift->IsHoldingSaves());
	TestTrue(TEXT("...the cage at the top, the rider on its landing"), Level.IsCageAt(EOreLiftEnd::Top) && Level.IsRiderOn(EOreLiftEnd::Top));
	TestTrue(TEXT("...a skipped ride still arrives"), Level.HasHeard(TEXT("Lift.Arrived.Top")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOreLiftDeathMidRideTest, "Looter.OreLift.DeathMidRide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOreLiftDeathMidRideTest::RunTest(const FString& Parameters)
{
	// A scene holds the player unhurt, so only the console kills a rider; never left hanging, all the same.
	FScenesOn Scenes;
	FLiftLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Build(RideStep, /*bDummyRider*/ true)))
	{
		return false;
	}
	AOreLift* Lift = Level.Lift;
	UHealthComponent* Health = Level.Rider->FindComponentByClass<UHealthComponent>();
	if (!TestNotNull(TEXT("The rider can die"), Health))
	{
		return false;
	}
	AddExpectedMessagePlain(TEXT("mid-ride"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessagePlain(TEXT("without its rider"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	TestTrue(TEXT("The ride starts"), Lift->Ride(EOreLiftEnd::Bottom, Level.Rider));
	Level.Tick(6.f);
	BossTestWorld::Hurt(Level.Rider, 1.0e7f);
	TestTrue(TEXT("The rider dies mid-ride"), Health->IsDead());
	Level.Tick(Frame);
	TestFalse(TEXT("The ride is over at once"), Level.Scenes->IsPlaying() || Lift->IsRiding());
	TestTrue(TEXT("...the cage at the top landing"), Level.IsCageAt(EOreLiftEnd::Top));
	TestTrue(TEXT("...the body on that landing, never over the ridge face"), Level.IsRiderOn(EOreLiftEnd::Top));
	TestFalse(TEXT("...nobody arrived"), Level.HasHeard(TEXT("Lift.Arrived.Top")));
	TestFalse(TEXT("...the autosave goes again"), Lift->IsHoldingSaves());
	TestFalse(TEXT("...and the first ride isn't counted"), Level.Campaign.HasFlag(Lift->RiddenFlag));
	TestTrue(TEXT("The lift runs on: its cage can be called back down"), Lift->CallTo(EOreLiftEnd::Bottom));
	return true;
}

#endif
