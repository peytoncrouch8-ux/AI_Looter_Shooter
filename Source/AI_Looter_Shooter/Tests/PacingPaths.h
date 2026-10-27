#pragma once

// Ransom's Rest's two paths on paper for Looter.Progression.Pacing (ProgressionPacingTests.cpp; Docs/Progression.md): the
// story alone, and everything once. Each stop is a mission turned in with the kills on the way to it (the documented budget:
// the counts the level's scripts place), or kills alone (the roaming dead). The mountain's and the new side missions'
// fights carry the design's spot (Docs/Areas/RansomsRest_Expansion.md section 6) for while the layout doesn't name them.
// The counts are the user's bigger fights' (2026-10-21: 6-10 a fight, the set pieces); the missions' experience was trimmed
// to hold the two paths' ends (Docs/Progression.md). Since 2026-10-27 (the user's note 32) the story goes through the mine
// (Dead Lift, Back Pay) before the boards, and the turn-ins were raised so a player arrives near each boss's level.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelKeeper.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/ForemanCreature.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"

namespace PacingPaths
{
	/** How a kill's rank comes about: fixed (a scripted fight's Basic or Restless), the area's odds, or its own (bosses). */
	enum class EPathRank : uint8
	{
		Basic,
		Restless,
		/** Rolled as a placed or spawned creature is on Ransom's Rest: 8% Restless, 2% Gravebound. */
		AreaRolled,
		/** The creature's own starting rank (Abel's Boss, the Gravemother's and the Foreman's Soulfed). */
		Own,
	};

	struct FPathKills
	{
		const ACreatureBase* Creature = nullptr;
		EPathRank Rank = EPathRank::Basic;
		int32 Count = 0;
		/** Where the fight stands: a story fight's id in the layout ("BluffNest"), a layout zone ("sink") or an encounter's id. */
		FString Where;
		/** The design's spot for it (layout cm: X north, Y east), used while the layout doesn't name Where. */
		FVector2D Planned = FVector2D::ZeroVector;
		bool bPlanned = false;
		/** Levels its spawner keeps whatever the area says (Tilly's cellar 3-4, the Gravemother's brood at her 4); 0: none. */
		int32 KeptLowest = 0;
		int32 KeptHighest = 0;
		/** Where Where is (found from the layout), or an encounter's middle: its levels are this spot's. */
		FVector Spot = FVector::ZeroVector;
	};

	inline FPathKills MakeKills(const ACreatureBase* Creature, EPathRank Rank, int32 Count, const TCHAR* Where)
	{
		FPathKills Made;
		Made.Creature = Creature;
		Made.Rank = Rank;
		Made.Count = Count;
		Made.Where = Where;
		return Made;
	}

	/** Kills at a fight the layout may not name yet: (X, Y) is where the design puts it, in layout cm. */
	inline FPathKills PlannedKills(const ACreatureBase* Creature, EPathRank Rank, int32 Count, const TCHAR* Where, double X, double Y)
	{
		FPathKills Made = MakeKills(Creature, Rank, Count, Where);
		Made.Planned = FVector2D(X, Y);
		Made.bPlanned = true;
		return Made;
	}

	/** Kills at levels of their own (a spawner group's, kept wherever it stands). */
	inline FPathKills KeptKills(const ACreatureBase* Creature, EPathRank Rank, int32 Count, const TCHAR* Where, double X, double Y,
		int32 Lowest, int32 Highest)
	{
		FPathKills Made = PlannedKills(Creature, Rank, Count, Where, X, Y);
		Made.KeptLowest = Lowest;
		Made.KeptHighest = Highest;
		return Made;
	}

	/** One stop on a path: a mission turned in (with the kills on the way to it), or kills alone (no mission). */
	struct FPathStop
	{
		FString Label;
		FName MissionId;
		TArray<FPathKills> Kills;
		/** The mission's experience as Docs/Progression.md lists it (used when its asset isn't made). */
		int32 PlannedXP = 0;
	};

	/** The roaming stop's label: the layout's roaming encounters join its kills. */
	inline const TCHAR* const RoamingLabel = TEXT("Roaming: boot hill, the north road, the west road");

	/** The creatures the paths kill, as their classes make them. */
	struct FPathCast
	{
		const ACreatureBase* Spider = nullptr;
		const ACreatureBase* Spitter = nullptr;
		const ACreatureBase* Unpaid = nullptr;
		const ACreatureBase* BellRinger = nullptr;
		const ACreatureBase* Miner = nullptr;
		const ACreatureBase* Abel = nullptr;
		const ACreatureBase* Gravemother = nullptr;
		const ACreatureBase* Foreman = nullptr;
	};

