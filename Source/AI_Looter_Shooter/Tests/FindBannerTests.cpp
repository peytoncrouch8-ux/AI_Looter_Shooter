#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionRewards.h"
#include "Tests/MissionTestWorld.h"
#include "World/FindRules.h"
#include "UI/HUD/HudFindBannerWidget.h"
#include "UI/HUD/HudGraveMarkedWidget.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudMissionCompleteWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The HUD's "FOUND" banner (FINDS.md section 3): its easing is a pure function of the time into the show; its words (the title,
// the "4 / 15 · RANSOM'S REST" count line, "LEDGER PAGE · 2 / 6", the experience counting up); finds close together show one
// after the other and a crowd collapses into one "3 FOUND"; it waits for the level-up, mission-complete and roost banners and
// the roost banner waits for it.

using namespace MissionTestWorld;

namespace
{
	/** A find as the subsystem hands it over: the words FindRules makes. */
	FFindBannerShow MakeFind(const TCHAR* Name, int32 Found, int32 Xp = 100, bool bLore = false)
	{
		FFindBannerShow Find;
		Find.Name = FText::FromString(Name);
		Find.Count = bLore ? FindRules::LoreLine(Found, 6) : FindRules::CountLine(Found, 15, FText::FromString(TEXT("Ransom's Rest")));
		Find.Xp = Xp;
		return Find;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFindBannerEaseTest, "Looter.UI.FindBanner.Ease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFindBannerEaseTest::RunTest(const FString& Parameters)
{
	using Banner = UHudFindBannerWidget;
	TestTrue(TEXT("About three seconds"), FMath::IsNearlyEqual(Banner::ShowSeconds, 2.8f, 0.3f));
	TestTrue(TEXT("Invisible before it and after it"), Banner::OpacityAt(-0.1f) == 0.f && Banner::OpacityAt(Banner::ShowSeconds) == 0.f);
	TestTrue(TEXT("Starting from nothing"), Banner::OpacityAt(0.f) == 0.f);
	TestTrue(TEXT("Eases in: more opaque as it goes"), Banner::OpacityAt(0.1f) > 0.f && Banner::OpacityAt(0.1f) < Banner::OpacityAt(0.2f));
	TestTrue(TEXT("Solid once in, held through the middle"), Banner::OpacityAt(0.4f) == 1.f && Banner::OpacityAt(1.5f) == 1.f);
	float Last = 1.f;
	bool bEasingOut = true;
	for (float Time = 2.0f; Time < Banner::ShowSeconds; Time += 0.05f)
	{
		const float Opacity = Banner::OpacityAt(Time);
		bEasingOut = bEasingOut && Opacity <= Last + 0.0001f;
		Last = Opacity;
	}
	TestTrue(TEXT("Eases out and only down"), bEasingOut && Last < 0.1f);
	TestTrue(TEXT("It pops in from smaller, swells a little past full and settles"), Banner::ScaleAt(0.f) < 1.f && Banner::ScaleAt(0.3f) > 1.f
		&& Banner::ScaleAt(0.5f) == 1.f && Banner::ScaleAt(2.f) == 1.f);

	// The experience counts up from nothing to all of it, never down.
	TestEqual(TEXT("No experience counted at first"), Banner::XpAt(120, 0.2f), 0);
	TestEqual(TEXT("All of it once the count ends"), Banner::XpAt(120, 2.f), 120);
	int32 Previous = 0;
	bool bCountsUp = true;
	for (float Time = 0.f; Time < Banner::ShowSeconds; Time += 0.05f)
	{
		const int32 Xp = Banner::XpAt(120, Time);
		bCountsUp = bCountsUp && Xp >= Previous && Xp <= 120;
		Previous = Xp;
	}
	TestTrue(TEXT("Only up, never past the amount"), bCountsUp);
	TestEqual(TEXT("A find that pays nothing counts nothing"), Banner::XpAt(0, 1.5f), 0);

	// The words.
	TestEqual(TEXT("Title"), Banner::TitleWords(MakeFind(TEXT("Box"), 4)), FString(TEXT("FOUND")));
	TestEqual(TEXT("Count line"), Banner::CountWords(MakeFind(TEXT("Box"), 4)), FString(TEXT("4 / 15 · RANSOM'S REST")));
	TestEqual(TEXT("A ledger page counts apart"), Banner::CountWords(MakeFind(TEXT("Page"), 2, 40, true)), FString(TEXT("LEDGER PAGE · 2 / 6")));
	TestEqual(TEXT("Experience line"), Banner::XpWords(120), FString(TEXT("+120 XP")));
	TestEqual(TEXT("A thousand has its comma"), Banner::XpWords(1000), FString(TEXT("+1,000 XP")));
	TestTrue(TEXT("No experience: no line"), Banner::XpWords(0).IsEmpty());
	// The area's last find names its own title, set smaller to fit between the rules; a short one keeps the full size.
	FFindBannerShow Last15 = MakeFind(TEXT(""), 15, 1000);
	Last15.Title = FindRules::CompleteTitle(FText::FromString(TEXT("Ransom's Rest")));
	TestEqual(TEXT("The last find: its own title"), Banner::TitleWords(Last15), FString(TEXT("EVERY FIND IN RANSOM'S REST")));
	TestTrue(TEXT("A long title is set smaller"), Banner::TitleSizeFor(Banner::TitleWords(Last15)) < Banner::TitleSizeFor(TEXT("FOUND"))
		&& Banner::TitleSizeFor(Banner::TitleWords(Last15)) >= 17);
	TestEqual(TEXT("A short title is full size"), Banner::TitleSizeFor(TEXT("FOUND")), 30);

	// Many as one: the latest count, everyone's experience.
	const FFindBannerShow Many = Banner::Merge({ MakeFind(TEXT("A"), 1, 100), MakeFind(TEXT("B"), 2, 120), MakeFind(TEXT("C"), 3, 140) });
	TestEqual(TEXT("Merged: three"), Many.Merged, 3);
	TestEqual(TEXT("Merged: the latest count line"), Banner::CountWords(Many), FString(TEXT("3 / 15 · RANSOM'S REST")));
	TestEqual(TEXT("Merged: all the experience"), Many.Xp, 360);
	TestEqual(TEXT("Merged: \"3 FOUND\""), Banner::TitleWords(Many), FString(TEXT("3 FOUND")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFindBannerQueueTest, "Looter.UI.FindBanner.Queue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFindBannerQueueTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UHudFindBannerWidget* Banner = CreateWidget<UHudFindBannerWidget>(World, UHudFindBannerWidget::StaticClass());
	if (!TestNotNull(TEXT("The banner"), Banner))
	{
		return false;
	}
	// Its Slate widget held for the test, as a viewport holds it in play: one let go destructs its widget at once.
	const TSharedRef<SWidget> BannerSlate = Banner->TakeWidget();
	TestFalse(TEXT("Nothing announced: idle"), Banner->IsBusy() || Banner->IsShowing());

	// A find: its words, the experience counting up, gone after its time.
	Banner->Announce(MakeFind(TEXT("The Widow's Strongbox"), 1, 120));
	TestTrue(TEXT("Announced: busy"), Banner->IsBusy());
	Banner->Advance(0.1f);
	TestTrue(TEXT("It shows"), Banner->IsShowing());
	TestEqual(TEXT("Title"), Banner->GetShownTitle(), FString(TEXT("FOUND")));
	TestEqual(TEXT("The name, in capitals"), Banner->GetShownName(), FString(TEXT("THE WIDOW'S STRONGBOX")));
	TestEqual(TEXT("The count line"), Banner->GetShownCount(), FString(TEXT("1 / 15 · RANSOM'S REST")));
	TestTrue(TEXT("No experience yet"), Banner->GetShownXp().IsEmpty());
	Banner->Advance(UHudFindBannerWidget::ShowSeconds * 0.6f);
	TestEqual(TEXT("Well in: all the experience counted"), Banner->GetShownXp(), FString(TEXT("+120 XP")));
	TestTrue(TEXT("Still showing"), Banner->IsShowing());
	Banner->Advance(UHudFindBannerWidget::ShowSeconds);
	TestFalse(TEXT("Its time over: gone"), Banner->IsShowing() || Banner->IsBusy());

	// Two finds close together: one after the other, none lost.
	Banner->Announce(MakeFind(TEXT("The Gang's Locker"), 2));
	Banner->Announce(MakeFind(TEXT("The Surveyor's Tin"), 3));
	Banner->Advance(0.1f);
	TestTrue(TEXT("The first shows, the second waits"), Banner->IsShowing() && Banner->GetQueued() == 1
		&& Banner->GetShownName() == TEXT("THE GANG'S LOCKER"));
	Banner->Advance(UHudFindBannerWidget::ShowSeconds);
	Banner->Advance(0.1f);
	TestTrue(TEXT("Then the second"), Banner->IsShowing() && Banner->GetShownName() == TEXT("THE SURVEYOR'S TIN"));
	Banner->Advance(UHudFindBannerWidget::ShowSeconds);
	TestFalse(TEXT("Then nothing"), Banner->IsBusy());

	// A ledger page: its own count.
	Banner->Announce(MakeFind(TEXT("Ledger page"), 2, 40, true));
	Banner->Advance(0.1f);
	TestEqual(TEXT("A ledger page"), Banner->GetShownCount(), FString(TEXT("LEDGER PAGE · 2 / 6")));
	Banner->Advance(UHudFindBannerWidget::ShowSeconds);

	// A crowd (an old save's finds, a console mark): more than two waiting become one "3 FOUND".
	Banner->Announce(MakeFind(TEXT("One"), 1, 100));
	Banner->Advance(0.1f);
	TestTrue(TEXT("One showing"), Banner->IsShowing());
	Banner->Announce(MakeFind(TEXT("Two"), 2, 100));
	Banner->Announce(MakeFind(TEXT("Three"), 3, 100));
	TestEqual(TEXT("Two waiting: still two"), Banner->GetQueued(), 2);
	Banner->Announce(MakeFind(TEXT("Four"), 4, 100));
	TestEqual(TEXT("A third waiting: collapsed to one"), Banner->GetQueued(), 1);
	Banner->Advance(UHudFindBannerWidget::ShowSeconds);
	Banner->Advance(0.1f);
	TestEqual(TEXT("It says 3 FOUND"), Banner->GetShownTitle(), FString(TEXT("3 FOUND")));
	TestEqual(TEXT("...at the latest count"), Banner->GetShownCount(), FString(TEXT("4 / 15 · RANSOM'S REST")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFindBannerWaitsTest, "Looter.UI.FindBanner.Waits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFindBannerWaitsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UHudFindBannerWidget* Banner = CreateWidget<UHudFindBannerWidget>(World, UHudFindBannerWidget::StaticClass());
	UHudLevelUpBannerWidget* LevelUp = CreateWidget<UHudLevelUpBannerWidget>(World, UHudLevelUpBannerWidget::StaticClass());
	UHudMissionCompleteWidget* Mission = CreateWidget<UHudMissionCompleteWidget>(World, UHudMissionCompleteWidget::StaticClass());
	UHudGraveMarkedWidget* Roost = CreateWidget<UHudGraveMarkedWidget>(World, UHudGraveMarkedWidget::StaticClass());
	if (!TestTrue(TEXT("The four banners"), Banner && LevelUp && Mission && Roost))
	{
		return false;
	}
	const TSharedRef<SWidget> BannerSlate = Banner->TakeWidget();
	const TSharedRef<SWidget> LevelUpSlate = LevelUp->TakeWidget();
	const TSharedRef<SWidget> MissionSlate = Mission->TakeWidget();
	const TSharedRef<SWidget> RoostSlate = Roost->TakeWidget();
	Banner->SetOtherBanners(LevelUp, Mission, Roost);
	Roost->SetOtherBanners(LevelUp, Mission, Banner);

	// The level-up banner showing: it waits, and the find is kept.
	LevelUp->Show(3);
	Banner->Announce(MakeFind(TEXT("Waiting on the level-up"), 1));
	Banner->Advance(0.1f);
	TestTrue(TEXT("Waits while the level-up banner shows"), Banner->IsBusy() && !Banner->IsShowing() && Banner->GetQueued() == 1);
	// The viewport ticks the level-up banner, not this test: it stays up, so the find keeps waiting.
	Banner->Advance(UHudFindBannerWidget::ShowSeconds);
	TestTrue(TEXT("...and keeps waiting while it stays up"), !Banner->IsShowing() && Banner->GetQueued() == 1);

	// A mission turned in: the find waits for it.
	UHudFindBannerWidget* Second = CreateWidget<UHudFindBannerWidget>(World, UHudFindBannerWidget::StaticClass());
	const TSharedRef<SWidget> SecondSlate = Second->TakeWidget();
	Second->SetOtherBanners(nullptr, Mission, Roost);
	UMissionDefinition* Turned = NewMission(CreatePackage(nullptr), TEXT("TestFindWait"), EMissionKind::Main, EMissionStart::Automatic, NAME_None);
	Turned->Title = FText::FromString(TEXT("A Mission"));
	Mission->Announce(*Turned, FMissionRewardsGiven());
	Mission->Advance(0.1f);
	Second->Announce(MakeFind(TEXT("Waiting on the mission"), 1));
	Second->Advance(0.1f);
	TestTrue(TEXT("Waits while the mission-complete banner shows"), Second->IsBusy() && !Second->IsShowing());
	Mission->Advance(UHudMissionCompleteWidget::ShowSeconds * 2.f);
	Second->Advance(0.1f);
	TestTrue(TEXT("...and shows when it is done"), Second->IsShowing());

	// The roost banner: it waits for a find on screen, and a find waits for a roost on screen.
	UHudFindBannerWidget* Third = CreateWidget<UHudFindBannerWidget>(World, UHudFindBannerWidget::StaticClass());
	UHudGraveMarkedWidget* RoostTwo = CreateWidget<UHudGraveMarkedWidget>(World, UHudGraveMarkedWidget::StaticClass());
	const TSharedRef<SWidget> ThirdSlate = Third->TakeWidget();
	const TSharedRef<SWidget> RoostTwoSlate = RoostTwo->TakeWidget();
	Third->SetOtherBanners(nullptr, nullptr, RoostTwo);
	RoostTwo->SetOtherBanners(nullptr, nullptr, Third);
	RoostTwo->Announce(FText::FromString(TEXT("Family plot")));
	RoostTwo->Advance(0.1f);
	Third->Announce(MakeFind(TEXT("Waiting on the roost"), 1));
	Third->Advance(0.1f);
	TestTrue(TEXT("A find waits while the roost banner shows"), RoostTwo->IsShowing() && Third->IsBusy() && !Third->IsShowing());
	RoostTwo->Advance(UHudGraveMarkedWidget::ShowSeconds);
	Third->Advance(0.1f);
	TestTrue(TEXT("...then shows"), Third->IsShowing());
	RoostTwo->Announce(FText::FromString(TEXT("Chapel yard")));
	RoostTwo->Advance(0.1f);
	TestTrue(TEXT("The roost banner waits while a find shows"), RoostTwo->IsBusy() && !RoostTwo->IsShowing());
	Third->Advance(UHudFindBannerWidget::ShowSeconds);
	RoostTwo->Advance(0.1f);
	TestTrue(TEXT("...then shows"), RoostTwo->IsShowing());
	return true;
}

#endif
