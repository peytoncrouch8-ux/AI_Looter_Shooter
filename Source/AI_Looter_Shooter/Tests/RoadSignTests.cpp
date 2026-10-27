#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/RoadSign.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoadSignFacesTest, "Looter.World.RoadSign.Faces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoadSignFacesTest::RunTest(const FString& Parameters)
{
	// The two boards point nearly opposite ways (a signpost names a branch and the way back): 190 degrees apart.
	const float Apart = FMath::Abs(FMath::FindDeltaAngleDegrees(ARoadSign::BoardYaw(0), ARoadSign::BoardYaw(1)));
	TestTrue(TEXT("The boards point nearly opposite ways"), Apart > 160.f);

	for (int32 Board = 0; Board < 2; ++Board)
	{
		const FRoadSignFace Front = ARoadSign::Face(Board, true);
		const FRoadSignFace Back = ARoadSign::Face(Board, false);
		const float Yaw = FMath::DegreesToRadians(ARoadSign::BoardYaw(Board));
		const FVector Along(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.f);
		// Each name lies along its own board (out from the post the way the arrow points), off the 12 cm post.
		TestTrue(TEXT("The name lies along its board"), FVector::DotProduct(Front.Middle, Along) > 20.f);
		TestTrue(TEXT("The back's name too"), FVector::DotProduct(Back.Middle, Along) > 20.f);
		// The two faces look out opposite ways, and the front's name stands out past the back's (the board between).
		TestTrue(TEXT("The faces look opposite ways"),
			FMath::IsNearlyEqual(FMath::Abs(FMath::FindDeltaAngleDegrees(Front.FacingYaw, Back.FacingYaw)), 180.f, 0.01f));
		const FVector FrontOut = FRotator(0.f, Front.FacingYaw, 0.f).Vector();
		TestTrue(TEXT("The board lies between its two names"),
			FVector::DotProduct(Front.Middle - Back.Middle, FrontOut) > 2.f);
		// The names stand at the boards' heights (1.98 m and 1.72 m), on a 16 cm board.
		TestTrue(TEXT("The name is on its board's height"), FMath::Abs(Front.Middle.Z - (Board == 0 ? 198.f : 172.f)) < 3.f);
		TestTrue(TEXT("There's room for a name"), Front.Room > 30.f && Front.Room <= 60.f);
	}
	TestTrue(TEXT("The upper board stands over the lower"), ARoadSign::Face(0, true).Middle.Z > ARoadSign::Face(1, true).Middle.Z + 20.f);
	return true;
}

#endif
