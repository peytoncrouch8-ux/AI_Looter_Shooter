#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureRankSettings.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRewards.h"
#include "Progression/LevelRules.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Progression/XPCurve.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Tests/AreaLevelsLayout.h"
#include "Tests/EncounterLayoutReader.h"
#include "Tests/PacingPaths.h"
#include "UObject/Package.h"
#include "UObject/TopLevelAssetPath.h"

// Ransom's Rest's pacing (Docs/Progression.md; the user's levels of 2026-10-13): the story alone (every main mission turned
// in, the fights on its way, the roaming dead once) ends about level 5.5, and everything there (every side mission too, the
// mountain, the Gravemother, every optional camp once) about 6.8. Experience comes only from kills and turned-in missions.
// This plays both paths (PacingPaths.h) on paper from the same data the game uses: the mission assets' rewards, the
// creatures' own XPReward, the rank settings, the creatures' levels where each fight stands (layout.json gameplay.levels: its
// zones and the set levels, never the player's) and Ransom's Rest's promotion odds, the progression settings' curve and kill
// rules. The kills on the way are the documented budget and the encounters between the fights, read from the layout
// (gameplay.encounters: camps, patrols, ambushes, each counted by its budget). A fight the layout doesn't name yet (the
// mountain's, placed by their own scripts) stands where the design puts it. An edit that moves either end out of its band
// fails here.

using namespace PacingPaths;

namespace
{
	/** One encounter between the fights, as the layout counts it (layout.json gameplay.encounters). */
	struct FLayoutEncounter
	{
		FString Id;
		/** main: on the story's path; roaming: with the few roaming kills; optional: an explorer's. */
		FString Budget;
		/** The stop (a mission's id) it's met on the way to, before that stop's own fights. */
		FName CountedBefore;
		TArray<FPathKills> Kills;
	};

	/** Every camp, patrol and ambush in the layout, with its kills; false (with the test's errors) when it can't be read. */
	bool ReadLayoutEncounters(FAutomationTestBase& Test, TArray<FLayoutEncounter>& Out)
	{
		FString Text;
		const FString LayoutFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Art/Levels/RansomsRest/layout.json"));
		TSharedPtr<FJsonObject> Layout;
		const TSharedPtr<FJsonObject>* Gameplay = nullptr;
		const TSharedPtr<FJsonObject>* Encounters = nullptr;
		if (!Test.TestTrue(TEXT("layout.json reads"), FFileHelper::LoadFileToString(Text, *LayoutFile))
			|| !Test.TestTrue(TEXT("...with gameplay.encounters"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Layout)
				&& Layout.IsValid() && Layout->TryGetObjectField(TEXT("gameplay"), Gameplay) && Gameplay
				&& (*Gameplay)->TryGetObjectField(TEXT("encounters"), Encounters) && Encounters))
		{
			return false;
		}
		for (const TCHAR* Section : { TEXT("camps"), TEXT("patrols"), TEXT("ambushes") })
		{
			const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
			if (!(*Encounters)->TryGetArrayField(Section, Entries))
			{
				continue;
			}
			for (const TSharedPtr<FJsonValue>& Value : *Entries)
			{
				const TSharedPtr<FJsonObject> Entry = Value->AsObject();
				if (!Entry.IsValid())
				{
					continue;
				}
				FLayoutEncounter& Made = Out.AddDefaulted_GetRef();
				Made.Id = Entry->GetStringField(TEXT("id"));
				Made.Budget = Entry->GetStringField(TEXT("budget"));
				Made.CountedBefore = FName(*Entry->GetStringField(TEXT("countedBefore")));
				const TArray<TSharedPtr<FJsonValue>>* Groups = nullptr;
				if (!Entry->TryGetArrayField(TEXT("groups"), Groups))
				{
					continue;
				}
				for (const TSharedPtr<FJsonValue>& GroupValue : *Groups)
				{
					const TSharedPtr<FJsonObject> Group = GroupValue->AsObject();
					const FString ClassName = Group->GetStringField(TEXT("class"));
					const UClass* Class = FindObject<UClass>(FTopLevelAssetPath(TEXT("/Script/AI_Looter_Shooter"), FName(*ClassName)));
					if (!Test.TestTrue(FString::Printf(TEXT("%s's %s is a creature class"), *Made.Id, *ClassName),
						Class && Class->IsChildOf(ACreatureBase::StaticClass())))
					{
						continue;
					}
					const FString Rank = Group->GetStringField(TEXT("rank"));
					FPathKills& Kills = Made.Kills.AddDefaulted_GetRef();
					Kills.Creature = Class->GetDefaultObject<ACreatureBase>();
					Kills.Rank = Rank == TEXT("Rare") ? EPathRank::Restless : Rank == TEXT("Area") ? EPathRank::AreaRolled : EPathRank::Basic;
					Kills.Count = static_cast<int32>(Group->GetNumberField(TEXT("count")));
					Kills.Where = Made.Id;
				}
			}
		}
		return true;
	}

