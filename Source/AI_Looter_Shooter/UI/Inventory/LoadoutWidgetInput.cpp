#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Inventory/LoadoutPaintLayer.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "AI_Looter_Shooter.h"
#include "UI/Inventory/LoadoutStage.h"
#include "UI/Style/LooterButton.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Rendering/DrawElements.h"
#include "Templates/UnrealTemplate.h"

using namespace LooterUI;
using namespace LoadoutParts;

// ---------------------------------------------------------------------------
// Cursor and actions
// ---------------------------------------------------------------------------

void ULoadoutWidget::MoveCursor(int32 Columns, int32 Rows)
{
	if (Columns < 0 && Zone == EZone::Backpack)
	{
		MoveCursorTo(EZone::Slots, ChosenSlot, false);
	}
	else if (Columns > 0 && Zone == EZone::Slots)
	{
		if (!ListCards.IsEmpty())
		{
			MoveCursorTo(EZone::Backpack, 0, true);
		}
	}
	else if (Rows != 0)
	{
		const int32 Count = Zone == EZone::Slots ? SlotCards.Num() : ListCards.Num();
		MoveCursorTo(Zone, FMath::Clamp(CursorIndex + Rows, 0, FMath::Max(Count - 1, 0)), true);
	}
}

void ULoadoutWidget::MoveCursorTo(EZone NewZone, int32 NewIndex, bool bScrollIntoView)
{
	if (NewZone == Zone && NewIndex == CursorIndex)
	{
		return;
	}
	Zone = NewZone;
	CursorIndex = NewIndex;
	// On the slots the cursor chooses the slot (unless one is picked up); the backpack list is for that slot.
	if (Zone == EZone::Slots && !PickedSlot.IsSet() && ChosenSlot != CursorIndex)
	{
		ChosenSlot = CursorIndex;
		RebuildList();
	}
	Restyle();
	RefreshDetails();
	RefreshPrompts();
	if (bScrollIntoView && Zone == EZone::Backpack && ListCards.IsValidIndex(CursorIndex))
	{
		ListBox->ScrollWidgetIntoView(ListCards[CursorIndex].Button, false, EDescendantScrollDestination::IntoView, 8.f);
	}
}

void ULoadoutWidget::Activate()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory)
	{
		return;
	}

	if (Zone == EZone::Slots)
	{
		const int32 SlotIndex = CursorIndex;
		if (PickedSlot.IsSet())
		{
			// Put the picked-up gun here: swap with the gun here, or move it into the free slot (it lands after the last gun).
			const int32 From = PickedSlot.GetValue();
			PickedSlot.Reset();
			if (From != SlotIndex)
			{
				const bool bSwap = SlotItem(SlotIndex) != nullptr;
				bool bMoved = false;
				{
					TGuardValue<bool> RefreshAfter(bApplyingAction, true);
					bMoved = bSwap ? Inventory->SwapSlots(From, SlotIndex) : Inventory->MoveSlot(From, SlotIndex);
				}
				ChosenSlot = CursorIndex = bMoved && !bSwap ? FMath::Max(Inventory->GetWeapons().Num() - 1, 0) : SlotIndex;
			}
			Refresh();
			return;
		}
		if (SlotItem(SlotIndex))
		{
			PickedSlot = SlotIndex;
			ChosenSlot = SlotIndex;
			Restyle();
			RefreshDetails();
			RefreshPrompts();
		}
		else if (!ListCards.IsEmpty())
		{
			// A free slot: choose something from the backpack for it.
			MoveCursorTo(EZone::Backpack, 0, true);
		}
		return;
	}

	// A free backpack slot: the chosen slot's gun is stored there, and the cursor follows it into the list.
	if (!ListOrder.IsValidIndex(CursorIndex))
	{
		if (SlotItem(ChosenSlot) && ListCards.IsValidIndex(CursorIndex))
		{
			PickedSlot.Reset();
			bool bStored = false;
			{
				TGuardValue<bool> RefreshAfter(bApplyingAction, true);
				bStored = Inventory->StashSlot(ChosenSlot);
			}
			Refresh();
			const int32 Row = bStored ? ListOrder.IndexOfByKey(Inventory->GetBackpack().Num() - 1) : INDEX_NONE;
			if (Row != INDEX_NONE)
			{
				MoveCursorTo(EZone::Backpack, Row, true);
			}
		}
		return;
	}

	// A backpack gun goes into the chosen slot, trading places with the gun there.
	const int32 BackpackIndex = ListOrder[CursorIndex];
	const bool bSlotFilled = SlotItem(ChosenSlot) != nullptr;
	PickedSlot.Reset();
	bool bEquipped = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		if (bSlotFilled)
		{
			Inventory->SwapSlotWithBackpack(ChosenSlot, BackpackIndex);
		}
		else
		{
			bEquipped = Inventory->MoveBackpackToSlot(BackpackIndex);
		}
	}
	if (bEquipped)
	{
		// Into a free slot (the first one): show it there.
		Zone = EZone::Slots;
		ChosenSlot = CursorIndex = FMath::Max(Inventory->GetWeapons().Num() - 1, 0);
	}
	Refresh();
	if (bSlotFilled)
	{
		// The cursor stays on the gun that came out of the slot, so pressing again swaps straight back.
		const int32 Row = ListOrder.IndexOfByKey(BackpackIndex);
		if (Row != INDEX_NONE)
		{
			MoveCursorTo(EZone::Backpack, Row, true);
		}
	}
}

