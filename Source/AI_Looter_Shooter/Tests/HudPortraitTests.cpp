#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Embers/HeroDefinition.h"
#include "UI/HUD/HudPortraitWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The HUD portrait per hero (2026-10-20, Looter.HUD.PortraitSets): each of the five has a portrait of its own (glass, bust and
// eye glows), each with texture names of its own (painted textures are shared by name), an id with no portrait gets
// Ellis's, and the widget draws the hero it is told, again when it is told another.

namespace
{
	/** The Images under a widget (the portrait's glass, bust, eyes, glows and flash), however deep. */
	int32 CountImages(const UWidget* Widget)
	{
		int32 Count = Widget && Widget->IsA<UImage>() ? 1 : 0;
		if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			for (int32 Child = 0; Child < Panel->GetChildrenCount(); ++Child)
			{
				Count += CountImages(Panel->GetChildAt(Child));
			}
		}
		return Count;
	}

	/** The Images a portrait with this many glows draws: glass, bust, open eyes, brows, squint, flare and the red flash. */
	int32 ImagesFor(const UHudPortraitWidget::FPortraitCounts& Counts)
	{
		constexpr int32 PicturesAndFlash = 7;
		return PicturesAndFlash + Counts.EyesOpenGlows + Counts.EyesHurtGlows + Counts.EyesFlareGlows;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudPortraitSetsTest, "Looter.HUD.PortraitSets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudPortraitSetsTest::RunTest(const FString& Parameters)
{
	const FName EllisId(TEXT("Ellis"));
	const UHudPortraitWidget::FPortraitCounts Ellis = UHudPortraitWidget::CountPortrait(EllisId);

	// Every hero in the roster: a whole portrait, under texture names of its own.
	TSet<FString> Prefixes;
	for (const FName Id : UHeroDefinition::RosterIds())
	{
		const UHudPortraitWidget::FPortraitCounts Counts = UHudPortraitWidget::CountPortrait(Id);
		TestTrue(*FString::Printf(TEXT("%s: glass and bust have layers (%d, %d)"), *Id.ToString(), Counts.GlassLayers, Counts.BustLayers),
			Counts.GlassLayers > 0 && Counts.BustLayers > 0);
		TestTrue(*FString::Printf(TEXT("%s: glows in the open, squinting and flaring eyes (%d, %d, %d)"), *Id.ToString(),
			Counts.EyesOpenGlows, Counts.EyesHurtGlows, Counts.EyesFlareGlows),
			Counts.EyesOpenGlows > 0 && Counts.EyesHurtGlows > 0 && Counts.EyesFlareGlows > 0);
		TestFalse(*FString::Printf(TEXT("%s: texture names of its own (%s)"), *Id.ToString(), *Counts.TexturePrefix), Prefixes.Contains(Counts.TexturePrefix));
		Prefixes.Add(Counts.TexturePrefix);
		if (Id == EllisId)
		{
			TestEqual(TEXT("Ellis's textures keep the names they had before the pick"), Counts.TexturePrefix, FString(TEXT("HudPortrait")));
		}
		else
		{
			TestEqual(*FString::Printf(TEXT("%s: textures named for the hero"), *Id.ToString()), Counts.TexturePrefix,
				FString(TEXT("HudPortrait")) + Id.ToString());
		}
	}
	TestEqual(TEXT("Five portraits"), Prefixes.Num(), UHeroDefinition::RosterIds().Num());

	// Crane has one eye: his open-eye glows are two, Ellis's four.
	const UHudPortraitWidget::FPortraitCounts Crane = UHudPortraitWidget::CountPortrait(FName(TEXT("Crane")));
	TestTrue(TEXT("Crane's one eye: fewer open-eye glows than Ellis's"), Crane.EyesOpenGlows < Ellis.EyesOpenGlows);

	// An id with no portrait (none, a hero taken out) shows Ellis's.
	for (const FName Stranger : { FName(NAME_None), FName(TEXT("Nobody")) })
	{
		const UHudPortraitWidget::FPortraitCounts Counts = UHudPortraitWidget::CountPortrait(Stranger);
		TestTrue(*FString::Printf(TEXT("'%s' gets Ellis's portrait"), *Stranger.ToString()),
			Counts.TexturePrefix == Ellis.TexturePrefix && Counts.GlassLayers == Ellis.GlassLayers && Counts.BustLayers == Ellis.BustLayers
			&& Counts.EyesOpenGlows == Ellis.EyesOpenGlows);
	}

	// Built, it draws the hero it's told, and redraws for another once it's on screen.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UHudPortraitWidget* Portrait = CreateWidget<UHudPortraitWidget>(TestLevel.GetTestWorld(), UHudPortraitWidget::StaticClass());
	if (!TestNotNull(TEXT("The portrait window"), Portrait))
	{
		return false;
	}
	// Its Slate widget held for the test, as a viewport holds it in play. A test world plays no hero: Ellis's.
	const TSharedRef<SWidget> PortraitSlate = Portrait->TakeWidget();
	TestEqual(TEXT("A test world plays no hero: Ellis's images"), CountImages(Portrait->GetRootWidget()), ImagesFor(Ellis));
	for (const FName Id : UHeroDefinition::RosterIds())
	{
		Portrait->SetHero(Id);
		TestEqual(*FString::Printf(TEXT("Drawn for %s: that portrait's images"), *Id.ToString()),
			CountImages(Portrait->GetRootWidget()), ImagesFor(UHudPortraitWidget::CountPortrait(Id)));
	}
	Portrait->SetHero(FName(TEXT("Nobody")));
	TestEqual(TEXT("Drawn for an unknown id: Ellis's images"), CountImages(Portrait->GetRootWidget()), ImagesFor(Ellis));
	return true;
}

#endif
