#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Game/MinimapSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinimapMappingTest, "Looter.UI.Minimap.Mapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMinimapMappingTest::RunTest(const FString& Parameters)
{
	// The baked picture has north (world +X) up and east (world +Y) to the right.
	const FBox2D Bounds(FVector2D(-1000.f), FVector2D(1000.f));
	TestTrue(TEXT("Center maps to the middle"), UMinimapSubsystem::WorldToMapUV(Bounds, FVector::ZeroVector).Equals(FVector2D(0.5, 0.5)));
	TestTrue(TEXT("North edge is the top"), UMinimapSubsystem::WorldToMapUV(Bounds, FVector(1000.f, 0.f, 0.f)).Equals(FVector2D(0.5, 0.0)));
	TestTrue(TEXT("East edge is the right"), UMinimapSubsystem::WorldToMapUV(Bounds, FVector(0.f, 1000.f, 0.f)).Equals(FVector2D(1.0, 0.5)));
	TestTrue(TEXT("South-west corner is bottom-left"), UMinimapSubsystem::WorldToMapUV(Bounds, FVector(-1000.f, -1000.f, 50.f)).Equals(FVector2D(0.0, 1.0)));

	// Markers: the view direction points up, whichever way you face.
	const float Scale = 0.1f;
	TestTrue(TEXT("Facing north, north is up"), UMinimapSubsystem::ViewOffset(FVector(100.f, 0.f, 0.f), 0.f, Scale).Equals(FVector2D(0.0, -10.0), 1e-3));
	TestTrue(TEXT("Facing north, east is right"), UMinimapSubsystem::ViewOffset(FVector(0.f, 100.f, 0.f), 0.f, Scale).Equals(FVector2D(10.0, 0.0), 1e-3));
	TestTrue(TEXT("Facing east, east is up"), UMinimapSubsystem::ViewOffset(FVector(0.f, 100.f, 0.f), 90.f, Scale).Equals(FVector2D(0.0, -10.0), 1e-3));
	TestTrue(TEXT("Facing east, north is left"), UMinimapSubsystem::ViewOffset(FVector(100.f, 0.f, 0.f), 90.f, Scale).Equals(FVector2D(-10.0, 0.0), 1e-3));
	TestTrue(TEXT("Behind you is down"), UMinimapSubsystem::ViewOffset(FVector(-100.f, 0.f, 0.f), 0.f, Scale).Equals(FVector2D(0.0, 10.0), 1e-3));
	TestTrue(TEXT("Height doesn't move a marker"), UMinimapSubsystem::ViewOffset(FVector(0.f, 0.f, 500.f), 37.f, Scale).IsNearlyZero(1e-3));
	return true;
}

#endif
