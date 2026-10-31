// USettingsMenuWidget's input: its buttons and sliders, listening for a new key, and Escape.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

using namespace SettingsMenu;

void USettingsMenuWidget::HandleMinimapSizeChanged(float Value)
{
	// The HUD takes the new size the moment it shows again; meanwhile the outline in its corner shows it.
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SetMinimapScale(Value, /*bSave*/ false);
	}
	ShowMinimapScale(Value);
	if (!bRefreshingSliders)
	{
		SizeState.Nudge();
	}
}

void USettingsMenuWidget::HandleMinimapSizeGrabbed()
{
	SizeState.Grab(MinimapSlider ? MinimapSlider->GetValue() : 0.f);
}

void USettingsMenuWidget::HandleMinimapSizeReleased()
{
	SizeState.Release();
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SaveSettings();
		SetStatus(FString::Printf(TEXT("Minimap size %d%%."), FMath::RoundToInt(Graphics->GetMinimapScale() * 100.f)), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

// The field of view and minimap zoom sliders, which preview what they set, are in SettingsMenuPreview.cpp.

void USettingsMenuWidget::HandleLookSensitivityChanged(float Value)
{
	// The view turns at the new speed from the next mouse move or stick push (as soon as the game resumes, under the
	// pause menu).
	if (UControlSettingsSubsystem* Controls = GetControls())
	{
		Controls->SetLookSensitivity(Value, /*bSave*/ false);
	}
	if (LookSensitivityValue)
	{
		LookSensitivityValue->SetText(FText::FromString(LookSensitivityText(UControlSettingsSubsystem::ClampLookSensitivity(Value))));
	}
}

void USettingsMenuWidget::HandleLookSensitivityReleased()
{
	if (UControlSettingsSubsystem* Controls = GetControls())
	{
		Controls->SaveSettings();
		SetStatus(FString::Printf(TEXT("Look sensitivity %s."), *LookSensitivityText(Controls->GetLookSensitivity())), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

void USettingsMenuWidget::HandleCameraShakeChanged(float Value)
{
	// The next frame's shakes and kicks show the new strength (the boss fights' too).
	if (UControlSettingsSubsystem* Controls = GetControls())
	{
		Controls->SetCameraShake(Value, /*bSave*/ false);
	}
	if (CameraShakeValue)
	{
		CameraShakeValue->SetText(FText::FromString(PercentText(UControlSettingsSubsystem::ClampCameraShake(Value))));
	}
}

void USettingsMenuWidget::HandleCameraShakeReleased()
{
	if (UControlSettingsSubsystem* Controls = GetControls())
	{
		Controls->SaveSettings();
		SetStatus(FString::Printf(TEXT("Camera shake %s."), *PercentText(Controls->GetCameraShake())), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

void USettingsMenuWidget::HandleControllerRumbleChanged(float Value)
{
	// The next shot or hit taken on a gamepad rumbles at the new strength.
	if (UControlSettingsSubsystem* Controls = GetControls())
	{
		Controls->SetControllerRumble(Value, /*bSave*/ false);
	}
	if (ControllerRumbleValue)
	{
		ControllerRumbleValue->SetText(FText::FromString(PercentText(UControlSettingsSubsystem::ClampControllerRumble(Value))));
	}
}

void USettingsMenuWidget::HandleControllerRumbleReleased()
{
	if (UControlSettingsSubsystem* Controls = GetControls())
	{
		Controls->SaveSettings();
		SetStatus(FString::Printf(TEXT("Controller rumble %s."), *PercentText(Controls->GetControllerRumble())), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

void USettingsMenuWidget::HandleTransparencyChanged(float Value)
{
	// Every UI follows at once, this menu included, so the effect shows while dragging.
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SetUITransparency(Value, /*bSave*/ false);
	}
	if (TransparencyValue)
	{
		TransparencyValue->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f))));
	}
}

void USettingsMenuWidget::HandleTransparencyReleased()
{
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SaveSettings();
		SetStatus(FString::Printf(TEXT("UI transparency %d%%."), FMath::RoundToInt(Graphics->GetUITransparency() * 100.f)), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

void USettingsMenuWidget::HandleButton(ULooterButton* Button)
{
	UKeyBindingSubsystem* Bindings = GetBindings();
	if (!Button)
	{
		return;
	}

	if (Button->Action == ActionClose)
	{
		StopListening();
		OnClose.ExecuteIfBound();
		return;
	}
	if (Button->Action == ActionTab)
	{
		// The tab's own click is its sound (a tab button sounds UI.Tab).
		StopListening();
		ShowPage(Button->Index);
		SetKeyboardFocus();
		return;
	}
	if (Button->Action == ActionQuality && Button->Index >= 0 && Button->Index < UE_ARRAY_COUNT(Qualities))
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			const EGraphicsQuality Quality = Qualities[Button->Index];
			Graphics->SetQuality(Quality);
			SetStatus(FString::Printf(TEXT("Quality: %s. %s"), *UGraphicsSettingsSubsystem::QualityName(Quality), *QualityPresetHelp(Quality)),
				LooterUI::Color::TextDim());
		}
		RefreshGraphics();
		RefreshKeyLabels();
		SetKeyboardFocus();
		return;
	}
	if (Button->Action == ActionMotionBlurOn || Button->Action == ActionMotionBlurOff)
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			const bool bBlur = Button->Action == ActionMotionBlurOn;
			Graphics->SetMotionBlurEnabled(bBlur);
			SetStatus(bBlur ? TEXT("Motion blur on.") : TEXT("Motion blur off."), LooterUI::Color::TextDim());
		}
		RefreshGraphics();
		RefreshKeyLabels();
		SetKeyboardFocus();
		return;
	}
	if (Button->Action == ActionMinimapOn || Button->Action == ActionMinimapOff)
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			const bool bMinimap = Button->Action == ActionMinimapOn;
			Graphics->SetMinimapShown(bMinimap);
			SetStatus(bMinimap ? TEXT("Minimap on.") : TEXT("Minimap off."), LooterUI::Color::TextDim());
		}
		RefreshGraphics();
		RefreshKeyLabels();
		SetKeyboardFocus();
		return;
	}
	if (Button->Action == ActionFrameRateOn || Button->Action == ActionFrameRateOff)
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			const bool bFrameRate = Button->Action == ActionFrameRateOn;
			Graphics->SetFrameRateShown(bFrameRate);
			SetStatus(bFrameRate ? TEXT("FPS counter on.") : TEXT("FPS counter off."), LooterUI::Color::TextDim());
		}
		RefreshGraphics();
		RefreshKeyLabels();
		SetKeyboardFocus();
		return;
	}
	if (Button->Action == ActionCrosshairColour && Button->Index >= 0 && Button->Index < UE_ARRAY_COUNT(AllCrosshairColours))
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			// The HUD reads it every frame: the crosshair wears it the moment the game resumes.
			const ECrosshairColour Colour = AllCrosshairColours[Button->Index];
			Graphics->SetCrosshairColour(Colour);
			SetStatus(FString::Printf(TEXT("Crosshair colour: %s."), *UGraphicsSettingsSubsystem::CrosshairColourName(Colour)), LooterUI::Color::TextDim());
		}
		RefreshGraphics();
		RefreshKeyLabels();
		SetKeyboardFocus();
		return;
	}
	if (Button->Action == ActionHintsOn || Button->Action == ActionHintsOff)
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			const bool bHints = Button->Action == ActionHintsOn;
			Graphics->SetControlHintsShown(bHints);
			SetStatus(bHints ? TEXT("Control hints on: a key shows until you've used it.") : TEXT("Control hints off."), LooterUI::Color::TextDim());
		}
		RefreshGraphics();
		RefreshKeyLabels();
		SetKeyboardFocus();
		return;
	}
	if (Button->Action == ActionSubtitlesOn || Button->Action == ActionSubtitlesOff)
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			// The caption widget follows the setting within a frame; a line on screen when the game resumes goes or stays at once.
			const bool bSubtitles = Button->Action == ActionSubtitlesOn;
			Graphics->SetSubtitlesShown(bSubtitles);
			SetStatus(bSubtitles ? TEXT("Subtitles on: spoken lines show their words.") : TEXT("Subtitles off: spoken lines show no words."),
				LooterUI::Color::TextDim());
		}
		RefreshGraphics();
		RefreshKeyLabels();
		SetKeyboardFocus();
		return;
	}

	if (Button->Action == ActionRebind)
	{
		StartListening(Button->Index);
	}
	else if ((Button->Action == ActionHoldMode || Button->Action == ActionToggleMode) && Bindings && Bindings->GetBindings().IsValidIndex(Button->Index))
	{
		StopListening();
		const FRebindableKey& Binding = Bindings->GetBindings()[Button->Index];
		const bool bToggle = Button->Action == ActionToggleMode;
		Bindings->SetToggleMode(Binding.Id, bToggle);
		const FString Name = Binding.DisplayName.ToString();
		SetStatus(bToggle ? FString::Printf(TEXT("%s: press to turn on or off."), *Name) : FString::Printf(TEXT("%s: hold the key."), *Name), LooterUI::Color::TextDim());
	}
	else if (Button->Action == ActionResetOne && Bindings && Bindings->GetBindings().IsValidIndex(Button->Index))
	{
		StopListening();
		Bindings->ResetKey(Bindings->GetBindings()[Button->Index].Id);
		SetStatus(TEXT("Restored default key."), LooterUI::Color::TextDim());
	}
	else if (Button->Action == ActionResetAll && Bindings)
	{
		StopListening();
		Bindings->ResetAll();
		SetStatus(TEXT("Every key and Hold / Toggle choice is back to its default."), LooterUI::Color::TextDim());
	}

	RefreshKeyLabels();
	// Buttons hand focus to the game viewport; take it back so we keep receiving keys.
	SetKeyboardFocus();
}