	inline FPathCast MakeCast()
	{
		FPathCast Made;
		Made.Spider = GetDefault<ASpiderCreature>();
		Made.Spitter = GetDefault<ASpitterSpider>();
		Made.Unpaid = GetDefault<AUnpaidCreature>();
		Made.BellRinger = GetDefault<ABellRingerCreature>();
		Made.Miner = GetDefault<ADeadMinerCreature>();
		Made.Abel = GetDefault<AAbelKeeper>();
		Made.Gravemother = GetDefault<AGravemotherCreature>();
		Made.Foreman = GetDefault<AForemanCreature>();
		return Made;
	}

	// The main missions' stops, in order (their experience as Docs/Progression.md plans it).
	inline FPathStop MainOneStop() { return { TEXT("Main 1, Seven Days"), TEXT("Main1"), {}, 450 }; }

	inline FPathStop MainTwoStop(const FPathCast& Foes)
	{
		// The bluff nest: four spiders and a Restless one (build_area_story.py NEST_PUSHES; five and one until the user's
		// "Soften early only", 2026-10-23).
		return { TEXT("Main 2, Shall We Talk Business?"), TEXT("Main2"),
			{ MakeKills(Foes.Spider, EPathRank::Basic, 4, TEXT("BluffNest")), MakeKills(Foes.Spider, EPathRank::Restless, 1, TEXT("BluffNest")) }, 550 };
	}

