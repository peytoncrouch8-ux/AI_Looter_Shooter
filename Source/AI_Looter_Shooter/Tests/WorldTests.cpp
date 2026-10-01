#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/TargetDummy.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/WeaponRack.h"
#include "Tutorial/TutorialDirector.h"
#include "World/PCGGroundFitFilter.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundFitTest, "Looter.World.GroundFit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGroundFitTest::RunTest(const FString& Parameters)
{
	using LooterGroundFit::SinkToFit;
	constexpr double Radius = 170.0;
	constexpr double MaxSink = 30.0;

	// Flat ground: a patch lying on it stays where it is.
	auto Flat = [](const FVector&) -> TOptional<double> { return 50.0; };
	const TOptional<double> OnFlat = SinkToFit(FTransform(FVector(0.0, 0.0, 50.0)), Radius, MaxSink, Flat);
	TestTrue(TEXT("Lies on flat ground"), OnFlat.IsSet() && FMath::IsNearlyZero(*OnFlat, 1e-6));

	// A 20 degree slope, with the patch tilted to it (as the meadow's raycast leaves it): it lies on the slope.
	const double Slope = FMath::Tan(FMath::DegreesToRadians(20.0));
	auto Sloped = [Slope](const FVector& Point) -> TOptional<double> { return Point.X * Slope; };
	const FTransform Tilted(FQuat(FVector::YAxisVector, FMath::DegreesToRadians(-20.0)), FVector::ZeroVector);
	const TOptional<double> OnSlope = SinkToFit(Tilted, Radius, MaxSink, Sloped);
	TestTrue(TEXT("Lies on a slope it's tilted to"), OnSlope.IsSet() && FMath::IsNearlyZero(*OnSlope, 0.01));

	// A gentle bump: the edge floats, so the patch goes down until it doesn't.
	auto Bump = [](const FVector& Point) -> TOptional<double> { return -0.0006 * (Point.X * Point.X + Point.Y * Point.Y); };
	const TOptional<double> OnBump = SinkToFit(FTransform::Identity, Radius, MaxSink, Bump);
	TestTrue(TEXT("Pressed into a bump"), OnBump.IsSet());
	TestEqual(TEXT("...by as much as its edge floated"), OnBump.Get(0.0), 0.0006 * FMath::Square(Radius * 0.95), 0.01);

	// Scaled up, the patch's edge is farther out, and floats more over the same bump: past MaxSink it's dropped.
	TestFalse(TEXT("A big patch on the bump would sink too far"),
		SinkToFit(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(1.5)), Radius, MaxSink, Bump).IsSet());

	// An edge: the ground drops away a meter past x = 100 (a cliff top), or isn't there at all (the island's rim).
	auto Step = [](const FVector& Point) -> TOptional<double> { return Point.X > 100.0 ? -100.0 : 0.0; };
	auto Rim = [](const FVector& Point) -> TOptional<double> { return Point.X > 100.0 ? TOptional<double>() : TOptional<double>(0.0); };
	TestFalse(TEXT("Dropped at a cliff top's edge"), SinkToFit(FTransform::Identity, Radius, MaxSink, Step).IsSet());
	TestFalse(TEXT("Dropped at the island's rim"), SinkToFit(FTransform::Identity, Radius, MaxSink, Rim).IsSet());
	TestTrue(TEXT("Kept a patch clear of the edge"), SinkToFit(FTransform(FVector(-100.0, 0.0, 0.0)), Radius, MaxSink, Step).IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialIslandGameplayTest, "Looter.World.TutorialIslandGameplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTutorialIslandGameplayTest::RunTest(const FString& Parameters)
{
	// The tutorial island has everything its tutorial walks through. A rebuild of only its gameplay actors once left it
	// without the gun rack, so the tutorial waited forever at "grab the rifle".
	const UWorld* Island = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_TutorialIsland.Lvl_TutorialIsland"));
	if (!TestNotNull(TEXT("The tutorial island loads"), Island) || !TestNotNull(TEXT("It has a level"), Island->PersistentLevel.Get()))
	{
		return false;
	}
	int32 Starts = 0, Directors = 0, Racks = 0, Dummies = 0, Spiders = 0, Slimes = 0;
	for (const AActor* Actor : Island->PersistentLevel->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		Starts += Actor->IsA<APlayerStart>() ? 1 : 0;
		Directors += Actor->IsA<ATutorialDirector>() ? 1 : 0;
		Dummies += Actor->IsA<ATargetDummy>() ? 1 : 0;
		Spiders += Actor->IsA<ASpiderCreature>() ? 1 : 0;
		Slimes += Actor->IsA<ASlimeCreature>() ? 1 : 0;
		if (const AWeaponRack* Rack = Cast<AWeaponRack>(Actor))
		{
			++Racks;
			TestNotNull(TEXT("The gun rack has a weapon to offer"), Rack->Weapon.Get());
		}
	}
	TestEqual(TEXT("One player start"), Starts, 1);
	TestEqual(TEXT("One tutorial director"), Directors, 1);
	TestEqual(TEXT("One gun rack (the tutorial's first rifle)"), Racks, 1);
	TestTrue(TEXT("Target dummies to shoot"), Dummies > 0);
	TestTrue(TEXT("Spiders to hunt"), Spiders > 0);
	TestTrue(TEXT("Slimes in the meadow"), Slimes > 0);
	AddInfo(FString::Printf(TEXT("%d dummies, %d spiders, %d slimes"), Dummies, Spiders, Slimes));
	return true;
}

#endif
