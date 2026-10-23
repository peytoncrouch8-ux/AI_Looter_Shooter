#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bounties/BountyRules.h"
#include "Bounties/BountyTuning.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreatureTraitRules.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Economy/EconomyRules.h"
#include "Loot/Chest.h"
#include "Progression/LevelRules.h"
#include "Progression/XPCurve.h"

// The bounty board's rules without a level (Docs/Work/2026-10-22-Next/BOUNTY.md): its kinds, tiers and twists, the draw,
// the timer, the fight's groups, the pay and the pacing band, the cards' words.

namespace
{
	FBountyArenaChoice Arena(const TCHAR* Id, TArray<FName> Kinds, int32 Low = 5, int32 High = 6)
	{
		FBountyArenaChoice Made;
		Made.Id = Id;
		Made.Kinds = MoveTemp(Kinds);
		Made.LevelLow = Low;
		Made.LevelHigh = High;
		return Made;
	}

	FBountyDrawInput DrawInput(int32 Serial, EBountyTier Highest)
	{
		FBountyDrawInput In;
		In.Arenas = { Arena(TEXT("Quarry"), { TEXT("Spider"), TEXT("Spitter"), TEXT("DeadMiner") }),
			Arena(TEXT("Scatter"), { TEXT("Unpaid") }, 2, 3), Arena(TEXT("Webwood"), { TEXT("Spider"), TEXT("Spitter") }) };
		In.HighestTier = Highest;
		In.Serial = Serial;
		In.Seed = BountyRules::SeedFor(TEXT("TestBoard"), Serial);
		return In;
	}

	int32 CountTier(const FBountyFightPlan& Plan, ECreatureRank Rank, int32 Wave = 0)
	{
		int32 Count = 0;
		for (const FEncounterGroup& Group : Plan.Groups)
		{
			if (Group.Rank == Rank && (Wave == 0 || Group.FirstWave == Wave))
			{
				Count += Group.Count;
			}
		}
		return Count;
	}

