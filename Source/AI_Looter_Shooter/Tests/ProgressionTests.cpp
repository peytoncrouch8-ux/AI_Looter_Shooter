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
	// The game's rules (the user's, 2026-10-13: "make it take more XP to level up", Borderlands' way, the top level 65):
	// reaching level L takes 180 x (L^2.8 - 1) in all, Borderlands 2's curve (60 x (L^2.8 - 1)) three times over. Each
	// level-up takes more than the last, by more each time.
	const FXPCurve Curve = GetDefault<UProgressionSettings>()->GetCurve();
	TestEqual(TEXT("Max level"), Curve.MaxLevel, 65);
	TestEqual(TEXT("Level 1 to 2"), Curve.XPToNextLevel(1), int64(1074));
	TestEqual(TEXT("Total to level 10"), Curve.TotalXPToReach(10), int64(113392));
	TestEqual(TEXT("Total to level 65, the top"), Curve.TotalXPToReach(65), int64(21449949));
	for (int32 Level = 2; Level <= Curve.MaxLevel; ++Level)
	{
		// Three times Borderlands 2's total at every level, give or take the rounding of both.
		const double Borderlands = FMath::RoundToDouble(60.0 * (FMath::Pow(static_cast<double>(Level), 2.8) - 1.0));
		TestTrue(FString::Printf(TEXT("Level %d: three times Borderlands 2's experience"), Level),
			FMath::Abs(static_cast<double>(Curve.TotalXPToReach(Level)) - 3.0 * Borderlands) <= 2.0);
	}
	for (int32 Level = 1; Level + 1 < Curve.MaxLevel; ++Level)
	{
		const int64 Step = Curve.XPToNextLevel(Level);
		const int64 Next = Curve.XPToNextLevel(Level + 1);
		TestTrue(FString::Printf(TEXT("Level %d takes more than level %d"), Level + 1, Level), Next > Step);
		TestEqual(FString::Printf(TEXT("Level %d's step adds up to the totals"), Level), Curve.TotalXPToReach(Level) + Step,
			Curve.TotalXPToReach(Level + 1));
	}
	// Steeper than linear: the increase itself keeps growing.
	TestTrue(TEXT("Increases grow"), Curve.XPToNextLevel(60) - Curve.XPToNextLevel(59) > Curve.XPToNextLevel(2) - Curve.XPToNextLevel(1));

	TestEqual(TEXT("Nothing past the max level"), Curve.XPToNextLevel(65), int64(0));
	TestEqual(TEXT("Nothing past the max level (above)"), Curve.XPToNextLevel(66), int64(0));
	TestEqual(TEXT("Total to level 1"), Curve.TotalXPToReach(1), int64(0));
	TestEqual(TEXT("Total to level 2"), Curve.TotalXPToReach(2), int64(1074));
	TestEqual(TEXT("Total to level 3"), Curve.TotalXPToReach(3), Curve.XPToNextLevel(1) + Curve.XPToNextLevel(2));
	TestEqual(TEXT("Total stops at the max level"), Curve.TotalXPToReach(80), Curve.TotalXPToReach(65));
	TestEqual(TEXT("Bar is full at the max level"), Curve.LevelProgress(65, 0), 1.f);
	TestEqual(TEXT("Bar is half full"), Curve.LevelProgress(1, 537), 0.5f);

	// The table, for tuning.
	for (const int32 Level : { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 50, 64 })
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
	const FXPCurve Curve;  // the shipped defaults: 180 x (L^2.8 - 1), 65 levels
	int32 Level = 1;
	int64 XP = 0;

	TestEqual(TEXT("Half a level: no level-up"), Curve.ApplyXP(Level, XP, 537), 0);
	TestEqual(TEXT("Half a level: level"), Level, 1);
	TestEqual(TEXT("Half a level: XP"), XP, int64(537));
	TestEqual(TEXT("Exactly to level 2"), Curve.ApplyXP(Level, XP, 537), 1);
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
	Level = 64;
	XP = 0;
	TestEqual(TEXT("To the max level"), Curve.ApplyXP(Level, XP, Curve.XPToNextLevel(64) * 3), 1);
	TestEqual(TEXT("Max level reached"), Level, 65);
	TestEqual(TEXT("No XP at the max level"), XP, int64(0));
	TestEqual(TEXT("No more levels"), Curve.ApplyXP(Level, XP, 1000000), 0);
	TestEqual(TEXT("Still the max level"), Level, 65);
	TestEqual(TEXT("Still no XP"), XP, int64(0));
	Level = 1;
	XP = 0;
	TestEqual(TEXT("Everything at once"), Curve.ApplyXP(Level, XP, TNumericLimits<int64>::Max()), 64);
	TestEqual(TEXT("Everything at once: level"), Level, 65);

	// Saves from an older curve keep their level, inside the current rules (Docs/Progression.md: a player saved on the
	// old curve, 100 XP for level 2 and up to 70, keeps their level and the XP they had into it, now a smaller part of a
	// bigger bar; one past 65 comes back to 65).
	Level = 6;
	XP = 150;
	Curve.Clamp(Level, XP);
	TestEqual(TEXT("An old save's level 6 stays level 6"), Level, 6);
	TestEqual(TEXT("...with its XP into the level"), XP, int64(150));
	Level = 0;
	XP = -5;
	Curve.Clamp(Level, XP);
	TestEqual(TEXT("Level below 1"), Level, 1);
	TestEqual(TEXT("Negative XP"), XP, int64(0));
	Level = 70;
	XP = 500;
	Curve.Clamp(Level, XP);
	TestEqual(TEXT("Level above the max (an old save at 70)"), Level, 65);
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
	// Level 1 creatures are worth 10 (their XPReward); a kill gives its share of that (the progression settings' kill scale:
	// 1 as shipped, so 10, a Borderlands early kill; the user's call of 2026-10-13, tuned with the turn-ins and the curve for
	// Ransom's Rest's story alone to end about level 5.5 and everything there about 6.8, Docs/Progression.md).
	// Target dummies are for practice and give none.
	const ASpiderCreature* Spider = GetDefault<ASpiderCreature>();
	const FLevelRules GameRules = UPlayerProgressionSubsystem::GetLevelRules();
	TestEqual(TEXT("Spider level"), Spider->Level, 1);
	TestEqual(TEXT("Spider XP reward"), Spider->XPReward, 10);
	TestEqual(TEXT("Killing a level 1 spider at level 1: its share of 10"), UPlayerProgressionSubsystem::KillXP(Spider, 1),
		FMath::Max<int64>(1, FMath::RoundToInt64(10.0 * GameRules.KillXPScale)));
	TestEqual(TEXT("The shipped kill rules: 10 for a level 1 Basic kill"), FLevelRules().KillXP(10, 1, 1), int64(10));
	TestEqual(TEXT("...69 for a level 5 one"), FLevelRules().KillXP(10, 5, 5), int64(69));
	TestEqual(TEXT("...and 2,066 for Abel (a Boss's 200 at his level 7)"), FLevelRules().KillXP(200, 7, 7), int64(2066));
	TestEqual(TEXT("Killing a dummy"), UPlayerProgressionSubsystem::KillXP(GetDefault<ATargetDummy>(), 1), int64(0));
	TestEqual(TEXT("Killing nothing"), UPlayerProgressionSubsystem::KillXP(nullptr, 1), int64(0));

	// The formula (Docs/Progression.md, "Kills"), at the full XPReward (its own rules, so tuning the settings can't move
	// these): 10 x level^1.2, rounded.
	FLevelRules Rules;
	Rules.KillXPScale = 1.0;
	Rules.KillXPPower = 1.2;
	TestEqual(TEXT("A level 2 kill (22.97)"), Rules.KillXP(10, 2, 2), int64(23));
	TestEqual(TEXT("A level 5 kill (68.99)"), Rules.KillXP(10, 5, 5), int64(69));
	TestEqual(TEXT("A level 10 kill (158.5)"), Rules.KillXP(10, 10, 10), int64(158));
	TestEqual(TEXT("A level 20 kill (364.1)"), Rules.KillXP(10, 20, 20), int64(364));
	TestEqual(TEXT("A level 50 kill (1,093.4)"), Rules.KillXP(10, 50, 50), int64(1093));

	// The falloff: 15 points of the kill for each level the creature is below the player, never under 10%.
	TestNearlyEqual(TEXT("At the player's level: all of it"), Rules.KillXPShare(9, 9), 1.0, 1e-9);
	TestNearlyEqual(TEXT("One level below: 85%"), Rules.KillXPShare(5, 6), 0.85, 1e-9);
	TestNearlyEqual(TEXT("Six levels below: the 10% floor"), Rules.KillXPShare(5, 11), 0.1, 1e-9);
	TestNearlyEqual(TEXT("Forty below: still 10%"), Rules.KillXPShare(20, 60), 0.1, 1e-9);
	TestEqual(TEXT("A level 5 kill one level below (58.6)"), Rules.KillXP(10, 5, 6), int64(59));
	TestEqual(TEXT("Two below (48.3)"), Rules.KillXP(10, 5, 7), int64(48));
	TestEqual(TEXT("Three below (37.9)"), Rules.KillXP(10, 5, 8), int64(38));
	TestEqual(TEXT("Six below (6.9)"), Rules.KillXP(10, 5, 11), int64(7));
	TestEqual(TEXT("A level 20 kill forty below (36.4)"), Rules.KillXP(10, 20, 60), int64(36));
	TestEqual(TEXT("Above the player: its level's growth, no more (85.9)"), Rules.KillXP(10, 6, 5), int64(86));
	TestEqual(TEXT("Any kill that's worth something gives at least 1"), Rules.KillXP(1, 1, 65), int64(1));
	TestEqual(TEXT("Nothing from nothing"), Rules.KillXP(0, 30, 1), int64(0));

	// Kills per level at the player's level grow slowly, since the curve's steps grow about as level^1.8 and the kill as
	// level^1.2: about 107 Basic kills for level 1, three times that at level 20, five times at 50 (about 600 at 60). The
	// turn-ins carry most of a level, as in Borderlands, and every level takes at least as many kills as the one before.
	const FXPCurve Curve;
	const FLevelRules Shipped;
	auto KillsFor = [&Curve, &Shipped](int32 Level)
	{
		return static_cast<double>(Curve.XPToNextLevel(Level))
			/ (10.0 * Shipped.KillXPScale * FMath::Pow(static_cast<double>(Level), Shipped.KillXPPower));
	};
	TestTrue(FString::Printf(TEXT("Level 1 takes about 107 kills (%.1f)"), KillsFor(1)), FMath::Abs(KillsFor(1) - 107.0) <= 5.0);
	for (const TPair<int32, double>& Expected : { TPair<int32, double>(20, 3.0), TPair<int32, double>(50, 5.0) })
	{
		const double Times = KillsFor(Expected.Key) / KillsFor(1);
		TestTrue(FString::Printf(TEXT("Level %d takes about %.0f times level 1's kills (%.2f)"), Expected.Key, Expected.Value, Times),
			FMath::Abs(Times - Expected.Value) <= Expected.Value * 0.1);
	}
	for (int32 Level = 1; Level + 1 < Curve.MaxLevel; ++Level)
	{
		TestTrue(FString::Printf(TEXT("Level %d takes at least level %d's kills"), Level + 1, Level), KillsFor(Level + 1) >= KillsFor(Level));
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
	TestNearlyEqual(TEXT("Level 65, the top: x6.12"), Rules.PlayerHealthScale(65), 6.12f, 1e-4f);
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

// Ransom's Rest's pacing (the story alone ending about level 5.5, everything about 6.8) is ProgressionPacingTests.cpp's.

#endif
