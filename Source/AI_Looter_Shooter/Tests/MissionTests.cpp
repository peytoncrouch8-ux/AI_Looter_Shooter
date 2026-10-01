#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionSubsystem.h"
#include "Tutorial/TutorialDirector.h"
#include "UI/HUD/HudMinimapWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTrackingTest, "Looter.Missions.Tracking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTrackingTest::RunTest(const FString& Parameters)
{
	// The subsystem's bookkeeping, without a world: what gets tracked as missions come and go.
	FMissionBook Book;
	TestTrue(TEXT("Nothing tracked at first"), Book.GetTracked() == INDEX_NONE);
	TestTrue(TEXT("Unknown id finds nothing"), Book.Find(1) == nullptr);

	const int32 First = Book.Add(FText::FromString(TEXT("First")));
	const int32 Second = Book.Add(FText::FromString(TEXT("Second")));
	const int32 Third = Book.Add(FText::FromString(TEXT("Third")));
	TestTrue(TEXT("Ids differ"), First != Second && Second != Third && First != Third);
	TestTrue(TEXT("Ids are real"), First != INDEX_NONE);
	TestTrue(TEXT("The first mission is tracked by itself"), Book.GetTracked() == First);
	TestTrue(TEXT("Later ones don't take over"), Book.GetMissions().Num() == 3 && Book.GetTracked() == First);
	TestTrue(TEXT("Listed in the order added"), Book.GetMissions()[1].Id == Second);
	TestTrue(TEXT("Title kept"), Book.Find(Second) && Book.Find(Second)->Title.ToString() == TEXT("Second"));

	// Picking another (a mission log would).
	TestTrue(TEXT("Track another"), Book.Track(Third));
	TestTrue(TEXT("It's tracked"), Book.GetTracked() == Third);
	TestFalse(TEXT("Tracking it again changes nothing"), Book.Track(Third));
	TestFalse(TEXT("Unknown id is ignored"), Book.Track(999));
	TestTrue(TEXT("Still tracked after an unknown id"), Book.GetTracked() == Third);

	// Objectives: text and a place. Only what a list shows (text, a waypoint coming or going) counts as a change.
	TestTrue(TEXT("New objective is a change"), Book.SetObjective(Third, FText::FromString(TEXT("Go")), FVector(100.f, 0.f, 0.f)));
	TestTrue(TEXT("Objective text kept"), Book.Find(Third)->Objective.ToString() == TEXT("Go"));
	TestTrue(TEXT("Waypoint kept"), Book.Find(Third)->Waypoint.IsSet() && Book.Find(Third)->Waypoint->Equals(FVector(100.f, 0.f, 0.f)));
	TestFalse(TEXT("Same again is no change"), Book.SetObjective(Third, FText::FromString(TEXT("Go")), FVector(100.f, 0.f, 0.f)));
	TestFalse(TEXT("A moving waypoint is no change"), Book.SetObjective(Third, FText::FromString(TEXT("Go")), FVector(150.f, 0.f, 0.f)));
	TestTrue(TEXT("But it moved"), Book.Find(Third)->Waypoint->Equals(FVector(150.f, 0.f, 0.f)));
	TestTrue(TEXT("Losing the waypoint is a change"), Book.SetObjective(Third, FText::FromString(TEXT("Go")), TOptional<FVector>()));
	TestFalse(TEXT("No waypoint now"), Book.Find(Third)->Waypoint.IsSet());
	TestTrue(TEXT("New text is a change"), Book.SetObjective(Third, FText::FromString(TEXT("Come back")), TOptional<FVector>()));
	TestFalse(TEXT("Unknown mission's objective is ignored"), Book.SetObjective(999, FText::FromString(TEXT("Nope")), FVector::ZeroVector));

	// Removing: an untracked one leaves tracking alone; the tracked one hands over to the mission that took its place.
	TestTrue(TEXT("Remove untracked"), Book.Remove(First));
	TestTrue(TEXT("Tracking unchanged"), Book.GetTracked() == Third);
	TestFalse(TEXT("Removing it again does nothing"), Book.Remove(First));
	const int32 Fourth = Book.Add(FText::FromString(TEXT("Fourth")));
	TestTrue(TEXT("The middle one is tracked"), Book.GetTracked() == Third);
	TestTrue(TEXT("Remove the tracked one"), Book.Remove(Third));
	TestTrue(TEXT("The next one is tracked"), Book.GetTracked() == Fourth);
	TestTrue(TEXT("Remove the last, tracked"), Book.Remove(Fourth));
	TestTrue(TEXT("The one before it is tracked"), Book.GetTracked() == Second);
	TestTrue(TEXT("Remove the only one"), Book.Remove(Second));
	TestTrue(TEXT("Nothing tracked when none are left"), Book.GetTracked() == INDEX_NONE && Book.GetMissions().IsEmpty());

	// Tracking none, then adding: the new mission is what the player does now.
	const int32 Fifth = Book.Add(FText::FromString(TEXT("Fifth")));
	TestTrue(TEXT("Track none"), Book.Track(INDEX_NONE));
	TestTrue(TEXT("None tracked"), Book.GetTracked() == INDEX_NONE);
	const int32 Sixth = Book.Add(FText::FromString(TEXT("Sixth")));
	TestTrue(TEXT("Added while none tracked: tracked"), Book.GetTracked() == Sixth && Sixth != Fifth);
	TestTrue(TEXT("Ids aren't reused"), Sixth > Fourth);

	// The tutorial has a name to show as a mission.
	TestFalse(TEXT("Tutorial mission title"), GetDefault<ATutorialDirector>()->MissionTitle.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinimapMissionArrowTest, "Looter.UI.Minimap.MissionArrow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMinimapMissionArrowTest::RunTest(const FString& Parameters)
{
	// 1 px per meter: a waypoint within 70 px of the center shows on the map, a farther one as an arrow 80 px out.
	const float PixelsPerCm = 0.01f;
	const float Inside = 70.f;
	const float Rim = 80.f;

	// Far ahead (north, facing north): at the top of the rim, pointing up.
	const FMinimapWaypoint Ahead = UHudMinimapWidget::PlaceWaypoint(FVector(20000.f, 0.f, 0.f), 0.f, PixelsPerCm, Inside, Rim);
	TestFalse(TEXT("Far ahead is past the rim"), Ahead.bInside);
	TestTrue(TEXT("Far ahead sits at the top of the rim"), Ahead.Position.Equals(FVector2D(0.0, -80.0), 1e-3));
	TestTrue(TEXT("Far ahead points up"), FMath::IsNearlyEqual(Ahead.Angle, 0.f, 1e-3f));

	// Far to the east while facing north: right, pointing right (90 degrees clockwise).
	const FMinimapWaypoint Right = UHudMinimapWidget::PlaceWaypoint(FVector(0.f, 20000.f, 0.f), 0.f, PixelsPerCm, Inside, Rim);
	TestFalse(TEXT("Far right is past the rim"), Right.bInside);
	TestTrue(TEXT("Far right sits on the right of the rim"), Right.Position.Equals(FVector2D(80.0, 0.0), 1e-3));
	TestTrue(TEXT("Far right points right"), FMath::IsNearlyEqual(Right.Angle, 90.f, 1e-3f));

	// Behind and to the left.
	const FMinimapWaypoint Behind = UHudMinimapWidget::PlaceWaypoint(FVector(-20000.f, 0.f, 0.f), 0.f, PixelsPerCm, Inside, Rim);
	TestTrue(TEXT("Behind points down"), FMath::IsNearlyEqual(FMath::Abs(Behind.Angle), 180.f, 1e-3f));
	const FMinimapWaypoint Left = UHudMinimapWidget::PlaceWaypoint(FVector(0.f, -20000.f, 0.f), 0.f, PixelsPerCm, Inside, Rim);
	TestTrue(TEXT("Left points left"), FMath::IsNearlyEqual(Left.Angle, -90.f, 1e-3f));

	// The map turns with the view: facing east, east is ahead.
	const FMinimapWaypoint TurnedEast = UHudMinimapWidget::PlaceWaypoint(FVector(0.f, 20000.f, 0.f), 90.f, PixelsPerCm, Inside, Rim);
	TestTrue(TEXT("Facing it, it points up"), FMath::IsNearlyEqual(TurnedEast.Angle, 0.f, 1e-3f));
	TestTrue(TEXT("Facing it, it's at the top"), TurnedEast.Position.Equals(FVector2D(0.0, -80.0), 1e-3));

	// Close by: on the map itself, where it is.
	const FMinimapWaypoint Near = UHudMinimapWidget::PlaceWaypoint(FVector(3000.f, 0.f, 500.f), 0.f, PixelsPerCm, Inside, Rim);
	TestTrue(TEXT("Near is inside"), Near.bInside);
	TestTrue(TEXT("Near sits where it is"), Near.Position.Equals(FVector2D(0.0, -30.0), 1e-3));
	TestTrue(TEXT("Just short of the edge is still inside"),
		UHudMinimapWidget::PlaceWaypoint(FVector(0.f, 6950.f, 0.f), 0.f, PixelsPerCm, Inside, Rim).bInside);
	TestTrue(TEXT("Standing on it is inside"), UHudMinimapWidget::PlaceWaypoint(FVector::ZeroVector, 45.f, PixelsPerCm, Inside, Rim).bInside);
	return true;
}

#endif
