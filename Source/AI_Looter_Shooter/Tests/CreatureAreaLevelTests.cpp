#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Find.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Bosses/AbelKeeper.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/ForemanCreature.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Progression/LevelRules.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Tests/AreaLevelsLayout.h"
#include "Tests/AutomationCommon.h"
#include "Tests/EncounterLayoutReader.h"
#include "Engine/World.h"
#include "Math/RandomStream.h"

// Docs/Progression.md, "Creature levels" (the user's call, 2026-10-12): "Monsters shouldn't scale with the player, creature
// levels are determined based on areas", and Abel, the second boss, above the first. Ransom's Rest's levels as its layout
// gives them (gameplay.levels: the zones rising along the story, the bosses at set levels), the spots of every fight and
// encounter in them, and creatures given their levels the way their area gives them.

namespace
{
	/** A place in the layout (a story fight, a layout zone, a camp, patrol or ambush) and the levels planned there. */
	struct FPlannedLevels
	{
		const TCHAR* Place;
		int32 Lowest;
		int32 Highest;
		const TCHAR* What;
	};

	/** A creature of Class standing at Spot in a test level, its level given as Area gives it (none chosen), begun. */
	ACreatureBase* PlaceCreature(UWorld& World, TSubclassOf<ACreatureBase> Class, const FVector& Spot, ECreatureRank Rank,
		const UAreaDefinition& Area, FRandomStream& Random, int32 ChosenLevel = 0)
	{
		ACreatureBase* Creature = World.SpawnActor<ACreatureBase>(Class, Spot + FVector(0.0, 0.0, 120.0), FRotator::ZeroRotator);
		if (!Creature)
		{
			return nullptr;
		}
		// As UAreaRulesSubsystem::RollLevelFor gives it when it begins play in a game (a test level has no area rules).
		Creature->Level = UAreaRulesSubsystem::RollLevelIn(&Area, Spot, Class.Get(), ChosenLevel > 0 ? ChosenLevel : Creature->Level, ChosenLevel > 0,
			Random);
		Creature->StartingRank = Rank;
		Creature->DispatchBeginPlay();
		return Creature;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureAreaLevelsTest, "Looter.Creatures.AreaLevels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureAreaLevelsTest::RunTest(const FString& Parameters)
{
	EncounterLayout::FAreaLayout Layout;
	if (!EncounterLayout::ReadLayout(*this, Layout))
	{
		return false;
	}
	const UAreaDefinition* Rest = AreaLevelsLayout::ReadArea(*this, Layout);
	if (!Rest)
	{
		return false;
	}
	// The band the station board shows: 1 to 7 since the mountain (2026-10-13), Abel at 7 with his rank (the Foreman 6 since
	// 2026-10-27); no zone above 6 before ranks.
	TestTrue(TEXT("Ransom's Rest's band is levels 1 to 7"), Rest->MinLevel == 1 && Rest->GetBandTop() == 7);
	bool bZonesToSix = true;
	for (const FAreaLevelZone& Zone : Rest->LevelZones)
	{
		bZonesToSix &= Zone.GetTop() <= 6;
	}
	TestTrue(TEXT("...and no zone's creatures above 6 before their ranks"), bZonesToSix);

	// The story's fights, where they stand: 1-2 near the start, 2-4 round the town and the fields, 4-6 from the Sink on.
	const FPlannedLevels Story[] = {
		{ TEXT("BluffNest"), 1, 2, TEXT("Main 2's spider nest on the bluff") },
		{ TEXT("TownGate"), 2, 3, TEXT("Main 3's Unpaid at the town gate") },
		{ TEXT("ChapelYard"), 3, 4, TEXT("Main 4's chapel yard") },
		{ TEXT("WhitlockHands"), 3, 4, TEXT("Side 2's hired hands") },
		{ TEXT("BootHill"), 3, 4, TEXT("boot hill's roaming Unpaid") },
		{ TEXT("NorthRoad"), 3, 4, TEXT("the north road's roaming Unpaid") },
		{ TEXT("sink"), 4, 5, TEXT("Main 5's Sink: its floor and the egg sacs") },
		{ TEXT("Abel"), 5, 6, TEXT("Main 6's deck: Abel's adds") } };
	TMap<FString, TPair<int32, int32>> StoryLevels;
	for (const FPlannedLevels& Planned : Story)
	{
		FVector Spot = FVector::ZeroVector;
		if (!TestTrue(FString::Printf(TEXT("%s is in the layout"), Planned.Place), AreaLevelsLayout::FindPlace(Layout, Planned.Place, Spot)))
		{
			continue;
		}
		int32 Lowest = 0;
		int32 Highest = 0;
		Rest->GetLevelsAt(Spot, Lowest, Highest);
		StoryLevels.Add(Planned.Place, TPair<int32, int32>(Lowest, Highest));
		TestTrue(FString::Printf(TEXT("%s: levels %d-%d (%d-%d)"), Planned.What, Planned.Lowest, Planned.Highest, Lowest, Highest),
			Lowest == Planned.Lowest && Highest == Planned.Highest);
	}
	// The main path's fights in their order never come at lower levels than the one before.
	const TCHAR* MainPath[] = { TEXT("BluffNest"), TEXT("TownGate"), TEXT("ChapelYard"), TEXT("sink"), TEXT("Abel") };
	for (int32 Index = 1; Index < static_cast<int32>(UE_ARRAY_COUNT(MainPath)); ++Index)
	{
		const TPair<int32, int32>* Before = StoryLevels.Find(MainPath[Index - 1]);
		const TPair<int32, int32>* After = StoryLevels.Find(MainPath[Index]);
		TestTrue(FString::Printf(TEXT("%s's levels are no lower than %s's"), MainPath[Index], MainPath[Index - 1]),
			Before && After && After->Key >= Before->Key && After->Value >= Before->Value);
	}

	// The encounters between the fights: each one in one zone (every spot and route point at the same levels as its middle),
	// at the levels planned for it. A new encounter needs its levels planned here and in the layout's zones.
	const FPlannedLevels Between[] = {
		{ TEXT("SheepFold"), 4, 5, TEXT("the sheep fold's camp, on the way to the deck") },
		{ TEXT("OrchardEdge"), 2, 3, TEXT("the orchard's edge, by the farm") },
		{ TEXT("MillCreek"), 3, 4, TEXT("Mill Creek's slimes, in the fields") },
		{ TEXT("WestRoad"), 4, 5, TEXT("the west road's walkers") },
		{ TEXT("QuarryTrack"), 5, 6, TEXT("the quarry track's spiders") },
		{ TEXT("Churchyard"), 3, 4, TEXT("the churchyard's risen dead") },
		{ TEXT("Webwood"), 5, 6, TEXT("the Webwood's drop") },
		// The new side missions' and the mountain's (2026-10-13), should the layout list them among its encounters: the
		// depot's waiting room and Tilly's cellar in the valley (the cellar's spawner keeps its own 3-4), the mountain 5-6.
		{ TEXT("DepotWaiting"), 2, 3, TEXT("the depot's waiting room (The Old Fare)") },
		{ TEXT("TillyCellar"), 2, 3, TEXT("Tilly's cellar's ground (Knocking Below; its spawner keeps 3-4)") },
		{ TEXT("QuarryCrew"), 5, 6, TEXT("the lift crew on the quarry floor (Dead Lift)") },
		{ TEXT("AbeHaskett"), 5, 6, TEXT("Abe Haskett with the clamp pin (Dead Lift)") },
		{ TEXT("RaisingSteam"), 5, 6, TEXT("the quarry held while she raises steam (Dead Lift)") },
		{ TEXT("CrestPatrol"), 5, 6, TEXT("the crest path's spiders") },
		{ TEXT("CalderPost"), 5, 6, TEXT("Calder's Post's yard (The Last Watch)") },
		{ TEXT("Watch"), 5, 6, TEXT("the watch at Calder's Post (The Last Watch)") },
		{ TEXT("DinnerPail"), 5, 6, TEXT("the Dinner Pail camp on the Bench") },
		{ TEXT("SpoilAmbush"), 5, 6, TEXT("the spoil heap's ambush at the Diggings") },
		{ TEXT("MineUpper"), 5, 6, TEXT("the Providence's upper gallery (Back Pay)") },
		{ TEXT("MineLower"), 5, 6, TEXT("the Providence's lower drift (Back Pay)") },
		{ TEXT("Foreman"), 5, 6, TEXT("the stope's shift, round the Foreman (Back Pay)") } };
	for (const EncounterLayout::FAreaEncounter& Encounter : Layout.Encounters)
	{
		const FPlannedLevels* Planned = Algo::FindByPredicate(Between, [&Encounter](const FPlannedLevels& Each) { return Encounter.Id == Each.Place; });
		if (!TestNotNull(*FString::Printf(TEXT("%s's levels are planned (here and in the layout's gameplay.levels zones)"), *Encounter.Id), Planned))
		{
			continue;
		}
		const FVector Middle = AreaLevelsLayout::EncounterSpot(Encounter);
		const FAreaLevelZone* Zone = Rest->FindLevelZone(Middle);
		int32 Lowest = 0;
		int32 Highest = 0;
		Rest->GetLevelsAt(Middle, Lowest, Highest);
		TestTrue(FString::Printf(TEXT("%s: levels %d-%d (%d-%d, zone %s)"), Planned->What, Planned->Lowest, Planned->Highest, Lowest, Highest,
			Zone ? *Zone->Id.ToString() : TEXT("none")), Lowest == Planned->Lowest && Highest == Planned->Highest);
		TArray<FVector2D> Points = Encounter.Spots;
		Points.Append(Encounter.Route);
		bool bOneZone = true;
		for (const FVector2D& Point : Points)
		{
			bOneZone &= Rest->FindLevelZone(FVector(Point.X, Point.Y, 0.0)) == Zone;
		}
		TestTrue(FString::Printf(TEXT("...every one of %s's spots in that zone"), *Encounter.Id), bOneZone);
	}

	// The bosses, at set levels wherever they stand, their ranks' levels on top: the Gravemother (Side 3, after Main 5) 4 and
	// Soulfed's two, level 6, over the Sink's 4-5; Abel (Main 6, the story's last boss) 6 and his Boss rank's
	// one, level 7, over the deck's adds, over her, and the area's highest.
	const int32 BossOffset = UCreatureRankSettings::Get(ECreatureRank::Boss).LevelOffset;
	const int32 SoulfedOffset = UCreatureRankSettings::Get(ECreatureRank::Legendary).LevelOffset;
	const int32 RestlessOffset = UCreatureRankSettings::Get(ECreatureRank::Rare).LevelOffset;
	const int32 Abel = Rest->FindKindLevel(AAbelKeeper::StaticClass()) + BossOffset;
	const int32 Gravemother = Rest->FindKindLevel(AGravemotherCreature::StaticClass()) + SoulfedOffset;
	TestEqual(TEXT("Abel is level 7"), Abel, 7);
	TestEqual(TEXT("The Gravemother is level 6"), Gravemother, 6);
	TestTrue(TEXT("...the Gravemother over the Sink's creatures"), StoryLevels.Contains(TEXT("sink")) && Gravemother > StoryLevels[TEXT("sink")].Value);
	TestTrue(TEXT("...Abel over his adds"), StoryLevels.Contains(TEXT("Abel")) && Abel > StoryLevels[TEXT("Abel")].Value);
	TestTrue(TEXT("...and over the Gravemother: the second boss above the first"), Abel > Gravemother);
	// The Foreman (Back Pay, the Providence's Legendary monster): 4 and Soulfed's two, level 6, over the mountain's 5-6. Was
	// level with Abel (2026-10-13); since the user's 2026-10-27 note 32 he's the story's second boss and Abel its last, so a
	// level under him.
	const int32 ForemanLevel = Rest->FindKindLevel(AForemanCreature::StaticClass()) + SoulfedOffset;
	TestEqual(TEXT("The Foreman is level 6"), ForemanLevel, 6);
	TestTrue(TEXT("...under Abel, the last boss"), ForemanLevel < Abel);

	// The mountain's fights where the design puts them (Docs/Areas/RansomsRest_Expansion.md section 6, layout cm): all 5-6
	// (the user's call), the quarry bowl, the crest, Calder's Post, the Bench, the Diggings and the Providence.
	struct FMountainSpot
	{
		const TCHAR* What;
		double X;
		double Y;
	};
	const FMountainSpot Mountain[] = {
		{ TEXT("the lift crew on the quarry floor"), 10800.0, 2000.0 },
		{ TEXT("the crest path"), 14200.0, 1500.0 },
		{ TEXT("Calder's Post's yard"), 16800.0, -6000.0 },
		{ TEXT("the Dinner Pail camp on the Bench"), 17000.0, -500.0 },
		{ TEXT("the Diggings' spoil heap"), 20500.0, 8000.0 },
		{ TEXT("the Providence's upper gallery"), 26800.0, 7400.0 },
		{ TEXT("the Providence's stope"), 33000.0, 7400.0 } };
	for (const FMountainSpot& Each : Mountain)
	{
		int32 Lowest = 0;
		int32 Highest = 0;
		Rest->GetLevelsAt(FVector(Each.X, Each.Y, 0.0), Lowest, Highest);
		TestTrue(FString::Printf(TEXT("%s: levels 5-6 (%d-%d)"), Each.What, Lowest, Highest), Lowest == 5 && Highest == 6);
	}
	bool bAbelHighest = true;
	for (const FAreaLevelZone& Zone : Rest->LevelZones)
	{
		bAbelHighest &= Abel >= Zone.GetTop() + RestlessOffset;
	}
	TestTrue(TEXT("...and no Restless creature anywhere is above Abel"), bAbelHighest);
	TestEqual(TEXT("The spiderlings she calls aren't Gravemothers: their spot's levels, or hers before her rank"),
		Rest->FindKindLevel(ASpiderCreature::StaticClass()), 0);
	FRandomStream Random(20261012);
	bool bBossesSteady = true;
	for (const FVector& Anywhere : { FVector::ZeroVector, FVector(-8000.0, -9300.0, 0.0), FVector(4600.0, 6500.0, 0.0) })
	{
		bBossesSteady &= UAreaRulesSubsystem::RollLevelIn(Rest, Anywhere, AAbelKeeper::StaticClass(), 1, false, Random) == Abel - BossOffset
			&& UAreaRulesSubsystem::RollLevelIn(Rest, Anywhere, AGravemotherCreature::StaticClass(), 1, false, Random) == Gravemother - SoulfedOffset;
	}
	TestTrue(TEXT("The bosses' levels are the same wherever they stand, never rolled"), bBossesSteady);

	// Never the player's: nothing the levels come from knows the player. A player back on the bluff at the story's end, or at
	// the level cap, still meets its spiders at 1-2; only the kill's experience changes, less from creatures below them.
	FVector Bluff = FVector::ZeroVector;
	if (AreaLevelsLayout::FindPlace(Layout, TEXT("BluffNest"), Bluff))
	{
		bool bStillLow = true;
		for (int32 Roll = 0; Roll < 1000; ++Roll)
		{
			const int32 Level = UAreaRulesSubsystem::RollLevelIn(Rest, Bluff, ASpiderCreature::StaticClass(), 1, false, Random);
			bStillLow &= Level >= 1 && Level <= 2;
		}
		TestTrue(TEXT("The bluff's spiders are 1-2 whatever the player's level"), bStillLow);
		const FLevelRules Rules = UPlayerProgressionSubsystem::GetLevelRules();
		TestTrue(FString::Printf(TEXT("...worth %lld XP to a level 1 player, %lld to a level 7 one"), Rules.KillXP(10, 2, 1), Rules.KillXP(10, 2, 7)),
			Rules.KillXP(10, 2, 7) < Rules.KillXP(10, 2, 1));
	}

	// Creatures in a test level given their levels as their area gives them, then begun: their level is their spot's, a
	// Restless one's a level higher, their health grown for it; one placed at a level of its own keeps it.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const FLevelRules Rules = UPlayerProgressionSubsystem::GetLevelRules();
	const float SpiderHealth = GetDefault<ASpiderCreature>()->FindComponentByClass<UHealthComponent>()->MaxHealth;
	ACreatureBase* Spider = PlaceCreature(*World, ASpiderCreature::StaticClass(), Bluff, ECreatureRank::Basic, *Rest, Random);
	FVector Chapel = FVector::ZeroVector;
	AreaLevelsLayout::FindPlace(Layout, TEXT("ChapelYard"), Chapel);
	ACreatureBase* Restless = PlaceCreature(*World, AUnpaidCreature::StaticClass(), Chapel, ECreatureRank::Rare, *Rest, Random);
	ACreatureBase* Chosen = PlaceCreature(*World, ASpiderCreature::StaticClass(), Bluff + FVector(600.0, 0.0, 0.0), ECreatureRank::Basic, *Rest, Random, 5);
	if (!TestTrue(TEXT("The creatures stand in the test level"), Spider && Restless && Chosen))
	{
		return false;
	}
	TestTrue(FString::Printf(TEXT("A spider on the bluff is level 1-2 (%d)"), Spider->Level), Spider->Level >= 1 && Spider->Level <= 2);
	const UHealthComponent* Health = Spider->FindComponentByClass<UHealthComponent>();
	TestTrue(TEXT("...with a spider's health grown for its level"), Health
		&& FMath::IsNearlyEqual(Health->MaxHealth, SpiderHealth * Rules.EnemyScale(Spider->Level), 0.05f));
	TestTrue(FString::Printf(TEXT("A Restless Unpaid in the chapel yard is its 3-4 and one more (%d)"), Restless->Level),
		Restless->Level >= 3 + RestlessOffset && Restless->Level <= 4 + RestlessOffset);
	TestEqual(TEXT("A spider placed at level 5 on the bluff keeps it"), Chosen->Level, 5);
	return true;
}

#endif
