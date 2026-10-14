#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/HUD/HudMissionTrackerShapes.h"
#include "UI/HUD/HudMissionTrackerWidget.h"
#include "UI/HUD/HudNotchTallyWidget.h"
#include "UI/HUD/HudPlayerFrameShapes.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudXpGainWidget.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/WeaponNotches.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The gameplay HUD's pieces the user asked for on 2026-10-14 (Looter.UI.HUD.*): the mission tracker under the minimap with
// circles that are checked once done, the "+XP" by the experience bar, the gun's notches beside its name, no crosshair
// while aiming down the sights, and the health and experience bars 25% bigger with nothing in the frame overlapping.

namespace
{
	/** 1080p, the screen every HUD number is written for. */
	constexpr float HudTestScreenWidth = 1920.f;

	/** Marks as words, for the tests' messages. */
	FString HudTestMarks(const TArray<EHudTrackerMark>& Marks)
	{
		FString Words;
		for (const EHudTrackerMark Mark : Marks)
		{
			Words += FString::Printf(TEXT("%d "), static_cast<int32>(Mark));
		}
		return Words;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudTrackerUnderMapTest, "Looter.UI.HUD.TrackerUnderMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudTrackerUnderMapTest::RunTest(const FString& Parameters)
{
	using TrackerRules = UHudMissionTrackerWidget;
	TestEqual(TEXT("Right-aligned to the minimap's column"), TrackerRules::RightInset, UHudMinimapWidget::Margin);

	// Under the minimap's whole frame (the bezel, the compass letters riding it, the place's name) at every size the
	// settings allow, and close under it.
	float LastTop = 0.f;
	for (const float MapScale : { UGraphicsSettingsSubsystem::MinMinimapScale, 1.f, UGraphicsSettingsSubsystem::MaxMinimapScale })
	{
		const float TrackerTop = TrackerRules::TopFor(MapScale, true);
		const float BezelBottom = UHudMinimapWidget::Margin + (UHudMinimapWidget::Diameter + UHudMinimapWidget::BezelReach) * MapScale;
		const float MapFrameBottom = UHudMinimapWidget::Margin + UHudMinimapWidget::FrameBottom(MapScale);
		TestTrue(FString::Printf(TEXT("At %.0f%%: under the bezel and the place's name (%.1f under %.1f)"), MapScale * 100.f,
			TrackerTop, MapFrameBottom), TrackerTop >= MapFrameBottom && MapFrameBottom > BezelBottom);
		TestTrue(FString::Printf(TEXT("At %.0f%%: close under it"), MapScale * 100.f), TrackerTop - MapFrameBottom <= 24.f);
		TestTrue(FString::Printf(TEXT("At %.0f%%: a bigger minimap pushes it down"), MapScale * 100.f), TrackerTop > LastTop);
		LastTop = TrackerTop;
	}
	TestEqual(TEXT("With the minimap off it takes the corner"), TrackerRules::TopFor(1.f, false), UHudMinimapWidget::Margin);

	// Its widest stays clear of what's at the top's middle, even in the corner with the minimap off: the level-up banner
	// and the boss bar's cluster (its rank gem reaches 60 px past the bar).
	const float TrackerLeftmost = HudTestScreenWidth - TrackerRules::RightInset - TrackerRules::MaxWidth;
	TestTrue(TEXT("Clear of the level-up banner"), TrackerLeftmost >= HudTestScreenWidth * 0.5f + UHudLevelUpBannerWidget::Width * 0.5f);
	TestTrue(TEXT("Clear of the boss bar"), TrackerLeftmost >= HudTestScreenWidth * 0.5f + UHudBossBarWidget::BarWidth * 0.5f + 60.f);

	// Built, it hangs from its top-right corner there.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UHudMissionTrackerWidget* TestTracker = CreateWidget<UHudMissionTrackerWidget>(TestLevel.GetTestWorld(), UHudMissionTrackerWidget::StaticClass());
	if (!TestNotNull(TEXT("The tracker"), TestTracker))
	{
		return false;
	}
	// Its Slate widget held for the test, as a viewport holds it in play.
	const TSharedRef<SWidget> TrackerSlate = TestTracker->TakeWidget();
	TestTracker->Advance(0.f);
	TestTrue(TEXT("Placed under the minimap, right-aligned"),
		TestTracker->GetPlacedCorner().Equals(FVector2D(TrackerRules::RightInset, TrackerRules::TopFor(1.f, true)), 0.5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudObjectiveChecksTest, "Looter.UI.HUD.ObjectiveChecks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudObjectiveChecksTest::RunTest(const FString& Parameters)
{
	using TrackerRules = UHudMissionTrackerWidget;
	// The rules: circles, checked once done (the user's call: no more straight lines).
	TestTrue(TEXT("An objective to do: its ring"), TrackerRules::ObjectiveMark(false, false) == EHudTrackerMark::Objective);
	TestTrue(TEXT("Done: its check"), TrackerRules::ObjectiveMark(true, false) == EHudTrackerMark::ObjectiveDone);
	TestTrue(TEXT("An optional one: a dashed ring, then its check"), TrackerRules::ObjectiveMark(false, true) == EHudTrackerMark::Optional
		&& TrackerRules::ObjectiveMark(true, true) == EHudTrackerMark::OptionalDone);
	TestTrue(TEXT("Steps: done checked, the current one orange, the rest to come"),
		TrackerRules::StepMark(0, 1, 1) == EHudTrackerMark::StepDone && TrackerRules::StepMark(1, 1, 1) == EHudTrackerMark::StepNow
		&& TrackerRules::StepMark(2, 1, 1) == EHudTrackerMark::StepToCome && TrackerRules::StepMark(1, 2, INDEX_NONE) == EHudTrackerMark::StepDone);
	TestTrue(TEXT("A check's picture is no smaller than its ring's"),
		HudTrackerShapes::MarkSize(EHudTrackerMark::ObjectiveDone).X >= HudTrackerShapes::MarkSize(EHudTrackerMark::Objective).X);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionSubsystem* TestMissions = World->GetSubsystem<UMissionSubsystem>();
	UHudMissionTrackerWidget* TestTracker = CreateWidget<UHudMissionTrackerWidget>(World, UHudMissionTrackerWidget::StaticClass());
	if (!TestNotNull(TEXT("The missions"), TestMissions) || !TestNotNull(TEXT("The tracker"), TestTracker))
	{
		return false;
	}
	const TSharedRef<SWidget> TrackerSlate = TestTracker->TakeWidget();

	// A mission of three steps on its second, with an optional objective.
	const int32 MissionId = TestMissions->AddMission(FText::FromString(TEXT("Test Errand")));
	FMissionTrackerParts Parts;
	Parts.Line = TEXT("Ring the bell");
	Parts.Step = 1;
	Parts.StepCount = 3;
	FMissionTrackerOptional Extra;
	Extra.Line = TEXT("Find the rope");
	Parts.Optionals.Add(Extra);
	TestMissions->SetObjective(MissionId, FText::FromString(TEXT("Ring the bell.")), TOptional<FVector>(), Parts);
	TestTracker->Advance(0.1f);
	const TArray<EHudTrackerMark> FirstSteps = { EHudTrackerMark::StepDone, EHudTrackerMark::StepNow, EHudTrackerMark::StepToCome };
	TestTrue(TEXT("The objective to do: its ring"), TestTracker->GetShownObjectiveMark() == EHudTrackerMark::Objective);
	TestTrue(FString::Printf(TEXT("Its steps: the first checked, this one orange, the last to come (%s)"),
		*HudTestMarks(TestTracker->GetShownStepMarks())), TestTracker->GetShownStepMarks() == FirstSteps);
	TestTrue(TEXT("The optional one: a dashed ring"), TestTracker->GetShownOptionalMarks() == TArray<EHudTrackerMark>({ EHudTrackerMark::Optional }));

	// The optional objective done: checked where it stands.
	Parts.Optionals[0].bDone = true;
	TestMissions->SetObjective(MissionId, FText::FromString(TEXT("Ring the bell.")), TOptional<FVector>(), Parts);
	TestTracker->Advance(0.1f);
	TestTrue(TEXT("The optional one done: checked"), TestTracker->GetShownOptionalMarks() == TArray<EHudTrackerMark>({ EHudTrackerMark::OptionalDone }));
	TestTrue(TEXT("...and the objective still to do"), TestTracker->GetShownObjectiveMark() == EHudTrackerMark::Objective);

	// Seen a while, then its step done (the next step up): the objective is checked, and its step's circle.
	TestTracker->Advance(0.5f);
	FMissionTrackerParts Next;
	Next.Line = TEXT("Leave the tower");
	Next.Step = 2;
	Next.StepCount = 3;
	TestMissions->SetObjective(MissionId, FText::FromString(TEXT("Leave the tower.")), TOptional<FVector>(), Next);
	TestTracker->Advance(0.01f);
	const TArray<EHudTrackerMark> DoneSteps = { EHudTrackerMark::StepDone, EHudTrackerMark::StepDone, EHudTrackerMark::StepToCome };
	TestTrue(TEXT("Done: the objective's check"), TestTracker->GetShownObjectiveMark() == EHudTrackerMark::ObjectiveDone);
	TestTrue(FString::Printf(TEXT("...and its step's circle checked (%s)"), *HudTestMarks(TestTracker->GetShownStepMarks())),
		TestTracker->GetShownStepMarks() == DoneSteps);

	// After its hold, the next objective: a ring again, its step's circle orange.
	TestTracker->Advance(UHudMissionTrackerWidget::DoneHoldSeconds + 0.1f);
	const TArray<EHudTrackerMark> LastSteps = { EHudTrackerMark::StepDone, EHudTrackerMark::StepDone, EHudTrackerMark::StepNow };
	TestTrue(TEXT("The next objective: its ring"), TestTracker->GetShownObjectiveMark() == EHudTrackerMark::Objective);
	TestTrue(FString::Printf(TEXT("...its step orange, the others checked (%s)"), *HudTestMarks(TestTracker->GetShownStepMarks())),
		TestTracker->GetShownStepMarks() == LastSteps);
	TestTrue(TEXT("...and no optional one"), TestTracker->GetShownOptionalMarks().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudXpByBarTest, "Looter.UI.HUD.XpByBar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudXpByBarTest::RunTest(const FString& Parameters)
{
	using GainRules = UHudXpGainWidget;
	// How it moves: it pops in big, fades in quickly, holds, then fades (only down) as it rises a little.
	TestTrue(TEXT("Pops in big and settles"), GainRules::ScaleAt(0.f) > 1.3f && GainRules::ScaleAt(GainRules::PopSeconds) == 1.f);
	TestTrue(TEXT("In quickly"), GainRules::OpacityAt(0.f) == 0.f && GainRules::OpacityAt(GainRules::PopSeconds) == 1.f);
	TestTrue(TEXT("Holds"), GainRules::OpacityAt(GainRules::HoldSeconds) == 1.f && GainRules::RiseAt(GainRules::HoldSeconds) == 0.f);
	float LastOpacity = 1.f;
	bool bOnlyDown = true;
	for (float Age = GainRules::HoldSeconds; Age < GainRules::HoldSeconds + GainRules::FadeSeconds; Age += 0.05f)
	{
		bOnlyDown = bOnlyDown && GainRules::OpacityAt(Age) <= LastOpacity + 0.0001f;
		LastOpacity = GainRules::OpacityAt(Age);
	}
	TestTrue(TEXT("Fades out, only down, rising"), bOnlyDown && GainRules::OpacityAt(GainRules::HoldSeconds + GainRules::FadeSeconds) == 0.f
		&& GainRules::RiseAt(GainRules::HoldSeconds + GainRules::FadeSeconds) > 0.f);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UHudXpGainWidget* TestGain = CreateWidget<UHudXpGainWidget>(World, UHudXpGainWidget::StaticClass());
	if (!TestNotNull(TEXT("The +XP"), TestGain))
	{
		return false;
	}
	const TSharedRef<SWidget> GainSlate = TestGain->TakeWidget();
	TestFalse(TEXT("Nothing earned: nothing shown"), TestGain->IsShowing());
	TestGain->AddGain(38);
	TestTrue(TEXT("A kill: shown"), TestGain->IsShowing());
	TestEqual(TEXT("...in the popup's words"), TestGain->GetShownText(), FString(TEXT("+38 XP")));
	TestGain->Advance(0.5f, false);
	TestGain->AddGain(40);
	TestEqual(TEXT("Quick kills add up"), TestGain->GetShownText(), FString(TEXT("+78 XP")));
	TestTrue(TEXT("...and pop again"), TestGain->GetShownScale() > 1.3f);
	TestGain->Advance(5.f, true);
	TestTrue(TEXT("It waits while the bar is still filling"), TestGain->IsShowing());
	TestGain->Advance(GainRules::HoldSeconds + GainRules::FadeSeconds + 0.1f, false);
	TestFalse(TEXT("Then it fades"), TestGain->IsShowing());
	TestGain->AddGain(5);
	TestEqual(TEXT("A later kill starts afresh"), TestGain->GetShownText(), FString(TEXT("+5 XP")));
	TestGain->AddGain(0);
	TestEqual(TEXT("Nothing earned adds nothing"), TestGain->GetShownTotal(), static_cast<int64>(5));

	// The player frame shows it by the experience bar, on the line of its numbers.
	UHudPlayerFrameWidget* TestFrame = CreateWidget<UHudPlayerFrameWidget>(World, UHudPlayerFrameWidget::StaticClass());
	if (!TestNotNull(TEXT("The player frame"), TestFrame))
	{
		return false;
	}
	const TSharedRef<SWidget> FrameSlate = TestFrame->TakeWidget();
	UHudXpGainWidget* FrameGain = TestFrame->GetXPGain();
	const UPanelWidget* GainLine = FrameGain ? FrameGain->GetParent() : nullptr;
	TestTrue(TEXT("The frame has it, after the bar's numbers on their line"), GainLine && GainLine->GetChildrenCount() == 2
		&& GainLine->GetChildIndex(FrameGain) == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudNotchesByNameTest, "Looter.UI.HUD.NotchesByName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudNotchesByNameTest::RunTest(const FString& Parameters)
{
	using TallyRules = UHudNotchTallyWidget;
	TestTrue(TEXT("No notches: nothing to say"), TallyRules::CountText(0).IsEmpty());
	TestEqual(TEXT("The count"), TallyRules::CountText(37), FString(TEXT("37")));
	TestEqual(TEXT("A big count reads as a number"), TallyRules::CountText(1024), FText::AsNumber(1024).ToString());
	TestTrue(TEXT("The mark in its tier's colour, as the gun's card has it"),
		TallyRules::TallyColor(1).Equals(LooterUI::Color::TextDim()) && TallyRules::TallyColor(WeaponNotches::BloodedKills).Equals(LooterUI::Color::Accent())
		&& TallyRules::TallyColor(WeaponNotches::NamedKills).Equals(LooterUI::Color::AccentLight())
		&& TallyRules::TallyColor(WeaponNotches::SoulForgedKills).Equals(LooterUI::Color::CyanText()));
	TestTrue(TEXT("A kill pops it"), TallyRules::PopScaleAt(0.f) > 1.3f && TallyRules::PopScaleAt(TallyRules::PopSeconds) == 1.f);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UHudNotchTallyWidget* TestTally = CreateWidget<UHudNotchTallyWidget>(World, UHudNotchTallyWidget::StaticClass());
	if (!TestNotNull(TEXT("The tally"), TestTally))
	{
		return false;
	}
	const TSharedRef<SWidget> TallySlate = TestTally->TakeWidget();
	TestFalse(TEXT("A gun with none: no pop"), TestTally->Update(0, true, 0.f));
	TestFalse(TEXT("...and nothing shown"), TestTally->IsShowing());
	TestFalse(TEXT("A gun drawn with notches: shown as it is"), TestTally->Update(12, true, 0.f));
	TestTrue(TEXT("...its count"), TestTally->IsShowing() && TestTally->GetShownText() == TEXT("12") && TestTally->GetCountScale() == 1.f);
	TestTrue(TEXT("A kill with it: the count rises"), TestTally->Update(13, false, 0.f));
	TestTrue(TEXT("...and pops"), TestTally->GetShownText() == TEXT("13") && TestTally->GetCountScale() > 1.3f);
	TestTally->Update(13, false, TallyRules::PopSeconds);
	TestTrue(TEXT("...and settles"), FMath::IsNearlyEqual(TestTally->GetCountScale(), 1.f));
	TestFalse(TEXT("Another gun in hand: no pop"), TestTally->Update(2, true, 0.f));
	TestTrue(TEXT("...its own count"), TestTally->GetShownText() == TEXT("2") && TestTally->GetCountScale() == 1.f);
	TestTally->Update(0, true, 0.f);
	TestFalse(TEXT("A gun with none hides it"), TestTally->IsShowing());

	// In the HUD: in the gun's name's row, before the name (the row ends at the column's right edge, so it grows leftward).
	UPlayerHUDWidget* TestHud = CreateWidget<UPlayerHUDWidget>(World, UPlayerHUDWidget::StaticClass());
	if (!TestNotNull(TEXT("The HUD"), TestHud))
	{
		return false;
	}
	const TSharedRef<SWidget> HudSlate = TestHud->TakeWidget();
	const UHudNotchTallyWidget* HudTally = TestHud->GetNotchTally();
	const UTextBlock* NameText = TestHud->GetWeaponNameText();
	const UPanelWidget* NameRow = NameText ? NameText->GetParent() : nullptr;
	TestTrue(TEXT("The notches share the name's row"), HudTally && NameRow && HudTally->GetParent() == NameRow);
	TestTrue(TEXT("...before the name"), NameRow && NameRow->GetChildIndex(HudTally) >= 0
		&& NameRow->GetChildIndex(HudTally) < NameRow->GetChildIndex(NameText));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudCrosshairHidesInADSTest, "Looter.UI.HUD.CrosshairHidesInADS",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudCrosshairHidesInADSTest::RunTest(const FString& Parameters)
{
	using HudRules = UPlayerHUDWidget;
	// The ticks: full from the hip, gone once the gun is part way to the eye, back as it comes down (the same curve both
	// ways), with any gun and in any view (the rule takes neither).
	TestEqual(TEXT("From the hip: the crosshair"), HudRules::CrosshairOpacity(0.f, 0.f, false), 1.f);
	TestEqual(TEXT("Aiming down the sights: none"), HudRules::CrosshairOpacity(1.f, 0.f, false), 0.f);
	TestEqual(TEXT("Gone before the sight settles"), HudRules::CrosshairOpacity(HudRules::CrosshairGoneAtAim, 0.f, false), 0.f);
	float LastOpacity = 1.f;
	bool bFadesOut = true;
	for (float Aim = 0.f; Aim <= 1.f; Aim += 0.05f)
	{
		const float Opacity = HudRules::CrosshairOpacity(Aim, 0.f, false);
		bFadesOut = bFadesOut && Opacity <= LastOpacity + 0.0001f;
		LastOpacity = Opacity;
	}
	TestTrue(TEXT("It fades as the gun comes up (and back as it comes down)"), bFadesOut);
	TestEqual(TEXT("Sprinting: none"), HudRules::CrosshairOpacity(0.f, 1.f, false), 0.f);
	TestEqual(TEXT("Facing the camera: none"), HudRules::CrosshairOpacity(0.f, 0.f, true), 0.f);

	// The dot: gone through a first-person sight (its reticle is the aim point), the aim point through a third-person aim
	// (no sight is seen), never dropping out on the way.
	TestEqual(TEXT("First person, aiming: no dot"), HudRules::AimDotOpacity(1.f, 0.f, false, true), 0.f);
	TestTrue(TEXT("Third person, aiming: the dot stays"), HudRules::AimDotOpacity(1.f, 0.f, false, false) > 0.5f);
	bool bDotHolds = true;
	for (float Aim = 0.f; Aim <= 1.f; Aim += 0.05f)
	{
		bDotHolds = bDotHolds && HudRules::AimDotOpacity(Aim, 0.f, false, false) >= 0.4f;
	}
	TestTrue(TEXT("...all the way up"), bDotHolds);
	TestTrue(TEXT("From the hip the dot shows in either view"), HudRules::AimDotOpacity(0.f, 0.f, false, true) > 0.5f
		&& HudRules::AimDotOpacity(0.f, 0.f, false, false) > 0.5f);
	TestEqual(TEXT("Sprinting, no dot"), HudRules::AimDotOpacity(1.f, 1.f, false, false), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudBarsBiggerTest, "Looter.UI.HUD.BarsBigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudBarsBiggerTest::RunTest(const FString& Parameters)
{
	using FrameSpec = UHudPlayerFrameWidget;
	namespace BarShapes = HudFrameShapes;
	// The user's call: 25% longer and 25% thicker, both bars.
	TestTrue(TEXT("Health bar 25% longer"), FMath::IsNearlyEqual(BarShapes::HealthLength, 459.f * 1.25f, 0.01f));
	TestTrue(TEXT("Health bar 25% thicker"), FMath::IsNearlyEqual(BarShapes::HealthThickness, 27.f * 1.25f, 0.01f));
	TestTrue(TEXT("Experience bar 25% longer"), FMath::IsNearlyEqual(BarShapes::XPLength, 326.4f * 1.25f, 0.01f));
	TestTrue(TEXT("Experience bar 25% thicker"), FMath::IsNearlyEqual(BarShapes::XPThickness, 13.6f * 1.25f, 0.01f));

	// Nothing overlaps. The health bar starts where the medallion's edge crosses its middle, so an empty bar still begins
	// at the medallion.
	const float HealthMiddle = FrameSpec::HealthBarY + BarShapes::HealthMid;
	const float MedallionEdge = FrameSpec::MedallionX + (HealthMiddle - (FrameSpec::MedallionY - FrameSpec::MedallionReach));
	TestTrue(FString::Printf(TEXT("The health track starts at the medallion's edge (%.1f, %.1f)"), FrameSpec::HealthBarX + BarShapes::TrackLeft,
		MedallionEdge), FMath::Abs(FrameSpec::HealthBarX + BarShapes::TrackLeft - MedallionEdge) <= 1.5f);
	TestTrue(TEXT("The health bar stays over the experience bar, with room"),
		FrameSpec::HealthBarY + BarShapes::HealthThickness + 8.f <= FrameSpec::XPBarY - BarShapes::XPEdgeWidth);
	// The level gem's right tip stays clear of the experience bar's leaning left end, at the tip's height.
	const float XPLeftAtGem = FrameSpec::XPBarX + (FrameSpec::XPBarY + BarShapes::XPMid - FrameSpec::GemY) * BarShapes::LeanPerPixel - BarShapes::XPEdgeWidth;
	TestTrue(FString::Printf(TEXT("The level gem clear of the experience bar (%.1f, %.1f)"), FrameSpec::GemX + FrameSpec::GemReach, XPLeftAtGem),
		FrameSpec::GemX + FrameSpec::GemReach + 2.f <= XPLeftAtGem);
	// The experience numbers after the bar's top-right corner (it leans furthest right there).
	TestTrue(TEXT("The experience numbers after the bar"),
		FrameSpec::XPNumbersX >= FrameSpec::XPBarX + BarShapes::XPLength + BarShapes::XPMid * BarShapes::LeanPerPixel + BarShapes::XPEdgeWidth + 1.f);
	// The frame's box holds the bigger bars and the numbers with the "+XP" after them.
	TestTrue(TEXT("The box holds the health bar's clamp"), FrameSpec::Width >= FrameSpec::HealthBarX + BarShapes::HealthBoxLeft + BarShapes::HealthBoxWidth);
	TestTrue(TEXT("...and the experience numbers and the +XP"), FrameSpec::Width >= FrameSpec::XPNumbersX + 250.f);
	TestTrue(TEXT("...and the health bar's top"), FrameSpec::HealthBarY + BarShapes::HealthBoxTop >= FrameSpec::ScreenBottom - FrameSpec::Height);
	// The pictures' boxes hold what's drawn in them.
	TestTrue(TEXT("The track's box holds the track"), BarShapes::TrackBoxTop <= BarShapes::TrackTop
		&& BarShapes::TrackBoxTop + BarShapes::TrackBoxHeight >= BarShapes::TrackBottom);
	TestTrue(TEXT("The sections' box holds the sections"), BarShapes::SectionBoxTop <= BarShapes::SectionTop
		&& BarShapes::SectionBoxTop + BarShapes::SectionBoxHeight >= BarShapes::SectionBottom);
	TestTrue(TEXT("The health bar's box holds the bar and its lean"), BarShapes::HealthBoxWidth + BarShapes::HealthBoxLeft
		>= BarShapes::HealthLength + BarShapes::HealthMid * BarShapes::LeanPerPixel);

	// It builds with them.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UHudPlayerFrameWidget* TestFrame = CreateWidget<UHudPlayerFrameWidget>(TestLevel.GetTestWorld(), UHudPlayerFrameWidget::StaticClass());
	if (!TestNotNull(TEXT("The player frame"), TestFrame))
	{
		return false;
	}
	const TSharedRef<SWidget> FrameSlate = TestFrame->TakeWidget();
	TestNotNull(TEXT("Built"), TestFrame->GetXPGain());
	return true;
}

#endif
