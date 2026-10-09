#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Bosses/AbelKeeper.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
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
#include "UObject/Package.h"
#include "UObject/TopLevelAssetPath.h"

// Ransom's Rest's pacing (Docs/Progression.md, the user's call of 2026-10-08): a player who fights every fight on the main
// path, does both side missions and kills a few roaming creatures ends the story (Main 7 turned in) about level 6-7, not
// 9-10. Experience comes only from kills and turned-in missions. This plays that path on paper from the same data the game
// uses: the mission assets' rewards, the creatures' own XPReward, the rank settings, Ransom's Rest's level band and its
// promotion odds, the progression settings' curve and kill rules; the kills on the way are the documented budget below
// (the counts the level's scripts place, Docs/Progression.md) and the encounters between the fights, read from the layout
// (layout.json gameplay.encounters: camps, patrols, ambushes, each counted by its budget). An edit that moves the end out
// of 6-7 fails here.

namespace
{
	/** How a kill's rank comes about: fixed (a scripted fight's Basic or Restless, Abel's Boss), or the area's promotion odds. */
	enum class EPathRank : uint8
	{
		Basic,
		Restless,
		/** Rolled as a placed or spawned creature is on Ransom's Rest: 8% Restless, 2% Gravebound. */
		AreaRolled,
		/** The creature's own starting rank (Abel's Boss). */
		Own,
	};

	struct FPathKills
	{
		const ACreatureBase* Creature = nullptr;
		EPathRank Rank = EPathRank::Basic;
		int32 Count = 0;
	};

	/** One stop on the story: a mission turned in (with the kills on the way to it), or kills alone (no mission). */
	struct FPathStop
	{
		FString Label;
		FName MissionId;
		TArray<FPathKills> Kills;
		/** The mission's experience as Docs/Progression.md lists it (used when its asset isn't made). */
		int32 PlannedXP = 0;
	};

	/** The roaming stop's label: the layout's roaming encounters join its kills. */
	const TCHAR* RoamingLabel = TEXT("Roaming: boot hill, the north road, the west road");

