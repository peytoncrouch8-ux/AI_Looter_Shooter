#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// An encounter under a roof (the play-through of 2026-10-14: Tilly's cellar's dead all came up on the yard over the cellar,
// and "Quiet the dead in the cellar" couldn't be done there). Its home is its own floor and its creatures stand on it,
// never on the workshop's floor or the yard over it, though those are less than a step up; out in the open the ground is
// found as it always was.

#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterGroundProbe.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/HuntingGround.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/EncounterTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Math/Box.h"
#include "Tests/AutomationCommon.h"

namespace
{
	/** What the level's buildings carry for the minimap: nothing spawns on top of one. */
	const FName ObstacleTag(TEXT("Obstacle"));

	// A cellar like Tilly's, with its collision as Art/Models/Buildings/WorkshopBright.py makes it (world cm): 12 x 8 m inside,
	// its floor 3.3 m under the yard, its vaults' collision a slab 2.55 m over the floor (its top 0.4 m under the yard), and the
	// workshop over part of it (its floor 0.3 m over the yard, its roof), the cellar reaching out under the yard past it.
	constexpr double FloorZ = -280.0;
	constexpr double VaultUnderZ = -25.0;
	constexpr double VaultTopZ = 10.0;
	constexpr double YardZ = 50.0;
	constexpr double ShopFloorZ = 80.0;
	const FBox Inside(FVector(-600.0, -400.0, FloorZ), FVector(600.0, 400.0, VaultUnderZ));

	/** Where the dead stand by their coffins, relative to the spawner (as build_area_missionprops_valley.py sets them). */
	TArray<FVector> CoffinPoints()
	{
		return { FVector(-450.0, 250.0, 0.0), FVector(-450.0, -250.0, 0.0), FVector(300.0, 250.0, 0.0), FVector(300.0, -250.0, 0.0) };
	}

