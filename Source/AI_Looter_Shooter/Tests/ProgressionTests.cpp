#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/TargetDummy.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SpiderCreature.h"
#include "Progression/LevelRules.h"
#include "Progression/LooterProgressSave.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Progression/ProgressionSettings.h"
#include "Progression/XPCurve.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXPCurveTest, "Looter.Progression.Curve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FXPCurveTest::RunTest(const FString& Parameters)
{
	// The game's rules (the user's, 2026-09-30): level 1 to 2 takes 100 XP, each level-up takes more than the last by a
	// constant factor (exponential, not linear), and the top level is 70.
	const FXPCurve Curve = GetDefault<UProgressionSettings>()->GetCurve();
	TestEqual(TEXT("Max level"), Curve.MaxLevel, 70);
	TestEqual(TEXT("Level 1 to 2"), Curve.XPToNextLevel(1), int64(100));
	TestTrue(TEXT("Growth over 1"), Curve.Growth > 1.0);

	for (int32 Level = 1; Level + 1 < Curve.MaxLevel; ++Level)
	{
		const int64 Step = Curve.XPToNextLevel(Level);
		const int64 Next = Curve.XPToNextLevel(Level + 1);
		TestTrue(FString::Printf(TEXT("Level %d takes more than level %d"), Level + 1, Level), Next > Step);
		// Each step is the last one times the growth, give or take the rounding of both.
		TestTrue(FString::Printf(TEXT("Level %d grows by the factor"), Level + 1),
			FMath::Abs(static_cast<double>(Next) - Step * Curve.Growth) <= 0.5 + 0.5 * Curve.Growth);
	}
	// Exponential, not linear: the increase itself keeps growing.
	TestTrue(TEXT("Increases grow"), Curve.XPToNextLevel(60) - Curve.XPToNextLevel(59) > Curve.XPToNextLevel(2) - Curve.XPToNextLevel(1));

	TestEqual(TEXT("Nothing past the max level"), Curve.XPToNextLevel(70), int64(0));
	TestEqual(TEXT("Nothing past the max level (above)"), Curve.XPToNextLevel(71), int64(0));
	TestEqual(TEXT("Total to level 1"), Curve.TotalXPToReach(1), int64(0));
	TestEqual(TEXT("Total to level 2"), Curve.TotalXPToReach(2), int64(100));
	TestEqual(TEXT("Total to level 3"), Curve.TotalXPToReach(3), Curve.XPToNextLevel(1) + Curve.XPToNextLevel(2));
	TestEqual(TEXT("Total stops at the max level"), Curve.TotalXPToReach(80), Curve.TotalXPToReach(70));
	TestEqual(TEXT("Bar is full at the max level"), Curve.LevelProgress(70, 0), 1.f);
	TestEqual(TEXT("Bar is half full"), Curve.LevelProgress(1, 50), 0.5f);

	// The table, for tuning.
	for (const int32 Level : { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 50, 69 })
	{
		AddInfo(FString::Printf(TEXT("Level %d -> %d: %lld XP (total to reach %d: %lld)"), Level, Level + 1, Curve.XPToNextLevel(Level),
			Level + 1, Curve.TotalXPToReach(Level + 1)));
	}
	AddInfo(FString::Printf(TEXT("Total to level %d: %lld XP"), Curve.MaxLevel, Curve.TotalXPToReach(Curve.MaxLevel)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXPGainTest, "Looter.Progression.Gain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FXPGainTest::RunTest(const FString& Parameters)
{
	const FXPCurve Curve;  // the shipped defaults: 100 base, 1.12 growth, 70 levels
	int32 Level = 1;
	int64 XP = 0;

	TestEqual(TEXT("Half a level: no level-up"), Curve.ApplyXP(Level, XP, 50), 0);
	TestEqual(TEXT("Half a level: level"), Level, 1);
	TestEqual(TEXT("Half a level: XP"), XP, int64(50));
	TestEqual(TEXT("Exactly to level 2"), Curve.ApplyXP(Level, XP, 50), 1);
	TestEqual(TEXT("Exactly to level 2: level"), Level, 2);
	TestEqual(TEXT("Exactly to level 2: XP"), XP, int64(0));
	TestEqual(TEXT("Nothing for nothing"), Curve.ApplyXP(Level, XP, 0), 0);
	TestEqual(TEXT("Nothing for a negative amount"), Curve.ApplyXP(Level, XP, -40), 0);
	TestEqual(TEXT("Still level 2"), Level, 2);

	// One big gain crosses several levels and keeps the remainder.
	Level = 1;
	XP = 30;
	const int64 Big = Curve.TotalXPToReach(6) - 30 + 17;
	TestEqual(TEXT("Big gain: levels gained"), Curve.ApplyXP(Level, XP, Big), 5);
	TestEqual(TEXT("Big gain: level"), Level, 6);
	TestEqual(TEXT("Big gain: remainder"), XP, int64(17));

	// The top level: the climb ends and experience stops counting.
	Level = 69;
	XP = 0;
	TestEqual(TEXT("To the max level"), Curve.ApplyXP(Level, XP, Curve.XPToNextLevel(69) * 3), 1);
	TestEqual(TEXT("Max level reached"), Level, 70);
	TestEqual(TEXT("No XP at the max level"), XP, int64(0));
	TestEqual(TEXT("No more levels"), Curve.ApplyXP(Level, XP, 1000000), 0);
	TestEqual(TEXT("Still the max level"), Level, 70);
	TestEqual(TEXT("Still no XP"), XP, int64(0));
	Level = 1;
	XP = 0;
	TestEqual(TEXT("Everything at once"), Curve.ApplyXP(Level, XP, TNumericLimits<int64>::Max()), 69);
	TestEqual(TEXT("Everything at once: level"), Level, 70);

	// Saves from an older curve keep their level, inside the current rules.
	Level = 0;
	XP = -5;
	Curve.Clamp(Level, XP);
	TestEqual(TEXT("Level below 1"), Level, 1);
	TestEqual(TEXT("Negative XP"), XP, int64(0));
	Level = 99;
	XP = 500;
	Curve.Clamp(Level, XP);
	TestEqual(TEXT("Level above the max"), Level, 70);
	TestEqual(TEXT("XP at the max level"), XP, int64(0));
	Level = 3;
	XP = 100000;
	Curve.Clamp(Level, XP);
	TestEqual(TEXT("XP over the level's need keeps the level"), Level, 3);
	TestEqual(TEXT("XP over the level's need stops short of it"), XP, Curve.XPToNextLevel(3) - 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgressSaveTest, "Looter.Progression.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FProgressSaveTest::RunTest(const FString& Parameters)
{
	// A new game starts at level 1 with no experience.
	const FPlayerProgressData Fresh;
	TestEqual(TEXT("New game level"), Fresh.Level, 1);
	TestEqual(TEXT("New game XP"), Fresh.XP, int64(0));
	TestTrue(TEXT("New game: nothing met or defeated (every bestiary page reads ???)"), Fresh.Encountered.IsEmpty() && Fresh.Defeated.IsEmpty());

	// The save from before sessions still reads, and becomes a session's progress; one older than version 3 has met
	// whatever it defeated (in memory: the player's real save stays untouched).
	const FString Spider = TEXT("/Script/AI_Looter_Shooter.SpiderCreature");
	ULooterProgressSave* Save = NewObject<ULooterProgressSave>();
	Save->Version = 2;
	Save->Level = 42;
	Save->XP = 123456;
	Save->bTutorialDone = true;
	Save->Defeated.Add(Spider, 3);
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Saved"), UGameplayStatics::SaveGameToMemory(Save, Bytes)))
	{
		return false;
	}
	const ULooterProgressSave* Loaded = Cast<ULooterProgressSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Loaded"), Loaded))
	{
		return false;
	}
	const FPlayerProgressData Progress = Loaded->ToProgress();
	TestEqual(TEXT("Level"), Progress.Level, 42);
	TestEqual(TEXT("XP"), Progress.XP, int64(123456));
	TestTrue(TEXT("Tutorial done"), Progress.bTutorialDone);
	TestEqual(TEXT("Defeat counts"), Progress.Defeated.FindRef(Spider), 3);
	TestTrue(TEXT("Kinds met (from the defeats of an old save)"), Progress.Encountered.Contains(Spider));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKillXPTest, "Looter.Progression.KillXP",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKillXPTest::RunTest(const FString& Parameters)
{
	// Level 1 creatures give 10 XP a kill (the user's number); target dummies are for practice and give none.
	const ASpiderCreature* Spider = GetDefault<ASpiderCreature>();
	TestEqual(TEXT("Spider level"), Spider->Level, 1);
	TestEqual(TEXT("Spider XP reward"), Spider->XPReward, 10);
	TestEqual(TEXT("Killing a level 1 spider at level 1"), UPlayerProgressionSubsystem::KillXP(Spider, 1), int64(10));
	TestEqual(TEXT("Killing a dummy"), UPlayerProgressionSubsystem::KillXP(GetDefault<ATargetDummy>(), 1), int64(0));
	TestEqual(TEXT("Killing nothing"), UPlayerProgressionSubsystem::KillXP(nullptr, 1), int64(0));

	// The formula (Docs/Story.md, "Kill XP"), with the shipped numbers (its own rules, so tuning the settings can't move
	// these): 10 x 1.08^(level - 1), rounded.
	const FLevelRules Rules;
	TestEqual(TEXT("A level 2 kill (10.8)"), Rules.KillXP(10, 2, 2), int64(11));
	TestEqual(TEXT("A level 5 kill (13.6)"), Rules.KillXP(10, 5, 5), int64(14));
	TestEqual(TEXT("A level 10 kill (19.99)"), Rules.KillXP(10, 10, 10), int64(20));
	TestEqual(TEXT("A level 20 kill (43.2)"), Rules.KillXP(10, 20, 20), int64(43));
	TestEqual(TEXT("A level 50 kill (433.7)"), Rules.KillXP(10, 50, 50), int64(434));

	// The falloff: 15 points of the kill for each level the creature is below the player, never under 10%.
	TestNearlyEqual(TEXT("At the player's level: all of it"), Rules.KillXPShare(9, 9), 1.0, 1e-9);
	TestNearlyEqual(TEXT("One level below: 85%"), Rules.KillXPShare(5, 6), 0.85, 1e-9);
	TestNearlyEqual(TEXT("Six levels below: the 10% floor"), Rules.KillXPShare(5, 11), 0.1, 1e-9);
	TestNearlyEqual(TEXT("Forty below: still 10%"), Rules.KillXPShare(20, 60), 0.1, 1e-9);
	TestEqual(TEXT("A level 5 kill one level below (11.6)"), Rules.KillXP(10, 5, 6), int64(12));
	TestEqual(TEXT("Two below (9.5)"), Rules.KillXP(10, 5, 7), int64(10));
	TestEqual(TEXT("Three below (7.5)"), Rules.KillXP(10, 5, 8), int64(7));
	TestEqual(TEXT("Six below (1.4)"), Rules.KillXP(10, 5, 11), int64(1));
	TestEqual(TEXT("A level 20 kill forty below (4.3)"), Rules.KillXP(10, 20, 60), int64(4));
	TestEqual(TEXT("Above the player: its level's growth, no more (14.7)"), Rules.KillXP(10, 6, 5), int64(15));
	TestEqual(TEXT("Any kill that's worth something gives at least 1"), Rules.KillXP(1, 1, 70), int64(1));
	TestEqual(TEXT("Nothing from nothing"), Rules.KillXP(0, 30, 1), int64(0));

	// Kills per level at the player's level grow slowly (Docs/Story.md: 10 at level 1, about 20 at level 20, about 60 at 50),
	// since the curve grows 12% a level and the kill 8%.
	const FXPCurve Curve;
	for (const TPair<int32, double>& Expected : { TPair<int32, double>(1, 10.0), TPair<int32, double>(20, 20.0), TPair<int32, double>(50, 60.0) })
	{
		const double Kills = static_cast<double>(Curve.XPToNextLevel(Expected.Key)) / (10.0 * FMath::Pow(Rules.KillXPGrowth, Expected.Key - 1.0));
		TestTrue(FString::Printf(TEXT("Level %d takes about %.0f kills (%.1f)"), Expected.Key, Expected.Value, Kills),
			FMath::Abs(Kills - Expected.Value) <= Expected.Value * 0.1);
	}

	// A ranked creature's XPReward has its rank's multiplier in it already: the kill counts the rank once.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ASpiderCreature* Restless = WorldWrapper.GetTestWorld()->SpawnActor<ASpiderCreature>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Spider spawned"), Restless))
	{
		return false;
	}
	Restless->StartingRank = ECreatureRank::Rare;
	Restless->DispatchBeginPlay();
	const FCreatureRankInfo& Rare = UCreatureRankSettings::Get(ECreatureRank::Rare);
	const FLevelRules GameRules = UPlayerProgressionSubsystem::GetLevelRules();
	const int32 RareXP = FMath::RoundToInt32(Spider->XPReward * Rare.XPMultiplier);
	TestEqual(TEXT("A Restless spider's experience is its rank's share"), Restless->XPReward, RareXP);
	TestEqual(TEXT("A Restless spider is its rank's levels higher"), Restless->Level, Spider->Level + Rare.LevelOffset);
	const int64 Once = GameRules.KillXP(RareXP, Restless->Level, Restless->Level);
	const int64 Twice = GameRules.KillXP(FMath::RoundToInt32(RareXP * Rare.XPMultiplier), Restless->Level, Restless->Level);
	TestEqual(TEXT("Killing it counts its rank once"), UPlayerProgressionSubsystem::KillXP(Restless, Restless->Level), Once);
	TestTrue(FString::Printf(TEXT("... not twice (%lld, not %lld)"), Once, Twice), Rare.XPMultiplier <= 1.f || Once < Twice);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelHealthTest, "Looter.Progression.LevelHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLevelHealthTest::RunTest(const FString& Parameters)
{
	// The first level-up reward (decided 2026-10-02): +8% of the character's own max health for each level above 1, the
	// pace of the enemies' damage, so a fight at the player's level takes the same hits to lose at every level.
	const FLevelRules Rules;
	TestEqual(TEXT("Level 1: the health the character was made with"), Rules.PlayerHealthScale(1), 1.f);
	TestNearlyEqual(TEXT("Level 2: +8%"), Rules.PlayerHealthScale(2), 1.08f, 1e-5f);
	TestNearlyEqual(TEXT("Level 10: +72%"), Rules.PlayerHealthScale(10), 1.72f, 1e-5f);
	TestNearlyEqual(TEXT("Level 70: x6.52"), Rules.PlayerHealthScale(70), 6.52f, 1e-4f);
	TestNearlyEqual(TEXT("The player's health keeps pace with an enemy's damage"), Rules.PlayerHealthScale(37), Rules.EnemyScale(37), 1e-5f);

	// On a health component (a training dummy's: 500, its class's), from the authored value every time.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ATargetDummy* Body = WorldWrapper.GetTestWorld()->SpawnActor<ATargetDummy>(FVector::ZeroVector, FRotator::ZeroRotator);
	UHealthComponent* Health = Body ? Body->FindComponentByClass<UHealthComponent>() : nullptr;
	if (!TestNotNull(TEXT("A body with health"), Health))
	{
		return false;
	}
	const FLevelRules GameRules = UPlayerProgressionSubsystem::GetLevelRules();
	const float Authored = Health->MaxHealth;
	UPlayerProgressionSubsystem::ApplyLevelHealth(*Health, 6);
	TestNearlyEqual(TEXT("Given before play (as it's possessed): level 6's health"), Health->MaxHealth, Authored * GameRules.PlayerHealthScale(6), 0.01f);
	Body->DispatchBeginPlay();
	TestNearlyEqual(TEXT("It starts full"), Health->GetHealth(), Health->MaxHealth, 0.01f);
	UPlayerProgressionSubsystem::ApplyLevelHealth(*Health, 6);
	TestNearlyEqual(TEXT("Given again: it never compounds"), Health->MaxHealth, Authored * GameRules.PlayerHealthScale(6), 0.01f);

	// A level-up while hurt: the new health comes with it, so the wound stays the same size.
	Health->SetHealth(Health->MaxHealth - 100.f);
	UPlayerProgressionSubsystem::ApplyLevelHealth(*Health, 7);
	const float Level7 = Authored * GameRules.PlayerHealthScale(7);
	TestNearlyEqual(TEXT("Level 7's health"), Health->MaxHealth, Level7, 0.01f);
	TestNearlyEqual(TEXT("The wound is the same size"), Health->GetHealth(), Level7 - 100.f, 0.01f);
	UPlayerProgressionSubsystem::ApplyLevelHealth(*Health, 1);
	TestNearlyEqual(TEXT("Back at level 1 (a reset): the authored health"), Health->MaxHealth, Authored, 0.01f);
	TestNearlyEqual(TEXT("Back at level 1: the same wound"), Health->GetHealth(), Authored - 100.f, 0.01f);
	TestFalse(TEXT("Never dead of it"), Health->IsDead());

	// It's saved through the progress: the level a session saves is all it takes, so a save from before the reward gets
	// its level's health too, and nothing new is written.
	FPlayerProgressData Saved;
	Saved.Level = 12;
	UPlayerProgressionSubsystem::ApplyLevelHealth(*Health, Saved.Level);
	TestNearlyEqual(TEXT("A session saved at level 12: level 12's health"), Health->MaxHealth, Authored * GameRules.PlayerHealthScale(12), 0.01f);
	return true;
}

namespace
{
	/** One main mission on Ransom's Rest (Docs/Areas/RansomsRest.md, "Missions"), with what's killed on the way to it and in it. */
	struct FMainPathStep
	{
		const TCHAR* Mission = TEXT("");
		/** Kills on the way and in its fights, spawned adds included (the boss not). */
		int32 Kills = 0;
		/** Of those, the Restless placed in its scripted fight. */
		int32 PlacedRestless = 0;
		/** Abel Ransom, the Keeper. */
		bool bBoss = false;
	};

	/** About 80 kills in all: the scripted fights (5, 4, 13, the egg sacs' 6, Abel's adds) and the creatures on the way. */
	const FMainPathStep RansomsRestMainPath[] = {
		{ TEXT("Main 1, Seven Days"), 0, 0, false },
		{ TEXT("Main 2, Shall We Talk Business?"), 10, 1, false },
		{ TEXT("Main 3, Cold Welcome"), 10, 1, false },
		{ TEXT("Main 4, Hallowed Ground"), 17, 1, false },
		{ TEXT("Main 5, The Keeper's Lantern"), 14, 0, false },
		{ TEXT("Main 6, The Gravewind"), 18, 0, true },
		{ TEXT("Main 7, The Lantern Leans"), 10, 0, false } };

	/** A main mission's experience (RansomsRest.md, "Missions"): 30% of what the player's current level takes. */
	constexpr double MainMissionShare = 0.3;

	/** A player climbing the curve. */
	struct FClimber
	{
		FXPCurve Curve;
		int32 Level = 1;
		int64 XP = 0;

		void Add(int64 Amount)
		{
			Curve.ApplyXP(Level, XP, Amount);
		}

		/** The level with how far through it, as a number (10.25 is a quarter through level 10). */
		double Reached() const
		{
			const int64 Needed = Curve.XPToNextLevel(Level);
			return Level + (Needed > 0 ? static_cast<double>(XP) / static_cast<double>(Needed) : 0.0);
		}

		int64 MainMissionXP() const
		{
			return FMath::RoundToInt64(MainMissionShare * static_cast<double>(Curve.XPToNextLevel(Level)));
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXPPacingTest, "Looter.Progression.Pacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FXPPacingTest::RunTest(const FString& Parameters)
{
	// RansomsRest.md, Risks, "XP pacing": about 80 main-path kills plus the main missions put a player near level 9 at the
	// end. Spawn counts and mission shares are tuned at step 27; this prints where the game's numbers land.
	const int32 BaseXP = GetDefault<ACreatureBase>()->XPReward;
	const FLevelRules Rules = UPlayerProgressionSubsystem::GetLevelRules();

	// As the design counts it: every kill a Basic creature at the player's level (Abel as one more), and each main mission
	// 30% of the level the player is on.
	FClimber Design;
	Design.Curve = UPlayerProgressionSubsystem::GetCurve();
	int32 Kills = 0;
	for (const FMainPathStep& Step : RansomsRestMainPath)
	{
		for (int32 Kill = 0; Kill < Step.Kills + (Step.bBoss ? 1 : 0); ++Kill)
		{
			Design.Add(Rules.KillXP(BaseXP, Design.Level, Design.Level));
			++Kills;
		}
		Design.Add(Design.MainMissionXP());
		AddInfo(FString::Printf(TEXT("Design: after %s, level %.2f"), Step.Mission, Design.Reached()));
	}
	AddInfo(FString::Printf(TEXT("Design: %d Basic kills at the player's level and 7 main missions end at level %.2f."), Kills, Design.Reached()));
	TestTrue(FString::Printf(TEXT("The main path ends near level 9 (%.2f), inside Ransom's Rest's band"), Design.Reached()),
		Design.Level >= 8 && Design.Level <= 10);

	// The same path as the game plays it, from a fixed seed: levels rolled around the player's inside Ransom's Rest's band
	// (1-10), 8% Restless and 2% Gravebound promotions, the Restless placed in Main 2-4, and Abel at his rank's experience.
	UAreaDefinition* Valley = NewObject<UAreaDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
	Valley->MinLevel = 1;
	Valley->MaxLevel = 10;
	Valley->RarePromotionChance = 0.08f;
	Valley->EpicPromotionChance = 0.02f;
	FRandomStream Random(20261002);
	FClimber Played;
	Played.Curve = Design.Curve;
	int64 FromKills = 0;
	int64 FromMissions = 0;
	int64 FromBoss = 0;
	int32 Ranked = 0;
	for (const FMainPathStep& Step : RansomsRestMainPath)
	{
		for (int32 Kill = 0; Kill < Step.Kills; ++Kill)
		{
			const ECreatureRank Rank = Kill < Step.PlacedRestless ? ECreatureRank::Rare : UAreaRulesSubsystem::RollPromotionIn(Valley, Random);
			const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Rank);
			const int32 KillLevel = UAreaRulesSubsystem::RollLevelIn(Valley, Played.Level, 1, Rank, Random) + Info.LevelOffset;
			const int64 Earned = Rules.KillXP(FMath::RoundToInt32(BaseXP * Info.XPMultiplier), KillLevel, Played.Level);
			Ranked += Rank != ECreatureRank::Basic ? 1 : 0;
			FromKills += Earned;
			Played.Add(Earned);
		}
		if (Step.bBoss)
		{
			const FCreatureRankInfo& Boss = UCreatureRankSettings::Get(ECreatureRank::Boss);
			const int32 BossLevel = UAreaRulesSubsystem::RollLevelIn(Valley, Played.Level, 1, ECreatureRank::Boss, Random) + Boss.LevelOffset;
			const int64 Earned = Rules.KillXP(FMath::RoundToInt32(BaseXP * Boss.XPMultiplier), BossLevel, Played.Level);
			AddInfo(FString::Printf(TEXT("Played: Abel at level %d gives %lld XP to a level %d player"), BossLevel, Earned, Played.Level));
			FromBoss += Earned;
			Played.Add(Earned);
		}
		const int64 MissionXP = Played.MainMissionXP();
		FromMissions += MissionXP;
		Played.Add(MissionXP);
		AddInfo(FString::Printf(TEXT("Played: after %s, level %.2f"), Step.Mission, Played.Reached()));
	}
	AddInfo(FString::Printf(TEXT("Played: %d kills (%d of them ranked) and Abel end at level %.2f: %lld XP from kills, %lld from Abel, %lld from missions. ")
		TEXT("Past the band's top (%d), Ransom's Rest's creatures give less and less, so later kills slow the climb."),
		Kills - 1, Ranked, Played.Reached(), FromKills, FromBoss, FromMissions, Valley->GetBandTop()));
	TestTrue(TEXT("Promotions and placed Restless add to the design's count"), Played.Reached() >= Design.Reached());
	TestTrue(FString::Printf(TEXT("The played path stays within reach of the band (level %d)"), Played.Level), Played.Level <= Valley->GetBandTop() + 4);
	return true;
}

#endif