	/** The documented budget: what's killed in each fight on the way, in the order a player meets them. */
	TArray<FPathStop> RansomsRestPath()
	{
		const ACreatureBase* Spider = GetDefault<ASpiderCreature>();
		const ACreatureBase* Unpaid = GetDefault<AUnpaidCreature>();
		const ACreatureBase* Abel = GetDefault<AAbelKeeper>();
		TArray<FPathStop> Path;
		// Main 1: no fights.
		Path.Add({ TEXT("Main 1, Seven Days"), TEXT("Main1"), {}, 20 });
		// The bluff nest: four spiders and a Restless one (build_area_story.py, BluffNest).
		Path.Add({ TEXT("Main 2, Shall We Talk Business?"), TEXT("Main2"),
			{ { Spider, EPathRank::Basic, 4 }, { Spider, EPathRank::Restless, 1 } }, 30 });
		// The town gate: three Unpaid and a Restless one (TownGate).
		Path.Add({ TEXT("Main 3, Cold Welcome"), TEXT("Main3"),
			{ { Unpaid, EPathRank::Basic, 3 }, { Unpaid, EPathRank::Restless, 1 } }, 30 });
		// The posters: no fights.
		Path.Add({ TEXT("Side 1, Wanted: Already Dead"), TEXT("Side1"), {}, 35 });
		// The chapel yard: twelve Unpaid in two waves and a Restless one (build_area_chapel.py, ChapelYard).
		Path.Add({ TEXT("Main 4, Hallowed Ground"), TEXT("Main4"),
			{ { Unpaid, EPathRank::Basic, 12 }, { Unpaid, EPathRank::Restless, 1 } }, 40 });
		// Amos's old hands: five Unpaid and a Restless one (build_area_whitlock.py, WhitlockHands).
		Path.Add({ TEXT("Side 2, Unfinished Business"), TEXT("Side2"),
			{ { Unpaid, EPathRank::Basic, 5 }, { Unpaid, EPathRank::Restless, 1 } }, 40 });
		// A few roaming creatures: boot hill's and the north road's Unpaid, three each, once (build_area_chapel.py, after
		// Main 4; they're back on every level load, which is what takes a player on to level 7). The layout's roaming
		// encounters (the west road's walkers) join them.
		Path.Add({ RoamingLabel, NAME_None, { { Unpaid, EPathRank::AreaRolled, 6 } }, 0 });
		// The Sink: its floor's four spiders and the egg sacs' six (build_area_sink.py, SinkFloor; two to a sac).
		Path.Add({ TEXT("Main 5, The Keeper's Lantern"), TEXT("Main5"),
			{ { Spider, EPathRank::AreaRolled, 4 }, { Spider, EPathRank::Basic, 6 } }, 40 });
		// The deck: about twelve of Abel's adds in a 3-4 minute fight (phase 1's two every 25 s, phase 2's two waves of four),
		// then Abel.
		Path.Add({ TEXT("Main 6, The Gravewind"), TEXT("Main6"),
			{ { Unpaid, EPathRank::Basic, 12 }, { Abel, EPathRank::Own, 1 } }, 45 });
		// Main 7: no fights.
		Path.Add({ TEXT("Main 7, The Lantern Leans"), TEXT("Main7"), {}, 45 });
		return Path;
	}

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
				}
			}
		}
		return true;
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
				if (!Budgets.Contains(Each.Budget))
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

	/** A kill's experience on average for a player at PlayerLevel, over its rank's odds and the band's level either side. */
	double ExpectedKillXP(const ACreatureBase& Creature, EPathRank PathRank, int32 PlayerLevel, const UAreaDefinition& Area, const FLevelRules& Rules)
	{
		TArray<TPair<ECreatureRank, double>, TInlineAllocator<3>> Odds;
		switch (PathRank)
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
			// As the area rolls a promotion: Gravebound first, then Restless.
			const double Epic = FMath::Clamp(static_cast<double>(Area.EpicPromotionChance), 0.0, 1.0);
			const double Rare = FMath::Clamp(static_cast<double>(Area.RarePromotionChance), 0.0, 1.0 - Epic);
			Odds.Add({ ECreatureRank::Epic, Epic });
			Odds.Add({ ECreatureRank::Rare, Rare });
			Odds.Add({ ECreatureRank::Basic, 1.0 - Epic - Rare });
			break;
		}
		}
		double XP = 0.0;
		for (const TPair<ECreatureRank, double>& Each : Odds)
		{
			const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Each.Key);
			const int32 RankedXP = FMath::Max(0, FMath::RoundToInt32(Creature.XPReward * Info.XPMultiplier));
			// A boss is the player's level kept in the band; anything else a level either side, or the same, a third each.
			const bool bBoss = Each.Key == ECreatureRank::Boss;
			double Sum = 0.0;
			int32 Rolls = 0;
			for (int32 Spread = bBoss ? 0 : -1; Spread <= (bBoss ? 0 : 1); ++Spread)
			{
				const int32 Level = Area.LevelFor(PlayerLevel, Spread, Creature.Level) + Info.LevelOffset;
				Sum += static_cast<double>(Rules.KillXP(RankedXP, Level, PlayerLevel));
				++Rolls;
			}
			XP += Each.Value * Sum / Rolls;
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
					const double XP = ExpectedKillXP(*Group.Creature, Group.Rank, FMath::FloorToInt32(LevelOf(Curve, Result.Total)), Area, Rules);
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

	// Ransom's Rest's band and promotions: its asset's, or the area script's numbers when it isn't made.
	const UAreaDefinition* Area = UAreaDefinition::FindByName(TEXT("RansomsRest"));
	if (!Area || !Area->HasLevelBand())
	{
		AddWarning(TEXT("DA_Area_RansomsRest isn't made (Tools/Unreal/create_area_assets.py): its band (1-10) and promotions (8%, 2%) as the script sets them."));
		UAreaDefinition* Stand = NewObject<UAreaDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
		Stand->MinLevel = 1;
		Stand->MaxLevel = 10;
		Stand->RarePromotionChance = 0.08f;
		Stand->EpicPromotionChance = 0.02f;
		Area = Stand;
	}

	// The encounters between the fights (layout.json gameplay.encounters): the user's budget is about 15 more kills on the
	// main path at most.
	TArray<FLayoutEncounter> Encounters;
	if (!ReadLayoutEncounters(*this, Encounters))
	{
		return false;
	}
	int32 MainAdded = 0;
	int32 RoamingAdded = 0;
	int32 OptionalAdded = 0;
	const TArray<FPathStop> Story = RansomsRestPath();
	for (const FLayoutEncounter& Each : Encounters)
	{
		int32 Count = 0;
		for (const FPathKills& Kills : Each.Kills)
		{
			Count += Kills.Count;
		}
		const bool bKnownStop = Story.ContainsByPredicate([&Each](const FPathStop& Stop) { return !Stop.MissionId.IsNone() && Stop.MissionId == Each.CountedBefore; });
		TestTrue(FString::Printf(TEXT("%s is counted before a stop on the path (%s)"), *Each.Id, *Each.CountedBefore.ToString()),
			Each.Budget == TEXT("roaming") || bKnownStop);
		TestTrue(FString::Printf(TEXT("%s's budget is main, roaming or optional (%s)"), *Each.Id, *Each.Budget),
			Each.Budget == TEXT("main") || Each.Budget == TEXT("roaming") || Each.Budget == TEXT("optional"));
		(Each.Budget == TEXT("main") ? MainAdded : Each.Budget == TEXT("roaming") ? RoamingAdded : OptionalAdded) += Count;
	}
	AddInfo(FString::Printf(TEXT("Between the fights: %d kills on the main path, %d roaming, %d optional."), MainAdded, RoamingAdded, OptionalAdded));
	TestTrue(FString::Printf(TEXT("No more than about 15 more kills on the main path (%d)"), MainAdded), MainAdded <= 15);

	// The missions' rewards, from their assets where they're made.
	const TArray<UMissionDefinition*> Missions = UMissionDefinition::LoadAll();
	const FPathResult Played = PlayPath(*this, WithEncounters(Story, Encounters, { TEXT("main"), TEXT("roaming") }), Missions, *Area, Curve, Rules,
		/*bReport*/ true);
	if (!Played.bValid)
	{
		return false;
	}
	const double Reached = LevelOf(Curve, Played.Total);
	AddInfo(FString::Printf(TEXT("The story ends at level %.2f after %d kills: %.0f XP from kills, %.0f from missions, %.0f in all (level 6 at %lld, 7 at %lld)."),
		Reached, Played.Kills, Played.FromKills, Played.FromMissions, Played.Total, Curve.TotalXPToReach(6), Curve.TotalXPToReach(7)));
	TestTrue(FString::Printf(TEXT("Ransom's Rest's story ends in level 6 (%.2f)"), Reached), Reached >= 6.0 && Reached < 7.0);
	TestTrue(FString::Printf(TEXT("...about 790 XP (%.0f)"), Played.Total), FMath::Abs(Played.Total - 790.0) <= 60.0);
	TestTrue(FString::Printf(TEXT("...and level 6 even without the roaming kills (%.2f)"), LevelOf(Curve, Played.WithoutRoaming)),
		LevelOf(Curve, Played.WithoutRoaming) >= 6.0);
	TestTrue(TEXT("Most of a turn-in's worth is the mission's, not a kill's: the missions give at least a third"), Played.FromMissions >= Played.Total / 3.0);

	// An explorer who also clears every optional camp, patrol and ambush once ends about level 7, not 8.
	const FPathResult Explored = PlayPath(*this, WithEncounters(Story, Encounters, { TEXT("main"), TEXT("roaming"), TEXT("optional") }), Missions,
		*Area, Curve, Rules, /*bReport*/ false);
	const double Explorer = LevelOf(Curve, Explored.Total);
	AddInfo(FString::Printf(TEXT("An explorer who clears everything once: level %.2f, %.0f XP after %d kills."), Explorer, Explored.Total, Explored.Kills));
	TestTrue(FString::Printf(TEXT("...an explorer ends about level 7 (%.2f, under 7.75)"), Explorer), Explored.bValid && Explorer < 7.75);

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