	/**
	 * Finds where each kill stands (FPathKills::Spot): its camp's, patrol's or ambush's middle, a story fight's ground or a
	 * layout zone, by the name in Where; else the design's spot when it has one (a fight its own script places, not named in
	 * the layout yet). False (with the test's errors) when it has none.
	 */
	bool PlaceKills(FAutomationTestBase& Test, const EncounterLayout::FAreaLayout& Layout, TArray<FPathKills>& Kills)
	{
		bool bAllPlaced = true;
		for (FPathKills& Each : Kills)
		{
			const EncounterLayout::FAreaEncounter* Encounter = Layout.Encounters.FindByPredicate(
				[&Each](const EncounterLayout::FAreaEncounter& Found) { return Found.Id == Each.Where; });
			if (Encounter)
			{
				Each.Spot = AreaLevelsLayout::EncounterSpot(*Encounter);
			}
			else if (!AreaLevelsLayout::FindPlace(Layout, Each.Where, Each.Spot))
			{
				if (Each.bPlanned)
				{
					Each.Spot = FVector(Each.Planned.X, Each.Planned.Y, 0.0);
				}
				else
				{
					bAllPlaced &= Test.TestTrue(FString::Printf(TEXT("%s is a place in the layout"), *Each.Where), false);
				}
			}
		}
		return bAllPlaced;
	}

	/** Whether Id is one of the path's own fights (the layout's entry of the same id isn't counted twice). */
	bool IsPathFight(const TArray<FPathStop>& Path, const FString& Id)
	{
		return Path.ContainsByPredicate([&Id](const FPathStop& Stop)
		{
			return Stop.Kills.ContainsByPredicate([&Id](const FPathKills& Kills) { return Kills.Where == Id; });
		});
	}

	/** The path with the layout's encounters of these budgets added, each before the fights of the stop it's counted before. */
	TArray<FPathStop> WithEncounters(const TArray<FPathStop>& Path, const TArray<FLayoutEncounter>& Encounters, const TArray<FString>& Budgets)
	{
		TArray<FPathStop> Made = Path;
		for (FPathStop& Stop : Made)
		{
			TArray<FPathKills> OnTheWay;
			for (const FLayoutEncounter& Each : Encounters)
			{
				if (!Budgets.Contains(Each.Budget) || IsPathFight(Path, Each.Id))
				{
					continue;
				}
				const bool bRoaming = Each.Budget == TEXT("roaming");
				if ((bRoaming && Stop.Label == RoamingLabel) || (!bRoaming && !Stop.MissionId.IsNone() && Stop.MissionId == Each.CountedBefore))
				{
					OnTheWay.Append(Each.Kills);
				}
			}
			OnTheWay.Append(Stop.Kills);
			Stop.Kills = MoveTemp(OnTheWay);
		}
		return Made;
	}

