// UVendorWidget: its buttons, keys and the mouse (W/S, A/D, E, J, R, Esc; a click acts, a hover chooses, a right-click marks
// junk).

#include "UI/Vendor/VendorWidget.h"
#include "Audio/LooterSoundCues.h"
#include "UI/Style/LooterButton.h"
#include "InputCoreTypes.h"

void UVendorWidget::HandleButton(ULooterButton* Button)
{
	if (!Button)
	{
		return;
	}
	if (Button->Action == VendorActions::Close)
	{
		Close();
		return;
	}
	if (Confirm == EConfirm::None)
	{
		if (Button->Action == VendorActions::Row)
		{
			Select(Button->Index);
			Act(Button->Index);
		}
	}
	else if (Button->Action == VendorActions::Accept)
	{
		AcceptConfirm();
	}
	else if (Button->Action == VendorActions::Back)
	{
		CloseConfirm();
		PlayCue(LooterSoundCue::Back);
	}
	// Clicking handed the keyboard to the game viewport (LooterButton); take it back so the keys keep reaching the screen.
	if (IsInViewport())
	{
		SetKeyboardFocus();
	}
}

void UVendorWidget::HandleHovered(ULooterButton* Button)
{
	if (Button && Confirm == EConfirm::None)
	{
		Select(Button->Index, true);
	}
}

FReply UVendorWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const bool bFresh = !InKeyEvent.IsRepeat();
	const bool bAct = bFresh && (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom);
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		if (Confirm != EConfirm::None)
		{
			CloseConfirm();
			PlayCue(LooterSoundCue::Back);
		}
		else
		{
			Close();
		}
		return FReply::Handled();
	}
	if (Confirm != EConfirm::None)
	{
		// A confirm takes only its own keys.
		if (bAct)
		{
			AcceptConfirm();
		}
		return FReply::Handled();
	}
	if (Key == EKeys::W || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		Step(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::S || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		Step(1);
		return FReply::Handled();
	}
	if (Key == EKeys::A || Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
	{
		SwitchSide(false);
		return FReply::Handled();
	}
	if (Key == EKeys::D || Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
	{
		SwitchSide(true);
		return FReply::Handled();
	}
	// The Interact key that opened the screen may still be held and repeating: only a fresh press acts.
	if (bAct)
	{
		Act(Selected);
		return FReply::Handled();
	}
	if (bFresh && (Key == EKeys::J || Key == EKeys::Gamepad_FaceButton_Top))
	{
		ToggleJunk();
		return FReply::Handled();
	}
	if (bFresh && (Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Left))
	{
		SellAllJunk();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UVendorWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A right-click marks the backpack gun under the pointer (hovering chose it) as junk.
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && Confirm == EConfirm::None)
	{
		ToggleJunk();
	}
	// A click on no button ends here rather than in the game viewport, which would take the keyboard from the screen.
	return FReply::Handled();
}