void USettingsMenuWidget::StartListening(int32 BindingIndex)
{
	ListeningIndex = BindingIndex;
	StopNavHold();
	// Let the mouse wheel be captured as a key instead of scrolling the list.
	Body->SetConsumeMouseWheel(EConsumeMouseWheel::Never);
	// The list is the keyboard's and the mouse's: a controller's buttons keep their own (B lets go instead).
	SetStatus(LooterUI::IsMenuGamepad() ? TEXT("Press the new keyboard key or mouse button (B to cancel).") : TEXT("Press the new key (Esc to cancel)."),
		LooterUI::Color::Accent());
	RefreshKeyLabels();
	RefreshPrompts();
}

void USettingsMenuWidget::StopListening()
{
	ListeningIndex = INDEX_NONE;
	// The wheel stays the page's, a row at a time (SettingsMenuScroll.cpp), never the list's own scroll.
	RefreshPrompts();
}

void USettingsMenuWidget::AssignKey(const FKey& Key)
{
	UKeyBindingSubsystem* Bindings = GetBindings();
	if (!Bindings || !Bindings->GetBindings().IsValidIndex(ListeningIndex))
	{
		StopListening();
		return;
	}

	const FName Id = Bindings->GetBindings()[ListeningIndex].Id;
	StopListening();

	if (Key.IsGamepadKey())
	{
		SetStatus(TEXT("Gamepad buttons can't be assigned here; use a keyboard key or mouse button."), LooterUI::Color::Worse());
	}
	else
	{
		Bindings->SetKey(Id, Key);
		const FName Conflict = Bindings->FindConflict(Id, Key);
		if (Conflict.IsNone())
		{
			SetStatus(FString::Printf(TEXT("%s is now %s."), *Bindings->GetDisplayName(Id).ToString(), *Key.GetDisplayName().ToString()), LooterUI::Color::TextDim());
		}
		else
		{
			SetStatus(FString::Printf(TEXT("Warning: %s is also bound to %s."), *Key.GetDisplayName().ToString(),
				*Bindings->GetDisplayName(Conflict).ToString()), LooterUI::Color::Worse());
		}
	}
	RefreshKeyLabels();
}