	/** A bounty's kills at its arena's top level for a player at PlayerLevel, every creature its fight brings (XP per kill as the game gives it). */
	int64 FightKillXP(const FBountyPosting& Posting, int32 PlayerLevel, const FLevelRules& Rules)
	{
		int64 Sum = 0;
		for (const FEncounterGroup& Group : BountyRules::MakeGroups(Posting).Groups)
		{
			const ACreatureBase* Kind = Group.CreatureClass ? Group.CreatureClass->GetDefaultObject<ACreatureBase>() : nullptr;
			const FCreatureRankInfo& Rank = UCreatureRankSettings::Get(Group.Rank);
			const int32 Base = FMath::RoundToInt32((Kind ? Kind->XPReward : 10) * Rank.XPMultiplier);
			Sum += Group.Count * Rules.KillXP(Base, Posting.LevelHigh + Rank.LevelOffset, PlayerLevel);
		}
		return Sum;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBountyRulesTest, "Looter.Bounty.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBountyRulesTest::RunTest(const FString& Parameters)
{
	using namespace BountyRules;
	// The kinds: the mark, its posse and its support; the dead rise.
	TestEqual(TEXT("Four kinds"), Kinds().Num(), 4);
	const FBountyKindInfo* Unpaid = FindKind(TEXT("Unpaid"));
	const FBountyKindInfo* Spitter = FindKind(TEXT("Spitter"));
	if (!TestNotNull(TEXT("The Unpaid"), Unpaid) || !TestNotNull(TEXT("The Spitter"), Spitter))
	{
		return false;
	}
	TestTrue(TEXT("The Unpaid rise, a Bell-Ringer supports them"), Unpaid->bRises && Unpaid->Mark == AUnpaidCreature::StaticClass() && Unpaid->Support);
	TestTrue(TEXT("A Spitter's posse is spiders"), Spitter->Mark == ASpitterSpider::StaticClass() && Spitter->Posse == ASpiderCreature::StaticClass());
	TestNull(TEXT("No such kind"), FindKind(TEXT("Dragon")));

	// The tiers: their marks' ranks and pushes.
	TestTrue(TEXT("Wanted: a Gravebound mark, two pushes"), GetTier(EBountyTier::Wanted).MarkRank == ECreatureRank::Epic && GetTier(EBountyTier::Wanted).Pushes == 2);
	TestTrue(TEXT("Dangerous: Hallowed, three"), GetTier(EBountyTier::Dangerous).MarkRank == ECreatureRank::Hallowed && GetTier(EBountyTier::Dangerous).Pushes == 3);
	TestTrue(TEXT("Deadly: Ethereal, three"), GetTier(EBountyTier::Deadly).MarkRank == ECreatureRank::Ethereal && GetTier(EBountyTier::Deadly).Pushes == 3);

	// Twists fit their kind as the traits do: never a Fleet or Grave-burst Spitter; a swift posse of spiders fits it.
	TestTrue(TEXT("A Fleet Unpaid"), TwistFits(EBountyTwist::Fleet, *Unpaid));
	TestFalse(TEXT("No Fleet Spitter"), TwistFits(EBountyTwist::Fleet, *Spitter));
	TestFalse(TEXT("No Grave-burst Spitter"), TwistFits(EBountyTwist::GraveBurst, *Spitter));
	TestTrue(TEXT("A Spitter's swift posse (its spiders)"), TwistFits(EBountyTwist::SwiftPosse, *Spitter));
	TestFalse(TEXT("A pack twist has no trait"), TraitOf(EBountyTwist::BigPosse).IsSet());

	// The draw: the same input, the same posting; a busy arena never drawn; none free, none posted.
	const FBountyPosting First = Draw(DrawInput(7, EBountyTier::Deadly));
	const FBountyPosting Again = Draw(DrawInput(7, EBountyTier::Deadly));
	TestTrue(TEXT("Drawn"), First.IsPosting() && First.Serial == 7);
	TestEqual(TEXT("The same seed draws the same posting"), Describe(First), Describe(Again));
	FBountyDrawInput Busy = DrawInput(8, EBountyTier::Wanted);
	Busy.BusyArenas = { TEXT("Quarry"), TEXT("Webwood") };
	TestEqual(TEXT("Only the free arena is drawn"), Draw(Busy).Arena, FName(TEXT("Scatter")));
	// Every arena up once: an arena is shared rather than a card left empty (a free one always first), until each holds PerArena.
	Busy.BusyArenas.Add(TEXT("Scatter"));
	TestTrue(TEXT("Every arena busy once: one is shared"), Draw(Busy).IsPosting());
	Busy.BusyArenas = { TEXT("Quarry"), TEXT("Quarry"), TEXT("Webwood") };
	TestEqual(TEXT("The arena with the fewest is drawn (one posting beats two)"), Draw(Busy).Arena, FName(TEXT("Scatter")));
	Busy.BusyArenas = { TEXT("Quarry"), TEXT("Webwood"), TEXT("Scatter"), TEXT("Webwood") };
	TestTrue(TEXT("Shared: not the one with two"), Draw(Busy).Arena != FName(TEXT("Webwood")));
	Busy.PerArena = 1;
	Busy.BusyArenas = { TEXT("Quarry"), TEXT("Webwood"), TEXT("Scatter") };
	TestFalse(TEXT("One to an arena (the old rule): every arena busy, nothing posted"), Draw(Busy).IsPosting());
	// One arena open, three slots: all three filled there, and a fourth would find it full (the default PerArena is three).
	FBountyDrawInput Single = DrawInput(9, EBountyTier::Wanted);
	Single.Arenas = { Arena(TEXT("Scatter"), { TEXT("Unpaid") }, 2, 3) };
	Single.BusyArenas = { TEXT("Scatter"), TEXT("Scatter") };
	TestEqual(TEXT("One arena open: the third slot is drawn there"), Draw(Single).Arena, FName(TEXT("Scatter")));
	Single.BusyArenas.Add(TEXT("Scatter"));
	TestFalse(TEXT("...a fourth is not (PerArena)"), Draw(Single).IsPosting());

	// Over many draws: tiers only up to the story's, each tier's twists, the mark's traits as its rank gives them.
	bool bWantedOnly = true;
	bool bSawEach[3] = { false, false, false };
	bool bTwistsRight = true;
	bool bTraitsRight = true;
	bool bPhasingRight = true;
	for (int32 Serial = 1; Serial <= 300; ++Serial)
	{
		bWantedOnly &= Draw(DrawInput(Serial, EBountyTier::Wanted)).Tier == EBountyTier::Wanted;
		const FBountyPosting Posting = Draw(DrawInput(Serial, EBountyTier::Deadly));
		bSawEach[FBountyTuning::TierIndex(Posting.Tier)] = true;
		const FBountyKindInfo* Kind = FindKind(Posting.Kind);
		int32 MarkTwists = 0;
		for (const EBountyTwist Twist : Posting.Twists)
		{
			bTwistsRight &= Kind && TwistFits(Twist, *Kind);
			MarkTwists += IsPackTwist(Twist) ? 0 : 1;
			if (const TOptional<ECreatureTrait> Trait = TraitOf(Twist))
			{
				bTraitsRight &= Posting.MarkTraits.Contains(*Trait);
			}
		}
		// Wanted one; Dangerous one, and a pack twist a third of the time; Deadly always two.
		const int32 Least = Posting.Tier == EBountyTier::Deadly ? 2 : 1;
		const int32 Most = Posting.Tier == EBountyTier::Wanted ? 1 : 2;
		bTwistsRight &= Posting.Twists.Num() >= Least && Posting.Twists.Num() <= Most;
		const ECreatureRank Rank = GetTier(Posting.Tier).MarkRank;
		bTraitsRight &= Posting.MarkTraits.Num() == MarkTwists + UCreatureRankSettings::Get(Rank).TraitCount;
		bPhasingRight &= Posting.MarkTraits.Contains(ECreatureTrait::Phasing) == (Rank == ECreatureRank::Ethereal);
	}
	TestTrue(TEXT("The story allowing Wanted only: every posting Wanted"), bWantedOnly);
	TestTrue(TEXT("Every tier drawn when all are open"), bSawEach[0] && bSawEach[1] && bSawEach[2]);
	TestTrue(TEXT("Twists fit their kind, as many as the tier gives"), bTwistsRight);
	TestTrue(TEXT("The mark's traits: its twists' and its rank's count more"), bTraitsRight);
	TestTrue(TEXT("Only an Ethereal mark phases"), bPhasingRight);

	// Names: never one up already.
	FBountyDrawInput Names = DrawInput(9, EBountyTier::Wanted);
	Names.Arenas = { Arena(TEXT("Scatter"), { TEXT("Unpaid") }) };
	for (int32 Index = 0; Index < NumNames(TEXT("Unpaid")); ++Index)
	{
		if (Index != 5)
		{
			Names.BusyNames.Add(NameKey(TEXT("Unpaid"), Index));
		}
	}
	TestEqual(TEXT("The one name free is taken"), Draw(Names).NameIndex, 5);

	// The fight's groups: the posse by push, the mark first in the last push, Fixed ranks; a big posse one more a push.
	FBountyPosting Wanted;
	Wanted.Serial = 1;
	Wanted.Kind = TEXT("Unpaid");
	Wanted.Tier = EBountyTier::Wanted;
	const FBountyFightPlan WantedPlan = MakeGroups(Wanted);
	TestEqual(TEXT("Wanted: eight in all"), WantedPlan.Total, 8);
	TestEqual(TEXT("...in two pushes"), WantedPlan.Pushes, 2);
	TestEqual(TEXT("...four in the first"), CountTier(WantedPlan, ECreatureRank::Basic, 1), 4);
	bool bMarkFirst = false;
	bool bFixed = true;
	for (const FEncounterGroup& Group : WantedPlan.Groups)
	{
		bFixed &= Group.RankRoll == EEncounterRankRoll::Fixed;
		if (Group.FirstWave == 2)
		{
			bMarkFirst = Group.Rank == ECreatureRank::Epic && Group.Count == 1;
			break;
		}
	}
	TestTrue(TEXT("...the mark the first of the last push"), bMarkFirst);
	TestTrue(TEXT("...every rank Fixed"), bFixed);
	FBountyPosting Dangerous = Wanted;
	Dangerous.Tier = EBountyTier::Dangerous;
	FBountyPosting Deadly = Wanted;
	Deadly.Tier = EBountyTier::Deadly;
	TestEqual(TEXT("Dangerous: twelve"), MakeGroups(Dangerous).Total, 12);
	TestEqual(TEXT("Deadly: thirteen, an Ethereal mark"), MakeGroups(Deadly).Total, 13);
	TestEqual(TEXT("...one Ethereal"), CountTier(MakeGroups(Deadly), ECreatureRank::Ethereal), 1);
	FBountyPosting Big = Deadly;
	Big.Twists = { EBountyTwist::BigPosse };
	TestEqual(TEXT("A big posse: one more in each of three pushes"), MakeGroups(Big).Total, 16);

	// The timer: a fresh board posts at once; every two hours of play it rotates.
	const float Rotate = FBountyTuning().RotateMinutes;
	TestTrue(TEXT("A fresh board rotates (posts) at once"), IsRotationDue(-1.0, 0.0, Rotate));
	TestFalse(TEXT("Not before its time"), IsRotationDue(100.0, 100.0 + Rotate * 60.0 - 1.0, Rotate));
	TestTrue(TEXT("At its time"), IsRotationDue(100.0, 100.0 + Rotate * 60.0, Rotate));
	TestNearlyEqual(TEXT("The time to the next"), RotationIn(0.0, 600.0, 120.f), 120.0 * 60.0 - 600.0, 0.01);
	TestEqual(TEXT("A wait in words"), DescribeWait(102.0 * 60.0).ToString(), FString(TEXT("1 h 42 min")));
	TestEqual(TEXT("...short"), DescribeWait(7.0 * 60.0).ToString(), FString(TEXT("7 min")));
	TestEqual(TEXT("...shorter"), DescribeWait(20.0).ToString(), FString(TEXT("under a minute")));

	// Names in code, back and forth.
	EBountyTier Tier;
	EBountyTwist Twist;
	TestTrue(TEXT("A tier by name"), ParseTier(TEXT("deadly"), Tier) && Tier == EBountyTier::Deadly);
	TestTrue(TEXT("A twist by its word"), ParseTwist(TEXT("Big posse"), Twist) && Twist == EBountyTwist::BigPosse);
	TestFalse(TEXT("No such twist"), ParseTwist(TEXT("Explosive"), Twist));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBountyRewardsTest, "Looter.Bounty.Rewards", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBountyRewardsTest::RunTest(const FString& Parameters)
{
	using namespace BountyRules;
	const FBountyTuning Tuning;
	const FXPCurve Curve;
	const FLevelRules Rules;
	// The purse: the tier's at level 1, grown as the economy grows.
	TestEqual(TEXT("A Wanted purse at 1"), PurseFor(EBountyTier::Wanted, 1, Tuning), 35);
	TestEqual(TEXT("...at 6, grown"), PurseFor(EBountyTier::Wanted, 6, Tuning), LooterEconomy::AtLevel(35, 6));
	TestTrue(TEXT("Each tier pays more"), PurseFor(EBountyTier::Deadly, 4, Tuning) > PurseFor(EBountyTier::Dangerous, 4, Tuning)
		&& PurseFor(EBountyTier::Dangerous, 4, Tuning) > PurseFor(EBountyTier::Wanted, 4, Tuning));

	// The experience: the tier's share of the step at the arena's level, less the kills' falloff for a player past it.
	const int64 AtLevel = ExperienceFor(EBountyTier::Wanted, 6, 6, Curve, Rules, Tuning);
	TestEqual(TEXT("Wanted at 6: a twentieth of the step"), AtLevel, FMath::RoundToInt64(0.05 * Curve.XPToNextLevel(6)));
	TestEqual(TEXT("A player below the arena gets the same"), ExperienceFor(EBountyTier::Wanted, 6, 4, Curve, Rules, Tuning), AtLevel);
	const int64 Past = ExperienceFor(EBountyTier::Wanted, 6, 9, Curve, Rules, Tuning);
	TestEqual(TEXT("Three levels past it: the kills' falloff"), Past, FMath::RoundToInt64(0.05 * Curve.XPToNextLevel(6) * Rules.KillXPShare(6, 9)));
	TestTrue(TEXT("...less, never nothing"), Past < AtLevel && Past > 0);
	TestEqual(TEXT("Nothing at the top level"), ExperienceFor(EBountyTier::Deadly, 6, Curve.MaxLevel, Curve, Rules, Tuning), static_cast<int64>(0));

	// The strongbox: the tier's guns at a Strongbox's luck, two ammo pickups, no mote.
	const FChestLootOverride Box = ChestLoot(EBountyTier::Deadly, Tuning);
	TestTrue(TEXT("Deadly: three guns at luck 1"), Box.bOverride && Box.Guns == 3 && FMath::IsNearlyEqual(Box.Luck, 1.f) && Box.AmmoPickups == 2);
	TestEqual(TEXT("Wanted: two"), ChestLoot(EBountyTier::Wanted, Tuning).Guns, 2);

	// The cvars start at the tuning's defaults (a fresh game and the tests agree).
	const FBountyTuning Now = FBountyTuning::Current();
	TestTrue(TEXT("The cvars hold the defaults"), Now.bEnabled == Tuning.bEnabled && FMath::IsNearlyEqual(Now.RotateMinutes, Tuning.RotateMinutes)
		&& Now.Purse[2] == Tuning.Purse[2] && FMath::IsNearlyEqual(Now.XPShare[1], Tuning.XPShare[1]) && Now.RewardGuns[2] == Tuning.RewardGuns[2]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBountyPacingTest, "Looter.Bounty.Pacing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBountyPacingTest::RunTest(const FString& Parameters)
{
	using namespace BountyRules;
	// Bounties are optional and repeat, so they stay out of Looter.Progression.Pacing's paths (as the vigil stones'
	// rematches do); here each pays the side-content share: its turn-in a side job's (0.04-0.10 of the level where it's met),
	// and with every kill of its fight no more than about a third of the player's level when its tier opens.
	const FBountyTuning Tuning;
	const FXPCurve Curve;
	const FLevelRules Rules;
	bool bTurnInBand = true;
	for (const EBountyTier Tier : { EBountyTier::Wanted, EBountyTier::Dangerous, EBountyTier::Deadly })
	{
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			const double Share = static_cast<double>(ExperienceFor(Tier, Level, Level, Curve, Rules, Tuning)) / Curve.XPToNextLevel(Level);
			bTurnInBand &= Share >= 0.04 && Share <= 0.10;
		}
	}
	TestTrue(TEXT("Every tier's turn-in is 4-10% of the level where it's met"), bTurnInBand);

	// The whole fight, at Ransom's Rest's arenas (tops 3-6), for a player at least where the tier opens (Main 3 leaves about
	// 2.7, Main 5 about 4.8, Main 6 about 6.6: Docs/Progression.md).
	const int32 Opens[] = { 3, 5, 6 };
	double Worst = 0.0;
	for (const EBountyTier Tier : { EBountyTier::Wanted, EBountyTier::Dangerous, EBountyTier::Deadly })
	{
		for (const TCHAR* Kind : { TEXT("Unpaid"), TEXT("Spider"), TEXT("DeadMiner") })
		{
			for (int32 ArenaTop = 3; ArenaTop <= 6; ++ArenaTop)
			{
				FBountyPosting Posting;
				Posting.Serial = 1;
				Posting.Kind = Kind;
				Posting.Tier = Tier;
				Posting.LevelLow = ArenaTop - 1;
				Posting.LevelHigh = ArenaTop;
				const int32 Player = FMath::Max(ArenaTop, Opens[FBountyTuning::TierIndex(Tier)]);
				const double All = static_cast<double>(FightKillXP(Posting, Player, Rules) + ExperienceFor(Tier, ArenaTop, Player, Curve, Rules, Tuning));
				Worst = FMath::Max(Worst, All / Curve.XPToNextLevel(Player));
			}
		}
	}
	AddInfo(FString::Printf(TEXT("The richest bounty all told: %.1f%% of the player's level."), Worst * 100.0));
	TestTrue(TEXT("No bounty, kills and all, is more than 35% of a level"), Worst <= 0.35);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBountyViewTest, "Looter.Bounty.View", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBountyViewTest::RunTest(const FString& Parameters)
{
	using namespace BountyRules;
	// An empty slot says when the next goes up, or that more come with the story.
	FBountyCardInput Empty;
	Empty.RefillIn = 7.0 * 60.0;
	const FBountyCardView Waiting = MakeCard(Empty);
	TestTrue(TEXT("Empty, waiting"), Waiting.bEmpty && Waiting.EmptyLine.ToString().Contains(TEXT("7 min")));
	Empty.RefillIn = 0.0;
	TestTrue(TEXT("Empty, waiting on the story"), MakeCard(Empty).EmptyLine.ToString().Contains(TEXT("Reaches")));

	// A posting: its words, NEW until read, takeable unless another is under way.
	FBountyPosting Posting;
	Posting.Serial = 3;
	Posting.Kind = TEXT("Spider");
	Posting.Tier = EBountyTier::Dangerous;
	Posting.Arena = TEXT("Webwood");
	Posting.LevelLow = 5;
	Posting.LevelHigh = 6;
	Posting.NameIndex = 0;
	Posting.Twists = { EBountyTwist::Warded, EBountyTwist::SwiftPosse };
	Posting.MarkTraits = { ECreatureTrait::Warded, ECreatureTrait::Mending, ECreatureTrait::Vengeful };
	FBountyCardInput In;
	In.Posting = &Posting;
	In.ArenaName = FText::FromString(TEXT("the Webwood"));
	In.Purse = 77;
	In.Experience = 1027;
	In.Guns = 2;
	const FBountyCardView Card = MakeCard(In);
	TestEqual(TEXT("The mark's name"), Card.Title.ToString(), MarkName(TEXT("Spider"), 0).ToString());
	TestEqual(TEXT("Its tier"), Card.TierWord.ToString(), FString(TEXT("Dangerous")));
	TestTrue(TEXT("Its rank, kind and levels (its rank's two on top)"), Card.KindLine.ToString().Contains(TEXT("Hallowed spider"))
		&& Card.KindLine.ToString().Contains(TEXT("level 7-8")));
	TestTrue(TEXT("Its traits"), Card.Traits.ToString().Contains(CreatureTraitRules::Word(ECreatureTrait::Warded).ToString()));
	TestTrue(TEXT("Its pack twist"), Card.Posse.ToString().Contains(TEXT("Swift posse")));
	TestEqual(TEXT("Where"), Card.Where.ToString(), FString(TEXT("Last seen: the Webwood")));
	TestTrue(TEXT("What it pays"), Card.Reward.ToString().Contains(TEXT("Strongbox, 2 guns or wards")) && Card.Reward.ToString().Contains(TEXT("+77 silver"))
		&& Card.Reward.ToString().Contains(TEXT("XP")));
	// The box's gun picks may come as wards (a Strongbox's table drops them): the words never promise guns alone.
	TestEqual(TEXT("One pick: a gun or a ward"), StrongboxWords(1).ToString(), FString(TEXT("Strongbox, 1 gun or ward")));
	TestEqual(TEXT("Three picks"), StrongboxWords(3).ToString(), FString(TEXT("Strongbox, 3 guns or wards")));
	// Paid: the box is only said to wait while it's still owed (the play-test's board said so after it was looted).
	TestTrue(TEXT("Paid, box still shut"), PaidLine(242, 41, true).ToString().Contains(TEXT("strongbox waits")));
	TestTrue(TEXT("Paid, box looted"), !PaidLine(242, 41, false).ToString().Contains(TEXT("strongbox"))
		&& PaidLine(242, 41, false).ToString().Contains(TEXT("+242 XP")) && PaidLine(242, 41, false).ToString().Contains(TEXT("+41 silver")));
	TestTrue(TEXT("New, to take"), Card.bNew && Card.bCanTake && !Card.bCanCollect && !Card.bCanGiveUp);
	In.bSeen = true;
	In.bOtherTaken = true;
	const FBountyCardView Read = MakeCard(In);
	TestTrue(TEXT("Read, another under way: not new, not takeable"), !Read.bNew && !Read.bCanTake && Read.StateWord.IsEmpty());
	Posting.State = EBountyState::Taken;
	TestTrue(TEXT("Taken: to give up"), MakeCard(In).bCanGiveUp && MakeCard(In).StateWord.ToString() == TEXT("Taken"));
	Posting.State = EBountyState::Won;
	TestTrue(TEXT("Won: to collect"), MakeCard(In).bCanCollect && !MakeCard(In).bCanGiveUp);
	TestEqual(TEXT("Its title on the tracker"), TitleFor(Posting).ToString(), FString::Printf(TEXT("Bounty: %s"), *MarkName(TEXT("Spider"), 0).ToString()));
	return true;
}

#endif
