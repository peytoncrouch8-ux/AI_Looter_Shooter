#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/BossComponent.h"
#include "Tests/BossTestWorld.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

// A beaten boss's bar leaves (the play-through of 2026-10-14: Abel's "He kneels" stayed on the HUD for good, seen 190 m away
// after a fast travel): when its boss stops ticking inside the bar's last moment (Abel sat on his board as his kneel ends),
// and wherever the player is far from the fight.

using namespace BossTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossBarTickOffTest, "Looter.Bosses.Won.TickOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossBarTickOffTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ACharacter* Player = nullptr;
	UBossComponent* Boss = StartTestFight(*this, WorldWrapper.GetTestWorld(), Player);
	if (!Boss)
	{
		return false;
	}
	TestTrue(TEXT("Its bar is up for the fight"), Boss->IsBarShown());

	Hurt(Boss->GetCreature(), 1.0e7f);
	TestTrue(TEXT("Won: its bar shows it beaten a moment"), Boss->IsWon() && Boss->IsBarShown());
	Boss->TickFight(UBossComponent::BarHideDelay * 0.5f);
	TestTrue(TEXT("...still up halfway through that moment"), Boss->IsBarShown());

	// Abel's way out of the story as his kneel ends: his boss's tick switched off (AAbelKeeper::SetPresent), so nothing
	// would count the moment down any more.
	Boss->SetComponentTickEnabled(false);
	TestFalse(TEXT("Its tick switched off: the bar goes at once"), Boss->IsBarShown());
	Boss->SetComponentTickEnabled(true);
	Boss->TickFight(UBossComponent::BarHideDelay);
	TestFalse(TEXT("...and doesn't come back when it ticks again"), Boss->IsBarShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossBarFarAwayTest, "Looter.Bosses.Won.FarAway",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossBarFarAwayTest::RunTest(const FString& Parameters)
{
	// The bar shows only near its fight (UBossComponent hands it its spot and BarReach): in the arena and anywhere a fight
	// is fought from, never across the map.
	const FVector Spot(1000.0, -2000.0, 300.0);
	const float Reach = UBossComponent::BarReach;
	TestTrue(TEXT("In the arena: shown"), UHudBossBarWidget::IsNearArena(Spot + FVector(1500.0, 800.0, 0.0), Spot, Reach));
	TestTrue(TEXT("Just inside its reach: shown"), UHudBossBarWidget::IsNearArena(Spot + FVector(Reach - 10.0, 0.0, 0.0), Spot, Reach));
	TestFalse(TEXT("190 m away (the play-through's fast travel): stepped aside"),
		UHudBossBarWidget::IsNearArena(Spot + FVector(19000.0, 0.0, 0.0), Spot, Reach));
	TestFalse(TEXT("Far under it (a mine below the arena): stepped aside"),
		UHudBossBarWidget::IsNearArena(Spot - FVector(0.0, 0.0, Reach + 100.0), Spot, Reach));
	TestTrue(TEXT("No reach given: anywhere"), UHudBossBarWidget::IsNearArena(Spot + FVector(1.0e6, 0.0, 0.0), Spot, 0.f));
	// Every fight's ground lies well inside it: a leash's 25 m, the test boss's ring, Abel's deck.
	const UBossComponent* Defaults = GetDefault<UBossComponent>();
	TestTrue(TEXT("The reach is far past any arena"), Reach >= 3.f * FMath::Max(Defaults->LeashRadius, BossTestSpider::SealRadius));
	return true;
}

#endif