void ULoadoutWidget::Hold()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory)
	{
		return;
	}
	PickedSlot.Reset();
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		if (Zone == EZone::Slots)
		{
			if (SlotItem(CursorIndex))
			{
				Inventory->EquipSlot(CursorIndex);
			}
		}
		else if (ListOrder.IsValidIndex(CursorIndex) && Inventory->EquipFromBackpack(ListOrder[CursorIndex]))
		{
			// It's in a slot now, in hand: follow it there.
			Zone = EZone::Slots;
			ChosenSlot = CursorIndex = FMath::Max(Inventory->GetActiveSlot(), 0);
		}
	}
	Refresh();
}

void ULoadoutWidget::Drop()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory)
	{
		return;
	}
	PickedSlot.Reset();
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		if (Zone == EZone::Slots)
		{
			if (SlotItem(CursorIndex))
			{
				Inventory->DropSlot(CursorIndex);
			}
		}
		else if (ListOrder.IsValidIndex(CursorIndex))
		{
			Inventory->DropFromBackpack(ListOrder[CursorIndex]);
		}
	}
	Refresh();
}

void ULoadoutWidget::CancelPick()
{
	if (PickedSlot.IsSet())
	{
		PickedSlot.Reset();
		Restyle();
		RefreshDetails();
		RefreshPrompts();
	}
}

void ULoadoutWidget::HandleCardClicked(ULooterButton* Button)
{
	HandleCardHovered(Button);
	Activate();
	// Clicking handed keyboard focus to the game viewport (LooterButton); take it back for the screen's keys.
	SetKeyboardFocus();
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

void ULoadoutWidget::HandleCardHovered(ULooterButton* Button)
{
	if (Button)
	{
		MoveCursorTo(Button->Action == ActionSlot ? EZone::Slots : EZone::Backpack, Button->Index, false);
	}
}

FReply ULoadoutWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;

	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		// Esc puts down a picked-up slot first, then closes.
		if (PickedSlot.IsSet())
		{
			CancelPick();
		}
		else
		{
			Close();
		}
		return FReply::Handled();
	}
	if (Bindings ? Bindings->IsInventoryKey(Key) : (Key == EKeys::Tab || Key == EKeys::I))
	{
		Close();
		return FReply::Handled();
	}
	if ((Key == EKeys::Two || Key == EKeys::Gamepad_RightShoulder) && !PickedSlot.IsSet())
	{
		if (ALooterHUD* HUD = OwningHUD.Get())
		{
			HUD->ShowInventoryPage(EInventoryPage::Bestiary);
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

	const bool bInteractKey = Bindings && Bindings->GetKey(TEXT("Interact")) == Key;
	if (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom || bInteractKey)
	{
		Activate();
		return FReply::Handled();
	}
	if (Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Top)
	{
		Hold();
		return FReply::Handled();
	}
	if (Key == EKeys::Q || Key == EKeys::Gamepad_FaceButton_Left)
	{
		Drop();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------------------
// Turning the stand-in
// ---------------------------------------------------------------------------

FReply ULoadoutWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && PickedSlot.IsSet())
	{
		CancelPick();
		return FReply::Handled();
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && StageImage && Stage.IsValid()
		&& StageImage->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		bDragging = true;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULoadoutWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging)
	{
		// Dragging right turns the stand-in's front to the right.
		if (ALoadoutStage* StagePtr = Stage.Get())
		{
			StagePtr->AddTurn(-InMouseEvent.GetCursorDelta().X * DragTurnRate);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply ULoadoutWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		SetKeyboardFocus();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void ULoadoutWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bDragging = false;
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
