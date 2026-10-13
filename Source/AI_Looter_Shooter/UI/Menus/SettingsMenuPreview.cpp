// USettingsMenuWidget's live previews: the field of view slider changes the view behind the menu (which steps aside while the
// slider is held), and the minimap zoom slider shows a live minimap in the HUD minimap's corner. The minimap size slider's
// outline is in SettingsMenuRows.cpp and SettingsMenuInput.cpp; the three share the corner and the fading (SizeState,
// ZoomState, FieldOfViewState).

#include "UI/Menus/SettingsMenuPreview.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"

using namespace SettingsMenu;

namespace
{
	/** A preview shown at Opacity: it fades with it, and is put away (not drawn, not ticked) once it is gone. */
	void ShowFaded(UWidget* Widget, float Opacity)
	{
		if (!Widget)
		{
			return;
		}
		if (Widget->GetRenderOpacity() != Opacity)
		{
			Widget->SetRenderOpacity(Opacity);
		}
		const ESlateVisibility Wanted = Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (Widget->GetVisibility() != Wanted)
		{
			Widget->SetVisibility(Wanted);
		}
	}
}

USlider* USettingsMenuWidget::GetFieldOfViewSlider() const
{
	return FieldOfViewSlider;
}

USlider* USettingsMenuWidget::GetMinimapZoomSlider() const
{
	return MinimapZoomSlider;
}

UHudMinimapWidget* USettingsMenuWidget::GetMinimapZoomPreview() const
{
	return MinimapZoomPreview;
}

void USettingsMenuWidget::UseViewForPreview(UPlayerViewComponent* View)
{
	ViewForPreview = View;
}

UPlayerViewComponent* USettingsMenuWidget::FindPlayerView() const
{
	if (UPlayerViewComponent* View = ViewForPreview.Get())
	{
		return View;
	}
	const APawn* Pawn = GetOwningPlayerPawn();
	return Pawn ? Pawn->FindComponentByClass<UPlayerViewComponent>() : nullptr;
}

// ---------------------------------------------------------------------------
// The previews, frame by frame
// ---------------------------------------------------------------------------

void USettingsMenuWidget::AdvancePreviews(float DeltaSeconds)
{
	// The minimap's outline (its size) and live copy (its zoom) sit in the same corner: whichever slider is held or pointed
	// at now has it, and the other is put away at once. Under keys or a controller, the slider with the focus is the one
	// pointed at (a still pointer resting on a slider isn't).
	const bool bKeysDrive = Nav.IsShown();
	const UWidget* Focused = bKeysDrive ? Nav.GetFocusedWidget() : nullptr;
	const bool bSizeAimed = MinimapSlider && (bKeysDrive ? Focused == MinimapSlider.Get() : MinimapSlider->IsHovered());
	const bool bZoomAimed = MinimapZoomSlider && (bKeysDrive ? Focused == MinimapZoomSlider.Get() : MinimapZoomSlider->IsHovered());
	const float Outline = SizeState.Tick(DeltaSeconds, bSizeAimed);
	const float Live = ZoomState.Tick(DeltaSeconds, bZoomAimed);
	const bool bSizeNow = SizeState.IsHeld() || bSizeAimed;
	const bool bZoomNow = ZoomState.IsHeld() || bZoomAimed;
	ShowFaded(MinimapPreview, bZoomNow ? 0.f : Outline);
	ShowFaded(MinimapZoomPreview, bSizeNow ? 0.f : Live);

	// The view behind the menu: while the field of view slider is held (and a moment after) the dimming goes and the panel
	// ghosts, so the world can be seen changing. Only a view that shows the change (first person) does it.
	const float Fade = FieldOfViewState.Tick(DeltaSeconds);
	if (!FieldOfViewState.IsActive() && Fade <= 0.f)
	{
		bFieldOfViewInView = false;
	}
	const float Wanted = bFieldOfViewInView ? Fade : 0.f;
	if (Wanted != AppliedViewFade)
	{
		ApplyViewFade(Wanted);
	}
}

void USettingsMenuWidget::ApplyViewFade(float Fade)
{
	AppliedViewFade = Fade;
	if (Dimmer)
	{
		// The dimming's own opacity: its brush color is the UI transparency setting's.
		Dimmer->SetRenderOpacity(1.f - Fade);
	}
	if (Panel)
	{
		Panel->SetRenderOpacity(FMath::Lerp(1.f, ViewPreviewPanelOpacity, Fade));
	}
}

// ---------------------------------------------------------------------------
// The minimap zoom slider
// ---------------------------------------------------------------------------

