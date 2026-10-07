// Developer console command that plays an area's main path on paper for its experience (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Areas/AreaDefinition.h"
#include "Bosses/BossComponent.h"
#include "Bosses/BossTypes.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSpawner.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRewards.h"
#include "Progression/LevelRules.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Progression/XPCurve.h"
#include "World/EggSac.h"

namespace
{
	/**
	 * How long a boss fight is taken to last, for the adds its repeating waves bring (Abel's is tuned to 3 to 4 minutes):
	 * each phase gets its share of it by the share of health it spans.
	 */
	constexpr float BossFightSeconds = 210.f;

	/** One creature the path kills, as the game would make it: its numbers, how its rank is rolled and where its level comes from. */
	struct FPathFoe
	{
		/** The placed creature, or the class default of what a spawner brings: its XPReward and level before any rank. */
		const ACreatureBase* Creature = nullptr;
		EEncounterRankRoll RankRoll = EEncounterRankRoll::Area;
		ECreatureRank Rank = ECreatureRank::Basic;
		float RareChance = 0.f;
		float EpicChance = 0.f;
		/** A level the spawner's group asks for is kept (ACreatureBase::SpawnAtRuntime); else the area's band gives one. */
		int32 KeptLevel = 0;
	};

	/** The game's rules, read once. */
	struct FPathRules
	{
		const UAreaDefinition* Area = nullptr;
		FXPCurve Curve;
		FLevelRules Levels;
	};

