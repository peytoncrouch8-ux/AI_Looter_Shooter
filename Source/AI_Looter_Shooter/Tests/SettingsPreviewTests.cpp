#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerViewComponent.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Tests/LocomotionTestWorld.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/Menus/SettingsMenuPreview.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/Slider.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "Tests/AutomationCommon.h"

// The settings menu's live previews: a slider shows what it sets while it moves. The first-person field of view changes
// the view behind the menu, and the menu steps aside while the slider is held; the minimap zoom shows a live minimap in
// the HUD minimap's corner. Esc while one is held lets go of it with the setting as it was. The sliders' settings
// themselves are in SettingsTests.cpp.

using namespace LocomotionTestWorld;

namespace
{
	/** The player's own field of view on the first-person camera (what the view draws the world at). */
	float WorldFieldOfView(const UPlayerLocomotionComponent* Locomotion)
	{
		const UCameraComponent* Camera = CameraOf(Locomotion);
		return Camera ? Camera->FieldOfView : 0.f;
	}

	/** The third-person camera: the one on the spring arm. */
	const UCameraComponent* FindThirdPersonCamera(const AActor* Owner)
	{
		TInlineComponentArray<UCameraComponent*> Cameras(Owner);
		for (const UCameraComponent* Camera : Cameras)
		{
			if (Cast<USpringArmComponent>(Camera->GetAttachParent()))
			{
				return Camera;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSliderPreviewTest, "Looter.Settings.SliderPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSliderPreviewTest::RunTest(const FString& Parameters)
{
	using namespace SettingsMenu;

	// Idle until the slider is touched.
	FSliderPreview Preview;
	TestFalse(TEXT("Idle at first"), Preview.IsActive() || Preview.IsHeld());
	TestEqual(TEXT("...and clear"), Preview.Tick(0.1f), 0.f);

	// Taken: up, and it stays up for as long as it is held, however long that is.
	Preview.Grab(90.f);
	TestTrue(TEXT("Taken: held and up"), Preview.IsHeld() && Preview.IsActive());
	TestEqual(TEXT("It remembers the value it was taken at"), Preview.GetGrabbedValue(), 90.f);
	float Opacity = 0.f;
	for (int32 Index = 0; Index < 30; ++Index)
	{
		Opacity = Preview.Tick(1.f / 60.f);
	}
	TestTrue(TEXT("It fades in within half a second"), Opacity > 0.99f);
	for (int32 Second = 0; Second < 10; ++Second)
	{
		Opacity = Preview.Tick(1.f);
	}
	TestTrue(TEXT("Held ten seconds on: still up"), Preview.IsActive() && Opacity == 1.f);

	// Let go: it lingers a moment, then fades away.
	Preview.Release();
	TestFalse(TEXT("Let go: no longer held"), Preview.IsHeld());
	TestTrue(TEXT("...but still up"), Preview.IsActive());
	Opacity = Preview.Tick(PreviewLingerSeconds * 0.5f);
	TestTrue(TEXT("Half the linger on: still up"), Preview.IsActive() && Opacity == 1.f);
	Opacity = Preview.Tick(PreviewLingerSeconds);
	TestFalse(TEXT("The linger over: away"), Preview.IsActive());
	TestEqual(TEXT("...faded out"), Opacity, 0.f);

	// A key held down on a focused slider only repeats moves: each keeps the preview up, and when they stop it goes.
	Preview.Nudge();
	TestTrue(TEXT("A move keeps it up without a grab"), Preview.IsActive() && !Preview.IsHeld());
	Preview.Tick(PreviewLingerSeconds * 0.6f);
	Preview.Nudge();
	Preview.Tick(PreviewLingerSeconds * 0.6f);
	TestTrue(TEXT("Moves 0.6 linger apart: always up"), Preview.IsActive());
	Preview.Tick(PreviewLingerSeconds * 2.f);
	TestFalse(TEXT("No more moves: away"), Preview.IsActive());

	// The pointer resting on the slider keeps it up (the minimap's two), and nothing carries over a reset.
	Preview.Tick(PreviewLingerSeconds * 2.f, /*bPointedAt*/ true);
	TestTrue(TEXT("Pointed at: up"), Preview.IsActive());
	Preview.Reset();
	TestFalse(TEXT("A reset clears it"), Preview.IsActive());
	TestEqual(TEXT("...opacity too"), Preview.GetOpacity(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldOfViewPreviewTest, "Looter.Settings.FieldOfViewPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFieldOfViewPreviewTest::RunTest(const FString& Parameters)
{
	using Graphics = UGraphicsSettingsSubsystem;

	// The real character in a test level: the preview drives its first-person camera as the settings menu does. No
	// player controller or settings here, so the saved view is the camera's own, 90 degrees.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UPlayerLocomotionComponent* Locomotion = SpawnPlayer(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("A player"), Locomotion))
	{
		return false;
	}
	UPlayerViewComponent* View = BodyOf(Locomotion)->FindComponentByClass<UPlayerViewComponent>();
	const UCameraComponent* Camera = CameraOf(Locomotion);
	if (!TestNotNull(TEXT("Its view"), View) || !TestNotNull(TEXT("Its camera"), Camera))
	{
		return false;
	}
	const float Saved = WorldFieldOfView(Locomotion);
	const float ViewModel = Camera->FirstPersonFieldOfView;
	TestTrue(TEXT("First person, not previewing"), View->IsFirstPerson() && !View->IsPreviewingFieldOfView());

	// Dragging the slider wide, then narrow: the camera takes each at once.
	TestTrue(TEXT("First person shows the preview"), View->PreviewFieldOfView(110.f));
	TestTrue(TEXT("...and it is previewing"), View->IsPreviewingFieldOfView());
	TestNearlyEqual(TEXT("Wide: 110"), WorldFieldOfView(Locomotion), 110.f, 0.05f);
	View->PreviewFieldOfView(70.f);
	TestNearlyEqual(TEXT("Narrow: 70"), WorldFieldOfView(Locomotion), 70.f, 0.05f);
	View->PreviewFieldOfView(150.f);
	TestNearlyEqual(TEXT("Past the end: held at the widest"), WorldFieldOfView(Locomotion), Graphics::MaxFieldOfView, 0.05f);
	View->PreviewFieldOfView(10.f);
	TestNearlyEqual(TEXT("...and the narrowest"), WorldFieldOfView(Locomotion), Graphics::MinFieldOfView, 0.05f);
	// The setting widens the world, not the gun: the view model keeps its angle through all of it.
	TestNearlyEqual(TEXT("The gun's own angle stays"), Camera->FirstPersonFieldOfView, ViewModel, 0.05f);

	// Cancelled (or the preview ended): back to the saved view.
	View->EndFieldOfViewPreview();
	TestFalse(TEXT("Ended: not previewing"), View->IsPreviewingFieldOfView());
	TestNearlyEqual(TEXT("...and the camera is back at the saved field of view"), WorldFieldOfView(Locomotion), Saved, 0.05f);

	// A preview left standing ends by itself the first frame the game runs again (it lasts only while paused under the menu).
	View->PreviewFieldOfView(100.f);
	TestNearlyEqual(TEXT("Previewing 100"), WorldFieldOfView(Locomotion), 100.f, 0.05f);
	static_cast<UActorComponent*>(View)->TickComponent(Frame, LEVELTICK_All, nullptr);
	TestFalse(TEXT("A frame of the game ends the preview"), View->IsPreviewingFieldOfView());
	TestNearlyEqual(TEXT("...back at the saved field of view"), WorldFieldOfView(Locomotion), Saved, 0.05f);

	// Third person keeps its own field of view, so there is nothing to show there.
	View->SetViewMode(EPlayerViewMode::ThirdPerson);
	const UCameraComponent* ThirdPerson = FindThirdPersonCamera(BodyOf(Locomotion));
	if (TestNotNull(TEXT("The third-person camera"), ThirdPerson))
	{
		const float Own = ThirdPerson->FieldOfView;
		TestFalse(TEXT("Third person: the preview has nothing to show"), View->PreviewFieldOfView(110.f));
		TestFalse(TEXT("...and isn't previewing"), View->IsPreviewingFieldOfView());
		TestNearlyEqual(TEXT("...the third-person camera is as it was"), ThirdPerson->FieldOfView, Own, 0.05f);
	}
	View->SetViewMode(EPlayerViewMode::FirstPerson);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldOfViewMenuPreviewTest, "Looter.Settings.FieldOfViewMenuPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFieldOfViewMenuPreviewTest::RunTest(const FString& Parameters)
{
	// The slider in the settings menu itself: dragged, the view follows and the menu steps aside; let go, it stays and
	// the menu comes back; cancelled with Esc, the view goes back to where the slider was taken.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UPlayerLocomotionComponent* Locomotion = SpawnPlayer(World, FVector::ZeroVector);
	UPlayerViewComponent* View = Locomotion ? BodyOf(Locomotion)->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	USettingsMenuWidget* Menu = CreateWidget<USettingsMenuWidget>(World, USettingsMenuWidget::StaticClass());
	if (!TestTrue(TEXT("A player and the menu"), View && Menu))
	{
		return false;
	}
	Menu->UseViewForPreview(View);
	// Its Slate widget held for the test, as a viewport holds it in play: one let go destructs its widget at once.
	const TSharedRef<SWidget> MenuSlate = Menu->TakeWidget();
	USlider* Slider = Menu->GetFieldOfViewSlider();
	if (!TestNotNull(TEXT("The field of view slider"), Slider))
	{
		return false;
	}
	const float Saved = WorldFieldOfView(Locomotion);
	Slider->SetValue(Saved);
	// Opened fresh: the slider at the saved view, nothing previewing, the menu as usual.
	Menu->Open(ESettingsMenuMode::Pause);
	TestFalse(TEXT("Opened: no preview"), Menu->IsFieldOfViewPreviewActive() || View->IsPreviewingFieldOfView());
	TestEqual(TEXT("...the menu as usual"), Menu->GetViewFade(), 0.f);

	// Dragged: the view follows each move, and the menu steps aside (the dimming goes, the panel ghosts) while it is held.
	Slider->OnMouseCaptureBegin.Broadcast();
	TestTrue(TEXT("Taken: previewing"), Menu->IsFieldOfViewPreviewActive());
	Slider->SetValue(110.f);
	TestNearlyEqual(TEXT("Dragged to 110: the view is"), WorldFieldOfView(Locomotion), 110.f, 0.05f);
	Menu->AdvancePreviews(0.5f);
	TestEqual(TEXT("...and the menu is stepping aside"), Menu->GetViewFade(), 1.f);
	Slider->SetValue(75.f);
	TestNearlyEqual(TEXT("Dragged to 75: the view is"), WorldFieldOfView(Locomotion), 75.f, 0.05f);
	Menu->AdvancePreviews(5.f);
	TestEqual(TEXT("Held five seconds on: still aside"), Menu->GetViewFade(), 1.f);

	// Let go: the view stays where it was put, and the menu comes back once the preview lingers out.
	Slider->OnMouseCaptureEnd.Broadcast();
	TestNearlyEqual(TEXT("Let go: the view stays at 75"), WorldFieldOfView(Locomotion), 75.f, 0.05f);
	TestTrue(TEXT("...the menu lingers aside a moment"), Menu->IsFieldOfViewPreviewActive());
	TestFalse(TEXT("...and nothing is left to cancel"), Menu->CancelPreviewDrag());
	Menu->AdvancePreviews(SettingsMenu::PreviewLingerSeconds + 1.f);
	TestFalse(TEXT("The linger over: no preview"), Menu->IsFieldOfViewPreviewActive());
	TestEqual(TEXT("...the menu is back"), Menu->GetViewFade(), 0.f);

	// Taken at 75 and dragged wide, then Esc: back to 75, the menu stays open (the drag was the cancel's).
	Slider->OnMouseCaptureBegin.Broadcast();
	Slider->SetValue(105.f);
	TestNearlyEqual(TEXT("Dragged to 105: the view is"), WorldFieldOfView(Locomotion), 105.f, 0.05f);
	TestTrue(TEXT("Esc while held cancels the drag"), Menu->CancelPreviewDrag());
	TestNearlyEqual(TEXT("...the slider is back at 75"), Slider->GetValue(), 75.f, 0.05f);
	TestNearlyEqual(TEXT("...and the view"), WorldFieldOfView(Locomotion), 75.f, 0.05f);
	TestFalse(TEXT("...a second Esc has nothing to cancel (it closes the menu)"), Menu->CancelPreviewDrag());

	// Opening it again starts clean: the view back at its saved field of view, nothing lingering.
	Menu->Open(ESettingsMenuMode::Pause);
	TestFalse(TEXT("Opened again: no preview"), Menu->IsFieldOfViewPreviewActive() || View->IsPreviewingFieldOfView());
	TestEqual(TEXT("...the menu as usual"), Menu->GetViewFade(), 0.f);
	TestNearlyEqual(TEXT("...the view at the saved field of view"), WorldFieldOfView(Locomotion), Saved, 0.05f);

	// In third person there is nothing to show, so the menu doesn't step aside.
	View->SetViewMode(EPlayerViewMode::ThirdPerson);
	Slider->OnMouseCaptureBegin.Broadcast();
	Slider->SetValue(100.f);
	Menu->AdvancePreviews(0.5f);
	TestEqual(TEXT("Third person: the menu stays"), Menu->GetViewFade(), 0.f);
	Slider->OnMouseCaptureEnd.Broadcast();
	View->SetViewMode(EPlayerViewMode::FirstPerson);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinimapZoomPreviewTest, "Looter.UI.Minimap.ZoomPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMinimapZoomPreviewTest::RunTest(const FString& Parameters)
{
	using Graphics = UGraphicsSettingsSubsystem;

	// The live minimap's range at a zoom: 35 m at 1x, half that at 2x.
	TestNearlyEqual(TEXT("1x shows 35 m around you"), UHudMinimapWidget::RangeAtZoom(1.f), 3500.f, 0.5f);
	TestNearlyEqual(TEXT("2x shows half"), UHudMinimapWidget::RangeAtZoom(2.f), 1750.f, 0.5f);
	TestNearlyEqual(TEXT("0.5x shows twice"), UHudMinimapWidget::RangeAtZoom(0.5f), 7000.f, 0.5f);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UHudMinimapWidget* Map = CreateWidget<UHudMinimapWidget>(World, UHudMinimapWidget::StaticClass());
	USettingsMenuWidget* Menu = CreateWidget<USettingsMenuWidget>(World, USettingsMenuWidget::StaticClass());
	if (!TestTrue(TEXT("A minimap and the menu"), Map && Menu))
	{
		return false;
	}

	// A preview copy shows the zoom it is given, kept in range, and follows the setting again when given none.
	Map->SetPreviewZoom(2.f);
	TestEqual(TEXT("A preview zoom of 2 shows 2x"), Map->GetZoom(), 2.f);
	Map->SetPreviewZoom(9.f);
	TestEqual(TEXT("Past the end: the most zoomed"), Map->GetZoom(), Graphics::MaxMinimapZoom);
	Map->SetPreviewZoom(0.1f);
	TestEqual(TEXT("...and the least"), Map->GetZoom(), Graphics::MinMinimapZoom);
	Map->SetPreviewZoom(0.f);
	TestTrue(TEXT("None: it follows the setting (inside the range)"), Map->GetZoom() >= Graphics::MinMinimapZoom && Map->GetZoom() <= Graphics::MaxMinimapZoom);

	// The settings menu's slider drives its live copy as it moves.
	const TSharedRef<SWidget> MenuSlate = Menu->TakeWidget();
	USlider* Slider = Menu->GetMinimapZoomSlider();
	UHudMinimapWidget* Live = Menu->GetMinimapZoomPreview();
	if (!TestTrue(TEXT("The zoom slider and the live minimap"), Slider && Live))
	{
		return false;
	}
	Slider->SetValue(1.f);
	Menu->Open(ESettingsMenuMode::Pause);
	TestFalse(TEXT("Opened: no preview"), Menu->IsMinimapZoomPreviewActive());
	TestTrue(TEXT("...the live copy is put away"), Live->GetVisibility() == ESlateVisibility::Collapsed);
	const float Setting = Live->GetZoom();

	Slider->OnMouseCaptureBegin.Broadcast();
	TestTrue(TEXT("Taken: previewing"), Menu->IsMinimapZoomPreviewActive());
	Slider->SetValue(2.f);
	TestEqual(TEXT("Dragged to 2x: the live copy shows 2x"), Live->GetZoom(), 2.f);
	Slider->SetValue(0.5f);
	TestEqual(TEXT("Dragged to 0.5x: it follows"), Live->GetZoom(), 0.5f);
	Slider->SetValue(1.5f);
	TestEqual(TEXT("Dragged to 1.5x: it follows"), Live->GetZoom(), 1.5f);
	Menu->AdvancePreviews(0.3f);
	TestTrue(TEXT("...and the live copy is shown"), Live->GetVisibility() != ESlateVisibility::Collapsed && Live->GetRenderOpacity() > 0.f);

	// Esc while it is held: back to where the slider was taken.
	TestTrue(TEXT("Esc while held cancels the drag"), Menu->CancelPreviewDrag());
	TestNearlyEqual(TEXT("...the slider is back at 1x"), Slider->GetValue(), 1.f, 0.01f);
	TestEqual(TEXT("...and the live copy"), Live->GetZoom(), 1.f);
	TestFalse(TEXT("...nothing left to cancel"), Menu->CancelPreviewDrag());

	// Let go for good, it fades away.
	Menu->AdvancePreviews(SettingsMenu::PreviewLingerSeconds + 1.f);
	TestFalse(TEXT("The linger over: no preview"), Menu->IsMinimapZoomPreviewActive());
	TestTrue(TEXT("...the live copy is put away again"), Live->GetVisibility() == ESlateVisibility::Collapsed);

	// Opening the menu again hands the live copy back to the setting.
	Slider->OnMouseCaptureBegin.Broadcast();
	Slider->SetValue(2.f);
	Menu->Open(ESettingsMenuMode::Pause);
	TestEqual(TEXT("Opened again: the live copy follows the setting"), Live->GetZoom(), Setting);
	return true;
}

#endif
