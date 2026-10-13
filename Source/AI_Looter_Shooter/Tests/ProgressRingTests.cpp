#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Scenes/SceneSkipPromptWidget.h"
#include "Tests/InteractionTestWorld.h"
#include "UI/HUD/HudInteractPromptWidget.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/HUD/MinimapCompass.h"
#include "UI/Inventory/MapWidget.h"
#include "UI/Style/LooterProgressRing.h"
#include "UI/Style/LooterProgressRingWidget.h"
#include "World/MinimapSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The hold ring (SLooterProgressRing and where it is used: the Interact prompt, the scene skip) and the compass letters
// (the minimap's bezel, the map page).

using namespace InteractionTestWorld;

namespace
{
	/** The text shown by the widget named Name in a user widget's tree (empty if none). */
	FString TextOf(const UUserWidget& Widget, const TCHAR* Name)
	{
		const UWidget* Found = Widget.WidgetTree ? Widget.WidgetTree->FindWidget(FName(Name)) : nullptr;
		const UTextBlock* Text = Cast<UTextBlock>(Found);
		return Text ? Text->GetText().ToString() : FString();
	}

	/** The world direction a compass point faces on the minimap's map (north +X, east +Y). */
	FVector Direction(MinimapCompass::EPoint Point)
	{
		switch (Point)
		{
		case MinimapCompass::EPoint::East:
			return FVector(0.0, 1.0, 0.0);
		case MinimapCompass::EPoint::South:
			return FVector(-1.0, 0.0, 0.0);
		case MinimapCompass::EPoint::West:
			return FVector(0.0, -1.0, 0.0);
		default:
			return FVector(1.0, 0.0, 0.0);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgressRingGeometryTest, "Looter.UI.ProgressRing.Geometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FProgressRingGeometryTest::RunTest(const FString& Parameters)
{
	// The fill runs clockwise from 12 o'clock, a quarter turn a quarter of the progress, and never past the circle.
	TestEqual(TEXT("Empty: no fill"), LooterRing::FillDegrees(0.f), 0.f, 0.001f);
	TestEqual(TEXT("A quarter: 90 degrees (3 o'clock)"), LooterRing::FillDegrees(0.25f), 90.f, 0.001f);
	TestEqual(TEXT("Half: 180 degrees (6 o'clock)"), LooterRing::FillDegrees(0.5f), 180.f, 0.001f);
	TestEqual(TEXT("Full: the whole circle"), LooterRing::FillDegrees(1.f), 360.f, 0.001f);
	TestEqual(TEXT("Below empty clamps to none"), LooterRing::FillDegrees(-0.5f), 0.f, 0.001f);
	TestEqual(TEXT("Past full clamps to the circle"), LooterRing::FillDegrees(1.5f), 360.f, 0.001f);

	// The segmented ring: equal parts with the gap centred between them, the first gap on 12 o'clock.
	const LooterRing::FArc First = LooterRing::SegmentArc(0, 4, 10.f);
	const LooterRing::FArc Last = LooterRing::SegmentArc(3, 4, 10.f);
	TestTrue(TEXT("The first of four starts half a gap past 12 o'clock and ends half a gap short of 3"),
		FMath::IsNearlyEqual(First.Start, 5.f, 0.001f) && FMath::IsNearlyEqual(First.End, 85.f, 0.001f));
	TestTrue(TEXT("The last of four ends half a gap short of 12 o'clock"),
		FMath::IsNearlyEqual(Last.Start, 275.f, 0.001f) && FMath::IsNearlyEqual(Last.End, 355.f, 0.001f));
	TestTrue(TEXT("A gap too wide for the parts never eats one"), LooterRing::SegmentArc(0, 10, 100.f).Sweep() > 0.f);

	// The parts fill one after another, each over its own share of the progress.
	TestTrue(TEXT("Half done: two lit, two not"), LooterRing::SegmentFill(0, 4, 0.5f) == 1.f && LooterRing::SegmentFill(1, 4, 0.5f) == 1.f
		&& LooterRing::SegmentFill(2, 4, 0.5f) == 0.f && LooterRing::SegmentFill(3, 4, 0.5f) == 0.f);
	TestEqual(TEXT("The third part is half lit at five eighths"), LooterRing::SegmentFill(2, 4, 0.625f), 0.5f, 0.001f);
	TestEqual(TEXT("Past full: every part whole"), LooterRing::SegmentFill(3, 4, 1.5f), 1.f, 0.001f);
	TestEqual(TEXT("Below empty: none lit"), LooterRing::SegmentFill(0, 4, -1.f), 0.f, 0.001f);

	// A gap given in pixels spans the angle it covers on the ring.
	TestEqual(TEXT("3 px on a ring of radius 12 is a quarter of a radian"), LooterRing::GapDegrees(3.f, 12.f), FMath::RadiansToDegrees(0.25f), 0.01f);
	TestEqual(TEXT("No ring, no gap"), LooterRing::GapDegrees(3.f, 0.f), 0.f, 0.001f);

	// The line's centre keeps its room inside the box.
	TestEqual(TEXT("A 36 box with a 3 line: the centre line at 13"), LooterRing::CenterRadius(36.f, 3.f), 13.f, 0.001f);
	TestEqual(TEXT("A box too small never gives a negative radius"), LooterRing::CenterRadius(4.f, 3.f), 0.f, 0.001f);

	// The arc's points: from 12 o'clock clockwise (y is down).
	TArray<FVector2f> Quarter;
	LooterRing::AppendArcPoints(Quarter, FVector2f(10.f, 10.f), 5.f, 0.f, 90.f);
	if (TestTrue(TEXT("A quarter arc has points"), Quarter.Num() >= 3))
	{
		TestTrue(TEXT("It starts at 12 o'clock"), Quarter[0].Equals(FVector2f(10.f, 5.f), 0.001f));
		TestTrue(TEXT("...and ends at 3 o'clock"), Quarter.Last().Equals(FVector2f(15.f, 10.f), 0.001f));
		TestTrue(TEXT("Steps no wider than asked: 90 degrees in 4s is 23 steps"), Quarter.Num() == 24);
	}
	TArray<FVector2f> Half;
	LooterRing::AppendArcPoints(Half, FVector2f(10.f, 10.f), 5.f, 0.f, 180.f);
	TestTrue(TEXT("Half way round is 6 o'clock, straight down"), Half.Last().Equals(FVector2f(10.f, 15.f), 0.001f));
	TArray<FVector2f> Three;
	LooterRing::AppendArcPoints(Three, FVector2f(10.f, 10.f), 5.f, 0.f, 270.f);
	TestTrue(TEXT("Three quarters is 9 o'clock"), Three.Last().Equals(FVector2f(5.f, 10.f), 0.001f));

	// The pop: swells a tenth midway and settles; the flash is brightest at once and fades.
	TestEqual(TEXT("The pop starts at full size"), LooterRing::PopScale(0.f), 1.f, 0.001f);
	TestEqual(TEXT("...swells a tenth midway"), LooterRing::PopScale(0.5f), 1.f + LooterRing::PopGrow, 0.001f);
	TestEqual(TEXT("...and settles back"), LooterRing::PopScale(1.f), 1.f, 0.001f);
	TestEqual(TEXT("Past its end it stays at full size"), LooterRing::PopScale(2.f), 1.f, 0.001f);
	TestEqual(TEXT("The flash is full as it starts"), LooterRing::PopFlash(0.f), 1.f, 0.001f);
	TestEqual(TEXT("...gone by the end"), LooterRing::PopFlash(1.f), 0.f, 0.001f);
	TestTrue(TEXT("...fading between"), LooterRing::PopFlash(0.5f) > 0.f && LooterRing::PopFlash(0.5f) < 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgressRingWidgetTest, "Looter.UI.ProgressRing.Widget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FProgressRingWidgetTest::RunTest(const FString& Parameters)
{
	ULooterProgressRing* Ring = NewObject<ULooterProgressRing>(GetTransientPackage());
	if (!TestNotNull(TEXT("The ring"), Ring))
	{
		return false;
	}
	Ring->Configure(36.f, 3.f);

	// Its progress stays inside 0-1.
	Ring->SetProgress(0.4f);
	TestEqual(TEXT("Progress as set"), Ring->GetProgress(), 0.4f, 0.001f);
	Ring->SetProgress(1.5f);
	TestEqual(TEXT("Past full: clamped"), Ring->GetProgress(), 1.f, 0.001f);
	Ring->SetProgress(-1.f);
	TestEqual(TEXT("Below empty: clamped"), Ring->GetProgress(), 0.f, 0.001f);

	// The Slate widget asks for the box it was set to.
	const TSharedRef<SWidget> Slate = Ring->TakeWidget();
	Slate->SlatePrepass(1.f);
	TestTrue(TEXT("It asks for its diameter"), FMath::IsNearlyEqual(static_cast<double>(Slate->GetDesiredSize().X), 36.0, 0.01)
		&& FMath::IsNearlyEqual(static_cast<double>(Slate->GetDesiredSize().Y), 36.0, 0.01));

	// A pop: starts at once, swells to a tenth over half its time, ends in full size with no light.
	TestFalse(TEXT("Not popping at first"), Ring->IsPopping());
	TestEqual(TEXT("...at full size"), Ring->GetPopScale(), 1.f, 0.001f);
	TestFalse(TEXT("Nothing to run when it isn't popping"), Ring->Advance(0.1f));
	Ring->Pop();
	TestTrue(TEXT("Popping"), Ring->IsPopping());
	TestEqual(TEXT("...with a full flash"), Ring->GetFlash(), 1.f, 0.001f);
	TestTrue(TEXT("Running on"), Ring->Advance(LooterRing::PopSeconds * 0.5f));
	TestEqual(TEXT("Halfway it has swelled a tenth"), Ring->GetPopScale(), 1.f + LooterRing::PopGrow, 0.001f);
	TestTrue(TEXT("...and the flash is fading"), Ring->GetFlash() > 0.f && Ring->GetFlash() < 1.f);
	TestTrue(TEXT("The frame it ends still counts as running"), Ring->Advance(LooterRing::PopSeconds));
	TestFalse(TEXT("Over"), Ring->IsPopping());
	TestEqual(TEXT("...back to full size"), Ring->GetPopScale(), 1.f, 0.001f);
	TestEqual(TEXT("...with no light"), Ring->GetFlash(), 0.f, 0.001f);
	TestFalse(TEXT("And nothing more to run"), Ring->Advance(0.1f));

	// The key's name inside the ring: no brackets, capitals, smaller as it gets longer.
	TestEqual(TEXT("[E] is E"), LooterUI::HoldKeyLabel(TEXT("[E]")), FString(TEXT("E")));
	TestEqual(TEXT("[Esc] is ESC"), LooterUI::HoldKeyLabel(TEXT("[Esc]")), FString(TEXT("ESC")));
	TestEqual(TEXT("A bare e is E"), LooterUI::HoldKeyLabel(TEXT("e")), FString(TEXT("E")));
	TestEqual(TEXT("A long name keeps its words"), LooterUI::HoldKeyLabel(TEXT(" Left Mouse Button ")), FString(TEXT("LEFT MOUSE BUTTON")));
	TestTrue(TEXT("Longer names get smaller type"), LooterUI::HoldKeyTextSize(1) > LooterUI::HoldKeyTextSize(2)
		&& LooterUI::HoldKeyTextSize(2) > LooterUI::HoldKeyTextSize(3) && LooterUI::HoldKeyTextSize(3) >= LooterUI::HoldKeyTextSize(6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractPromptRingTest, "Looter.UI.InteractPrompt.Ring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractPromptRingTest::RunTest(const FString& Parameters)
{
	// The ring wears a hold and nothing else: a door (a tap) is plain text, the bell (a hold) has the ring round its key,
	// which fills with the hold, pops when the hold completes, and is seen out before the prompt fades.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	AInteractableProp* Door = SpawnDoor(World, Ahead(150.0));
	AInteractableProp* Bell = SpawnBell(World, FVector(0.0, 150.0, EyeHeight));
	UHudInteractPromptWidget* Prompt = CreateWidget<UHudInteractPromptWidget>(World, UHudInteractPromptWidget::StaticClass());
	if (!TestTrue(TEXT("Stand-in, door, bell and prompt"), Player && Interaction && Door && Bell && Prompt))
	{
		return false;
	}
	const TSharedRef<SWidget> PromptSlate = Prompt->TakeWidget();
	TestFalse(TEXT("Nothing to say yet: not shown"), Prompt->GetShown().bVisible);

	// The door: only a tap, so plain text and no ring.
	Interaction->UpdateInteraction(0.f);
	Prompt->Update(Interaction, TEXT("E"), 0.05f);
	UHudInteractPromptWidget::FShown Shown = Prompt->GetShown();
	TestTrue(TEXT("The door's prompt shows"), Shown.bVisible);
	TestTrue(TEXT("...as a tap line"), Shown.bTapLine);
	TestFalse(TEXT("...with no ring"), Shown.bHoldLine);

	// The bell: only a hold, with its empty ring round the key.
	Face(Player, 90.0);
	Interaction->UpdateInteraction(0.f);
	Prompt->Update(Interaction, TEXT("E"), 0.05f);
	Shown = Prompt->GetShown();
	TestTrue(TEXT("The bell's prompt shows the hold line, with the ring"), Shown.bVisible && Shown.bHoldLine);
	TestFalse(TEXT("...and no tap line"), Shown.bTapLine);
	TestEqual(TEXT("The key sits inside the ring, without brackets"), Shown.RingKey, FString(TEXT("E")));
	TestEqual(TEXT("The words beside it"), Shown.HoldWords, FString(TEXT("RING THE BELL")));
	TestEqual(TEXT("The ring is empty before the key is held"), Shown.RingProgress, 0.f, 0.001f);

	// Held: the ring fills with the hold.
	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.5f);
	Prompt->Update(Interaction, TEXT("E"), 0.016f);
	Shown = Prompt->GetShown();
	TestEqual(TEXT("Halfway through the hold: half the ring"), Shown.RingProgress, 0.5f, 0.02f);
	TestFalse(TEXT("...not popping yet"), Shown.bRingPopping);

	// The hold completes: the ring pops at once, stays full for the pop and keeps the prompt up though the bell can't be
	// rung again for a moment (which would otherwise fade it at once).
	Interaction->UpdateInteraction(0.6f);
	TestTrue(TEXT("The hold completed: the ring pops"), Prompt->GetShown().bRingPopping);
	Prompt->Update(Interaction, TEXT("E"), 0.016f);
	Shown = Prompt->GetShown();
	TestTrue(TEXT("Popping: the prompt is still up"), Shown.bVisible && Shown.bHoldLine && Shown.bRingPopping);
	TestEqual(TEXT("...with the ring full"), Shown.RingProgress, 1.f, 0.001f);

	// The pop ends, and then the prompt fades out (the bell is still cooling down).
	for (int32 Frame = 0; Frame < 4; ++Frame)
	{
		Prompt->Update(Interaction, TEXT("E"), 0.05f);
	}
	Shown = Prompt->GetShown();
	TestFalse(TEXT("The pop is over after its time"), Shown.bRingPopping);
	for (int32 Frame = 0; Frame < 4; ++Frame)
	{
		Prompt->Update(Interaction, TEXT("E"), 0.05f);
	}
	TestFalse(TEXT("Nothing more to say: faded out"), Prompt->GetShown().bVisible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneSkipPromptRingTest, "Looter.UI.SceneSkipPrompt.Ring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSceneSkipPromptRingTest::RunTest(const FString& Parameters)
{
	// The scene skip wears the ring too, for the held key only: HOLD (E) TO SKIP fills with the hold and pops as the scene
	// is skipped; PRESS [ESC] AGAIN TO SKIP has no ring.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	USceneSkipPromptWidget* Prompt = CreateWidget<USceneSkipPromptWidget>(TestLevel.GetTestWorld(), USceneSkipPromptWidget::StaticClass());
	if (!TestNotNull(TEXT("The prompt"), Prompt))
	{
		return false;
	}
	const TSharedRef<SWidget> PromptSlate = Prompt->TakeWidget();
	TestTrue(TEXT("Only the HOLD lead wears the ring"), USceneSkipPromptWidget::IsHoldLead(TEXT("HOLD")) && !USceneSkipPromptWidget::IsHoldLead(TEXT("PRESS")));

	// The key is down, not yet held for long: the ring round the key, empty.
	Prompt->Update(true, TEXT("HOLD"), TEXT("[E]"), TEXT("TO SKIP"), 0.f, 0.05f);
	USceneSkipPromptWidget::FShown Shown = Prompt->GetShown();
	TestTrue(TEXT("Shown with the ring"), Shown.bVisible && Shown.bRing);
	TestEqual(TEXT("The key sits inside, without brackets"), Shown.Key, FString(TEXT("E")));
	TestEqual(TEXT("Empty at first"), Shown.RingProgress, 0.f, 0.001f);

	Prompt->Update(true, TEXT("HOLD"), TEXT("[E]"), TEXT("TO SKIP"), 0.5f, 0.016f);
	Shown = Prompt->GetShown();
	TestEqual(TEXT("Half held: half the ring"), Shown.RingProgress, 0.5f, 0.001f);
	TestFalse(TEXT("...not popping"), Shown.bRingPopping);

	// The hold ends: the ring pops (once), and stays up while the scene is skipped and the prompt is told to go.
	Prompt->Update(true, TEXT("HOLD"), TEXT("[E]"), TEXT("TO SKIP"), 1.f, 0.016f);
	Shown = Prompt->GetShown();
	TestTrue(TEXT("Held to the end: the ring pops, full"), Shown.bRingPopping && FMath::IsNearlyEqual(Shown.RingProgress, 1.f, 0.001f));
	Prompt->Update(false, TEXT("HOLD"), FString(), TEXT("TO SKIP"), 0.f, 0.05f);
	TestTrue(TEXT("The scene is gone, but the pop is seen out"), Prompt->GetShown().bVisible);
	for (int32 Frame = 0; Frame < 8; ++Frame)
	{
		Prompt->Update(false, TEXT("HOLD"), FString(), TEXT("TO SKIP"), 0.f, 0.05f);
	}
	TestFalse(TEXT("Then it fades out"), Prompt->GetShown().bVisible);

	// An Escape: the key in brackets, no ring.
	Prompt->Update(true, TEXT("PRESS"), TEXT("[ESC]"), TEXT("AGAIN TO SKIP"), 0.f, 0.05f);
	Shown = Prompt->GetShown();
	TestTrue(TEXT("The Escape prompt shows"), Shown.bVisible);
	TestFalse(TEXT("...with no ring"), Shown.bRing);
	TestEqual(TEXT("...and the key in brackets"), Shown.Key, FString(TEXT("[ESC]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinimapCompassTest, "Looter.UI.Minimap.Compass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMinimapCompassTest::RunTest(const FString& Parameters)
{
	using MinimapCompass::EPoint;

	// Where each letter rides the bezel for a few headings: clockwise degrees from straight up (the way you face).
	struct FHeading
	{
		float Yaw;
		float North;
		float East;
		float South;
		float West;
	};
	const FHeading Headings[] = {
		{ 0.f, 0.f, 90.f, 180.f, 270.f },     // facing north: N up, E right
		{ 90.f, 270.f, 0.f, 90.f, 180.f },    // facing east: E up, N left
		{ 180.f, 180.f, 270.f, 0.f, 90.f },   // facing south: S up
		{ -90.f, 90.f, 180.f, 270.f, 0.f },   // facing west: W up, N right
		{ 45.f, 315.f, 45.f, 135.f, 225.f },  // north-east: N up and to the left
	};
	for (const FHeading& Heading : Headings)
	{
		const FString Facing = FString::Printf(TEXT("Facing %.0f"), Heading.Yaw);
		TestEqual(*(Facing + TEXT(": N")), MinimapCompass::BezelDegrees(EPoint::North, Heading.Yaw), Heading.North, 0.01f);
		TestEqual(*(Facing + TEXT(": E")), MinimapCompass::BezelDegrees(EPoint::East, Heading.Yaw), Heading.East, 0.01f);
		TestEqual(*(Facing + TEXT(": S")), MinimapCompass::BezelDegrees(EPoint::South, Heading.Yaw), Heading.South, 0.01f);
		TestEqual(*(Facing + TEXT(": W")), MinimapCompass::BezelDegrees(EPoint::West, Heading.Yaw), Heading.West, 0.01f);
	}

	// The same places as the markers' own mapping puts a direction on the map (the bezel's letters ride the way the markers do).
	for (const float Yaw : { 0.f, 30.f, 90.f, 135.f, -60.f, 270.f })
	{
		for (const EPoint Point : MinimapCompass::All)
		{
			const FVector2D Letter = MinimapCompass::BezelOffset(Point, Yaw, 10.f);
			const FVector2D Marker = UMinimapSubsystem::ViewOffset(Direction(Point) * 100.0, Yaw, 0.1f);
			TestTrue(*FString::Printf(TEXT("%s at yaw %.0f rides where a marker in its direction would"), MinimapCompass::Letter(Point), Yaw),
				Letter.Equals(Marker, 1e-3));
		}
	}

	// The map page (north up): the middles of the frame's edges.
	const FVector2D Position(60.0, 100.0);
	const FVector2D Size(1030.0, 732.0);
	TestTrue(TEXT("N: the middle of the top edge"), MinimapCompass::EdgeMidpoint(EPoint::North, Position, Size).Equals(FVector2D(575.0, 100.0)));
	TestTrue(TEXT("E: the middle of the right edge"), MinimapCompass::EdgeMidpoint(EPoint::East, Position, Size).Equals(FVector2D(1090.0, 466.0)));
	TestTrue(TEXT("S: the middle of the bottom edge"), MinimapCompass::EdgeMidpoint(EPoint::South, Position, Size).Equals(FVector2D(575.0, 832.0)));
	TestTrue(TEXT("W: the middle of the left edge"), MinimapCompass::EdgeMidpoint(EPoint::West, Position, Size).Equals(FVector2D(60.0, 466.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompassLettersTest, "Looter.UI.Compass.Letters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCompassLettersTest::RunTest(const FString& Parameters)
{
	// Both widgets carry all four letters, and the map page keeps "north up" in its header.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UHudMinimapWidget* Minimap = CreateWidget<UHudMinimapWidget>(World, UHudMinimapWidget::StaticClass());
	UMapWidget* Page = CreateWidget<UMapWidget>(World, UMapWidget::StaticClass());
	if (!TestTrue(TEXT("A minimap and the map page"), Minimap && Page))
	{
		return false;
	}
	const TSharedRef<SWidget> MinimapSlate = Minimap->TakeWidget();
	Page->Open(nullptr);
	const TSharedRef<SWidget> PageSlate = Page->TakeWidget();

	for (const MinimapCompass::EPoint Point : MinimapCompass::All)
	{
		const FString Letter = MinimapCompass::Letter(Point);
		TestEqual(*FString::Printf(TEXT("The minimap's bezel carries the %s"), *Letter), TextOf(*Minimap, *FString::Printf(TEXT("Compass_%s"), *Letter)), Letter);
		TestEqual(*FString::Printf(TEXT("The map page carries the %s"), *Letter), TextOf(*Page, *FString::Printf(TEXT("MapCompass_%s"), *Letter)), Letter);
	}

	bool bNorthUp = false;
	if (Page->WidgetTree)
	{
		Page->WidgetTree->ForEachWidget([&bNorthUp](UWidget* Widget)
		{
			const UTextBlock* Text = Cast<UTextBlock>(Widget);
			bNorthUp |= Text && Text->GetText().ToString().Contains(TEXT("NORTH UP"));
		});
	}
	TestTrue(TEXT("The map page's header still says NORTH UP"), bNorthUp);
	return true;
}

#endif
