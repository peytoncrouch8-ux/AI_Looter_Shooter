#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HUD/HudMinimapWidget.h"
#include "World/MinimapSubsystem.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinimapMarkerPoolTest, "Looter.UI.Minimap.MarkerPool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMinimapMarkerPoolTest::RunTest(const FString& Parameters)
{
	// The markers go to the hostiles first, then the guns, then the ammo. They once went in drawing order (ammo, loot,
	// hostiles), so a fight's pile of drops (kept lying there through every Save & Quit) left no marker for the creatures.
	const FMinimapMarkerSplit Fight = UHudMinimapWidget::SplitMarkers(32, 3, 10, 50);
	TestEqual(TEXT("Every hostile is marked beside a pile of drops"), Fight.Hostiles, 3);
	TestEqual(TEXT("Then every gun"), Fight.Guns, 10);
	TestEqual(TEXT("And ammo with what's left"), Fight.Ammo, 19);

	const FMinimapMarkerSplit Swarm = UHudMinimapWidget::SplitMarkers(32, 40, 5, 5);
	TestTrue(TEXT("A swarm takes every marker"), Swarm.Hostiles == 32 && Swarm.Guns == 0 && Swarm.Ammo == 0);

	const FMinimapMarkerSplit Quiet = UHudMinimapWidget::SplitMarkers(32, 2, 1, 4);
	TestTrue(TEXT("With room for all, all are marked"), Quiet.Hostiles == 2 && Quiet.Guns == 1 && Quiet.Ammo == 4);

	const FMinimapMarkerSplit None = UHudMinimapWidget::SplitMarkers(0, 3, 3, 3);
	TestTrue(TEXT("No markers, nothing marked"), None.Hostiles == 0 && None.Guns == 0 && None.Ammo == 0);
	return true;
}

#endif
