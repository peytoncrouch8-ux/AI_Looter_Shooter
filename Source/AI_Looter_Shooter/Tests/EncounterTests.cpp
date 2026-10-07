#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/HuntingGround.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Tests/EncounterTestWorld.h"
#include "World/SafeGround.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The rules an encounter plays by: its groups' ranks and waves, where its creatures may stand, and the caps. How one plays
// out (waves, the story, safe zones, nothing coming back) is in EncounterPlayTests.cpp.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterGroupsTest, "Looter.Encounters.Groups",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterGroupsTest::RunTest(const FString& Parameters)
{
	// Ranks as a group rolls them: one rank for all ("one of them Restless" is a group of one), the group's own chances, or
	// the area's promotions (8% Restless and 2% Gravebound on Ransom's Rest; Basic in a level that is no area's).
	const FEncounterGroup Lieutenant = EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 1, ECreatureRank::Rare);
	TestTrue(TEXT("A fixed group: its rank, whatever the roll"), EncounterRules::PickRank(Lieutenant, nullptr, 0.f) == ECreatureRank::Rare
		&& EncounterRules::PickRank(Lieutenant, nullptr, 0.99f) == ECreatureRank::Rare);
	FEncounterGroup OwnChances = EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 4);
	OwnChances.RankRoll = EEncounterRankRoll::Chances;
	OwnChances.RareChance = 0.2f;
	OwnChances.EpicChance = 0.1f;
	TestTrue(TEXT("Its own chances: Gravebound first"), EncounterRules::PickRank(OwnChances, nullptr, 0.05f) == ECreatureRank::Epic);
	TestTrue(TEXT("...then Restless"), EncounterRules::PickRank(OwnChances, nullptr, 0.25f) == ECreatureRank::Rare);
	TestTrue(TEXT("...then Basic"), EncounterRules::PickRank(OwnChances, nullptr, 0.31f) == ECreatureRank::Basic);
	FEncounterGroup ByArea = EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 4);
	ByArea.RankRoll = EEncounterRankRoll::Area;
	UAreaDefinition* Valley = NewObject<UAreaDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
	Valley->RarePromotionChance = 0.08f;
	Valley->EpicPromotionChance = 0.02f;
	TestTrue(TEXT("The area's promotions: Gravebound under 2%"), EncounterRules::PickRank(ByArea, Valley, 0.01f) == ECreatureRank::Epic);
	TestTrue(TEXT("...Restless in the 8% after"), EncounterRules::PickRank(ByArea, Valley, 0.05f) == ECreatureRank::Rare);
	TestTrue(TEXT("...Basic otherwise"), EncounterRules::PickRank(ByArea, Valley, 0.5f) == ECreatureRank::Basic);
	TestTrue(TEXT("No area: Basic"), EncounterRules::PickRank(ByArea, nullptr, 0.01f) == ECreatureRank::Basic);

	// The largest rank each can roll, for the room its creatures need: its fixed one, or the biggest its own chances or the
	// area's promotions can give.
	auto SizeOf = [](ECreatureRank Rank) { return UCreatureRankSettings::Get(Rank).Size; };
	const float UpToGravebound = FMath::Max3(SizeOf(ECreatureRank::Basic), SizeOf(ECreatureRank::Rare), SizeOf(ECreatureRank::Epic));
	TestNearlyEqual(TEXT("A fixed group: its rank's size"), EncounterRules::LargestRankSize(Lieutenant, nullptr), SizeOf(ECreatureRank::Rare),
		0.001f);
	TestNearlyEqual(TEXT("Its own chances: up to Gravebound's"), EncounterRules::LargestRankSize(OwnChances, nullptr), UpToGravebound, 0.001f);
	TestNearlyEqual(TEXT("The area's promotions: up to Gravebound's"), EncounterRules::LargestRankSize(ByArea, Valley), UpToGravebound, 0.001f);
	TestNearlyEqual(TEXT("No area: Basic's"), EncounterRules::LargestRankSize(ByArea, nullptr), SizeOf(ECreatureRank::Basic), 0.001f);

	// Which waves a group joins: every one from its first, or only those up to its last.
	FEncounterGroup SecondOnly = Lieutenant;
	SecondOnly.FirstWave = 2;
	SecondOnly.LastWave = 2;
	TestTrue(TEXT("A group for the second wave only"), !EncounterRules::JoinsWave(SecondOnly, 1) && EncounterRules::JoinsWave(SecondOnly, 2)
		&& !EncounterRules::JoinsWave(SecondOnly, 3));
	TestTrue(TEXT("A plain group joins every wave"), EncounterRules::JoinsWave(ByArea, 1) && EncounterRules::JoinsWave(ByArea, 5));

	// In a level: three spiders, a Restless spiderling at level 7, and two slimes that are always Gravebound, tagged for a
	// mission to count, round a spawner the player comes near.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(2500.0, 0.0, 0.0));
	AEncounterSpawner* Spawner = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, [](AEncounterSpawner& Setup)
	{
		FEncounterGroup Spiderling = EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 1, ECreatureRank::Rare);
		Spiderling.BodyScale = 0.45f;
		Spiderling.HealthScale = 0.2f;
		Spiderling.Level = 7;
		FEncounterGroup Slimes = EncounterTestWorld::MakeGroup(ASlimeCreature::StaticClass(), 2);
		Slimes.RankRoll = EEncounterRankRoll::Chances;
		Slimes.EpicChance = 1.f;
		Setup.SpawnerId = TEXT("TestGate");
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 3), Spiderling, Slimes };
		Setup.CreatureTags = { FName(TEXT("Unpaid_TestGate")) };
		Setup.SpawnRadius = 1200.f;
		Setup.Spacing = 300.f;
		Setup.GiveUpRadius = 3000.f;
		Setup.ActivationRadius = 3000.f;
	});
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(World);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Spawner"), Spawner) || !TestNotNull(TEXT("The level's encounters"), Encounters))
	{
		return false;
	}
	TestTrue(TEXT("Its story asks nothing: it waits for the player"), Spawner->GetState() == EEncounterState::Waiting && Spawner->NumAlive() == 0);
	TestTrue(TEXT("The level finds it by its id"), Encounters->FindSpawner(TEXT("testgate")) == Spawner);

	Spawner->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Out = Spawner->GetAliveCreatures();
	TestTrue(TEXT("The player 25 m off: its first wave is out"), Spawner->GetState() == EEncounterState::Engaged && Spawner->GetWavesStarted() == 1);
	TestEqual(TEXT("Six creatures"), Out.Num(), 6);
	TestEqual(TEXT("...three Basic spiders"), EncounterTestWorld::CountOf(Out, ASpiderCreature::StaticClass(), ECreatureRank::Basic), 3);
	TestEqual(TEXT("...one Restless spider"), EncounterTestWorld::CountOf(Out, ASpiderCreature::StaticClass(), ECreatureRank::Rare), 1);
	TestEqual(TEXT("...two Gravebound slimes"), EncounterTestWorld::CountOf(Out, ASlimeCreature::StaticClass(), ECreatureRank::Epic), 2);
	TestEqual(TEXT("...all of them counted in the level"), Encounters->CountAlive(), 6);
	const FCreatureRankInfo& Restless = UCreatureRankSettings::Get(ECreatureRank::Rare);
	for (const ACreatureBase* Creature : Out)
	{
		TestTrue(TEXT("Tagged for the mission"), Creature->ActorHasTag(TEXT("Unpaid_TestGate")));
		TestFalse(TEXT("Never comes back once killed"), Creature->WillRespawn());
		const FHuntingGround& Turf = Creature->HuntingGround;
		TestTrue(TEXT("Its hunting ground is the spawner's: 30 m round it"), Turf.IsSet() && !Turf.bAroundHome
			&& FMath::IsNearlyEqual(Turf.Radius, 3000.f) && Turf.Center.Equals(Spawner->GetActorLocation(), 1.0));
		TestTrue(TEXT("It stands within the spawn radius"), FVector::Dist2D(Creature->GetActorLocation(), Spawner->GetActorLocation()) <= 1201.0);
		TestTrue(TEXT("Its home is where it appeared"), Creature->GetHome().GetLocation().Equals(Creature->GetActorLocation(), 1.0));
		if (Creature->GetRank() == ECreatureRank::Rare)
		{
			TestNearlyEqual(TEXT("The spiderling: its group's size, times its rank's"), Creature->GetSizeScale(), 0.45f * Restless.Size, 0.001f);
			TestEqual(TEXT("...its group's level, plus its rank's"), Creature->Level, 7 + Restless.LevelOffset);
		}
	}
	TestTrue(TEXT("At least 3 m apart"), EncounterTestWorld::AllApart(EncounterTestWorld::PlacesOf(Out), 299.0));
	Spawner->UpdateEncounter(0.5f);
	TestEqual(TEXT("One wave: nothing more comes"), Spawner->NumAlive(), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterGroundTest, "Looter.Encounters.Ground",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterGroundTest::RunTest(const FString& Parameters)
{
	// Where a group may stand: candidate spots spread over its radius, and of them only ground on the spawner's own level
	// (no more than 4 m up or down) that a creature can stand on, out of the way, spaced apart. A field at height 0, with a
	// shelf 6 m up east of x = 10 m, a pit 7 m down west of x = -10 m, and a boulder at (0, 8 m) nobody stands on top of.
	auto Field = [](const FVector& Spot)
	{
		FEncounterGroundHit Hit;
		Hit.bFound = true;
		const double Height = Spot.X > 1000.0 ? 600.0 : (Spot.X < -1000.0 ? -700.0 : 0.0);
		Hit.Point = FVector(Spot.X, Spot.Y, Height);
		Hit.bStandable = FVector::Dist2D(Spot, FVector(0.0, 800.0, 0.0)) > 200.0;
		return Hit;
	};
	auto Open = [](const FVector&) { return false; };
	const TArray<FVector> Candidates = EncounterRules::CandidateSpots(FVector::ZeroVector, 2000.f, TArray<FVector>(), 30, 200.f, 0.f);
	TestTrue(TEXT("More candidates than wanted"), Candidates.Num() > 30);
	TestFalse(TEXT("...all within the radius"), Candidates.ContainsByPredicate([](const FVector& Spot) { return Spot.Size2D() > 2000.1; }));
	const TArray<FVector> Spots = EncounterRules::ChooseSpots(Candidates, 30, 0.0, 400.f, 200.f, TArray<FVector>(), Field, Open);
	TestTrue(TEXT("Spots on the field"), Spots.Num() > 5);
	TestFalse(TEXT("None on the shelf or in the pit"), Spots.ContainsByPredicate([](const FVector& Spot)
	{
		return FMath::Abs(Spot.X) > 1000.0 || !FMath::IsNearlyZero(Spot.Z);
	}));
	TestFalse(TEXT("None on top of the boulder"), Spots.ContainsByPredicate([](const FVector& Spot)
	{
		return FVector::Dist2D(Spot, FVector(0.0, 800.0, 0.0)) <= 200.0;
	}));
	TestTrue(TEXT("Each at least 2 m from the next"), EncounterTestWorld::AllApart(Spots, 199.9));

	// The step's limit: 3.5 m up is the same level of ground, 4.5 m up or down isn't; no ground at all isn't either.
	const TArray<FVector> OneSpot = { FVector(500.0, 0.0, 0.0) };
	auto RaisedBy = [](double Rise)
	{
		return [Rise](const FVector& Spot)
		{
			FEncounterGroundHit Hit;
			Hit.bFound = true;
			Hit.Point = FVector(Spot.X, Spot.Y, Rise);
			return Hit;
		};
	};
	auto NoGround = [](const FVector&) { return FEncounterGroundHit(); };
	TestEqual(TEXT("3.5 m up: the same level"), EncounterRules::ChooseSpots(OneSpot, 1, 0.0, 400.f, 200.f, TArray<FVector>(), RaisedBy(350.0), Open).Num(), 1);
	TestEqual(TEXT("4.5 m up: another level"), EncounterRules::ChooseSpots(OneSpot, 1, 0.0, 400.f, 200.f, TArray<FVector>(), RaisedBy(450.0), Open).Num(), 0);
	TestEqual(TEXT("4.5 m down: another level"), EncounterRules::ChooseSpots(OneSpot, 1, 0.0, 400.f, 200.f, TArray<FVector>(), RaisedBy(-450.0), Open).Num(), 0);
	TestEqual(TEXT("No ground under it: skipped"), EncounterRules::ChooseSpots(OneSpot, 1, 0.0, 400.f, 200.f, TArray<FVector>(), NoGround, Open).Num(), 0);

	// Blocked spots (a safe zone, off the playable area or its hunting ground, by the player) are skipped, and new spots keep
	// clear of creatures standing there already.
	auto NorthBlocked = [](const FVector& Spot) { return Spot.Y > 0.0; };
	const TArray<FVector> South = EncounterRules::ChooseSpots(Candidates, 30, 0.0, 400.f, 200.f, TArray<FVector>(), Field, NorthBlocked);
	TestTrue(TEXT("Half blocked: every spot in the other half"), South.Num() > 0 && !South.ContainsByPredicate([](const FVector& Spot) { return Spot.Y > 0.0; }));
	const TArray<FVector> Standing = { FVector(0.0, -300.0, 0.0) };
	const TArray<FVector> Around = EncounterRules::ChooseSpots(Candidates, 30, 0.0, 400.f, 200.f, Standing, Field, Open);
	TestFalse(TEXT("None within 2 m of a creature standing there"), Around.ContainsByPredicate([](const FVector& Spot)
	{
		return FVector::Dist2D(Spot, FVector(0.0, -300.0, 0.0)) < 200.0;
	}));

	// Points of its own: each one first, level with its ground, then round them.
	const TArray<FVector> Points = { FVector(400.0, 0.0, 0.0), FVector(-400.0, 300.0, 0.0) };
	const TArray<FVector> AtPoints = EncounterRules::CandidateSpots(FVector(0.0, 0.0, 50.0), 0.f, Points, 5, 200.f, 0.f);
	TestTrue(TEXT("Its points come first, level with its ground"), AtPoints.Num() > 5 && AtPoints[0].Equals(FVector(400.0, 0.0, 50.0), 0.1)
		&& AtPoints[1].Equals(FVector(-400.0, 300.0, 50.0), 0.1));

	// Room for the body: spots with room for the whole of it come first; with too few, the roomiest of the rest fill in (its
	// own point first among equals) rather than none, and never one with no room at all. Three spots in a row, 5 m apart:
	// its point, then one east and one west of it.
	const TArray<FVector> InRow = { FVector(0.0, 0.0, 0.0), FVector(500.0, 0.0, 0.0), FVector(-500.0, 0.0, 0.0) };
	auto RoomsInRow = [](float AtPoint, float East, float West)
	{
		return [AtPoint, East, West](const FVector& Spot)
		{
			FEncounterGroundHit Hit;
			Hit.bFound = true;
			Hit.Point = Spot;
			Hit.Room = Spot.X > 1.0 ? East : (Spot.X < -1.0 ? West : AtPoint);
			return Hit;
		};
	};
	auto ChooseInRow = [&InRow, &Open](int32 Wanted, TFunctionRef<FEncounterGroundHit(const FVector&)> GroundAt)
	{
		return EncounterRules::ChooseSpots(InRow, Wanted, 0.0, 400.f, 200.f, TArray<FVector>(), GroundAt, Open);
	};
	const TArray<FVector> PastTight = ChooseInRow(1, RoomsInRow(0.4f, 1.f, 0.f));
	TestTrue(TEXT("Its point too tight for the whole body: the next spot that fits it"), PastTight.Num() == 1 && PastTight[0].X > 1.0);
	const TArray<FVector> Roomiest = ChooseInRow(1, RoomsInRow(0.4f, 0.7f, 0.f));
	TestTrue(TEXT("None fits it whole: the roomiest"), Roomiest.Num() == 1 && Roomiest[0].X > 1.0);
	const TArray<FVector> Tied = ChooseInRow(1, RoomsInRow(0.7f, 0.7f, 0.7f));
	TestTrue(TEXT("...its own point first among equals"), Tied.Num() == 1 && FMath::Abs(Tied[0].X) < 1.0);
	TestEqual(TEXT("No room anywhere: no spot"), ChooseInRow(1, RoomsInRow(0.f, 0.f, 0.f)).Num(), 0);
	const TArray<FVector> TopUp = ChooseInRow(3, RoomsInRow(1.f, 0.5f, 0.f));
	TestTrue(TEXT("Too few fit it whole: a tighter one fills in after them, never one with no room"), TopUp.Num() == 2
		&& FMath::Abs(TopUp[0].X) < 1.0 && TopUp[1].X > 1.0);

	// The room a spawner's spots need is the largest body its groups bring: each class's capsule at its group's size and the
	// largest rank it can roll. A spawner of spiders and a giant one needs the giant's.
	const float SpiderRadius = GetDefault<ASpiderCreature>()->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	const float BasicSize = UCreatureRankSettings::Get(ECreatureRank::Basic).Size;
	const float EpicSize = UCreatureRankSettings::Get(ECreatureRank::Epic).Size;
	FEncounterGroup Giant = EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 1);
	Giant.BodyScale = 1.8f;
	TestNearlyEqual(TEXT("A spider at 1.8: its capsule 1.8 times as wide"), EncounterRules::LargestBody({ Giant }, nullptr).Radius,
		SpiderRadius * 1.8f * BasicSize, 0.01f);
	FEncounterGroup MaybeGravebound = EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 4);
	MaybeGravebound.RankRoll = EEncounterRankRoll::Chances;
	MaybeGravebound.EpicChance = 0.1f;
	const FEncounterBody Mixed = EncounterRules::LargestBody({ MaybeGravebound, Giant,
		EncounterTestWorld::MakeGroup(ASlimeCreature::StaticClass(), 2) }, nullptr);
	TestNearlyEqual(TEXT("Spiders that may be Gravebound, a giant one and slimes: room for the biggest"), Mixed.Radius,
		SpiderRadius * FMath::Max(1.8f * BasicSize, EpicSize), 0.01f);
	TestNearlyEqual(TEXT("No creature to measure: a man-sized body"), EncounterRules::LargestBody(TArray<FEncounterGroup>(), nullptr).Radius,
		FEncounterBody().Radius, 0.01f);

	// A hunting ground as a polygon (the churchyard's fence): inside, within its margin past the fence, past it; its height band.
	FHuntingGround Yard;
	Yard.Corners = { FVector(0.0, 0.0, 0.0), FVector(2000.0, 0.0, 0.0), FVector(2000.0, 1000.0, 0.0), FVector(0.0, 1000.0, 0.0) };
	Yard.Margin = 200.f;
	Yard.MaxRise = 400.f;
	Yard.bAroundHome = false;
	Yard.Center = FVector(1000.0, 500.0, 0.0);
	const FVector Home = FVector::ZeroVector;
	TestTrue(TEXT("Inside the yard"), Yard.Contains(FVector(1000.0, 500.0, 100.0), Home));
	TestTrue(TEXT("Just over its fence, within the margin"), Yard.Contains(FVector(2150.0, 500.0, 100.0), Home));
	TestFalse(TEXT("Well past the fence"), Yard.Contains(FVector(2500.0, 500.0, 100.0), Home));
	TestFalse(TEXT("Over it, 6 m up"), Yard.Contains(FVector(1000.0, 500.0, 600.0), Home));
	TestFalse(TEXT("The margin is no place to spawn"), Yard.ContainsSpot(FVector(2150.0, 500.0, 0.0), Home));
	FHuntingGround RoundHome;
	RoundHome.Radius = 1000.f;
	TestTrue(TEXT("A radius round its home"), RoundHome.Contains(FVector(900.0, 0.0, 0.0), Home) && !RoundHome.Contains(FVector(1100.0, 0.0, 0.0), Home));
	TestTrue(TEXT("No hunting ground: everywhere"), FHuntingGround().Contains(FVector(1.0e6, 0.0, 0.0), Home));

	// In a level: a safe zone that's on over the north half, and the spawner's own yard east of x = -5 m; every creature
	// appears in the yard's south half, out of the zone.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	EncounterTestWorld::SpawnPlayer(World, FVector(-4000.0, 0.0, 0.0));
	const ASafeGround* North = EncounterTestWorld::SpawnSquareZone(World, FVector(0.0, 3000.0, 0.0), 3000.0, [](ASafeGround&) {});
	AEncounterSpawner* Spawner = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, [](AEncounterSpawner& Setup)
	{
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 6) };
		Setup.SpawnRadius = 1500.f;
		Setup.Spacing = 250.f;
		Setup.GroundCorners = { FVector(-500.0, -1500.0, 0.0), FVector(1500.0, -1500.0, 0.0), FVector(1500.0, 1500.0, 0.0), FVector(-500.0, 1500.0, 0.0) };
		Setup.ActivationRadius = 6000.f;
	});
	if (!TestNotNull(TEXT("Safe zone"), North) || !TestNotNull(TEXT("Spawner"), Spawner))
	{
		return false;
	}
	TestTrue(TEXT("The zone is on"), North->IsActive());
	Spawner->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Out = Spawner->GetAliveCreatures();
	TestEqual(TEXT("All six found room"), Out.Num(), 6);
	for (const ACreatureBase* Creature : Out)
	{
		const FVector Where = Creature->GetActorLocation();
		TestFalse(TEXT("None in the safe zone"), North->Contains(Where));
		TestTrue(TEXT("Each in its yard"), FHuntingGround::IsInsidePolygon(Spawner->GroundCorners, FVector2D(Where.X, Where.Y)));
		TestTrue(TEXT("Each level with the spawner (no ground in a test level)"), FMath::IsNearlyEqual(Creature->GetActorLocation().Z
			- Creature->GetSimpleCollisionHalfHeight(), Spawner->GetActorLocation().Z, 5.0));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterCapsTest, "Looter.Encounters.Caps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterCapsTest::RunTest(const FString& Parameters)
{
	// Room under a cap: what's left of it, never below none; no cap, as many as asked.
	TestEqual(TEXT("12 at most, 9 alive: room for 3"), EncounterRules::Room(9, 12), 3);
	TestEqual(TEXT("At the cap: none"), EncounterRules::Room(12, 12), 0);
	TestEqual(TEXT("Past it: none"), EncounterRules::Room(15, 12), 0);
	TestTrue(TEXT("No cap: room for any number"), EncounterRules::Room(500, 0) > 100000);

	// The area's numbers (Docs/Areas/RansomsRest.md, "Performance plan"): at most 16 creatures within 80 m of the player, and
	// 12 Unpaid at once, by the class's path so the cap holds from the moment the Unpaid exist.
	const UEncounterSettings& Settings = UEncounterSettings::Get();
	TestEqual(TEXT("16 creatures near the player"), Settings.MaxCreaturesNearPlayer, 16);
	TestNearlyEqual(TEXT("...within 80 m"), Settings.NearPlayerRadius, 8000.f, 1.f);
	TestTrue(TEXT("12 Unpaid at once"), Settings.ClassCaps.ContainsByPredicate([](const FEncounterClassCap& Cap)
	{
		return Cap.MaxAlive == 12 && Cap.CreatureClass.ToSoftObjectPath().ToString().EndsWith(TEXT(".UnpaidCreature"));
	}));

	// A kind's cap reaches its children, and the nearest kind's wins.
	UEncounterSettings* Caps = NewObject<UEncounterSettings>(GetTransientPackage(), NAME_None, RF_Transient);
	FEncounterClassCap AnyCreature;
	AnyCreature.CreatureClass = ACreatureBase::StaticClass();
	AnyCreature.MaxAlive = 30;
	FEncounterClassCap Spiders;
	Spiders.CreatureClass = ASpiderCreature::StaticClass();
	Spiders.MaxAlive = 12;
	Caps->ClassCaps = { AnyCreature, Spiders };
	const UClass* Counted = nullptr;
	TestEqual(TEXT("Spiders: their own cap"), Caps->FindClassCap(ASpiderCreature::StaticClass(), &Counted), 12);
	TestTrue(TEXT("...counting spiders"), Counted == ASpiderCreature::StaticClass());
	TestEqual(TEXT("Slimes: their parent's"), Caps->FindClassCap(ASlimeCreature::StaticClass(), &Counted), 30);
	TestTrue(TEXT("...counting every creature"), Counted == ACreatureBase::StaticClass());
	Caps->ClassCaps.Reset();
	TestEqual(TEXT("No caps: none"), Caps->FindClassCap(ASpiderCreature::StaticClass(), &Counted), 0);

	// In a level, the player in the middle: every spawner keeps to the caps, and what a cap holds back comes once there's room.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(World);
	const ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("The level's encounters"), Encounters) || !TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}
	const FVector PlayerSpot = Player->GetActorLocation();

	// A kind's cap set by its group: five spiders asked for, three at most alive in the level.
	AEncounterSpawner* Kinds = EncounterTestWorld::SpawnSpawner(World, FVector(3000.0, 0.0, 0.0), [](AEncounterSpawner& Setup)
	{
		FEncounterGroup Five = EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 5);
		Five.MaxAliveOfClass = 3;
		Setup.Groups = { Five };
		Setup.SpawnRadius = 1200.f;
		Setup.ActivationRadius = 5000.f;
	});
	// Its own cap: six slimes, two at a time.
	AEncounterSpawner* Pairs = EncounterTestWorld::SpawnSpawner(World, FVector(-3000.0, 0.0, 0.0), [](AEncounterSpawner& Setup)
	{
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASlimeCreature::StaticClass(), 6) };
		Setup.MaxAlive = 2;
		Setup.SpawnRadius = 1200.f;
		Setup.ActivationRadius = 5000.f;
	});
	if (!TestNotNull(TEXT("Spawners"), Kinds) || !TestNotNull(TEXT("Spawners"), Pairs))
	{
		return false;
	}
	Kinds->UpdateEncounter(0.5f);
	TestTrue(TEXT("A kind's cap: three spiders out, two waiting their turn"), Kinds->NumAlive() == 3 && Kinds->NumOwed() == 2);
	TestEqual(TEXT("...three spiders in the level"), Encounters->CountAlive(ASpiderCreature::StaticClass()), 3);
	const TArray<ACreatureBase*> SpidersOut = Kinds->GetAliveCreatures();
	if (!TestTrue(TEXT("Spiders out to kill"), SpidersOut.Num() > 0))
	{
		return false;
	}
	BossTestWorld::Kill(SpidersOut[0]);
	Kinds->UpdateEncounter(0.5f);
	TestTrue(TEXT("One killed: the next comes, still three"), Kinds->NumAlive() == 3 && Kinds->NumOwed() == 1
		&& Encounters->CountAlive(ASpiderCreature::StaticClass()) == 3);
	Pairs->UpdateEncounter(0.5f);
	TestTrue(TEXT("Its own cap: two slimes out, four waiting"), Pairs->NumAlive() == 2 && Pairs->NumOwed() == 4);

	// 16 near the player: a crowd fills the level to two short of it, and a wave of five brings only two.
	const int32 NearCap = Settings.MaxCreaturesNearPlayer;
	TArray<ACreatureBase*> Crowd;
	for (int32 Index = 0; Index < 40 && Encounters->CountAliveNear(PlayerSpot, Settings.NearPlayerRadius) < NearCap - 2; ++Index)
	{
		const FVector Spot(-1500.0 + 300.0 * (Index % 10), 4000.0 + 300.0 * (Index / 10), 0.0);
		if (ACreatureBase* Extra = EncounterTestWorld::SpawnCreature(World, ASlimeCreature::StaticClass(), Spot))
		{
			Encounters->TrackCreature(Extra);
			Crowd.Add(Extra);
		}
	}
	TestEqual(TEXT("Two short of the cap near the player"), Encounters->CountAliveNear(PlayerSpot, Settings.NearPlayerRadius), NearCap - 2);
	AEncounterSpawner* Crowded = EncounterTestWorld::SpawnSpawner(World, FVector(0.0, -3000.0, 0.0), [](AEncounterSpawner& Setup)
	{
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASlimeCreature::StaticClass(), 5) };
		Setup.SpawnRadius = 1200.f;
		Setup.ActivationRadius = 5000.f;
	});
	if (!TestNotNull(TEXT("Spawner"), Crowded) || !TestTrue(TEXT("A crowd of three or more"), Crowd.Num() >= 3))
	{
		return false;
	}
	Crowded->UpdateEncounter(0.5f);
	TestTrue(TEXT("A wave of five near the crowd: two come, three wait"), Crowded->NumAlive() == 2 && Crowded->NumOwed() == 3);
	TestEqual(TEXT("...16 round the player, no more"), Encounters->CountAliveNear(PlayerSpot, Settings.NearPlayerRadius), NearCap);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		BossTestWorld::Kill(Crowd[Index]);
	}
	Crowded->UpdateEncounter(0.5f);
	TestTrue(TEXT("Three of the crowd killed: the three waiting come"), Crowded->NumAlive() == 5 && Crowded->NumOwed() == 0);
	TestEqual(TEXT("...16 again"), Encounters->CountAliveNear(PlayerSpot, Settings.NearPlayerRadius), NearCap);

	// Far from the player the near cap holds nothing back: two slimes 200 m off come, though 16 stand near the player.
	AEncounterSpawner* Far = EncounterTestWorld::SpawnSpawner(World, FVector(20000.0, 0.0, 0.0), [](AEncounterSpawner& Setup)
	{
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASlimeCreature::StaticClass(), 2) };
		Setup.ActivationRadius = 25000.f;
		Setup.DespawnRadius = 0.f;
	});
	if (TestNotNull(TEXT("Spawner"), Far))
	{
		Far->UpdateEncounter(0.5f);
		TestEqual(TEXT("Far off, the near cap leaves them be"), Far->NumAlive(), 2);
	}
	return true;
}

#endif