	inline FPathStop MainThreeStop(const FPathCast& Foes)
	{
		// The town gate: six Unpaid and a Restless one (TownGate); then Main Street's set piece on the way to Tilly (layout
		// gameplay.encounters.fights MainStreet): six Unpaid, a Restless one and the balcony's Bell-Ringer, at the valley's 2-3
		// (nine and one until the user's "Soften early only", 2026-10-23).
		return { TEXT("Main 3, Cold Welcome"), TEXT("Main3"),
			{ MakeKills(Foes.Unpaid, EPathRank::Basic, 6, TEXT("TownGate")), MakeKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("TownGate")),
			  PlannedKills(Foes.Unpaid, EPathRank::Basic, 6, TEXT("MainStreet"), 0.0, 3400.0),
			  PlannedKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("MainStreet"), 0.0, 3400.0),
			  PlannedKills(Foes.BellRinger, EPathRank::Basic, 1, TEXT("MainStreet"), 0.0, 3400.0) }, 1000 };
	}

	inline FPathStop MainFourStop(const FPathCast& Foes)
	{
		// The chapel yard: fifteen Unpaid in two waves (eight, then seven) and a Restless one (build_area_chapel.py, ChapelYard).
		return { TEXT("Main 4, Hallowed Ground"), TEXT("Main4"),
			{ MakeKills(Foes.Unpaid, EPathRank::Basic, 15, TEXT("ChapelYard")), MakeKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("ChapelYard")) }, 1300 };
	}

	inline FPathStop RoamingStop(const FPathCast& Foes)
	{
		// A few roaming creatures once: boot hill's and the north road's Unpaid, five each (build_area_chapel.py, after Main 4;
		// they're back on every level load). The layout's roaming encounters (the west road's walkers) join them.
		return { RoamingLabel, NAME_None,
			{ MakeKills(Foes.Unpaid, EPathRank::AreaRolled, 5, TEXT("BootHill")), MakeKills(Foes.Unpaid, EPathRank::AreaRolled, 5, TEXT("NorthRoad")) }, 0 };
	}

	inline FPathStop MainFiveStop(const FPathCast& Foes)
	{
		// The Sink: its floor's six spiders and the egg sacs' six (build_area_sink.py, SinkFloor; two to a sac).
		return { TEXT("Main 5, The Keeper's Lantern"), TEXT("Main5"),
			{ MakeKills(Foes.Spider, EPathRank::AreaRolled, 6, TEXT("sink")), MakeKills(Foes.Spider, EPathRank::Basic, 6, TEXT("sink")) }, 3000 };
	}

	inline FPathStop DeadLiftStop(const FPathCast& Foes, bool bWithCrest)
	{
		// The quarry floor (the mountain's 5-6): the lift crew (five) and Abe Haskett, their Restless brakeman with the pin, then
		// the raising-steam fight (two waves of two spiders and a Spitter, two dead miners with the second); the crest's five
		// spiders after the first ride (optional: an explorer's, not the story's). A main mission since 2026-10-27 (the user's
		// note 32: Cyrus Teague the second boss), so it carries a main mission's share of a level.
		FPathStop Stop = { TEXT("Dead Lift"), TEXT("DeadLift"),
			{ PlannedKills(Foes.Miner, EPathRank::Basic, 5, TEXT("QuarryCrew"), 10800.0, 2000.0),
			  PlannedKills(Foes.Miner, EPathRank::Restless, 1, TEXT("AbeHaskett"), 10800.0, 2000.0),
			  PlannedKills(Foes.Spider, EPathRank::Basic, 4, TEXT("RaisingSteam"), 10500.0, 2500.0),
			  PlannedKills(Foes.Spitter, EPathRank::Basic, 2, TEXT("RaisingSteam"), 10500.0, 2500.0),
			  PlannedKills(Foes.Miner, EPathRank::Basic, 2, TEXT("RaisingSteam"), 10500.0, 2500.0) }, 5000 };
		if (bWithCrest)
		{
			Stop.Kills.Add(PlannedKills(Foes.Spider, EPathRank::AreaRolled, 5, TEXT("CrestPatrol"), 14200.0, 1500.0));
		}
		return Stop;
	}

	inline FPathStop BackPayStop(const FPathCast& Foes)
	{
		// The spoil heap's ambush, the Providence's two shifts, the Foreman (set 4 since 2026-10-27, Soulfed's 2: level 6, the
		// story's second boss under Abel's 7) with his two waves of three, and the gallery's cave-in on the way out with the
		// payroll (the set piece: nine miners and a Restless one).
		return { TEXT("Back Pay"), TEXT("BackPay"),
			{ PlannedKills(Foes.Miner, EPathRank::Basic, 6, TEXT("SpoilAmbush"), 20500.0, 8000.0),
			  PlannedKills(Foes.Miner, EPathRank::Basic, 7, TEXT("MineUpper"), 26800.0, 7400.0),
			  PlannedKills(Foes.Miner, EPathRank::Restless, 1, TEXT("MineUpper"), 26800.0, 7400.0),
			  PlannedKills(Foes.Miner, EPathRank::Basic, 7, TEXT("MineLower"), 30000.0, 7400.0),
			  PlannedKills(Foes.Miner, EPathRank::Restless, 1, TEXT("MineLower"), 30000.0, 7400.0),
			  PlannedKills(Foes.BellRinger, EPathRank::Basic, 1, TEXT("MineLower"), 30000.0, 7400.0),
			  PlannedKills(Foes.Miner, EPathRank::Basic, 6, TEXT("Foreman"), 33000.0, 7400.0),
			  PlannedKills(Foes.Foreman, EPathRank::Own, 1, TEXT("Foreman"), 33000.0, 7400.0),
			  PlannedKills(Foes.Miner, EPathRank::Basic, 9, TEXT("CaveIn"), 27200.0, 7400.0),
			  PlannedKills(Foes.Miner, EPathRank::Restless, 1, TEXT("CaveIn"), 27200.0, 7400.0) }, 6500 };
	}

	inline FPathStop MainSixStop(const FPathCast& Foes)
	{
		// The deck: about twelve of Abel's adds in a 3-4 minute fight (phase 1's two every 25 s, phase 2's two waves of four),
		// then Abel.
		return { TEXT("Main 6, The Gravewind"), TEXT("Main6"),
			{ MakeKills(Foes.Unpaid, EPathRank::Basic, 12, TEXT("Abel")), MakeKills(Foes.Abel, EPathRank::Own, 1, TEXT("Abel")) }, 5000 };
	}

	inline FPathStop MainSevenStop() { return { TEXT("Main 7, The Lantern Leans"), TEXT("Main7"), {}, 4500 }; }

	/** The story alone, as a player who only follows it meets it: since 2026-10-27 up the ore lift and into the Providence
	 * (Dead Lift, Back Pay: the Foreman, the second boss) between the Sink and the boards. */
	inline TArray<FPathStop> StoryPath(const FPathCast& Foes)
	{
		return { MainOneStop(), MainTwoStop(Foes), MainThreeStop(Foes), MainFourStop(Foes), RoamingStop(Foes), MainFiveStop(Foes),
			DeadLiftStop(Foes, /*bWithCrest*/ false), BackPayStop(Foes), MainSixStop(Foes), MainSevenStop() };
	}

	/**
	 * Everything on Ransom's Rest once, each mission as soon as it opens (the valley's side missions before Main 5, the
	 * Gravemother and the mountain before Main 6), with their fights (the mountain's at the design's spots and counts). The
	 * optional camps join from the layout.
	 */
	inline TArray<FPathStop> EverythingPath(const FPathCast& Foes)
	{
		TArray<FPathStop> Path = { MainOneStop(), MainTwoStop(Foes), MainThreeStop(Foes) };
		Path.Add({ TEXT("Side 1, Wanted: Already Dead"), TEXT("Side1"), {}, 300 });
		Path.Add({ TEXT("Calder's Caches"), TEXT("CaldersCaches"), {}, 250 });
		// The depot's waiting room (the valley's 2-3): four Unpaid, a Restless one and a Bell-Ringer; and its second push as
		// Fenn's coins come down the stair (DepotReturn, 2026-10-21): four Unpaid and a Restless one.
		Path.Add({ TEXT("The Old Fare"), TEXT("OldFare"),
			{ PlannedKills(Foes.Unpaid, EPathRank::Basic, 4, TEXT("DepotWaiting"), 465.0, 11150.0),
			  PlannedKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("DepotWaiting"), 465.0, 11150.0),
			  PlannedKills(Foes.BellRinger, EPathRank::Basic, 1, TEXT("DepotWaiting"), 465.0, 11150.0),
			  PlannedKills(Foes.Unpaid, EPathRank::Basic, 4, TEXT("DepotReturn"), 465.0, 11150.0),
			  PlannedKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("DepotReturn"), 465.0, 11150.0) }, 450 });
		// Mr. Sloane's keys and the window's shutter (2026-10-27, the user's note 37): no fight, a short errand.
		Path.Add({ TEXT("Lost and Found"), TEXT("LostAndFound"), {}, 150 });
		Path.Add(MainFourStop(Foes));
		// Amos's old hands: seven Unpaid and a Restless one (build_area_whitlock.py, WhitlockHands).
		Path.Add({ TEXT("Side 2, Unfinished Business"), TEXT("Side2"),
			{ MakeKills(Foes.Unpaid, EPathRank::Basic, 7, TEXT("WhitlockHands")), MakeKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("WhitlockHands")) }, 600 });
		// Tilly's cellar, under Main Street: five Unpaid and a Restless one, its spawner's own 3-4.
		Path.Add({ TEXT("Knocking Below"), TEXT("KnockingBelow"),
			{ KeptKills(Foes.Unpaid, EPathRank::Basic, 5, TEXT("TillyCellar"), -2400.0, 6200.0, 3, 4),
			  KeptKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("TillyCellar"), -2400.0, 6200.0, 3, 4) }, 600 });
		Path.Add(RoamingStop(Foes));
		Path.Add(MainFiveStop(Foes));
		// The Gravemother (set 4, Soulfed's 2) and about six of her brood (spiderlings at her 4).
		Path.Add({ TEXT("Side 3, The Gravemother"), TEXT("Side3"),
			{ MakeKills(Foes.Gravemother, EPathRank::Own, 1, TEXT("sink")),
			  KeptKills(Foes.Spider, EPathRank::Basic, 6, TEXT("sink"), 5550.0, 6500.0, 4, 4) }, 900 });
		Path.Add(DeadLiftStop(Foes, /*bWithCrest*/ true));
		// Calder's Post's yard and the watch; the Bench's Dinner Pail camp on the way (optional).
		Path.Add({ TEXT("The Last Watch"), TEXT("LastWatch"),
			{ PlannedKills(Foes.Miner, EPathRank::Basic, 5, TEXT("CalderPost"), 16800.0, -6000.0),
			  PlannedKills(Foes.Miner, EPathRank::Restless, 1, TEXT("CalderPost"), 16800.0, -6000.0),
			  PlannedKills(Foes.Miner, EPathRank::Basic, 6, TEXT("Watch"), 16800.0, -6000.0),
			  PlannedKills(Foes.Miner, EPathRank::Basic, 4, TEXT("DinnerPail"), 17000.0, -500.0),
			  PlannedKills(Foes.Miner, EPathRank::Restless, 1, TEXT("DinnerPail"), 17000.0, -500.0),
			  PlannedKills(Foes.BellRinger, EPathRank::Basic, 1, TEXT("DinnerPail"), 17000.0, -500.0) }, 900 });
		Path.Add(BackPayStop(Foes));
		Path.Add(MainSixStop(Foes));
		Path.Add(MainSevenStop());
		return Path;
	}
}

#endif