	/** The levels a kill's creature is rolled between: kept by its spawner, its kind's set level, else its spot's zone's. */
	void LevelsOf(const FPathKills& Kills, const UAreaDefinition& Area, int32& OutLowest, int32& OutHighest)
	{
		if (Kills.KeptLowest > 0)
		{
			OutLowest = Kills.KeptLowest;
			OutHighest = FMath::Max(Kills.KeptHighest, Kills.KeptLowest);
		}
		else if (const int32 Set = Area.FindKindLevel(Kills.Creature->GetClass()))
		{
			OutLowest = OutHighest = Set;
		}
		else
		{
			Area.GetLevelsAt(Kills.Spot, OutLowest, OutHighest);
		}
	}

	/**
	 * A kill's experience on average for a player at PlayerLevel, over its rank's odds and the levels where it stands (each
	 * as likely): the player's level only takes its share off a kill below it.
	 */
	double ExpectedKillXP(const FPathKills& Kills, int32 PlayerLevel, const UAreaDefinition& Area, const FLevelRules& Rules)
	{
		const ACreatureBase& Creature = *Kills.Creature;
		TArray<TPair<ECreatureRank, double>, TInlineAllocator<LooterRanks::NumTiers>> Odds;
		switch (Kills.Rank)
		{
		case EPathRank::Basic:
			Odds.Add({ ECreatureRank::Basic, 1.0 });
			break;
		case EPathRank::Restless:
			Odds.Add({ ECreatureRank::Rare, 1.0 });
			break;
		case EPathRank::Own:
			Odds.Add({ Creature.StartingRank, 1.0 });
			break;
		case EPathRank::AreaRolled:
		{
			// As the area rolls a promotion: the rarest tier first, down to Restless (UAreaDefinition::PickPromotion).
			double Left = 1.0;
			for (int32 Tier = LooterRanks::NumTiers - 1; Tier >= 1; --Tier)
			{
				const double Chance = FMath::Clamp(static_cast<double>(Area.PromotionChance(LooterRanks::Tiers[Tier])), 0.0, Left);
				Odds.Add({ LooterRanks::Tiers[Tier], Chance });
				Left -= Chance;
			}
			Odds.Add({ ECreatureRank::Basic, Left });
			break;
		}
		}
		int32 Lowest = 1;
		int32 Highest = 1;
		LevelsOf(Kills, Area, Lowest, Highest);
		double XP = 0.0;
		for (const TPair<ECreatureRank, double>& Each : Odds)
		{
			const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Each.Key);
			const int32 RankedXP = FMath::Max(0, FMath::RoundToInt32(Creature.XPReward * Info.XPMultiplier));
			double Sum = 0.0;
			for (int32 OwnLevel = Lowest; OwnLevel <= Highest; ++OwnLevel)
			{
				Sum += static_cast<double>(Rules.KillXP(RankedXP, OwnLevel + Info.LevelOffset, PlayerLevel));
			}
			XP += Each.Value * Sum / (Highest - Lowest + 1);
		}
		return XP;
	}

	/** The level from experience earned in all, with how far into it (6.5 is half way through level 6). */
	double LevelOf(const FXPCurve& Curve, double TotalXP)
	{
		int32 Level = 1;
		while (!Curve.IsMaxLevel(Level) && static_cast<double>(Curve.TotalXPToReach(Level + 1)) <= TotalXP)
		{
			++Level;
		}
		const int64 Needed = Curve.XPToNextLevel(Level);
		return Level + (Needed > 0 ? (TotalXP - static_cast<double>(Curve.TotalXPToReach(Level))) / static_cast<double>(Needed) : 0.0);
	}

	const UMissionDefinition* FindMission(const TArray<UMissionDefinition*>& Missions, FName Id)
	{
		UMissionDefinition* const* Found = Missions.FindByPredicate([Id](const UMissionDefinition* Mission) { return Mission && Mission->GetMissionId() == Id; });
		return Found ? *Found : nullptr;
	}

	/** What a path comes to. */
	struct FPathResult
	{
		double Total = 0.0;
		double FromKills = 0.0;
		double FromMissions = 0.0;
		/** The total less the kills at stops with no mission (the roaming ones). */
		double WithoutRoaming = 0.0;
		int32 Kills = 0;
		bool bValid = true;
	};

	/** Plays a path on paper: kills one at a time at the level the ones before left the player on, then each turn-in. */
	FPathResult PlayPath(FAutomationTestBase& Test, const TArray<FPathStop>& Path, const TArray<UMissionDefinition*>& Missions,
		const UAreaDefinition& Area, const FXPCurve& Curve, const FLevelRules& Rules, bool bReport)
	{
		FPathResult Result;
		for (const FPathStop& Stop : Path)
		{
			double StopKillXP = 0.0;
			int32 StopKills = 0;
			for (const FPathKills& Group : Stop.Kills)
			{
				if (!Test.TestNotNull(*FString::Printf(TEXT("%s: its creatures' class"), *Stop.Label), Group.Creature))
				{
					Result.bValid = false;
					return Result;
				}
				for (int32 Each = 0; Each < Group.Count; ++Each)
				{
					const double XP = ExpectedKillXP(Group, FMath::FloorToInt32(LevelOf(Curve, Result.Total)), Area, Rules);
					StopKillXP += XP;
					Result.Total += XP;
					++StopKills;
				}
			}
			if (Stop.MissionId.IsNone())
			{
				Result.WithoutRoaming -= StopKillXP;
			}

			// Turned in: the mission's experience at the level reached (a fixed amount each).
			double Reward = 0.0;
			if (!Stop.MissionId.IsNone())
			{
				const UMissionDefinition* Mission = FindMission(Missions, Stop.MissionId);
				if (Mission)
				{
					Reward = static_cast<double>(MissionRewards::ExperienceOf(Mission->Rewards, FMath::FloorToInt32(LevelOf(Curve, Result.Total)), Curve));
					if (bReport && Mission->Rewards.Experience != Stop.PlannedXP)
					{
						Test.AddWarning(FString::Printf(TEXT("%s gives %d XP; Docs/Progression.md plans %d: update the table there."), *Stop.Label,
							Mission->Rewards.Experience, Stop.PlannedXP));
					}
				}
				else
				{
					if (bReport)
					{
						Test.AddWarning(FString::Printf(TEXT("%s isn't made (Tools/Unreal/create_*mission_assets.py): its planned %d XP."), *Stop.Label,
							Stop.PlannedXP));
					}
					Reward = Stop.PlannedXP;
				}
			}
			Result.Total += Reward;
			Result.FromKills += StopKillXP;
			Result.FromMissions += Reward;
			Result.Kills += StopKills;
			if (bReport)
			{
				Test.AddInfo(FString::Printf(TEXT("%-50s %2d kills %+6.1f XP, turned in %+3.0f XP: %6.1f XP in all, level %.2f"), *Stop.Label, StopKills,
					StopKillXP, Reward, Result.Total, LevelOf(Curve, Result.Total)));
			}
		}
		Result.WithoutRoaming += Result.Total;
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXPPacingTest, "Looter.Progression.Pacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FXPPacingTest::RunTest(const FString& Parameters)
{
	const FXPCurve Curve = UPlayerProgressionSubsystem::GetCurve();
	const FLevelRules Rules = UPlayerProgressionSubsystem::GetLevelRules();

	// Ransom's Rest's creature levels as its layout gives them (gameplay.levels, which create_area_assets.py writes into the
	// asset; Looter.Areas.Bands holds the two the same), with the asset's promotions, or the script's when it isn't made.
	EncounterLayout::FAreaLayout Layout;
	if (!EncounterLayout::ReadLayout(*this, Layout))
	{
		return false;
	}
	UAreaDefinition* Area = AreaLevelsLayout::ReadArea(*this, Layout);
	if (!Area)
	{
		return false;
	}
	const UAreaDefinition* Asset = UAreaDefinition::FindByName(TEXT("RansomsRest"));
	if (!Asset)
	{
		AddWarning(TEXT("DA_Area_RansomsRest isn't made (Tools/Unreal/create_area_assets.py): its promotions (8%, 2%) as the script sets them."));
	}
	Area->RarePromotionChance = Asset ? Asset->RarePromotionChance : 0.08f;
	Area->EpicPromotionChance = Asset ? Asset->EpicPromotionChance : 0.02f;

	// The encounters between the fights (layout.json gameplay.encounters): the user's budget is about 15 more kills on the
	// main path at most.
	TArray<FLayoutEncounter> Encounters;
	if (!ReadLayoutEncounters(*this, Encounters))
	{
		return false;
	}
	const FPathCast Foes = MakeCast();
	TArray<FPathStop> Story = StoryPath(Foes);
	TArray<FPathStop> Everything = EverythingPath(Foes);
	bool bPlaced = true;
	for (TArray<FPathStop>* Path : { &Story, &Everything })
	{
		for (FPathStop& Stop : *Path)
		{
			bPlaced &= PlaceKills(*this, Layout, Stop.Kills);
		}
	}
	for (FLayoutEncounter& Each : Encounters)
	{
		bPlaced &= PlaceKills(*this, Layout, Each.Kills);
	}
	if (!bPlaced)
	{
		return false;
	}
	int32 MainAdded = 0;
	int32 RoamingAdded = 0;
	int32 OptionalAdded = 0;
	for (const FLayoutEncounter& Each : Encounters)
	{
		if (IsPathFight(Everything, Each.Id))
		{
			// A mission's own fight the layout lists too: counted once, as the path's.
			continue;
		}
		int32 Count = 0;
		for (const FPathKills& Kills : Each.Kills)
		{
			Count += Kills.Count;
		}
		const bool bKnownStop = Everything.ContainsByPredicate([&Each](const FPathStop& Stop) { return !Stop.MissionId.IsNone() && Stop.MissionId == Each.CountedBefore; });
		const bool bOnTheStory = Story.ContainsByPredicate([&Each](const FPathStop& Stop) { return !Stop.MissionId.IsNone() && Stop.MissionId == Each.CountedBefore; });
		TestTrue(FString::Printf(TEXT("%s is counted before a stop on the path (%s)"), *Each.Id, *Each.CountedBefore.ToString()),
			Each.Budget == TEXT("roaming") || bKnownStop);
		TestTrue(FString::Printf(TEXT("%s's budget is main, roaming or optional (%s)"), *Each.Id, *Each.Budget),
			Each.Budget == TEXT("main") || Each.Budget == TEXT("roaming") || Each.Budget == TEXT("optional"));
		if (Each.Budget == TEXT("main"))
		{
			// The user's budget is the main path's (the story's); a side mission's own way is its own.
			MainAdded += bOnTheStory ? Count : 0;
		}
		else
		{
			(Each.Budget == TEXT("roaming") ? RoamingAdded : OptionalAdded) += Count;
		}
	}
	AddInfo(FString::Printf(TEXT("Between the fights: %d kills on the main path, %d roaming, %d optional."), MainAdded, RoamingAdded, OptionalAdded));
	TestTrue(FString::Printf(TEXT("No more than about 15 more kills on the main path (%d)"), MainAdded), MainAdded <= 15);

	// The story alone: the main missions' rewards, from their assets where they're made.
	const TArray<UMissionDefinition*> Missions = UMissionDefinition::LoadAll();
	AddInfo(TEXT("The story alone:"));
	const FPathResult Played = PlayPath(*this, WithEncounters(Story, Encounters, { TEXT("main"), TEXT("roaming") }), Missions, *Area, Curve, Rules,
		/*bReport*/ true);
	if (!Played.bValid)
	{
		return false;
	}
	const double Reached = LevelOf(Curve, Played.Total);
	AddInfo(FString::Printf(TEXT("The story ends at level %.2f after %d kills: %.0f XP from kills, %.0f from missions, %.0f in all (level 5 at %lld, 6 at %lld)."),
		Reached, Played.Kills, Played.FromKills, Played.FromMissions, Played.Total, Curve.TotalXPToReach(5), Curve.TotalXPToReach(6)));
	TestTrue(FString::Printf(TEXT("Ransom's Rest's story alone ends about level 5.5 (%.2f, 5.3 to 5.8)"), Reached), Reached >= 5.3 && Reached <= 5.8);
	TestTrue(FString::Printf(TEXT("...and level 5 even without the roaming kills (%.2f)"), LevelOf(Curve, Played.WithoutRoaming)),
		LevelOf(Curve, Played.WithoutRoaming) >= 5.0);
	TestTrue(TEXT("Most of a level's worth is the turn-ins', not the kills': the missions give at least two thirds"),
		Played.FromMissions >= Played.Total * 2.0 / 3.0);

	// Everything once: every mission (the valley's side missions, the Gravemother, the mountain's three), every optional
	// camp, patrol and ambush, the roaming dead once.
	AddInfo(TEXT("Everything:"));
	const FPathResult Explored = PlayPath(*this, WithEncounters(Everything, Encounters, { TEXT("main"), TEXT("roaming"), TEXT("optional") }),
		Missions, *Area, Curve, Rules, /*bReport*/ true);
	const double Explorer = LevelOf(Curve, Explored.Total);
	AddInfo(FString::Printf(TEXT("Everything once ends at level %.2f after %d kills: %.0f XP from kills, %.0f from missions, %.0f in all (level 7 at %lld)."),
		Explorer, Explored.Kills, Explored.FromKills, Explored.FromMissions, Explored.Total, Curve.TotalXPToReach(7)));
	TestTrue(FString::Printf(TEXT("...and everything once about level 6.8 (%.2f, 6.6 to 7.0)"), Explorer), Explored.bValid && Explorer >= 6.6 && Explorer <= 7.0);

	// The bluff's spiders stay level 1-2 whoever comes: a player back there at the story's end (level 7) gets a share of what
	// a level 1 player gets, as the kill rules take it off a creature below the player (at the full XPReward, so the shipped
	// scale's rounding to a whole point can't hide the share).
	FPathKills Spider = MakeKills(Foes.Spider, EPathRank::Basic, 1, TEXT("BluffNest"));
	if (TestTrue(TEXT("The bluff's nest is in the layout"), AreaLevelsLayout::FindPlace(Layout, TEXT("BluffNest"), Spider.Spot)))
	{
		FLevelRules Full = Rules;
		Full.KillXPScale = 1.0;
		const double Fresh = ExpectedKillXP(Spider, 1, *Area, Full);
		const double Strong = ExpectedKillXP(Spider, 7, *Area, Full);
		TestTrue(FString::Printf(TEXT("A bluff spider's full worth: %.1f to a level 1 player, %.1f to a level 7 one (under half)"), Fresh, Strong),
			Strong < Fresh * 0.5);
	}

	// Skyreach gives none: its tutorial and the skiff reward nothing (its kills give none either: Looter.Station tests that).
	for (const UMissionDefinition* Mission : Missions)
	{
		if (Mission && Mission->Kind == EMissionKind::Tutorial)
		{
			TestFalse(*FString::Printf(TEXT("%s, on Skyreach, gives no experience"), *Mission->GetMissionId().ToString()), Mission->Rewards.GivesExperience());
		}
	}
	return true;
}

#endif
