#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudMissionCompleteWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The HUD's mission-complete banner: what it says (the mission's name and what it gave), that it takes turns with the
// level-up banner (a turn-in's level-up waits for it; it waits for one showing), and that it announces the story's
// missions as the runner finishes them, not the tutorial's.

using namespace MissionTestWorld;

namespace
{
	/** The level-up banner is on screen: its root shows only while it plays. */
	bool IsUp(const UHudLevelUpBannerWidget& Banner)
	{
		const UWidget* Root = Banner.GetRootWidget();
		return Root && Root->GetVisibility() != ESlateVisibility::Collapsed;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionCompleteBannerTest, "Looter.UI.MissionComplete.Banner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionCompleteBannerTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UHudMissionCompleteWidget* Banner = CreateWidget<UHudMissionCompleteWidget>(World, UHudMissionCompleteWidget::StaticClass());
	UHudLevelUpBannerWidget* LevelUp = CreateWidget<UHudLevelUpBannerWidget>(World, UHudLevelUpBannerWidget::StaticClass());
	if (!TestNotNull(TEXT("The banner"), Banner) || !TestNotNull(TEXT("The level-up banner"), LevelUp))
	{
		return false;
	}
	// Their Slate widgets held for the test, as a viewport holds them in play: one let go destructs its widget at once.
	const TSharedRef<SWidget> BannerSlate = Banner->TakeWidget();
	const TSharedRef<SWidget> LevelUpSlate = LevelUp->TakeWidget();
	Banner->SetLevelUpBanner(LevelUp);
	TestFalse(TEXT("Nothing to announce: idle, hidden"), Banner->IsBusy() || Banner->IsShowing());
	TestFalse(TEXT("A level-up while it's idle shows at once"), Banner->DeferLevelUp(2));

	// A main mission turned in: its name and what it gave, in the order the banner says them.
	UMissionDefinition* Mission = NewMission(CreatePackage(nullptr), TEXT("TestHomeward"), EMissionKind::Main, EMissionStart::Automatic, NAME_None);
	Mission->Title = FText::FromString(TEXT("The Long Way Home"));
	FMissionRewardsGiven Given;
	Given.Experience = 40;
	Given.bGun = true;
	Given.GunRarity = EWeaponRarity::Rare;
	Given.NamedGun = FText::FromString(TEXT("Heirloom"));
	Given.AreasOpened = { FText::FromString(TEXT("The Gilded Lily")) };
	Banner->Announce(*Mission, Given);
	TestTrue(TEXT("Announced: busy"), Banner->IsBusy());
	TestTrue(TEXT("A level-up from the turn-in waits for it"), Banner->DeferLevelUp(4));
	Banner->Advance(0.1f);
	TestTrue(TEXT("It shows"), Banner->IsShowing());
	TestEqual(TEXT("The mission's name, in capitals"), Banner->GetShownName(), FString(TEXT("THE LONG WAY HOME")));
	const TArray<FString> Rewards = Banner->GetShownRewards();
	if (TestEqual(TEXT("A reward each"), Rewards.Num(), 4))
	{
		TestEqual(TEXT("The experience"), Rewards[0], FString(TEXT("+40 XP")));
		TestEqual(TEXT("The gun's rarity"), Rewards[1], FString(TEXT("RARE GUN")));
		TestEqual(TEXT("The named gun"), Rewards[2], FString(TEXT("HEIRLOOM")));
		TestEqual(TEXT("The area opened"), Rewards[3], FString(TEXT("OPENS THE GILDED LILY")));
	}
	TestFalse(TEXT("The level-up banner waits meanwhile"), IsUp(*LevelUp));
	Banner->Advance(UHudMissionCompleteWidget::ShowSeconds);
	TestFalse(TEXT("Its time over: gone"), Banner->IsShowing() || Banner->IsBusy());
	TestTrue(TEXT("...and the level-up it kept shows now"), IsUp(*LevelUp));

	// While the level-up banner shows, the next mission's end waits for it.
	Banner->Announce(*Mission, FMissionRewardsGiven());
	Banner->Advance(0.1f);
	TestTrue(TEXT("Another mission's end waits while the level-up shows"), Banner->IsBusy() && !Banner->IsShowing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionCompleteHeardTest, "Looter.UI.MissionComplete.Heard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionCompleteHeardTest::RunTest(const FString& Parameters)
{
	// The banner hears the level's mission runner: a story mission's end is announced (with nothing when it gave nothing),
	// the tutorial's isn't (it has its own closing line).
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	UHudMissionCompleteWidget* Banner = CreateWidget<UHudMissionCompleteWidget>(World, UHudMissionCompleteWidget::StaticClass());
	if (!Runner || !Player || !TestNotNull(TEXT("The banner"), Banner))
	{
		return false;
	}
	// Held as a viewport holds it; the banner hears the runner from its creation either way (NativeOnInitialized).
	const TSharedRef<SWidget> BannerSlate = Banner->TakeWidget();
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Tutorial = NewMission(Scratch, TEXT("TestPractice"), EMissionKind::Tutorial, EMissionStart::Manual, NAME_None);
	AddObjective<UMissionEventObjective>(Tutorial, 0)->Event = TEXT("Test.Practised");
	UMissionDefinition* Story = NewMission(Scratch, TEXT("TestChapter"), EMissionKind::Side, EMissionStart::Manual, NAME_None);
	AddObjective<UMissionEventObjective>(Story, 0)->Event = TEXT("Test.Told");
	Story->Rewards.Experience = 30;
	Runner->BeginForTesting({ Tutorial, Story }, Campaign, Player);
	Runner->Update(0.f);

	Runner->CompleteMission(TEXT("TestPractice"));
	TestFalse(TEXT("The tutorial's end isn't announced"), Banner->IsBusy());
	Runner->CompleteMission(TEXT("TestChapter"));
	TestTrue(TEXT("A story mission's end is"), Banner->IsBusy());
	Banner->Advance(0.1f);
	TestTrue(TEXT("...with its experience"), Banner->GetShownRewards() == TArray<FString>({ TEXT("+30 XP") }));
	return true;
}

#endif
