#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/TargetDummy.h"
#include "Creatures/SpiderCreature.h"
#include "Progression/LooterProgressSave.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Progression/ProgressionSettings.h"
#include "Progression/XPCurve.h"
#include "Kismet/GameplayStatics.h"

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
	const ULooterProgressSave* Fresh = GetDefault<ULooterProgressSave>();
	TestEqual(TEXT("New game level"), Fresh->Level, 1);
	TestEqual(TEXT("New game XP"), Fresh->XP, int64(0));

	// Round trip through the save format, in memory (the player's real save slot stays untouched).
	ULooterProgressSave* Save = NewObject<ULooterProgressSave>();
	Save->Version = ULooterProgressSave::CurrentVersion;
	Save->Level = 42;
	Save->XP = 123456;
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
	TestEqual(TEXT("Version"), Loaded->Version, ULooterProgressSave::CurrentVersion);
	TestEqual(TEXT("Level"), Loaded->Level, 42);
	TestEqual(TEXT("XP"), Loaded->XP, int64(123456));
	AddInfo(FString::Printf(TEXT("Save size: %d bytes"), Bytes.Num()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKillXPTest, "Looter.Progression.KillXP",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKillXPTest::RunTest(const FString& Parameters)
{
	// Tutorial island creatures are level 1 and give 10 XP a kill; target dummies are for practice and give none.
	const ASpiderCreature* Spider = GetDefault<ASpiderCreature>();
	TestEqual(TEXT("Spider level"), Spider->Level, 1);
	TestEqual(TEXT("Spider XP reward"), Spider->XPReward, 10);
	TestEqual(TEXT("Killing a spider"), UPlayerProgressionSubsystem::KillXP(Spider), int64(10));
	TestEqual(TEXT("Killing a dummy"), UPlayerProgressionSubsystem::KillXP(GetDefault<ATargetDummy>()), int64(0));
	TestEqual(TEXT("Killing nothing"), UPlayerProgressionSubsystem::KillXP(nullptr), int64(0));
	return true;
}

#endif
