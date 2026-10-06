#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/TargetDummy.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/WeaponRack.h"
#include "Tutorial/TutorialDirector.h"
#include "World/PCGGroundFitFilter.h"
#include "World/SkiffJetty.h"
#include "Algo/AnyOf.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/UObjectHash.h"

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
	// The tutorial island has everything its tutorial walks through, and the way off it. A rebuild of only its gameplay
	// actors once left it without the gun rack, so the tutorial waited forever at "grab the rifle".
	const UWorld* Island = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_TutorialIsland.Lvl_TutorialIsland"));
	if (!TestNotNull(TEXT("The tutorial island loads"), Island) || !TestNotNull(TEXT("It has a level"), Island->PersistentLevel.Get()))
	{
		return false;
	}
	int32 Starts = 0, Directors = 0, Racks = 0, Dummies = 0, Spiders = 0, Slimes = 0, Jetties = 0;
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
		Jetties += Actor->IsA<ASkiffJetty>() ? 1 : 0;
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
	// The way off Skyreach: its jetty carries the landing practice trips arrive at (Landing_Jetty).
	TestEqual(TEXT("One skiff jetty (build_area.py's gameplay places it)"), Jetties, 1);
	AddInfo(FString::Printf(TEXT("%d dummies, %d spiders, %d slimes"), Dummies, Spiders, Slimes));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialIslandObstaclesTest, "Looter.World.TutorialIslandObstacles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTutorialIslandObstaclesTest::RunTest(const FString& Parameters)
{
	// The island's scattered trees, rocks, stumps and logs block whoever walks into them, the player and the creatures
	// alike, with their hulls; the ground cover (grass, flowers, ferns, bushes, reeds, pebbles) never collides. PCG's
	// instances don't collide unless told to, and for a while the trees and rocks were walk-through: spiders walked
	// straight through them.
	const UWorld* Island = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_TutorialIsland.Lvl_TutorialIsland"));
	if (!TestNotNull(TEXT("The tutorial island loads"), Island) || !TestNotNull(TEXT("It has a level"), Island->PersistentLevel.Get()))
	{
		return false;
	}
	static const TCHAR* const SolidPrefixes[] = { TEXT("SM_Rock_"), TEXT("SM_Boulder_"), TEXT("SM_Stump_"), TEXT("SM_Log_") };
	int32 Trees = 0, Solids = 0, GroundCover = 0;
	ForEachObjectWithOuter(Island->PersistentLevel.Get(), [&](UObject* Object)
	{
		const UInstancedStaticMeshComponent* Instances = Cast<UInstancedStaticMeshComponent>(Object);
		const UStaticMesh* Mesh = Instances ? Instances->GetStaticMesh().Get() : nullptr;
		if (!Mesh || Instances->GetInstanceCount() == 0)
		{
			return;
		}
		const FString Name = Mesh->GetName();
		const bool bTree = Instances->ComponentHasTag(TEXT("Tree"));
		const bool bSolid = bTree || Algo::AnyOf(SolidPrefixes, [&Name](const TCHAR* Prefix) { return Name.StartsWith(Prefix); });
		if (!bSolid)
		{
			++GroundCover;
			TestTrue(Name + TEXT(" (ground cover) never collides"), Instances->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			return;
		}
		(bTree ? Trees : Solids) += Instances->GetInstanceCount();
		TestTrue(Name + TEXT(" collides"), Instances->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
		TestTrue(Name + TEXT(" blocks a walking pawn"), Instances->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
		const UBodySetup* Body = Mesh->GetBodySetup();
		TestTrue(Name + TEXT(" has hulls to collide with"), Body && Body->AggGeom.GetElementCount() > 0);
		if (bTree && Body)
		{
			// A tree's trunk hull is all of it a bullet meets; never the see-through parts of its leaf cards.
			TestTrue(Name + TEXT(" collides with its hull only"), Body->CollisionTraceFlag == CTF_UseSimpleAsComplex);
		}
	}, /*bIncludeNestedObjects*/ true);
	TestTrue(TEXT("Trees on the island"), Trees > 0);
	TestTrue(TEXT("Rocks, stumps and logs on the island"), Solids > 0);
	TestTrue(TEXT("Ground cover on the island"), GroundCover > 0);
	AddInfo(FString::Printf(TEXT("%d trees, %d rocks, stumps and logs, %d ground cover meshes"), Trees, Solids, GroundCover));
	return true;
}

#endif
