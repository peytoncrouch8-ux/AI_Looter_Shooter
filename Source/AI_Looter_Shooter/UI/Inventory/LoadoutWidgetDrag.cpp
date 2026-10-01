#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Templates/UnrealTemplate.h"

using namespace LooterUI;
using namespace LoadoutParts;

// ---------------------------------------------------------------------------
// Drag and drop: a gun pressed and moved follows the mouse and lands where it's let go
// ---------------------------------------------------------------------------

namespace
{
	bool IsUnder(const UWidget* Widget, const FVector2D& ScreenPosition)
	{
		return Widget && Widget->GetCachedGeometry().IsUnderLocation(ScreenPosition);
	}
}

bool ULoadoutWidget::FindGunCard(const FVector2D& ScreenPosition, EZone& OutZone, int32& OutIndex) const
{
	for (int32 SlotIndex = 0; SlotIndex < SlotCards.Num(); ++SlotIndex)
	{
		if (SlotItem(SlotIndex) && IsUnder(SlotCards[SlotIndex].Button, ScreenPosition))
		{
			OutZone = EZone::Slots;
			OutIndex = SlotIndex;
			return true;
		}
	}
	// Rows scrolled out of the list keep their geometry: only the list's visible part counts.
	if (IsUnder(ListBox, ScreenPosition))
	{
		for (int32 Row = 0; Row < ListCards.Num(); ++Row)
		{
			if (ListItem(Row) && IsUnder(ListCards[Row].Button, ScreenPosition))
			{
				OutZone = EZone::Backpack;
				OutIndex = Row;
				return true;
			}
		}
	}
	return false;
}

ULoadoutWidget::FDropTarget ULoadoutWidget::FindDropTarget(const FVector2D& ScreenPosition) const
{
	for (int32 SlotIndex = 0; SlotIndex < SlotCards.Num(); ++SlotIndex)
	{
		if (IsUnder(SlotCards[SlotIndex].Button, ScreenPosition))
		{
			return { EDropKind::Slot, SlotIndex };
		}
	}
	if (IsUnder(ListBox, ScreenPosition))
	{
		for (int32 Row = 0; Row < ListCards.Num(); ++Row)
		{
			if (IsUnder(ListCards[Row].Button, ScreenPosition))
			{
				return { EDropKind::BackpackRow, Row };
			}
		}
		return { EDropKind::Backpack, INDEX_NONE };
	}
	if (IsUnder(StageImage, ScreenPosition))
	{
		return { EDropKind::Stage, INDEX_NONE };
	}
	return {};
}

FString ULoadoutWidget::DropActionText(const FDropTarget& Target) const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory)
	{
		return FString();
	}
	const bool bBackpackRoom = Inventory->GetBackpack().Num() < Inventory->BackpackCapacity;
	if (PressZone == EZone::Slots)
	{
		switch (Target.Kind)
		{
		case EDropKind::Slot:
			// Into a free slot it lands after the last gun (so the last gun stays where it is).
			if (Target.Index == PressIndex || (!SlotItem(Target.Index) && PressIndex == NumWeapons() - 1))
			{
				return FString();
			}
			return SlotItem(Target.Index) ? FString::Printf(TEXT("Swap with slot %d"), Target.Index + 1)
				: FString::Printf(TEXT("Move to slot %d"), NumWeapons());
		case EDropKind::BackpackRow:
			if (ListItem(Target.Index))
			{
				return TEXT("Swap with this gun");
			}
			return bBackpackRoom ? TEXT("Store in the backpack") : FString();
		case EDropKind::Backpack:
			return bBackpackRoom ? TEXT("Store in the backpack") : FString();
		case EDropKind::Stage:
			return PressIndex != GetActiveSlot() ? TEXT("Take in hand") : FString();
		default:
			return FString();
		}
	}
	switch (Target.Kind)
	{
	case EDropKind::Slot:
		return SlotItem(Target.Index) ? FString::Printf(TEXT("Swap into slot %d"), Target.Index + 1)
			: FString::Printf(TEXT("Equip in slot %d"), NumWeapons() + 1);
	case EDropKind::Stage:
		return TEXT("Take in hand");
	default:
		return FString();
	}
}

