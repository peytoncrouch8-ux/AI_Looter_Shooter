// UBenchWidget: the keys and the mouse: moving, choosing, scrapping and throwing out by key, a right-click on a box part,
// and turning the stand.

#include "UI/Bench/BenchWidget.h"
#include "Audio/LooterSound.h"
#include "UI/Bench/BenchStage.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Components/Image.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"

using namespace LoadoutParts;

// ---------------------------------------------------------------------------
// Keys
// ---------------------------------------------------------------------------

FReply UBenchWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	// The Interact key held to open the bench repeats: only fresh presses act.
	const bool bFresh = !InKeyEvent.IsRepeat();
	const bool bBackKey = Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right;
	const bool bInteractKey = Bindings ? Bindings->GetKey(TEXT("Interact")) == Key : Key == EKeys::E;
	const bool bInventoryKey = Bindings ? Bindings->IsInventoryKey(Key) : (Key == EKeys::Tab || Key == EKeys::I);
	const bool bAcceptKey = !bInteractKey && (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Bottom);

	if (Confirm != EConfirm::None)
	{
		// The confirm holds every key: Esc backs out of it, Enter goes ahead.
		if (bBackKey)
		{
			CloseConfirm();
			PlayCue(LooterSoundCue::Back);
		}
		else if (bAcceptKey && bFresh)
		{
			AcceptConfirm();
		}
		return FReply::Handled();
	}
	if (bBackKey)
	{
		// Esc backs out of a scrapping first, then closes.
		if (bScrapping)
		{
			StopScrapping();
		}
		else
		{
			Close();
		}
		return FReply::Handled();
	}
	if (bInteractKey || bInventoryKey)
	{
		if (bFresh)
		{
			Close();
		}
		return FReply::Handled();
	}

	struct FMove { FKey Keys[3]; int32 Columns; int32 Rows; };
	const FMove Moves[] = {
		{ { EKeys::Up, EKeys::W, EKeys::Gamepad_DPad_Up }, 0, -1 },
		{ { EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down }, 0, 1 },
		{ { EKeys::Left, EKeys::A, EKeys::Gamepad_DPad_Left }, -1, 0 },
		{ { EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right }, 1, 0 },
	};
	for (const FMove& Move : Moves)
	{
		if (Key == Move.Keys[0] || Key == Move.Keys[1] || Key == Move.Keys[2])
		{
			MoveCursor(Move.Columns, Move.Rows);
			return FReply::Handled();
		}
	}
	if (bAcceptKey)
	{
		if (bFresh)
		{
			Activate();
		}
		return FReply::Handled();
	}
	if (Key == EKeys::X || Key == EKeys::Gamepad_FaceButton_Top)
	{
		if (bFresh)
		{
			BeginScrap();
		}
		return FReply::Handled();
	}
	if (Key == EKeys::Q || Key == EKeys::Gamepad_FaceButton_Left)
	{
		if (bFresh && Column == EColumn::Parts && !bScrapping)
		{
			AskDiscard(CursorIndex);
		}
		return FReply::Handled();
	}
	if ((Key == EKeys::Gamepad_LeftShoulder || Key == EKeys::Gamepad_RightShoulder) && !bScrapping && Guns.Num() > 1)
	{
		// The shoulders step through the guns, wrapping round.
		const int32 Step = Key == EKeys::Gamepad_RightShoulder ? 1 : -1;
		ChooseGun((ChosenGun + Step + Guns.Num()) % Guns.Num());
		PlayCue(LooterSoundCue::Click);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------------------
// The mouse: throwing parts out, and turning the stand
// ---------------------------------------------------------------------------

FReply UBenchWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FKey Button = InMouseEvent.GetEffectingButton();
	const FVector2D Where = InMouseEvent.GetScreenSpacePosition();
	if (Confirm != EConfirm::None)
	{
		// Over the confirm, a right-click backs out like Esc; anything else waits for its buttons.
		if (Button == EKeys::RightMouseButton)
		{
			CloseConfirm();
			PlayCue(LooterSoundCue::Back);
		}
		return FReply::Handled();
	}
	if (Button == EKeys::RightMouseButton)
	{
		if (bScrapping)
		{
			StopScrapping();
			return FReply::Handled();
		}
		// A right-click on a box part asks to throw it out.
		for (int32 Row = 0; Row < PartRows.Num(); ++Row)
		{
			if (PartRows[Row].Button && PartRows[Row].Button->GetCachedGeometry().IsUnderLocation(Where))
			{
				MoveCursorTo(EColumn::Parts, Row, false);
				AskDiscard(Row);
				break;
			}
		}
		return FReply::Handled();
	}
	if (Button == EKeys::LeftMouseButton && StageImage && Stage.IsValid() && StageImage->GetCachedGeometry().IsUnderLocation(Where))
	{
		bDragging = true;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	// A click on nothing ends here rather than in the game viewport, which would take the keyboard from the screen.
	return FReply::Handled();
}

FReply UBenchWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging)
	{
		// Dragging right turns the gun's front to the right.
		if (ABenchStage* StagePtr = Stage.Get())
		{
			StagePtr->AddTurn(-InMouseEvent.GetCursorDelta().X * DragTurnRate);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UBenchWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		SetKeyboardFocus();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UBenchWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bDragging = false;
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

FReply UBenchWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	if (InAnalogEvent.GetKey() == EKeys::Gamepad_RightX)
	{
		const float Value = InAnalogEvent.GetAnalogValue();
		TurnInput = FMath::Abs(Value) > 0.2f ? -Value : 0.f;
		return FReply::Handled();
	}
	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}
