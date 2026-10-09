#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/Style/LooterUIStyle.h"
#include "UI/World/CreatureHealthBarWidget.h"

// The creature tag's rank-sting pop: the rank word starts a quarter bigger and in the accent color and settles to its own
// size and color in about 0.6 s. The pop is a pure function of the sting's age (UCreaturePackComponent::GetRankStingAge),
// so the numbers are tested here without a creature.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureRankFlashTest, "Looter.Creatures.RankFlash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureRankFlashTest::RunTest(const FString& Parameters)
{
	using UBar = UCreatureHealthBarWidget;
	const FLinearColor Rank(0.2f, 0.45f, 1.f, 1.f);

	TestNearlyEqual(TEXT("A pop of about 0.6 s"), UBar::RankFlashSeconds, 0.6f, 0.001f);
	TestNearlyEqual(TEXT("...a quarter bigger at its peak"), UBar::RankFlashPeakScale, 1.25f, 0.001f);

	// At the sting: the peak and the accent color.
	TestNearlyEqual(TEXT("Peak size at the sting"), UBar::RankFlashScaleAt(0.f), UBar::RankFlashPeakScale, 0.001f);
	TestTrue(TEXT("Accent color at the sting"), UBar::RankFlashColorAt(0.f, Rank).Equals(LooterUI::Color::Accent(), 0.001f));

	// Settling: it only gets smaller and more its own, and is home by the end.
	float LastScale = UBar::RankFlashScaleAt(0.f);
	float LastGap = FLinearColor::Dist(UBar::RankFlashColorAt(0.f, Rank), Rank);
	for (int32 Step = 1; Step <= 12; ++Step)
	{
		const float Age = UBar::RankFlashSeconds * Step / 12.f;
		const float Scale = UBar::RankFlashScaleAt(Age);
		const float Gap = FLinearColor::Dist(UBar::RankFlashColorAt(Age, Rank), Rank);
		TestTrue(FString::Printf(TEXT("%.2f s: not bigger than before"), Age), Scale <= LastScale + 0.0001f);
		TestTrue(FString::Printf(TEXT("%.2f s: no farther from its color"), Age), Gap <= LastGap + 0.0001f);
		TestTrue(FString::Printf(TEXT("%.2f s: between its size and the peak"), Age), Scale >= 1.f && Scale <= UBar::RankFlashPeakScale);
		LastScale = Scale;
		LastGap = Gap;
	}
	TestNearlyEqual(TEXT("Its own size at the end"), UBar::RankFlashScaleAt(UBar::RankFlashSeconds), 1.f, 0.001f);
	TestTrue(TEXT("Its own color at the end"), UBar::RankFlashColorAt(UBar::RankFlashSeconds, Rank).Equals(Rank, 0.001f));

	// Quick: the size is mostly settled by the pop's middle.
	TestTrue(TEXT("Half-way, most of the size is gone"), UBar::RankFlashScaleAt(UBar::RankFlashSeconds * 0.5f) < 1.f + (UBar::RankFlashPeakScale - 1.f) * 0.25f);

	// Never stung (a large age, or none) or not yet: nothing happens to the word.
	TestNearlyEqual(TEXT("Never stung: its own size"), UBar::RankFlashScaleAt(TNumericLimits<float>::Max()), 1.f, 0.001f);
	TestTrue(TEXT("Never stung: its own color"), UBar::RankFlashColorAt(TNumericLimits<float>::Max(), Rank).Equals(Rank, 0.001f));
	TestNearlyEqual(TEXT("A negative age (none): its own size"), UBar::RankFlashScaleAt(-1.f), 1.f, 0.001f);
	TestTrue(TEXT("A negative age (none): its own color"), UBar::RankFlashColorAt(-1.f, Rank).Equals(Rank, 0.001f));
	return true;
}

#endif
