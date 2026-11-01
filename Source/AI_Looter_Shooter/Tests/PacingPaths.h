#pragma once

// Ransom's Rest's two paths on paper for Looter.Progression.Pacing (ProgressionPacingTests.cpp; Docs/Progression.md): the
// story alone, and everything once. Each stop is a mission turned in with the kills on the way to it (the documented budget:
// the counts the level's scripts place), or kills alone (the roaming dead). The mountain's and the new side missions'
// fights carry the design's spot (Docs/Areas/RansomsRest_Expansion.md section 6) for while the layout doesn't name them.
// The counts are the user's bigger fights' (2026-10-21: 6-10 a fight, the set pieces); the missions' experience was trimmed
// to hold the two paths' ends (Docs/Progression.md). Since 2026-10-27 (the user's note 32) the story goes through the mine
// (Dead Lift, Back Pay) before the boards, and the turn-ins were raised so a player arrives near each boss's level. On
// 2026-11-01 Mains 4-5's Dredgers (+17 kills) were paid for out of Main 4 (1,000), Main 5 (1,400) and Paid on Delivery (150).
// On 2026-11-01 (lane MAIN67) Main 6 took the barge's 20 Dredgers and the Chief Stoker, Main 7 Abel (from Main 6) and the west
// road's crew: Main 6 5,000 -> 3,000, Main 7 4,500 -> 3,000.

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
#include "Creatures/StoneSentinelCreature.h"
#include "Creatures/UnpaidCreature.h"

namespace PacingPaths
{
	/** How a kill's rank comes about: fixed (a scripted fight's Basic or Restless), the area's odds, or its own (bosses). */
	enum class EPathRank : uint8
	{
		Basic,
		Restless,
		/** A Gravebound leader (the chapel yard's Sexton's Debt, the lower drift's, the Tollman). */
		Gravebound,
		/** Hallowed (the barge's Chief Stoker, Main 6). */
		Hallowed,
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
		/** The Stone Sentinels (four after Main 4: build_area_sentinels.py). */
		const ACreatureBase* Sentinel = nullptr;
		/** The placeholders (build_area_placeholders.py), their BP_Placeholder_<Kind> classes' (null until they're made). */
		const ACreatureBase* CoffinRat = nullptr;
		const ACreatureBase* BoneHound = nullptr;
		const ACreatureBase* Knocker = nullptr;
		const ACreatureBase* Gravepicker = nullptr;
		const ACreatureBase* GravepickerLookout = nullptr;
		/** Vane's Dredgers (Landfall, 2026-10-31: ADredgerRifleman, ADredgerHoseMan), found by class name (null until built). */
		const ACreatureBase* DredgerRifleman = nullptr;
		const ACreatureBase* DredgerHoseMan = nullptr;
	};


	/** A placeholder kind's class default (create_placeholder_creatures.py's BP_Placeholder_<Kind>), or null until it's made. */
	inline const ACreatureBase* PlaceholderOf(const TCHAR* Kind)
	{
		const FString Path = FString::Printf(TEXT("/Game/Data/Placeholders/BP_Placeholder_%s.BP_Placeholder_%s_C"), Kind, Kind);
		const UClass* Class = LoadClass<ACreatureBase>(nullptr, *Path);
		return Class ? Class->GetDefaultObject<ACreatureBase>() : nullptr;
	}

	/**
	 * A game class's default by its C++ name (/Script/AI_Looter_Shooter.<Name>), or null while it isn't in the module: the
	 * Dredgers are found so, so these paths never include another lane's headers.
	 */
	inline const ACreatureBase* ScriptCreatureOf(const TCHAR* ClassName)
	{
		const FString Path = FString::Printf(TEXT("/Script/AI_Looter_Shooter.%s"), ClassName);
		const UClass* Class = FindObject<UClass>(nullptr, *Path);
		return Class && Class->IsChildOf(ACreatureBase::StaticClass()) ? Class->GetDefaultObject<ACreatureBase>() : nullptr;
	}

	/** Kills added only for a creature whose class is made (a placeholder before its BP exists counts nothing). */
	inline void AddKills(TArray<FPathKills>& Kills, const FPathKills& Made)
	{
		if (Made.Creature)
		{
			Kills.Add(Made);
		}
	}

