#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HUD/HudGraveMarkedWidget.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudMissionCompleteWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The HUD's "GRAVE MARKED" banner (the user's call, 2026-10-13): its easing is a pure function of the time into the show; it
// says the grave's name in capitals, shows for 2.5 s, and shows marks that come close together one after the other.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGraveMarkedBannerEasingTest, "Looter.UI.GraveMarked.Easing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGraveMarkedBannerEasingTest::RunTest(const FString& Parameters)
{
	using Banner = UHudGraveMarkedWidget;
	TestTrue(TEXT("About two and a half seconds"), FMath::IsNearlyEqual(Banner::ShowSeconds, 2.5f, 0.2f));
	TestTrue(TEXT("Invisible before it and after it"), Banner::OpacityAt(-0.1f) == 0.f && Banner::OpacityAt(Banner::ShowSeconds) == 0.f);
	TestTrue(TEXT("Starting from nothing"), Banner::OpacityAt(0.f) == 0.f);
	TestTrue(TEXT("Eases in: more opaque as it goes"), Banner::OpacityAt(0.1f) > 0.f && Banner::OpacityAt(0.1f) < Banner::OpacityAt(0.2f));
	TestTrue(TEXT("Solid once in, held through the middle"), Banner::OpacityAt(0.4f) == 1.f && Banner::OpacityAt(1.2f) == 1.f);
	float Last = 1.f;
	bool bEasingOut = true;
	for (float Time = 1.5f; Time < Banner::ShowSeconds; Time += 0.05f)
	{
		const float Opacity = Banner::OpacityAt(Time);
		bEasingOut = bEasingOut && Opacity <= Last + 0.0001f;
		Last = Opacity;
	}
	TestTrue(TEXT("Eases out and only down"), bEasingOut && Last < 0.1f);
	TestTrue(TEXT("It pops in from smaller, swells a little past full and settles"), Banner::ScaleAt(0.f) < 1.f && Banner::ScaleAt(0.3f) > 1.f
		&& Banner::ScaleAt(0.5f) == 1.f && Banner::ScaleAt(2.f) == 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGraveMarkedBannerShowTest, "Looter.UI.GraveMarked.Show",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGraveMarkedBannerShowTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UHudGraveMarkedWidget* Banner = CreateWidget<UHudGraveMarkedWidget>(World, UHudGraveMarkedWidget::StaticClass());
	UHudLevelUpBannerWidget* LevelUp = CreateWidget<UHudLevelUpBannerWidget>(World, UHudLevelUpBannerWidget::StaticClass());
	UHudMissionCompleteWidget* Mission = CreateWidget<UHudMissionCompleteWidget>(World, UHudMissionCompleteWidget::StaticClass());
	if (!TestNotNull(TEXT("The banner"), Banner) || !TestNotNull(TEXT("The level-up banner"), LevelUp) || !TestNotNull(TEXT("The mission banner"), Mission))
	{
		return false;
	}
	// Their Slate widgets held for the test, as a viewport holds them in play: one let go destructs its widget at once.
	const TSharedRef<SWidget> BannerSlate = Banner->TakeWidget();
	const TSharedRef<SWidget> LevelUpSlate = LevelUp->TakeWidget();
	const TSharedRef<SWidget> MissionSlate = Mission->TakeWidget();
	Banner->SetOtherBanners(LevelUp, Mission);
	TestFalse(TEXT("Nothing announced: idle"), Banner->IsBusy() || Banner->IsShowing());

	// A grave marked: its name in capitals.
	Banner->Announce(FText::FromString(TEXT("Family plot")));
	TestTrue(TEXT("Announced: busy"), Banner->IsBusy());
	Banner->Advance(0.1f);
	TestTrue(TEXT("It shows"), Banner->IsShowing());
	TestEqual(TEXT("The grave's name, in capitals"), Banner->GetShownName(), FString(TEXT("FAMILY PLOT")));
	Banner->Advance(UHudGraveMarkedWidget::ShowSeconds * 0.5f);
	TestTrue(TEXT("Still showing halfway"), Banner->IsShowing());
	Banner->Advance(UHudGraveMarkedWidget::ShowSeconds);
	TestFalse(TEXT("Its time over: gone"), Banner->IsShowing() || Banner->IsBusy());

	// Two marks close together: one after the other, none lost.
	Banner->Announce(FText::FromString(TEXT("Chapel yard")));
	Banner->Announce(FText::FromString(TEXT("Keeper's grave")));
	Banner->Advance(0.1f);
	TestTrue(TEXT("The first shows, the second waits"), Banner->IsShowing() && Banner->GetQueued() == 1
		&& Banner->GetShownName() == TEXT("CHAPEL YARD"));
	Banner->Advance(UHudGraveMarkedWidget::ShowSeconds);
	Banner->Advance(0.1f);
	TestTrue(TEXT("Then the second"), Banner->IsShowing() && Banner->GetShownName() == TEXT("KEEPER'S GRAVE"));
	Banner->Advance(UHudGraveMarkedWidget::ShowSeconds);
	TestFalse(TEXT("Then nothing"), Banner->IsBusy());

	// The level-up banner showing: it waits for it, and the mark is kept.
	LevelUp->Show(3);
	Banner->Announce(FText::FromString(TEXT("Boot hill")));
	Banner->Advance(0.1f);
	TestTrue(TEXT("Waits while the level-up banner shows"), Banner->IsBusy() && !Banner->IsShowing() && Banner->GetQueued() == 1);
	return true;
}

#endif
