#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/Menus/StartupLogosRules.h"
#include "UI/Menus/StartupLogosSettings.h"
#include "UI/Menus/StartupLogosWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"

// The startup cards before the main menu (the user, 2026-10-27: creator and software logos on opening; any key skips one
// card, the first launch shows them all): their clock, what a key does, and the cards built as the settings say.

namespace
{
	/** Steps the clock Seconds on, a frame at a time. */
	void RunLogos(FStartupLogos& Logos, float Seconds)
	{
		constexpr float Frame = 1.f / 60.f;
		for (float Time = 0.f; Time < Seconds - 1e-4f; Time += Frame)
		{
			Logos.Tick(Frame);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStartupLogosTimingTest, "Looter.UI.StartupLogos.Timing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStartupLogosTimingTest::RunTest(const FString& Parameters)
{
	const FStartupLogoTiming Timing;
	FStartupLogos Logos;
	Logos.Start(2, true, Timing);
	TestTrue(TEXT("Starts dark, before the first card"), Logos.GetPhase() == EStartupLogoPhase::LeadIn && Logos.GetCardOpacity() == 0.f
		&& Logos.GetCover() == 1.f);
	TestEqual(TEXT("...no sting yet"), Logos.TakeStartedCard(), int32(INDEX_NONE));

	RunLogos(Logos, Timing.LeadIn + Timing.FadeIn * 0.5f);
	TestEqual(TEXT("The first card fades in"), Logos.GetCard(), 0);
	TestTrue(TEXT("...partway"), Logos.GetCardOpacity() > 0.05f && Logos.GetCardOpacity() < 0.95f);
	TestEqual(TEXT("...its sting due once"), Logos.TakeStartedCard(), 0);
	TestEqual(TEXT("...and only once"), Logos.TakeStartedCard(), int32(INDEX_NONE));

	RunLogos(Logos, Timing.FadeIn * 0.5f + Timing.Hold * 0.5f);
	TestTrue(TEXT("It holds at full"), Logos.GetPhase() == EStartupLogoPhase::Hold && Logos.GetCardOpacity() == 1.f);

	RunLogos(Logos, Timing.Hold * 0.5f + Timing.FadeOut + Timing.Gap * 0.5f);
	TestTrue(TEXT("Dark between the cards"), Logos.GetPhase() == EStartupLogoPhase::Gap && Logos.GetCardOpacity() == 0.f
		&& Logos.GetCover() == 1.f);
	TestEqual(TEXT("...the next one coming"), Logos.GetCard(), 1);

	RunLogos(Logos, Timing.Gap * 0.5f + Timing.FadeIn + 0.05f);
	TestEqual(TEXT("The second card, its sting due"), Logos.TakeStartedCard(), 1);
	RunLogos(Logos, Timing.Hold + Timing.FadeOut + Timing.Reveal * 0.5f);
	TestTrue(TEXT("Then the cover fades off the menu"), Logos.IsRevealing() && !Logos.IsDone() && Logos.GetCard() == INDEX_NONE
		&& Logos.GetCover() > 0.f && Logos.GetCover() < 1.f);
	RunLogos(Logos, Timing.Reveal * 0.5f + 0.05f);
	TestTrue(TEXT("...and they're done"), Logos.IsDone() && Logos.GetCover() == 0.f);

	// The whole run, as FullLength says.
	FStartupLogos Whole;
	Whole.Start(2, true, Timing);
	RunLogos(Whole, FStartupLogos::FullLength(2, Timing) - 0.1f);
	TestFalse(TEXT("Not done before its length"), Whole.IsDone());
	RunLogos(Whole, 0.15f);
	TestTrue(TEXT("Done at its length"), Whole.IsDone());

	// No cards: straight to the cover's fade; zero lengths never stick.
	FStartupLogos None;
	None.Start(0, true, Timing);
	TestTrue(TEXT("No cards: the cover fades at once"), None.IsRevealing() && None.GetCard() == INDEX_NONE);
	FStartupLogoTiming Zero;
	Zero.LeadIn = Zero.FadeIn = Zero.Hold = Zero.FadeOut = Zero.Gap = Zero.Reveal = 0.f;
	FStartupLogos Instant;
	Instant.Start(3, true, Zero);
	TestTrue(TEXT("Zero lengths: done at once"), Instant.IsDone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStartupLogosSkipTest, "Looter.UI.StartupLogos.Skip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStartupLogosSkipTest::RunTest(const FString& Parameters)
{
	const FStartupLogoTiming Timing;

	// The first launch shows every card in full: keys do nothing.
	FStartupLogos First;
	First.Start(2, false, Timing);
	RunLogos(First, Timing.LeadIn + Timing.FadeIn + 0.5f);
	TestFalse(TEXT("First launch: a key skips nothing"), First.Skip());
	TestTrue(TEXT("...the card holds"), First.GetPhase() == EStartupLogoPhase::Hold && First.GetCard() == 0);

	FStartupLogos Logos;
	Logos.Start(2, true, Timing);
	TestFalse(TEXT("A key in the lead-in (held from launching) skips nothing"), Logos.Skip());
	RunLogos(Logos, Timing.LeadIn + Timing.MinShown * 0.5f);
	TestFalse(TEXT("...nor in a card's first moments"), Logos.Skip());
	RunLogos(Logos, Timing.MinShown);
	const float Before = Logos.GetCardOpacity();
	TestTrue(TEXT("After that a key cuts the card short"), Logos.Skip());
	TestTrue(TEXT("...it fades from where it was"), Logos.GetPhase() == EStartupLogoPhase::FadeOut
		&& FMath::IsNearlyEqual(Logos.GetCardOpacity(), Before, 0.05f));
	TestFalse(TEXT("One press, one card: a second press while it fades does nothing"), Logos.Skip());
	RunLogos(Logos, Timing.SkipFadeOut + 0.02f);
	TestTrue(TEXT("...quickly gone, the next card coming"), Logos.GetPhase() == EStartupLogoPhase::Gap && Logos.GetCard() == 1);
	TestTrue(TEXT("A key in the dark skips the card coming"), Logos.Skip());
	TestTrue(TEXT("...the last: the cover fades off the menu"), Logos.IsRevealing());
	TestFalse(TEXT("Keys do nothing more"), Logos.Skip());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStartupLogosCardsTest, "Looter.UI.StartupLogos.Cards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStartupLogosCardsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	TestFalse(TEXT("Never during tests (nor -NoLogos, nor PIE unless asked)"), UStartupLogosWidget::ShouldPlay(TestLevel.GetTestWorld()));

	const UStartupLogosSettings& Settings = UStartupLogosSettings::Get();
	TestFalse(TEXT("The studio has a name (a placeholder until the user's)"), Settings.StudioName.IsEmpty());

	UStartupLogosWidget* Cards = CreateWidget<UStartupLogosWidget>(TestLevel.GetTestWorld(), UStartupLogosWidget::StaticClass());
	if (!TestNotNull(TEXT("The startup cards"), Cards))
	{
		return false;
	}
	const TSharedRef<SWidget> CardsSlate = Cards->TakeWidget();
	TestEqual(TEXT("A card for each one the settings show"), Cards->NumCards(), int32(Settings.bShowStudioCard) + int32(Settings.bShowEngineCard));
	if (Settings.bShowStudioCard)
	{
		TestTrue(TEXT("...the studio's name on its card"), Cards->GetStudioNameText()
			&& Cards->GetStudioNameText()->GetText().ToString() == Settings.StudioName.ToUpper());
	}
	const bool bLogoImported = FPackageName::DoesPackageExist(Settings.EngineLogo.GetLongPackageName());
	TestEqual(TEXT("Epic's logo when it's imported, the plain wordmark until then"), Cards->IsShowingEngineLogo(),
		Settings.bShowEngineCard && bLogoImported);

	int32 Revealed = 0;
	int32 Finished = 0;
	Cards->OnRevealing.BindLambda([&Revealed, &Finished]() { Revealed += Finished == 0 ? 1 : 100; });
	Cards->OnFinished.BindLambda([&Finished]() { ++Finished; });
	Cards->Begin(true);
	TestTrue(TEXT("Every key is theirs while they show"), Cards->HandleKey(EKeys::SpaceBar));
	const float Length = FStartupLogos::FullLength(Cards->NumCards(), FStartupLogoTiming::FromConsole());
	for (float Time = 0.f; Time < Length + 0.5f; Time += 1.f / 30.f)
	{
		Cards->Advance(1.f / 30.f);
	}
	TestEqual(TEXT("The menu was told to arrive once, before they were done"), Revealed, 1);
	TestEqual(TEXT("...then they were done, once"), Finished, 1);
	TestFalse(TEXT("Done: keys go to the menu"), Cards->HandleKey(EKeys::SpaceBar));
	return true;
}

#endif