FReply USettingsMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (ListeningIndex != INDEX_NONE)
	{
		// The key that started the wait (Enter, A) may still be held and repeating: only a fresh press is the new key. A
		// stick's nudge (its direction keys) is no answer either.
		if (InKeyEvent.IsRepeat() || MenuNav::IsStickKey(Key))
		{
			return FReply::Handled();
		}
		if (MenuNav::IsBack(Key))
		{
			StopListening();
			SetStatus(TEXT("Cancelled."), LooterUI::Color::TextDim());
			LooterSound::Play2D(this, LooterSoundCue::Back);
			RefreshKeyLabels();
		}
		else
		{
			AssignKey(Key);
		}
		return FReply::Handled();
	}

	// Everything else, Esc and the controller included: SettingsMenuNav.cpp.
	return HandleNavKey(Key, InKeyEvent.IsRepeat()) ? FReply::Handled() : Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply USettingsMenuWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	NotePointer();
	if (ListeningIndex != INDEX_NONE)
	{
		AssignKey(InMouseEvent.GetEffectingButton());
		return FReply::Handled();
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USettingsMenuWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (ListeningIndex != INDEX_NONE)
	{
		AssignKey(InMouseEvent.GetWheelDelta() > 0.f ? EKeys::MouseScrollUp : EKeys::MouseScrollDown);
		return FReply::Handled();
	}
	NotePointer();
	// The list's own scroll is off (it would stop part-way down a row): a notch is a whole row.
	if (InMouseEvent.GetWheelDelta() != 0.f)
	{
		ScrollByRows(InMouseEvent.GetWheelDelta() > 0.f ? -1 : 1);
		return FReply::Handled();
	}
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}