void USettingsMenuWidget::HandleMinimapZoomChanged(float Value)
{
	// Giving the slider its saved value (opening the menu) is not a move.
	if (bRefreshingSliders)
	{
		return;
	}
	// The HUD's own map follows the moment the menu closes; the live copy in the corner shows it as the slider moves.
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SetMinimapZoom(Value, /*bSave*/ false);
	}
	if (MinimapZoomValue)
	{
		MinimapZoomValue->SetText(FText::FromString(FString::Printf(TEXT("%.1fx"), Value)));
	}
	ZoomState.Nudge();
	if (MinimapZoomPreview)
	{
		MinimapZoomPreview->SetPreviewZoom(Value);
	}
}

void USettingsMenuWidget::HandleMinimapZoomGrabbed()
{
	ZoomState.Grab(MinimapZoomSlider ? MinimapZoomSlider->GetValue() : 0.f);
}

void USettingsMenuWidget::HandleMinimapZoomReleased()
{
	// Only a slider that was taken is let go (a drag cancelled with Esc has been, already).
	if (!ZoomState.IsHeld())
	{
		return;
	}
	ZoomState.Release();
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SaveSettings();
		SetStatus(FString::Printf(TEXT("Minimap zoom %.1fx."), Graphics->GetMinimapZoom()), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

// ---------------------------------------------------------------------------
// The field of view slider
// ---------------------------------------------------------------------------

void USettingsMenuWidget::HandleFieldOfViewChanged(float Value)
{
	// Giving the slider its saved value (opening the menu) is not a move.
	if (bRefreshingSliders)
	{
		return;
	}
	const float Degrees = UGraphicsSettingsSubsystem::ClampFieldOfView(Value);
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SetFirstPersonFieldOfView(Value, /*bSave*/ false);
	}
	if (FieldOfViewValue)
	{
		FieldOfViewValue->SetText(FText::FromString(DegreesText(Degrees)));
	}
	ShowFieldOfViewPreview(Degrees);
}

void USettingsMenuWidget::ShowFieldOfViewPreview(float Degrees)
{
	FieldOfViewState.Nudge();
	// The menu from the main menu has no view behind it.
	UPlayerViewComponent* View = FindPlayerView();
	if (!View)
	{
		bFieldOfViewInView = false;
		return;
	}
	bFieldOfViewInView = View->PreviewFieldOfView(Degrees);
	if (!bFieldOfViewInView)
	{
		SetStatus(TEXT("Third person keeps its own field of view: switch to first person to see this one."), LooterUI::Color::TextDim());
	}
}

void USettingsMenuWidget::HandleFieldOfViewGrabbed()
{
	FieldOfViewState.Grab(FieldOfViewSlider ? FieldOfViewSlider->GetValue() : 0.f);
	// The menu starts stepping aside as the handle is taken, before it has moved.
	const UPlayerViewComponent* View = FindPlayerView();
	bFieldOfViewInView = View && View->IsFirstPerson();
}

void USettingsMenuWidget::HandleFieldOfViewReleased()
{
	// Only a slider that was taken is let go (a drag cancelled with Esc has been, already).
	if (!FieldOfViewState.IsHeld())
	{
		return;
	}
	FieldOfViewState.Release();
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SaveSettings();
		FString Message = FString::Printf(TEXT("First-person field of view %s."), *DegreesText(Graphics->GetFirstPersonFieldOfView()));
		if (FindPlayerView() && !bFieldOfViewInView)
		{
			Message += TEXT(" Third person keeps its own.");
		}
		SetStatus(Message, LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

// ---------------------------------------------------------------------------
// Cancelling a drag
// ---------------------------------------------------------------------------

bool USettingsMenuWidget::CancelPreviewDrag()
{
	const bool bFieldOfView = FieldOfViewState.IsHeld();
	const bool bZoom = !bFieldOfView && ZoomState.IsHeld();
	if (!bFieldOfView && !bZoom)
	{
		return false;
	}

	// Back to the value the slider had when it was taken: its own change event puts the setting and the preview back (the
	// view, the live minimap), then it is let go as if the mouse had been (saving the setting as it now is).
	if (bFieldOfView)
	{
		const float Back = FieldOfViewState.GetGrabbedValue();
		if (FieldOfViewSlider)
		{
			FieldOfViewSlider->SetValue(Back);
		}
		HandleFieldOfViewReleased();
		SetStatus(FString::Printf(TEXT("Field of view back to %s."), *DegreesText(UGraphicsSettingsSubsystem::ClampFieldOfView(Back))), LooterUI::Color::TextDim());
	}
	else
	{
		const float Back = ZoomState.GetGrabbedValue();
		if (MinimapZoomSlider)
		{
			MinimapZoomSlider->SetValue(Back);
		}
		HandleMinimapZoomReleased();
		SetStatus(FString::Printf(TEXT("Minimap zoom back to %.1fx."), Back), LooterUI::Color::TextDim());
	}

	// The mouse button may still be down: the slider lets go of the pointer too, so it stops following it.
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().ReleaseAllPointerCapture();
	}
	return true;
}