void ULoadoutWidget::BeginItemDrag()
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	const FWeaponInstanceData* Item = !Inventory ? nullptr
		: PressZone == EZone::Slots ? SlotItem(PressIndex)
		: Inventory->GetBackpack().IsValidIndex(PressIndex) ? &Inventory->GetBackpack()[PressIndex] : nullptr;
	if (!Item || !DragGhost)
	{
		return;
	}
	bItemDrag = true;
	PickedSlot.Reset();
	DropTarget = { EDropKind::Slot, INDEX_NONE - 1 };

	GhostBox->ClearChildren();
	GhostBox->AddChildToVerticalBox(MakeGunPicture(WidgetTree, *Item, FVector2D(120.f, 40.f)))
		->SetHorizontalAlignment(HAlign_Center);
	GhostBox->AddChildToVerticalBox(FittedLabel(WidgetTree, LooterWeaponText::Name(*Item), 10, LooterWeaponText::Color(*Item), 50))
		->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	DragGhost->SetVisibility(ESlateVisibility::HitTestInvisible);
	UpdateItemDrag(PressPosition);
}

void ULoadoutWidget::UpdateItemDrag(const FVector2D& ScreenPosition)
{
	// Just below and right of the pointer, so the card under it stays in sight.
	GhostSlot->SetPosition(ToPage(ScreenPosition) + FVector2D(18.f, 14.f));
	const FDropTarget Target = FindDropTarget(ScreenPosition);
	if (Target == DropTarget)
	{
		return;
	}
	DropTarget = Target;
	const FString Action = DropActionText(Target);
	GhostAction->SetText(FText::FromString((Action.IsEmpty() ? FString(TEXT("Let go to put it back")) : Action).ToUpper()));
	GhostAction->SetColorAndOpacity(FSlateColor(Action.IsEmpty() ? Color::TextDim() : Color::Accent()));
	Restyle();
	RefreshPrompts();
}

void ULoadoutWidget::FinishItemDrag(const FVector2D& ScreenPosition)
{
	const FDropTarget Target = FindDropTarget(ScreenPosition);
	const bool bApplies = !DropActionText(Target).IsEmpty();
	CancelItemDrag();
	if (bApplies)
	{
		ApplyDrop(Target);
	}
}

void ULoadoutWidget::CancelItemDrag()
{
	bPressPending = false;
	bItemDrag = false;
	DropTarget = {};
	if (DragGhost)
	{
		DragGhost->SetVisibility(ESlateVisibility::Collapsed);
	}
	Restyle();
	RefreshPrompts();
	SetKeyboardFocus();
}

void ULoadoutWidget::ApplyDrop(const FDropTarget& Target)
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory)
	{
		return;
	}
	// Where the gun ends up, for the cursor to follow it there.
	int32 NewSlot = INDEX_NONE;
	int32 NewBackpackIndex = INDEX_NONE;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		if (PressZone == EZone::Slots)
		{
			const int32 From = PressIndex;
			switch (Target.Kind)
			{
			case EDropKind::Slot:
				if (SlotItem(Target.Index))
				{
					Inventory->SwapSlots(From, Target.Index);
					NewSlot = Target.Index;
				}
				else if (Inventory->MoveSlot(From, Target.Index))
				{
					NewSlot = Inventory->GetWeapons().Num() - 1;
				}
				break;
			case EDropKind::BackpackRow:
				if (ListItem(Target.Index))
				{
					NewBackpackIndex = ListOrder[Target.Index];
					Inventory->SwapSlotWithBackpack(From, NewBackpackIndex);
					break;
				}
				// A free row: stored like anywhere else on the backpack.
				[[fallthrough]];
			case EDropKind::Backpack:
				if (Inventory->StashSlot(From))
				{
					NewBackpackIndex = Inventory->GetBackpack().Num() - 1;
				}
				break;
			case EDropKind::Stage:
				Inventory->EquipSlot(From);
				NewSlot = From;
				break;
			default:
				break;
			}
		}
		else
		{
			const int32 BackpackIndex = PressIndex;
			switch (Target.Kind)
			{
			case EDropKind::Slot:
				if (SlotItem(Target.Index))
				{
					Inventory->SwapSlotWithBackpack(Target.Index, BackpackIndex);
					NewSlot = Target.Index;
				}
				else if (Inventory->MoveBackpackToSlot(BackpackIndex))
				{
					NewSlot = Inventory->GetWeapons().Num() - 1;
				}
				break;
			case EDropKind::Stage:
				if (Inventory->EquipFromBackpack(BackpackIndex))
				{
					NewSlot = FMath::Max(Inventory->GetActiveSlot(), 0);
				}
				break;
			default:
				break;
			}
		}
	}

	// The cursor follows the gun to where it landed.
	if (NewSlot != INDEX_NONE)
	{
		Zone = EZone::Slots;
		ChosenSlot = CursorIndex = NewSlot;
	}
	Refresh();
	if (NewBackpackIndex != INDEX_NONE)
	{
		const int32 Row = ListOrder.IndexOfByKey(NewBackpackIndex);
		if (Row != INDEX_NONE)
		{
			MoveCursorTo(EZone::Backpack, Row, true);
		}
	}
}
