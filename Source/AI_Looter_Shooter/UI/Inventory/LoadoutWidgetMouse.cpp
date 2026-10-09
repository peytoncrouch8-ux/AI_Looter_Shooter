// ULoadoutWidget: the mouse. Clicks on rows, tabs and key hints; a press on a gun's row that becomes a drag
// (LoadoutWidgetDrag.cpp); the showcase (drag to turn the gun, click to inspect it); right-click to back out; the
// gamepad's right stick turning the gun.

#include "UI/Inventory/LoadoutWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutGunStage.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "Components/Image.h"
#include "InputCoreTypes.h"

using namespace LoadoutParts;

// ---------------------------------------------------------------------------
// Clicks
// ---------------------------------------------------------------------------

void ULoadoutWidget::HandleRowClicked(ULooterButton* Button)
{
	HandleRowHovered(Button);
	Activate();
	// Clicking handed keyboard focus to the game viewport (LooterButton); take it back for the screen's keys.
	SetKeyboardFocus();
}

void ULoadoutWidget::HandleRowHovered(ULooterButton* Button)
{
	if (Button && !bInspecting)
	{
		MoveCursorTo(Button->Action == ActionSlot ? EZone::Slots : EZone::Backpack, Button->Index, false, false);
	}
}

void ULoadoutWidget::HandleTabClicked(ULooterButton* Button)
{
	ALooterHUD* HUD = OwningHUD.Get();
	if (HUD && Button && Button->Index != static_cast<int32>(EInventoryPage::Loadout))
	{
		HUD->ShowInventoryPage(static_cast<EInventoryPage>(Button->Index));
	}
	else
	{
		SetKeyboardFocus();
	}
}

void ULoadoutWidget::HandlePromptClicked(ULooterButton* Button)
{
	if (Button)
	{
		RunAction(static_cast<EAction>(Button->Index));
	}
	SetKeyboardFocus();
}

// ---------------------------------------------------------------------------
// Presses, drags and the showcase
// ---------------------------------------------------------------------------

FReply ULoadoutWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A press on a gun is ours rather than its row button's: a click if the mouse lets go where it pressed, a drag once
	// it moves (NativeOnMouseMove), and a drop where it's let go (NativeOnMouseButtonUp).
	EZone CardZone = EZone::Slots;
	int32 CardIndex = INDEX_NONE;
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !bPressPending && !bShowcasePress
		&& FindGunCard(InMouseEvent.GetScreenSpacePosition(), CardZone, CardIndex))
	{
		bPressPending = true;
		bItemDrag = false;
		PressZone = CardZone;
		PressIndex = CardZone == EZone::Slots ? CardIndex : ListOrder[CardIndex];
		PressPosition = InMouseEvent.GetScreenSpacePosition();
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULoadoutWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		// Right-click backs out: a dragged gun goes back, a picked-up slot is put down, Inspect closes.
		if (bPressPending)
		{
			CancelItemDrag();
			return FReply::Handled().ReleaseMouseCapture();
		}
		if (PickedSlot.IsSet())
		{
			CancelPick();
			return FReply::Handled();
		}
		if (bInspecting)
		{
			ToggleInspect();
			return FReply::Handled();
		}
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && StageImage && Stage.IsValid() && Stage->HasGun()
		&& StageImage->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		bShowcasePress = true;
		bTurning = false;
		PressPosition = InMouseEvent.GetScreenSpacePosition();
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULoadoutWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPressPending)
	{
		const FVector2D Position = InMouseEvent.GetScreenSpacePosition();
		if (!bItemDrag && FVector2D::Distance(Position, PressPosition) >= DragStartDistance)
		{
			BeginItemDrag();
		}
		if (bItemDrag)
		{
			UpdateItemDrag(Position);
		}
		return FReply::Handled();
	}
	if (bShowcasePress)
	{
		if (!bTurning && FVector2D::Distance(InMouseEvent.GetScreenSpacePosition(), PressPosition) >= DragStartDistance)
		{
			bTurning = true;
		}
		// Dragging right turns the gun's front to the right.
		ALoadoutGunStage* StagePtr = Stage.Get();
		if (bTurning && StagePtr)
		{
			StagePtr->AddTurn(-InMouseEvent.GetCursorDelta().X * DragTurnRate);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply ULoadoutWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPressPending && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (bItemDrag)
		{
			FinishItemDrag(InMouseEvent.GetScreenSpacePosition());
		}
		else
		{
			// It never moved: a click on the row it was pressed on.
			bPressPending = false;
			const int32 Row = PressZone == EZone::Slots ? PressIndex : ListOrder.IndexOfByKey(PressIndex);
			const TArray<FRow>& Rows = PressZone == EZone::Slots ? SlotRows : ListRows;
			if (Rows.IsValidIndex(Row))
			{
				HandleRowClicked(Rows[Row].Button);
			}
		}
		return FReply::Handled().ReleaseMouseCapture();
	}
	if (bShowcasePress && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// A click on the gun (no turn): Inspect, or back.
		const bool bClicked = !bTurning;
		bShowcasePress = bTurning = false;
		if (bClicked)
		{
			ToggleInspect();
		}
		SetKeyboardFocus();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void ULoadoutWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bShowcasePress = bTurning = false;
	if (bPressPending)
	{
		CancelItemDrag();
	}
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

FReply ULoadoutWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	if (InAnalogEvent.GetKey() == EKeys::Gamepad_RightX)
	{
		const float Value = InAnalogEvent.GetAnalogValue();
		TurnInput = FMath::Abs(Value) > 0.2f ? -Value : 0.f;
		return FReply::Handled();
	}
	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}
