#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/AmbushSpawner.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/SpiderCreature.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/StoryCondition.h"
#include "Tests/EncounterLayoutReader.h"
#include "Tests/EncounterTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

// Ransom's Rest's encounters between the story's fights (layout.json gameplay.encounters; Tools/Unreal/build_area_camps.py
// places them): where they stand, read from the layout itself (EncounterLayoutReader.h), and when the story lets them on.
// The level as built is checked inside its boundary by Looter.World.RansomsRest.InsideBoundary; how they play is in
// EncounterPackTests.cpp and EncounterPatrolTests.cpp.

using namespace EncounterLayout;

namespace
{
	/** Obstacle kinds that stand solid on the ground (a creature can't stand in one), and the line kinds a body can't stand on. */
	const TArray<FString> SolidKinds = { TEXT("outcrop"), TEXT("ruin"), TEXT("boulders"), TEXT("trees"), TEXT("props"), TEXT("graves") };
	const TArray<FString> LineKinds = { TEXT("fence"), TEXT("wall"), TEXT("graves"), TEXT("cairns") };

	/** The margins the placement keeps (cm): from the boundary's edge, a safe zone or arrival, a scripted fight's ground, a solid thing. */
	constexpr double BoundaryClear = 300.0;
	constexpr double KeepOutClear = 200.0;
	constexpr double FightClear = 300.0;
	constexpr double SolidClear = 100.0;
	constexpr double FenceClear = 150.0;
	constexpr double LineClear = 120.0;
	/** Off a mesa's cliff (Coffin Rock), and past a ridge's half width (the Nose) (cm). */
	constexpr double MesaClear = 400.0;
	constexpr double RidgeClear = 200.0;
	/** Out of the Dry Wash past its half width; a creek camp out of the water past its half width, anything else out of its bottom (cm). */
	constexpr double WashClear = 150.0;
	constexpr double WaterClear = 100.0;
	constexpr double BottomClear = 200.0;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterCampPlacementTest, "Looter.Encounters.Camps.Placement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterCampPlacementTest::RunTest(const FString& Parameters)
{
	// Every camp, patrol and ambush between the story's fights stands where the route passes but no fight happens: inside
	// the boundary, out of the safe zones and arrivals, off every scripted fight's ground while that fight can happen, on
	// walkable ground (off rocks, ruins, trees, fences and walls, out of the Dry Wash, the creek's water, Coffin Rock and
	// the Nose), its spots on its own ground, and no two sharing ground. Camps have a Restless leader among 3-5 creatures.
	FAreaLayout Layout;
	if (!ReadLayout(*this, Layout))
	{
		return false;
	}
	const FAreaPiece* Wash = Layout.Feature(TEXT("dryWash"));
	const FAreaPiece* Creek = Layout.Feature(TEXT("millCreek"));
	for (const FAreaEncounter& Each : Layout.Encounters)
	{
		const FString Name = FString::Printf(TEXT("%s (%s)"), *Each.Id, *Each.Section);
		TestTrue(FString::Printf(TEXT("%s waits for the story (after a mission)"), *Name), Each.After.Num() > 0);
		TestTrue(FString::Printf(TEXT("%s has 3-5 creatures (%d)"), *Name, Each.Creatures), Each.Creatures >= 3 && Each.Creatures <= 5);
		if (Each.Section == TEXT("camps"))
		{
			TestTrue(FString::Printf(TEXT("%s has a Restless or better leader, and a crate"), *Name), Each.bLeader && Each.Crates.Num() > 0);
		}
		TestTrue(FString::Printf(TEXT("%s has its ground"), *Name), Each.Ground.Polygon.Num() >= 3 || Each.Ground.Radius > 0.0);

		TArray<FVector2D> Places = Each.Spots;
		Places.Append(Each.Route);
		TArray<FVector2D> Everything = Places;
		Everything.Append(Each.Crates);
		const bool bCreekCamp = Creek && Places.ContainsByPredicate([Creek](const FVector2D& Spot)
		{
			return DistanceToLine(Creek->Points, Spot, false) <= Creek->BottomWidth * 0.5;
		});
		for (const FVector2D& Spot : Everything)
		{
			const FString At = FString::Printf(TEXT("%s's (%.0f, %.0f)"), *Name, Spot.X, Spot.Y);
			const bool bCrate = Each.Crates.Contains(Spot) && !Places.Contains(Spot);
			TestTrue(FString::Printf(TEXT("%s is inside the boundary, 3 m from its edge"), *At),
				IsInside(Layout.Boundary, Spot) && DistanceToLine(Layout.Boundary, Spot, true) >= BoundaryClear);
			for (int32 Zone = 0; Zone < Layout.KeepOut.Num(); ++Zone)
			{
				TestTrue(FString::Printf(TEXT("%s keeps out of %s"), *At, *Layout.KeepOutNames[Zone]), Layout.KeepOut[Zone].Distance(Spot) >= KeepOutClear);
			}
			for (const FAreaFight& Fight : Layout.Fights)
			{
				if (MayOverlapInTime(Each.After, Fight))
				{
					TestTrue(FString::Printf(TEXT("%s is off %s's ground"), *At, *Fight.Id), Fight.Ground.Distance(Spot) >= FightClear);
				}
			}
			for (const FAreaPiece& Piece : Layout.Obstacles)
			{
				if (Piece.Id == Each.OwnObstacle)
				{
					continue;
				}
				if (Piece.bPolygon && SolidKinds.Contains(Piece.Kind))
				{
					const double Edge = DistanceToLine(Piece.Points, Spot, true);
					const double Outside = IsInside(Piece.Points, Spot) ? -Edge : Edge;
					// A crate may stand tucked against a wall; a creature needs room round it.
					TestTrue(FString::Printf(TEXT("%s stands clear of %s"), *At, *Piece.Id), Outside >= (bCrate ? 0.0 : SolidClear));
				}
				else if (Piece.bPolygon && Piece.Kind == TEXT("fence"))
				{
					TestTrue(FString::Printf(TEXT("%s stands clear of the fence %s"), *At, *Piece.Id),
						!IsInside(Piece.Points, Spot) && DistanceToLine(Piece.Points, Spot, true) >= FenceClear);
				}
				else if (!Piece.bPolygon && LineKinds.Contains(Piece.Kind))
				{
					TestTrue(FString::Printf(TEXT("%s stands clear of the line %s"), *At, *Piece.Id), DistanceToLine(Piece.Points, Spot, false) >= LineClear);
				}
			}
			if (Wash)
			{
				TestTrue(FString::Printf(TEXT("%s is out of the Dry Wash"), *At), DistanceToLine(Wash->Points, Spot, false) >= Wash->Width * 0.5 + WashClear);
			}
			if (Creek && !bCrate)
			{
				// A creek camp stands in the bottom, out of the water; anything else keeps out of the bottom.
				const double Reach = bCreekCamp ? Creek->Width * 0.5 + WaterClear : Creek->BottomWidth * 0.5 + BottomClear;
				TestTrue(FString::Printf(TEXT("%s keeps out of Mill Creek's %s"), *At, bCreekCamp ? TEXT("water") : TEXT("bottom")),
					DistanceToLine(Creek->Points, Spot, false) >= Reach);
			}
			for (const FAreaPiece& Piece : Layout.Features)
			{
				if (Piece.Kind == TEXT("mesa") && Piece.bPolygon)
				{
					TestTrue(FString::Printf(TEXT("%s keeps off %s"), *At, *Piece.Id),
						!IsInside(Piece.Points, Spot) && DistanceToLine(Piece.Points, Spot, true) >= MesaClear);
				}
				else if (Piece.Kind == TEXT("ridge"))
				{
					TestTrue(FString::Printf(TEXT("%s keeps off %s"), *At, *Piece.Id), DistanceToLine(Piece.Points, Spot, false) >= Piece.Width * 0.5 + RidgeClear);
				}
			}
		}
		for (const FVector2D& Spot : Places)
		{
			TestTrue(FString::Printf(TEXT("%s's (%.0f, %.0f) is on its own ground"), *Name, Spot.X, Spot.Y), Each.Ground.Distance(Spot) <= 0.0);
		}
		// Its ground keeps off a scripted fight that can happen while it's on: no chase runs from one into the other.
		for (const FVector2D& Corner : Each.Ground.Samples())
		{
			for (const FAreaFight& Fight : Layout.Fights)
			{
				if (!Fight.During.IsNone() && MayOverlapInTime(Each.After, Fight))
				{
					TestTrue(FString::Printf(TEXT("%s's ground at (%.0f, %.0f) is off %s's"), *Name, Corner.X, Corner.Y, *Fight.Id),
						Fight.Ground.Distance(Corner) >= 0.0);
				}
			}
		}
	}
	// No two of them share ground: each is its own fight.
	for (int32 First = 0; First < Layout.Encounters.Num(); ++First)
	{
		for (int32 Second = First + 1; Second < Layout.Encounters.Num(); ++Second)
		{
			const FAreaEncounter& A = Layout.Encounters[First];
			const FAreaEncounter& B = Layout.Encounters[Second];
			bool bApart = true;
			for (const FVector2D& Corner : A.Ground.Samples())
			{
				bApart &= B.Ground.Distance(Corner) > 0.0;
			}
			for (const FVector2D& Corner : B.Ground.Samples())
			{
				bApart &= A.Ground.Distance(Corner) > 0.0;
			}
			TestTrue(FString::Printf(TEXT("%s and %s don't share ground"), *A.Id, *B.Id), bApart);
		}
	}
	AddInfo(FString::Printf(TEXT("%d encounters between the fights checked against %d story fights and %d keep-outs."), Layout.Encounters.Num(),
		Layout.Fights.Num(), Layout.KeepOut.Num()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterCampGatingTest, "Looter.Encounters.Camps.Gating",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterCampGatingTest::RunTest(const FString& Parameters)
{
	// Nothing between the fights comes before the town (Main 3): the first hour's path stays the story's. Each waits for its
	// missions turned in; the churchyard's dead, the Webwood's spiders, the sheep fold's camp and the west road's walkers
	// wait for Main 4 (the yard's own fight, the dead walking after it, Aldana sending Ellis to the Sink). In a level, a
	// spawner with such a condition stays off until then, and comes on as the missions change.
	FAreaLayout Layout;
	if (!ReadLayout(*this, Layout))
	{
		return false;
	}
	auto ConditionOf = [](const FAreaEncounter& Each)
	{
		FStoryCondition When;
		When.AfterMissions = Each.After;
		return When;
	};
	auto CampaignAfter = [](int32 MainsDone)
	{
		FCampaignRecord Campaign;
		for (int32 Index = 0; Index < MainsDone; ++Index)
		{
			Campaign.Complete(MainOrder()[Index]);
		}
		return Campaign;
	};
	for (const FAreaEncounter& Each : Layout.Encounters)
	{
		const FStoryCondition When = ConditionOf(Each);
		TestFalse(FString::Printf(TEXT("%s is off on a new game"), *Each.Id), When.IsMet(CampaignAfter(0)));
		TestFalse(FString::Printf(TEXT("%s is off before the town (Main 2 done)"), *Each.Id), When.IsMet(CampaignAfter(2)));
		TestTrue(FString::Printf(TEXT("%s is on by Main 4's end"), *Each.Id), When.IsMet(CampaignAfter(4)));
		TestTrue(FString::Printf(TEXT("%s stays on after the story (Main 7 done)"), *Each.Id), When.IsMet(CampaignAfter(7)));
		if (Each.Id == TEXT("Churchyard") || Each.Id == TEXT("Webwood") || Each.Id == TEXT("SheepFold") || Each.Id == TEXT("WestRoad"))
		{
			TestFalse(FString::Printf(TEXT("%s waits for Main 4 (off with Main 3 done)"), *Each.Id), When.IsMet(CampaignAfter(3)));
		}
	}

	// In a level: the churchyard's kind of ambush (after Main 4) and a camp (after Main 3), the campaign at Main 2's end.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(2500.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("The level has a mission runner"), Runner) || !TestNotNull(TEXT("...and encounters"), Encounters)
		|| !TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}
	FCampaignRecord Campaign = CampaignAfter(2);
	Runner->BeginForTesting({}, Campaign, Player);
	AAmbushSpawner* Yard = World->SpawnActor<AAmbushSpawner>(FVector::ZeroVector, FRotator::ZeroRotator);
	AEncounterSpawner* Camp = World->SpawnActor<AEncounterSpawner>(FVector(-6000.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Spawners"), Yard) || !TestNotNull(TEXT("Spawners"), Camp))
	{
		return false;
	}
	Yard->Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 3) };
	Yard->ActiveWhen.AfterMissions = { TEXT("Main4") };
	Camp->Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 3) };
	Camp->ActiveWhen.AfterMissions = { TEXT("Main3") };
	Yard->DispatchBeginPlay();
	Camp->DispatchBeginPlay();
	TestTrue(TEXT("Main 2 done: both off"), Yard->GetState() == EEncounterState::Off && Camp->GetState() == EEncounterState::Off);
	Campaign.Complete(TEXT("Main3"));
	Encounters->RefreshStory();
	TestTrue(TEXT("Main 3 turned in: the camp is on, the yard still off"), Camp->GetState() == EEncounterState::Waiting
		&& Yard->GetState() == EEncounterState::Off);
	Campaign.Complete(TEXT("Main4"));
	Encounters->RefreshStory();
	TestTrue(TEXT("Main 4 turned in: the yard's dead wait for the player"), Yard->GetState() == EEncounterState::Waiting && Yard->NumAlive() == 0);
	return true;
}

#endif