	/** The cellar's dead: three Unpaid and a Restless one. */
	TArray<FEncounterGroup> CellarDead()
	{
		return { EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 3),
			EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 1, ECreatureRank::Rare) };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterCellarTest, "Looter.Encounters.Cellar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterCellarTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	// The workshop and its cellar are one building, tagged Obstacle, as the level's is; the yard is plain ground round it.
	const AActor* Building = EncounterTestWorld::SpawnBlocks(World, {
		FBox(FVector(-640.0, -440.0, FloorZ - 30.0), FVector(640.0, 440.0, FloorZ)),			// the cellar's floor
		FBox(FVector(-640.0, -440.0, FloorZ - 30.0), FVector(-600.0, 440.0, ShopFloorZ)),		// its walls
		FBox(FVector(600.0, -440.0, FloorZ - 30.0), FVector(640.0, 440.0, ShopFloorZ)),
		FBox(FVector(-640.0, -440.0, FloorZ - 30.0), FVector(640.0, -400.0, ShopFloorZ)),
		FBox(FVector(-640.0, 400.0, FloorZ - 30.0), FVector(640.0, 440.0, ShopFloorZ)),
		FBox(FVector(-600.0, -400.0, VaultUnderZ), FVector(600.0, 400.0, VaultTopZ)),			// its vaults
		FBox(FVector(-200.0, -350.0, YardZ), FVector(800.0, 350.0, ShopFloorZ)),				// the workshop's floor
		FBox(FVector(-200.0, -350.0, 400.0), FVector(800.0, 350.0, 440.0)) },					// and its roof
		ObstacleTag);
	const AActor* Yard = EncounterTestWorld::SpawnBlocks(World, { FBox(FVector(-3000.0, -3000.0, YardZ - 10.0), FVector(3000.0, 3000.0, YardZ)) });
	if (!TestTrue(TEXT("The workshop over its cellar, and the yard"), Building && Yard))
	{
		return false;
	}

	// The ground a spawner on the cellar's floor finds, for the body of its dead (the Restless one's), as it stands there: on
	// the floor itself.
	const FEncounterBody Body = EncounterRules::LargestBody(CellarDead(), nullptr);
	FEncounterGroundProbe Below(World, nullptr, TEXT("TestCellar"), Body, 400.f);
	const bool bCellarHome = Below.FindHome(FVector(0.0, 0.0, FloorZ), 300.0, 2000.0);
	TestTrue(*FString::Printf(TEXT("A spawner on the cellar's floor: its ground is that floor, not the vaults' top 2.9 m over it (found %.0f)"),
		Below.GetHomeZ()), bCellarHome && FMath::IsNearlyEqual(Below.GetHomeZ(), FloorZ, 1.0));
	TestTrue(*FString::Printf(TEXT("...under a roof: the vaults (found %.0f)"), Below.GetRoofZ()), Below.HasRoof()
		&& FMath::IsNearlyEqual(Below.GetRoofZ(), VaultUnderZ, 1.0));
	const FEncounterGroundHit UnderYard = Below.Look(FVector(-450.0, 250.0, 0.0));
	TestTrue(*FString::Printf(TEXT("Its spot under the yard: the cellar's floor, with room for the whole body (at %.0f)"), UnderYard.Point.Z),
		UnderYard.bFound && UnderYard.bStandable && UnderYard.Room >= 1.f && FMath::IsNearlyEqual(UnderYard.Point.Z, FloorZ, 1.0));
	const FEncounterGroundHit UnderShop = Below.Look(FVector(300.0, -250.0, 0.0));
	TestTrue(*FString::Printf(TEXT("...and under the workshop: the cellar's floor, not the workshop's (at %.0f)"), UnderShop.Point.Z),
		UnderShop.bFound && UnderShop.bStandable && UnderShop.Room >= 1.f && FMath::IsNearlyEqual(UnderShop.Point.Z, FloorZ, 1.0));
	TestFalse(TEXT("The yard past the cellar's walls, 3.3 m up: another storey, not found"), Below.Look(FVector(-1500.0, 0.0, 0.0)).bFound);

	// Out in the open, as always: a spawner on the yard finds the yard, and over the cellar the yard is its ground too; the
	// workshop is a building's, no ground of the yard's.
	FEncounterGroundProbe Over(World, nullptr, TEXT("TestCellar"), Body, 400.f);
	const bool bYardHome = Over.FindHome(FVector(-1500.0, 0.0, YardZ + 50.0), 300.0, 2000.0);
	TestTrue(TEXT("A spawner on the yard: its ground is the yard, under no roof"), bYardHome
		&& FMath::IsNearlyEqual(Over.GetHomeZ(), YardZ, 1.0) && !Over.HasRoof());
	const FEncounterGroundHit OverCellar = Over.Look(FVector(-450.0, 250.0, 0.0));
	TestTrue(TEXT("...the yard over the cellar is its ground"), OverCellar.bFound && OverCellar.bStandable
		&& FMath::IsNearlyEqual(OverCellar.Point.Z, YardZ, 1.0));
	const FEncounterGroundHit Shop = Over.Look(FVector(300.0, -250.0, 0.0));
	TestTrue(TEXT("...the workshop isn't"), Shop.bFound && !Shop.bStandable);

	// The cellar's fight as build_area_missionprops_valley.py places it: its spawner on the floor, its points by the coffins,
	// its ground the cellar alone (no more than 2.5 m above or below its floor). The player comes near in the yard over it,
	// 16 m off, as in the play-through: all four come out on the cellar's floor.
	const ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-1400.0, -700.0, YardZ + 100.0));
	AEncounterSpawner* Cellar = EncounterTestWorld::SpawnSpawner(World, FVector(0.0, 0.0, FloorZ), [](AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("TestCellar");
		Setup.Groups = CellarDead();
		Setup.CreatureTags = { FName(TEXT("Unpaid_TestCellar")) };
		Setup.SpawnPoints = CoffinPoints();
		Setup.GroundCorners = { FVector(Inside.Min.X, Inside.Min.Y, FloorZ), FVector(Inside.Max.X, Inside.Min.Y, FloorZ),
			FVector(Inside.Max.X, Inside.Max.Y, FloorZ), FVector(Inside.Min.X, Inside.Max.Y, FloorZ) };
		Setup.GroundMaxRise = 250.f;
		Setup.ActivationRadius = 2500.f;
	});
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The cellar's spawner"), Cellar))
	{
		return false;
	}
	Cellar->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Out = Cellar->GetAliveCreatures();
	TestEqual(TEXT("The player in the yard over it: the cellar's four are out"), Out.Num(), 4);
	for (const ACreatureBase* Dead : Out)
	{
		const FVector At = Dead->GetActorLocation();
		const double Feet = At.Z - Dead->GetSimpleCollisionHalfHeight();
		TestTrue(*FString::Printf(TEXT("%s on the cellar's floor (feet at %.0f), not on the yard or the workshop's floor over it"),
			*Dead->GetName(), Feet), FMath::IsNearlyEqual(Feet, FloorZ, 10.0));
		TestTrue(*FString::Printf(TEXT("%s inside the cellar (%.0f, %.0f)"), *Dead->GetName(), At.X, At.Y), Inside.IsInsideXY(At));
		TestTrue(TEXT("...its home there too"), FMath::IsNearlyEqual(Dead->GetHome().GetLocation().Z, At.Z, 1.0));
		TestFalse(TEXT("...and the yard over the cellar isn't its hunting ground"),
			Dead->HuntingGround.Contains(FVector(-450.0, 0.0, YardZ + 90.0), Dead->GetHome().GetLocation()));
	}
	return true;
}

#endif
