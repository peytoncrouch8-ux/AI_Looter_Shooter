#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "UI/World/XpPopupActor.h"
#include "UI/World/XpPopupSubsystem.h"
#include "UI/World/XpPopupWidget.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"

// Experience shown where it's earned (concept F, "XP popups"; Looter.UI.XpPopup.*): what a popup says, how it moves (a
// kill's beside its spot and rising, a turn-in's slamming in big), and the pool: used again once done, never more than
// MaxPopups (the oldest taken over), none for a victim whose damage numbers are hidden or for nothing earned.

namespace
{
	/** Moves a popup on Seconds in steps, as its tick would. */
	void XpPopupRun(AXpPopupActor& Popup, float Seconds)
	{
		constexpr float XpPopupStep = 1.f / 30.f;
		for (float Time = 0.f; Time < Seconds; Time += XpPopupStep)
		{
			Popup.Advance(XpPopupStep);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXpPopupWordsTest, "Looter.UI.XpPopup.Words",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FXpPopupWordsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("A kill's"), UXpPopupWidget::Words(38), FString(TEXT("+38 XP")));
	TestEqual(TEXT("Never a minus"), UXpPopupWidget::Words(-5), FString(TEXT("+0 XP")));
	const FString Big = UXpPopupWidget::Words(1240);
	TestTrue(FString::Printf(TEXT("A turn-in's, its thousands grouped as the game writes numbers (%s)"), *Big),
		Big.StartsWith(TEXT("+1")) && Big.EndsWith(TEXT("240 XP")));

	// A kill's: beside its spot, rising, fading in and out; a turn-in's: slams in big and settles.
	const AXpPopupActor::FLook Start = AXpPopupActor::LookAt(0.f, false);
	const AXpPopupActor::FLook Middle = AXpPopupActor::LookAt(0.5f, false);
	const AXpPopupActor::FLook End = AXpPopupActor::LookAt(AXpPopupActor::SmallSeconds, false);
	TestTrue(TEXT("A kill's fades in"), Start.Opacity < 0.01f && Middle.Opacity > 0.99f);
	TestTrue(TEXT("...pops in a little big"), Start.Scale > 1.f && FMath::IsNearlyEqual(Middle.Scale, 1.f, 0.01f));
	TestTrue(TEXT("...sits beside the damage number"), FMath::IsNearlyEqual(static_cast<float>(Middle.Offset.X), AXpPopupActor::SmallBeside));
	TestTrue(TEXT("...rises"), Middle.Offset.Y < 0.0 && End.Offset.Y < Middle.Offset.Y);
	TestTrue(TEXT("...and is gone by its end"), End.Opacity < 0.01f);
	const AXpPopupActor::FLook BigStart = AXpPopupActor::LookAt(0.f, true);
	const AXpPopupActor::FLook BigSettled = AXpPopupActor::LookAt(0.5f, true);
	TestTrue(TEXT("A turn-in's slams in big and settles, centred"), BigStart.Scale > 1.4f && FMath::IsNearlyEqual(BigSettled.Scale, 1.f, 0.01f)
		&& BigSettled.Offset.X == 0.0);
	TestTrue(TEXT("...and stays up longer"), AXpPopupActor::BigSeconds > AXpPopupActor::SmallSeconds);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXpPopupPoolTest, "Looter.UI.XpPopup.Pool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FXpPopupPoolTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UXpPopupSubsystem* Popups = UXpPopupSubsystem::Get(World);
	if (!TestNotNull(TEXT("The popups"), Popups))
	{
		return false;
	}
	TestNull(TEXT("Nothing earned: nothing shown"), Popups->ShowKillAt(0, FVector::ZeroVector));
	TestNull(TEXT("A turn-in with no player to show it to: nothing"), Popups->ShowMission(500, nullptr, 0.f));

	// A kill's: the right amount, small, a beat after the kill, then gone and kept.
	AXpPopupActor* First = Popups->ShowKillAt(38, FVector(100.0, 0.0, 50.0));
	if (!TestNotNull(TEXT("A kill's popup"), First))
	{
		return false;
	}
	TestEqual(TEXT("It says what was earned"), First->GetWords(), FString(TEXT("+38 XP")));
	TestTrue(TEXT("...small, at the kill"), !First->IsBig() && First->IsShowing() && First->GetActorLocation().Equals(FVector(100.0, 0.0, 50.0)));
	XpPopupRun(*First, UXpPopupSubsystem::KillDelay + AXpPopupActor::SmallSeconds + 0.1f);
	TestFalse(TEXT("Gone after its time"), First->IsShowing());
	TestTrue(TEXT("...but kept, not destroyed"), IsValid(First) && Popups->NumPooled() == 1);
	TestTrue(TEXT("The next kill's uses it again"), Popups->ShowKillAt(12, FVector::ZeroVector) == First && Popups->NumPooled() == 1);
	TestEqual(TEXT("...with its own amount"), First->GetWords(), FString(TEXT("+12 XP")));

	// A burst of kills: never more than MaxPopups; the oldest is taken over when all are up.
	AXpPopupActor* Last = nullptr;
	for (int32 Kill = 0; Kill < UXpPopupSubsystem::MaxPopups + 4; ++Kill)
	{
		if (AXpPopupActor* Shown = Popups->ShowKillAt(10 + Kill, FVector(Kill * 50.0, 0.0, 0.0)))
		{
			Last = Shown;
			XpPopupRun(*Shown, 0.05f);
		}
	}
	TestEqual(TEXT("At most MaxPopups made"), Popups->NumPooled(), UXpPopupSubsystem::MaxPopups);
	TestEqual(TEXT("...all up"), Popups->NumShowing(), UXpPopupSubsystem::MaxPopups);
	TestTrue(TEXT("...the latest kill's shown"), Last && Last->GetWords() == UXpPopupWidget::Words(10 + UXpPopupSubsystem::MaxPopups + 3));

	// A turn-in's: big.
	AXpPopupActor* TurnIn = Popups->ShowMissionAt(1240, FVector(0.0, 0.0, 150.0), 0.55f);
	TestTrue(TEXT("A turn-in's popup is the big one"), TurnIn && TurnIn->IsBig() && TurnIn->GetWords() == UXpPopupWidget::Words(1240));

	// With the damage numbers: a victim whose numbers are hidden (a boss between phases) shows none.
	AActor* Victim = World->SpawnActor<AActor>();
	UHealthComponent* Health = Victim ? NewObject<UHealthComponent>(Victim, TEXT("Health")) : nullptr;
	if (TestNotNull(TEXT("A victim with health"), Health))
	{
		Health->bShowDamageNumbers = false;
		TestNull(TEXT("Its numbers hidden: no popup"), Popups->ShowKill(40, Victim));
		Health->bShowDamageNumbers = true;
		TestNotNull(TEXT("Its numbers shown: a popup"), Popups->ShowKill(40, Victim));
	}
	return true;
}

#endif