	/** A stop of kills alone (no mission), each group's only if its class is made. */
	inline FPathStop KillsStop(const TCHAR* Label, std::initializer_list<FPathKills> Groups)
	{
		FPathStop Stop = { Label, NAME_None, {}, 0 };
		for (const FPathKills& Each : Groups)
		{
			AddKills(Stop.Kills, Each);
		}
		return Stop;
	}
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
		Made.Sentinel = GetDefault<AStoneSentinelCreature>();
		Made.CoffinRat = PlaceholderOf(TEXT("CoffinRat"));
		Made.BoneHound = PlaceholderOf(TEXT("BoneHound"));
		Made.Knocker = PlaceholderOf(TEXT("Knocker"));
		Made.Gravepicker = PlaceholderOf(TEXT("Gravepicker"));
		Made.GravepickerLookout = PlaceholderOf(TEXT("GravepickerLookout"));
		Made.DredgerRifleman = ScriptCreatureOf(TEXT("DredgerRifleman"));
		Made.DredgerHoseMan = ScriptCreatureOf(TEXT("DredgerHoseMan"));
		return Made;
	}

	// The main missions' stops, in order (their experience as Docs/Progression.md plans it).
	inline FPathStop MainOneStop(const FPathCast& Foes)
	{
		// Landfall (2026-10-31, Docs/Work/2026-10-31-Wardens/CONTRACT.md "Encounters"): the boarders on the Lamplighter's deck
		// (3 riflemen and a hose-man; the tender's own two riflemen fall away with it, uncounted), over the canyon short of
		// the Mooring Ledge; the four Unpaid climbing onto the ledge (LedgeClimbers); the farm's fight at Delia's steps
		// (FarmPorch: 3 riflemen, 2 hose-men, 3 Unpaid). Spots in layout cm (X north, Y east): the ledge about (-10100, -10200),
		// the deck about 120 m west of it, the farmhouse (-4000, -5800). The Dredgers count once their classes are built.
		FPathStop Stop = { TEXT("Main 1, Landfall"), TEXT("Main1"), {}, 450 };
		AddKills(Stop.Kills, PlannedKills(Foes.DredgerRifleman, EPathRank::Basic, 3, TEXT("LamplighterDeck"), -10100.0, -22200.0));
		AddKills(Stop.Kills, PlannedKills(Foes.DredgerHoseMan, EPathRank::Basic, 1, TEXT("LamplighterDeck"), -10100.0, -22200.0));
		AddKills(Stop.Kills, PlannedKills(Foes.Unpaid, EPathRank::Basic, 4, TEXT("LedgeClimbers"), -10100.0, -10200.0));
		AddKills(Stop.Kills, PlannedKills(Foes.DredgerRifleman, EPathRank::Basic, 3, TEXT("FarmPorch"), -4000.0, -5800.0));
		AddKills(Stop.Kills, PlannedKills(Foes.DredgerHoseMan, EPathRank::Basic, 2, TEXT("FarmPorch"), -4000.0, -5800.0));
		AddKills(Stop.Kills, PlannedKills(Foes.Unpaid, EPathRank::Basic, 3, TEXT("FarmPorch"), -4000.0, -5800.0));
		return Stop;
	}

	inline FPathStop MainTwoStop(const FPathCast& Foes)
	{
		// The Ferryman's Price (2026-11-01, build_area_main2.py): the Three Widows' nest, four spiders and a Restless one (the
		// user's "Soften early only" numbers, moved off the Point); the boards' crew at level 2 whatever the deck's zone says
		// (three riflemen, a hose-man, then the cart's spill: four Unpaid), on Gravewind Point's deck (layout (1200, -15000)).
		FPathStop Stop = { TEXT("Main 2, The Ferryman's Price"), TEXT("Main2"),
			{ MakeKills(Foes.Spider, EPathRank::Basic, 4, TEXT("BluffNest")), MakeKills(Foes.Spider, EPathRank::Restless, 1, TEXT("BluffNest")),
			  KeptKills(Foes.Unpaid, EPathRank::Basic, 4, TEXT("BoardsCrew"), 1200.0, -15000.0, 2, 2) }, 400 };
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 3, TEXT("BoardsCrew"), 1200.0, -15000.0, 2, 2));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 1, TEXT("BoardsCrew"), 1200.0, -15000.0, 2, 2));
		return Stop;
	}

	inline FPathStop MainThreeStop(const FPathCast& Foes)
	{
		// Under New Management (2026-11-01, build_area_main3.py): the Company's checkpoint (TownGate: three riflemen, a
		// hose-man, two Unpaid and a Restless one); then Main Street's set piece (layout gameplay.encounters.fights MainStreet):
		// six Unpaid, a Restless one, the balcony's Bell-Ringer and, with its second push, two riflemen, at the valley's 2-3.
		FPathStop Stop = { TEXT("Main 3, Under New Management"), TEXT("Main3"),
			{ MakeKills(Foes.Unpaid, EPathRank::Basic, 2, TEXT("TownGate")), MakeKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("TownGate")),
			  PlannedKills(Foes.Unpaid, EPathRank::Basic, 6, TEXT("MainStreet"), 0.0, 3400.0),
			  PlannedKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("MainStreet"), 0.0, 3400.0),
			  PlannedKills(Foes.BellRinger, EPathRank::Basic, 1, TEXT("MainStreet"), 0.0, 3400.0) }, 800 };
		AddKills(Stop.Kills, MakeKills(Foes.DredgerRifleman, EPathRank::Basic, 3, TEXT("TownGate")));
		AddKills(Stop.Kills, MakeKills(Foes.DredgerHoseMan, EPathRank::Basic, 1, TEXT("TownGate")));
		AddKills(Stop.Kills, PlannedKills(Foes.DredgerRifleman, EPathRank::Basic, 2, TEXT("MainStreet"), 0.0, 3400.0));
		return Stop;
	}

	inline FPathStop MainFourStop(const FPathCast& Foes)
	{
		// A Coal Under Ash (2026-11-01, build_area_chapel.py YARD_PUSHES, build_area_main4.py): Saint Ada's Yard in two pushes
		// (eight Unpaid; then the crew's two hose-men, five Unpaid and the Gravebound Leftover), then the barge's answer down the
		// tender's ropes (BellAnswered: four riflemen at level 3, two hose-men at 4) round the churchyard (layout (6700, -1900)).
		FPathStop Stop = { TEXT("Main 4, A Coal Under Ash"), TEXT("Main4"),
			{ MakeKills(Foes.Unpaid, EPathRank::Basic, 13, TEXT("ChapelYard")), MakeKills(Foes.Unpaid, EPathRank::Gravebound, 1, TEXT("ChapelYard")) }, 1000 };
		AddKills(Stop.Kills, MakeKills(Foes.DredgerHoseMan, EPathRank::Basic, 2, TEXT("ChapelYard")));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 4, TEXT("BellAnswered"), 6700.0, -1900.0, 3, 3));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 2, TEXT("BellAnswered"), 6700.0, -1900.0, 4, 4));
		return Stop;
	}

	inline FPathStop RoamingStop(const FPathCast& Foes)
	{
		// A few roaming creatures once: boot hill's and the north road's Unpaid, five each and a Restless leader (build_area_chapel.py,
		// after Main 4; they're back on every level load). The layout's roaming encounters (the west road's walkers) join them.
		return { RoamingLabel, NAME_None,
			{ MakeKills(Foes.Unpaid, EPathRank::AreaRolled, 5, TEXT("BootHill")), MakeKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("BootHill")),
			  MakeKills(Foes.Unpaid, EPathRank::AreaRolled, 5, TEXT("NorthRoad")), MakeKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("NorthRoad")) }, 0 };
	}

	inline FPathStop MainFiveStop(const FPathCast& Foes)
	{
		// Choke Point (2026-11-01, build_area_sink.py, build_area_main5.py): the floor's six spiders and the egg sacs' six; the
		// Intake's set piece: two drops down the hose (four riflemen, two hose-men) and three spiders down the ramp, the Sink's 4-5.
		FPathStop Stop = { TEXT("Main 5, Choke Point"), TEXT("Main5"),
			{ MakeKills(Foes.Spider, EPathRank::AreaRolled, 6, TEXT("sink")), MakeKills(Foes.Spider, EPathRank::Basic, 6, TEXT("sink")),
			  MakeKills(Foes.Spider, EPathRank::AreaRolled, 3, TEXT("sink")) }, 1400 };
		AddKills(Stop.Kills, MakeKills(Foes.DredgerRifleman, EPathRank::Basic, 4, TEXT("sink")));
		AddKills(Stop.Kills, MakeKills(Foes.DredgerHoseMan, EPathRank::Basic, 2, TEXT("sink")));
		return Stop;
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
			  PlannedKills(Foes.Miner, EPathRank::Gravebound, 1, TEXT("MineLower"), 30000.0, 7400.0),
			  PlannedKills(Foes.BellRinger, EPathRank::Basic, 1, TEXT("MineLower"), 30000.0, 7400.0),
			  PlannedKills(Foes.Miner, EPathRank::Basic, 6, TEXT("Foreman"), 33000.0, 7400.0),
			  PlannedKills(Foes.Foreman, EPathRank::Own, 1, TEXT("Foreman"), 33000.0, 7400.0),
			  PlannedKills(Foes.Miner, EPathRank::Basic, 9, TEXT("CaveIn"), 27200.0, 7400.0),
			  PlannedKills(Foes.Miner, EPathRank::Restless, 1, TEXT("CaveIn"), 27200.0, 7400.0) }, 6500 };
	}

	inline FPathStop MainSixStop(const FPathCast& Foes)
	{
		// The Pilot Light (2026-11-01, lane MAIN67): the escort tender's boarders over the Lamplighter's plank (two riflemen and a
		// hose-man; the tenders' own deck riflemen fall with them, uncounted, as in Landfall), the barge's top deck (BargeDeck:
		// six riflemen, three hose-men) and its furnace hall (FurnaceHall: the Chief Stoker, a Hallowed hose-man at 5 + 2 = 7;
		// four riflemen, three hose-men), all kept at level 6, 25 m over the chapel knoll (layout (6700, -1900)).
		FPathStop Stop = { TEXT("Main 6, The Pilot Light"), TEXT("Main6"), {}, 3000 };
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 2, TEXT("LamplighterSortie"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 1, TEXT("LamplighterSortie"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 6, TEXT("BargeDeck"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 3, TEXT("BargeDeck"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 4, TEXT("FurnaceHall"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 3, TEXT("FurnaceHall"), 6700.0, -1900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Hallowed, 1, TEXT("FurnaceHall"), 6700.0, -1900.0, 5, 5));
		return Stop;
	}

	inline FPathStop MainSevenStop(const FPathCast& Foes, bool bWithHounds)
	{
		// Given to the Wind (lane MAIN67): about twelve of Abel's adds in a 5-6 minute fight, then Abel (7); the west road's
		// crew off the barge on the carry home (WestRoadCrew: three riflemen, two hose-men, kept 6). The keeper's road's Bone
		// Hounds (placeholders) on the way to the boards, met by everything once (the keeper's path is optional).
		FPathStop Stop = { TEXT("Main 7, Given to the Wind"), TEXT("Main7"),
			{ MakeKills(Foes.Unpaid, EPathRank::Basic, 12, TEXT("Abel")), MakeKills(Foes.Abel, EPathRank::Own, 1, TEXT("Abel")) }, 3000 };
		AddKills(Stop.Kills, KeptKills(Foes.DredgerRifleman, EPathRank::Basic, 3, TEXT("WestRoadCrew"), 2800.0, -7900.0, 6, 6));
		AddKills(Stop.Kills, KeptKills(Foes.DredgerHoseMan, EPathRank::Basic, 2, TEXT("WestRoadCrew"), 2800.0, -7900.0, 6, 6));
		if (bWithHounds)
		{
			AddKills(Stop.Kills, PlannedKills(Foes.BoneHound, EPathRank::Basic, 4, TEXT("KeepersRoadHounds"), 2000.0, -10000.0));
			AddKills(Stop.Kills, PlannedKills(Foes.BoneHound, EPathRank::Restless, 1, TEXT("KeepersRoadHounds"), 2000.0, -10000.0));
		}
		return Stop;
	}

	/** The story alone, as a player who only follows it meets it: since 2026-10-27 up the ore lift and into the Providence
	 * (Dead Lift, Back Pay: the Foreman, the second boss) between the Sink and the boards. */
	inline TArray<FPathStop> StoryPath(const FPathCast& Foes)
	{
		return { MainOneStop(Foes), MainTwoStop(Foes), MainThreeStop(Foes), MainFourStop(Foes), RoamingStop(Foes), MainFiveStop(Foes),
			DeadLiftStop(Foes, /*bWithCrest*/ false), BackPayStop(Foes), MainSixStop(Foes), MainSevenStop(Foes, /*bWithHounds*/ false) };
	}

	/**
	 * Everything on Ransom's Rest once, each mission as soon as it opens (the valley's side missions before Main 5, the
	 * Gravemother and the mountain before Main 6), with their fights (the mountain's at the design's spots and counts). The
	 * optional camps join from the layout.
	 */
	inline TArray<FPathStop> EverythingPath(const FPathCast& Foes)
	{
		TArray<FPathStop> Path = { MainOneStop(Foes), MainTwoStop(Foes), MainThreeStop(Foes) };
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
		// The baggage room's coffin rats (placeholders, beside the depot's fight: build_area_placeholders.py BaggageRats).
		AddKills(Path.Last().Kills, PlannedKills(Foes.CoffinRat, EPathRank::Basic, 4, TEXT("BaggageRats"), 465.0, 11150.0));
		// Mr. Sloane's keys and the window's shutter (2026-10-27, the user's note 37): no fight, a short errand.
		Path.Add({ TEXT("Lost and Found"), TEXT("LostAndFound"), {}, 150 });
		// Boot Hill's grave robbers after Cold Welcome, gone after Hallowed Ground (placeholders: BootHillDiggers).
		Path.Add(KillsStop(TEXT("Boot Hill's Gravepickers (placeholders)"),
			{ PlannedKills(Foes.Gravepicker, EPathRank::Basic, 4, TEXT("BootHillDiggers"), 3200.0, 0.0),
			  PlannedKills(Foes.GravepickerLookout, EPathRank::Basic, 2, TEXT("BootHillDiggers"), 3200.0, 0.0) }));
		Path.Add(MainFourStop(Foes));
		// After Hallowed Ground: the four Stone Sentinels (build_area_sentinels.py: two on Boot Hill, two on the churchyard's east
		// rows; the area's rank odds) (the hounds moved to Main 7, lane MAIN67).
		Path.Add(KillsStop(TEXT("Stone Sentinels"),
			{ PlannedKills(Foes.Sentinel, EPathRank::AreaRolled, 2, TEXT("SentinelsBootHill"), 3275.0, -1050.0),
			  PlannedKills(Foes.Sentinel, EPathRank::AreaRolled, 2, TEXT("SentinelsChurchyard"), 7200.0, -750.0) }));
		// Amos's old hands, Barn Doors (FIGHTS lane 6): five Unpaid and a Restless leader in the yard, then four out of the barn
		// and a Bell-Ringer (build_area_whitlock.py, WhitlockHands).
		Path.Add({ TEXT("Side 2, Unfinished Business"), TEXT("Side2"),
			{ MakeKills(Foes.Unpaid, EPathRank::Basic, 9, TEXT("WhitlockHands")), MakeKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("WhitlockHands")),
			  MakeKills(Foes.BellRinger, EPathRank::Basic, 1, TEXT("WhitlockHands")) }, 600 });
		// Tilly's cellar, under Main Street: five Unpaid and a Restless one, its spawner's own 3-4.
		Path.Add({ TEXT("Knocking Below"), TEXT("KnockingBelow"),
			{ KeptKills(Foes.Unpaid, EPathRank::Basic, 5, TEXT("TillyCellar"), -2400.0, 6200.0, 3, 4),
			  KeptKills(Foes.Unpaid, EPathRank::Restless, 1, TEXT("TillyCellar"), -2400.0, 6200.0, 3, 4) }, 600 });
		// The cellar's coffin rats, with its fight and at its levels (placeholders: CellarRats, like Encounter_TillyCellar).
		AddKills(Path.Last().Kills, KeptKills(Foes.CoffinRat, EPathRank::Basic, 4, TEXT("CellarRats"), -2400.0, 6200.0, 3, 4));
		Path.Add(RoamingStop(Foes));
		Path.Add(MainFiveStop(Foes));
		// The Gravemother (set 4, Soulfed's 2) and about six of her brood (spiderlings at her 4).
		Path.Add({ TEXT("Side 3, The Gravemother"), TEXT("Side3"),
			{ MakeKills(Foes.Gravemother, EPathRank::Own, 1, TEXT("sink")),
			  KeptKills(Foes.Spider, EPathRank::Basic, 6, TEXT("sink"), 5550.0, 6500.0, 4, 4) }, 900 });
		Path.Add(DeadLiftStop(Foes, /*bWithCrest*/ true));
		// The tramway's knockers after Dead Lift (placeholders: TramwayKnockers).
		Path.Add(KillsStop(TEXT("The tramway's Knockers (placeholders)"),
			{ PlannedKills(Foes.Knocker, EPathRank::Basic, 5, TEXT("TramwayKnockers"), 17300.0, 3900.0),
			  PlannedKills(Foes.Knocker, EPathRank::Restless, 1, TEXT("TramwayKnockers"), 17300.0, 3900.0) }));
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
		// Paid on Delivery (2026-11-01, lanes/SECRETS.md): Sexton's price, the manifest handed over at his ferry. No fight.
		Path.Add({ TEXT("Paid on Delivery"), TEXT("PaidOnDelivery"), {}, 150 });
		Path.Add(MainSevenStop(Foes, /*bWithHounds*/ true));
		return Path;
	}
}

#endif
