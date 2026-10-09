// USettingsMenuWidget's input: its buttons and sliders, listening for a new key, and Escape.

#include "UI/Menus/SettingsMenuWidget.h"
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

namespace
{
	/** What a preset gives, for the status line when it's picked. */
	const TCHAR* QualityHint(EGraphicsQuality Quality)
	{
		switch (Quality)
		{
		case EGraphicsQuality::Low:    return TEXT("Fastest: no Lumen or Nanite, simple shadows, thinner grass.");
		case EGraphicsQuality::Medium: return TEXT("120 fps at 1080p on a Radeon RX 580, the minimum spec.");
		case EGraphicsQuality::High:   return TEXT("Lumen lighting and Nanite detail.");
		case EGraphicsQuality::Epic:   return TEXT("Everything at its best, with TSR anti-aliasing.");
		}
		return TEXT("");
	}
}

void USettingsMenuWidget::HandleMinimapSizeChanged(float Value)
{
	// The HUD takes the new size the moment it shows again; meanwhile the outline in its corner shows it.
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SetMinimapScale(Value, /*bSave*/ false);
	}
	ShowMinimapScale(Value);
	PreviewLinger = 0.8f;
}

void USettingsMenuWidget::HandleMinimapSizeGrabbed()
{
	bMinimapSliderHeld = true;
}

void USettingsMenuWidget::HandleMinimapSizeReleased()
{
	bMinimapSliderHeld = false;
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SaveSettings();
		SetStatus(FString::Printf(TEXT("Minimap size %d%%."), FMath::RoundToInt(Graphics->GetMinimapScale() * 100.f)), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

void USettingsMenuWidget::HandleMinimapZoomChanged(float Value)
{
	// The HUD's map follows the moment the menu closes.
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SetMinimapZoom(Value, /*bSave*/ false);
	}
	if (MinimapZoomValue)
	{
		MinimapZoomValue->SetText(FText::FromString(FString::Printf(TEXT("%.1fx"), Value)));
	}
}

void USettingsMenuWidget::HandleMinimapZoomReleased()
{
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SaveSettings();
		SetStatus(FString::Printf(TEXT("Minimap zoom %.1fx."), Graphics->GetMinimapZoom()), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

void USettingsMenuWidget::HandleFieldOfViewChanged(float Value)
{
	// The first-person view follows on the game's next frame (as soon as it resumes, under the pause menu).
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SetFirstPersonFieldOfView(Value, /*bSave*/ false);
	}
	if (FieldOfViewValue)
	{
		FieldOfViewValue->SetText(FText::FromString(DegreesText(UGraphicsSettingsSubsystem::ClampFieldOfView(Value))));
	}
}

void USettingsMenuWidget::HandleFieldOfViewReleased()
{
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SaveSettings();
		SetStatus(FString::Printf(TEXT("First-person field of view %s."), *DegreesText(Graphics->GetFirstPersonFieldOfView())), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

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
	if (Button->Action == ActionSaveQuit)
	{
		StopListening();
		SetStatus(TEXT("Saving..."), LooterUI::Color::TextDim());
		OnSaveAndQuit.ExecuteIfBound();
		return;
	}
	if (Button->Action == ActionQuality && Button->Index >= 0 && Button->Index < UE_ARRAY_COUNT(Qualities))
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			const EGraphicsQuality Quality = Qualities[Button->Index];
			Graphics->SetQuality(Quality);
			SetStatus(FString::Printf(TEXT("Quality: %s. %s"), *UGraphicsSettingsSubsystem::QualityName(Quality), QualityHint(Quality)),
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
		SetStatus(TEXT("All controls reset to defaults."), LooterUI::Color::TextDim());
	}

	RefreshKeyLabels();
	// Buttons hand focus to the game viewport; take it back so we keep receiving keys.
	SetKeyboardFocus();
}

void USettingsMenuWidget::StartListening(int32 BindingIndex)
{
	ListeningIndex = BindingIndex;
	// Let the mouse wheel be captured as a key instead of scrolling the list.
	Body->SetConsumeMouseWheel(EConsumeMouseWheel::Never);
	SetStatus(TEXT("Press the new key (Esc to cancel)."), LooterUI::Color::Accent());
	RefreshKeyLabels();
}

void USettingsMenuWidget::StopListening()
{
	ListeningIndex = INDEX_NONE;
	if (Body)
	{
		Body->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	}
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
		if (Key == EKeys::Escape)
		{
			StopListening();
			SetStatus(TEXT("Cancelled."), LooterUI::Color::TextDim());
			RefreshKeyLabels();
		}
		else
		{
			AssignKey(Key);
		}
		return FReply::Handled();
	}

	if (Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Right)
	{
		// The pause menu's close is the HUD's to sound; the main menu has no HUD to.
		if (Mode == ESettingsMenuMode::MainMenu)
		{
			LooterSound::Play2D(this, LooterSoundCue::Close);
		}
		OnClose.ExecuteIfBound();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply USettingsMenuWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
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
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}
