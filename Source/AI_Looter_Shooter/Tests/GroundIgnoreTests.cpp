#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/GroundIgnoreSubsystem.h"
#include "World/PlayableArea.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundIgnoreCacheTest, "Looter.World.GroundIgnoreCache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGroundIgnoreCacheTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UGroundIgnoreSubsystem* Cache = World ? World->GetSubsystem<UGroundIgnoreSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("The preview world keeps the ground ignore list"), Cache))
	{
		return false;
	}
	auto Ignores = [World](const AActor* Actor)
	{
		return LooterWorld::StaticGeometryParams(World, TEXT("GroundIgnoreTest")).GetIgnoredSourceObjects().Contains(Actor->GetUniqueID());
	};

	// Asking again changes nothing, so the world's actors are looked through once, not on every call.
	LooterWorld::StaticGeometryParams(World, TEXT("GroundIgnoreTest"));
	LooterWorld::StaticGeometryParams(World, TEXT("GroundIgnoreTest"), nullptr, false);
	TestEqual(TEXT("Two calls in an unchanged world look once"), Cache->GetLookupCount(), 1);

	// A volume spawned after the first call is left out of ground traces from then on.
	APostProcessVolume* Volume = World->SpawnActor<APostProcessVolume>();
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		return false;
	}
	const uint32 VolumeId = Volume->GetUniqueID();
	TestTrue(TEXT("A volume spawned after the first call is ignored"), Ignores(Volume));
	TestEqual(TEXT("...found by looking again"), Cache->GetLookupCount(), 2);

	// So is a playable area spawned later.
	APlayableArea* Area = World->SpawnActor<APlayableArea>();
	if (!TestNotNull(TEXT("Playable area spawned"), Area))
	{
		return false;
	}
	TestTrue(TEXT("A playable area spawned later is ignored"), Ignores(Area));
	TestTrue(TEXT("...and the volume still is"), Ignores(Volume));
	TestEqual(TEXT("...with one more look"), Cache->GetLookupCount(), 3);

	// Another kind of actor changes nothing.
	World->SpawnActor<AActor>();
	LooterWorld::StaticGeometryParams(World, TEXT("GroundIgnoreTest"));
	TestEqual(TEXT("An ordinary actor spawned leaves the list alone"), Cache->GetLookupCount(), 3);

	// A destroyed volume leaves the list: its id may be handed to a new object later.
	Volume->Destroy();
	const FCollisionQueryParams After = LooterWorld::StaticGeometryParams(World, TEXT("GroundIgnoreTest"));
	TestFalse(TEXT("A destroyed volume is no longer ignored"), After.GetIgnoredSourceObjects().Contains(VolumeId));
	TestTrue(TEXT("...the playable area still is"), After.GetIgnoredSourceObjects().Contains(Area->GetUniqueID()));
	TestEqual(TEXT("...found by looking again"), Cache->GetLookupCount(), 4);

	// The actor a trace starts from is ignored too, cached list or not.
	AActor* Shooter = World->SpawnActor<AActor>();
	const FCollisionQueryParams WithShooter = LooterWorld::StaticGeometryParams(World, TEXT("GroundIgnoreTest"), Shooter);
	TestTrue(TEXT("The ignored actor passed in is skipped"), WithShooter.GetIgnoredSourceObjects().Contains(Shooter->GetUniqueID()));
	return true;
}

#endif