	/** A kill's experience on average for a player at PlayerLevel: over its rank's odds, and the band's level either side. */
	double ExpectedKillXP(const FPathFoe& Foe, int32 PlayerLevel, const FPathRules& Rules)
	{
		if (!Foe.Creature || (Rules.Area && !Rules.Area->GivesKillExperience()))
		{
			return 0.0;
		}
		// The rank's odds as EncounterRules::PickRank and UAreaDefinition::PickPromotion roll them: Epic first, then Rare.
		TArray<TPair<ECreatureRank, double>, TInlineAllocator<3>> Ranks;
		auto AddOdds = [&Ranks](double Epic, double Rare)
		{
			Epic = FMath::Clamp(Epic, 0.0, 1.0);
			Rare = FMath::Clamp(Rare, 0.0, 1.0 - Epic);
			Ranks.Add({ECreatureRank::Epic, Epic});
			Ranks.Add({ECreatureRank::Rare, Rare});
			Ranks.Add({ECreatureRank::Basic, 1.0 - Epic - Rare});
		};
		if (Foe.RankRoll == EEncounterRankRoll::Fixed)
		{
			Ranks.Add({Foe.Rank, 1.0});
		}
		else if (Foe.RankRoll == EEncounterRankRoll::Chances)
		{
			AddOdds(Foe.EpicChance, Foe.RareChance);
		}
		else if (Rules.Area && Rules.Area->HasPromotions())
		{
			AddOdds(Rules.Area->EpicPromotionChance, Rules.Area->RarePromotionChance);
		}
		else
		{
			Ranks.Add({ECreatureRank::Basic, 1.0});
		}

		double XP = 0.0;
		for (const TPair<ECreatureRank, double>& Odds : Ranks)
		{
			if (Odds.Value <= 0.0)
			{
				continue;
			}
			const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Odds.Key);
			const int32 RankedXP = FMath::Max(0, FMath::RoundToInt32(Foe.Creature->XPReward * Info.XPMultiplier));
			// A boss is the player's level kept in the band; anything else a level either side, or the same, a third each.
			const bool bBoss = Odds.Key == ECreatureRank::Boss;
			double Average = 0.0;
			int32 Rolls = 0;
			for (int32 Spread = bBoss ? 0 : -1; Spread <= (bBoss ? 0 : 1); ++Spread)
			{
				const int32 OwnLevel = Foe.KeptLevel > 0 ? Foe.KeptLevel
					: Rules.Area ? Rules.Area->LevelFor(PlayerLevel, Spread, Foe.Creature->Level) : FMath::Max(Foe.Creature->Level, 1);
				Average += Rules.Levels.KillXP(RankedXP, OwnLevel + Info.LevelOffset, PlayerLevel);
				++Rolls;
			}
			XP += Odds.Value * Average / Rolls;
		}
		return XP;
	}

	/** Everything a spawner brings over all its waves, wave by wave and group by group, stopping at its MaxTotal. */
	void AddSpawnerFoes(const AEncounterSpawner& Spawner, TArray<FPathFoe>& Out)
	{
		const int32 Waves = EncounterRules::PlannedWaves(Spawner.NumWaves, Spawner.MaxTotal);
		int32 Total = 0;
		for (int32 Wave = 1; Wave <= Waves; ++Wave)
		{
			for (const FEncounterGroup& Group : Spawner.Groups)
			{
				const ACreatureBase* Default = Group.CreatureClass ? Group.CreatureClass->GetDefaultObject<ACreatureBase>() : nullptr;
				if (!Default || !EncounterRules::JoinsWave(Group, Wave))
				{
					continue;
				}
				for (int32 Each = 0; Each < Group.Count && (Spawner.MaxTotal <= 0 || Total < Spawner.MaxTotal); ++Each, ++Total)
				{
					FPathFoe& Foe = Out.AddDefaulted_GetRef();
					Foe.Creature = Default;
					Foe.RankRoll = Group.RankRoll;
					Foe.Rank = Group.Rank;
					Foe.RareChance = Group.RareChance;
					Foe.EpicChance = Group.EpicChance;
					Foe.KeptLevel = Group.Level;
				}
			}
		}
	}

	/** What a boss's fight raises (its phases' add waves, repeating ones over their phase's share of the fight), then the boss. */
	void AddBossFoes(const ACreatureBase& Boss, const FPathFoe& Itself, TArray<FPathFoe>& Out)
	{
		if (const UBossComponent* Fight = Boss.FindComponentByClass<UBossComponent>())
		{
			TArray<FBossPhase> Phases = Fight->Phases;
			Phases.StableSort([](const FBossPhase& A, const FBossPhase& B) { return A.HealthShare > B.HealthShare; });
			for (int32 Index = 0; Index < Phases.Num(); ++Index)
			{
				const float Below = Phases.IsValidIndex(Index + 1) ? Phases[Index + 1].HealthShare : 0.f;
				const float Seconds = BossFightSeconds * FMath::Max(Phases[Index].HealthShare - Below, 0.f);
				for (const FBossPhaseEvent& Event : Phases[Index].Events)
				{
					const ACreatureBase* Default = Event.Wave.CreatureClass ? Event.Wave.CreatureClass->GetDefaultObject<ACreatureBase>() : nullptr;
					if (Event.Kind != EBossEventKind::AddWave || !Default)
					{
						continue;
					}
					// A wave given once always comes (they come early in their phase); a repeating one as often as it fits.
					const int32 Waves = Event.RepeatEvery <= 0.f ? 1
						: Event.Delay > Seconds ? 0 : 1 + FMath::FloorToInt32((Seconds - Event.Delay) / Event.RepeatEvery);
					for (int32 Each = 0; Each < Waves * Event.Wave.Count; ++Each)
					{
						FPathFoe& Add = Out.AddDefaulted_GetRef();
						Add.Creature = Default;
						Add.RankRoll = EEncounterRankRoll::Fixed;
						Add.Rank = Event.Wave.Rank;
						Add.KeptLevel = Event.Wave.Level;
					}
				}
			}
		}
		Out.Add(Itself);
	}

	/** An egg sac's spiders: one at each of its sockets, of its rank (spawned in play, so never promoted). */
	void AddEggSacFoes(const AEggSac& Sac, TArray<FPathFoe>& Out)
	{
		const ACreatureBase* Default = Sac.SpiderClass ? Sac.SpiderClass->GetDefaultObject<ACreatureBase>() : nullptr;
		for (int32 Each = 0; Default && Each < Sac.SpawnSockets.Num(); ++Each)
		{
			FPathFoe& Spider = Out.AddDefaulted_GetRef();
			Spider.Creature = Default;
			Spider.RankRoll = EEncounterRankRoll::Fixed;
			Spider.Rank = Sac.SpiderRank;
		}
	}

	/** A creature placed in the level, as it begins play: its own rank, or a Basic one promoted by the area's odds. */
	FPathFoe PlacedFoe(const ACreatureBase& Creature)
	{
		FPathFoe Foe;
		Foe.Creature = &Creature;
		Foe.RankRoll = Creature.StartingRank == ECreatureRank::Basic ? EEncounterRankRoll::Area : EEncounterRankRoll::Fixed;
		Foe.Rank = Creature.StartingRank;
		return Foe;
	}

	/** The player's level and how far into it, from their experience in all. */
	double LevelWithProgress(const FXPCurve& Curve, double TotalXP)
	{
		int32 Level = 1;
		while (!Curve.IsMaxLevel(Level) && static_cast<double>(Curve.TotalXPToReach(Level + 1)) <= TotalXP)
		{
			++Level;
		}
		if (Curve.IsMaxLevel(Level))
		{
			return Level;
		}
		const double Into = TotalXP - static_cast<double>(Curve.TotalXPToReach(Level));
		return Level + Into / FMath::Max<double>(1.0, static_cast<double>(Curve.XPToNextLevel(Level)));
	}

	void PrintXPPath(const TArray<FString>& Args, UWorld* World)
	{
		// The level's numbers as authored: in play a creature's XPReward already carries its rank's multiplier.
		if (!World || World->IsGameWorld())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.XP.Path: run it in the editor with the area's level open, not in play."));
			return;
		}
		float OpenWorldShare = 0.5f;
		bool bSides = false;
		for (const FString& Arg : Args)
		{
			if (Arg.Equals(TEXT("sides"), ESearchCase::IgnoreCase))
			{
				bSides = true;
			}
			else if (Arg.IsNumeric())
			{
				OpenWorldShare = FMath::Clamp(FCString::Atof(*Arg), 0.f, 1.f);
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Usage: Looter.XP.Path [open-world share 0-1, default 0.5] [sides]"));
				return;
			}
		}

		FPathRules Rules;
		Rules.Area = UAreaDefinition::FindByMap(UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()));
		Rules.Curve = UPlayerProgressionSubsystem::GetCurve();
		Rules.Levels = UPlayerProgressionSubsystem::GetLevelRules();
		if (!Rules.Area)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.XP.Path: %s is no area's level (open the area's map first)."), *World->GetMapName());
			return;
		}
		const FName AreaId = Rules.Area->GetAreaId();

		// The path: the tutorial first (a new player plays it before they land), then the area's missions in their order.
		TArray<const UMissionDefinition*> Path;
		TSet<FName> OnPath;
		for (const UMissionDefinition* Mission : UMissionDefinition::LoadAll())
		{
			const bool bTutorial = Mission->Kind == EMissionKind::Tutorial;
			const bool bHere = Mission->Area == AreaId && (Mission->Kind == EMissionKind::Main || (bSides && Mission->Kind == EMissionKind::Side));
			if (bTutorial || bHere)
			{
				Path.Add(Mission);
				OnPath.Add(Mission->GetMissionId());
			}
		}
		Path.StableSort([](const UMissionDefinition& A, const UMissionDefinition& B)
		{
			return (A.Kind == EMissionKind::Tutorial) > (B.Kind == EMissionKind::Tutorial);
		});

		// Each mission's own fights: the encounters that play during it, met after it (an "after" encounter on the way to
		// the next one), and the bosses its named kills ask for. The open world (placed creatures and encounters with no
		// mission) is shared over the path at OpenWorldShare: the creatures a main-path player kills on the way.
		TMap<FName, TArray<FPathFoe>> MissionFoes;
		TSet<FName> NamedTags;
		for (const UMissionDefinition* Mission : Path)
		{
			for (const FMissionStep& Step : Mission->Steps)
			{
				for (const UMissionObjective* Objective : Step.Objectives)
				{
					if (const UMissionKillNamedObjective* Named = Cast<UMissionKillNamedObjective>(Objective))
					{
						NamedTags.Add(Named->ActorTag);
						for (TActorIterator<ACreatureBase> It(World); It; ++It)
						{
							if (It->ActorHasTag(Named->ActorTag))
							{
								AddBossFoes(**It, PlacedFoe(**It), MissionFoes.FindOrAdd(Mission->GetMissionId()));
							}
						}
					}
				}
			}
		}
		for (TActorIterator<AEggSac> It(World); It; ++It)
		{
			if (OnPath.Contains(It->ShootableWhen.DuringMission))
			{
				AddEggSacFoes(**It, MissionFoes.FindOrAdd(It->ShootableWhen.DuringMission));
			}
		}
		TArray<FPathFoe> OpenWorld;
		int32 LairCount = 0;
		int32 OffPathCount = 0;
		for (TActorIterator<AEncounterSpawner> It(World); It; ++It)
		{
			const AEncounterSpawner& Spawner = **It;
			const FStoryCondition& When = Spawner.ActiveWhen;
			if (!Spawner.LegendaryId.IsNone())
			{
				++LairCount;
				continue;
			}
			if (!When.DuringMission.IsNone())
			{
				if (OnPath.Contains(When.DuringMission))
				{
					AddSpawnerFoes(Spawner, MissionFoes.FindOrAdd(When.DuringMission));
				}
				else
				{
					++OffPathCount;
				}
				continue;
			}
			// After some missions: met on the way to the first mission of the path that comes after them all.
			int32 After = -1;
			for (const FName& Done : When.AfterMissions)
			{
				After = FMath::Max(After, Path.IndexOfByPredicate([&Done](const UMissionDefinition* M) { return M->GetMissionId() == Done; }));
			}
			if (When.AfterMissions.Num() == 0)
			{
				AddSpawnerFoes(Spawner, OpenWorld);
			}
			else if (Path.IsValidIndex(After + 1))
			{
				AddSpawnerFoes(Spawner, MissionFoes.FindOrAdd(Path[After + 1]->GetMissionId()));
			}
			else
			{
				++OffPathCount;
			}
		}
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			bool bNamed = false;
			for (const FName& Tag : It->Tags)
			{
				bNamed |= NamedTags.Contains(Tag);
			}
			// A Legendary monster or a boss with no mission asking for it is a hunt of its own, not the path's.
			if (!bNamed && It->StartingRank != ECreatureRank::Legendary && It->StartingRank != ECreatureRank::Boss)
			{
				OpenWorld.Add(PlacedFoe(**It));
			}
		}

		int32 AreaMissions = 0;
		for (const UMissionDefinition* Mission : Path)
		{
			AreaMissions += Mission->Kind == EMissionKind::Tutorial ? 0 : 1;
		}
		const double OpenWorldKillsEach = AreaMissions > 0 ? OpenWorld.Num() * OpenWorldShare / AreaMissions : 0.0;
		UE_LOG(LogLooter, Log, TEXT("Looter.XP.Path: %s's %s from level 1. Open world: %d creatures, %.0f%% of them killed once, spread over the area's %d missions."),
			*Rules.Area->DisplayName.ToString(), bSides ? TEXT("main and side missions") : TEXT("main path"), OpenWorld.Num(),
			OpenWorldShare * 100.f, AreaMissions);

		double TotalXP = 0.0;
		double KillXP = 0.0;
		double MissionXP = 0.0;
		double Kills = 0.0;
		auto LevelNow = [&]() { return FMath::FloorToInt32(LevelWithProgress(Rules.Curve, TotalXP)); };
		for (const UMissionDefinition* Mission : Path)
		{
			// Kills one at a time, each at the level the last left the player on, as play adds them.
			double MissionKillXP = 0.0;
			static const TArray<FPathFoe> NoFoes;
			const TArray<FPathFoe>* Found = MissionFoes.Find(Mission->GetMissionId());
			const TArray<FPathFoe>& Foes = Found ? *Found : NoFoes;
			for (const FPathFoe& Foe : Foes)
			{
				const double XP = ExpectedKillXP(Foe, LevelNow(), Rules);
				MissionKillXP += XP;
				TotalXP += XP;
			}
			double MissionKills = Foes.Num();
			if (Mission->Kind != EMissionKind::Tutorial && OpenWorld.Num() > 0)
			{
				double Average = 0.0;
				for (const FPathFoe& Foe : OpenWorld)
				{
					Average += ExpectedKillXP(Foe, LevelNow(), Rules);
				}
				const double OpenXP = Average / OpenWorld.Num() * OpenWorldKillsEach;
				MissionKillXP += OpenXP;
				TotalXP += OpenXP;
				MissionKills += OpenWorldKillsEach;
			}
			const int64 Reward = MissionRewards::ExperienceFor(Mission->Rewards.ExperienceShare, LevelNow(), Rules.Curve);
			TotalXP += Reward;
			KillXP += MissionKillXP;
			MissionXP += Reward;
			Kills += MissionKills;
			UE_LOG(LogLooter, Log, TEXT("  %-12s %-28s %5.1f kills %+7.0f XP   mission %+5lld XP (%.0f%% of a level)   -> level %.2f"),
				*Mission->GetMissionId().ToString(), *Mission->Title.ToString().Left(28), MissionKills, MissionKillXP, Reward,
				Mission->Rewards.ExperienceShare * 100.f, LevelWithProgress(Rules.Curve, TotalXP));
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.XP.Path: ends at level %.2f after %.0f kills: %.0f XP from kills, %.0f from missions (%.0f in all; level 9 takes %lld)."),
			LevelWithProgress(Rules.Curve, TotalXP), Kills, KillXP, MissionXP, TotalXP, Rules.Curve.TotalXPToReach(9));
		if (LairCount > 0 || OffPathCount > 0)
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.XP.Path: not counted: %d legendary lairs, and %d encounters of missions off this path."),
				LairCount, OffPathCount);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs XPPathCommand(
		TEXT("Looter.XP.Path"),
		TEXT("Plays the open area's missions on paper and prints the level after each, for tuning experience: Looter.XP.Path [open-world share 0-1, default 0.5] [sides]. In the editor, with the level open."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PrintXPPath));
}

#endif
