// ULoadoutWidget: the cursor, the keys and what they do (swap, hold, move, stow, drop, inspect, sort). The mouse is
// LoadoutWidgetMouse.cpp, dragging guns LoadoutWidgetDrag.cpp.

#include "UI/Inventory/LoadoutWidget.h"
#include "Audio/LooterSound.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutGunStage.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "UI/Style/LooterButton.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Components/ScrollBox.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "Templates/UnrealTemplate.h"

using namespace LoadoutParts;

namespace
{
	/** How far A / D turn the gun on Inspect (degrees a press). */
	constexpr float KeyTurnDegrees = 20.f;
}

// ---------------------------------------------------------------------------
// The cursor
// ---------------------------------------------------------------------------

void ULoadoutWidget::MoveCursor(int32 Rows)
{
	// One list from the first slot to the last backpack gun: W / S walk straight through it.
	const int32 NumSlotRows = SlotRows.Num();
	const int32 Total = NumSlotRows + ListRows.Num();
	if (Total == 0)
	{
		return;
	}
	const int32 Here = Zone == EZone::Slots ? CursorIndex : NumSlotRows + CursorIndex;
	const int32 There = FMath::Clamp(Here + Rows, 0, Total - 1);
	if (There < NumSlotRows)
	{
		MoveCursorTo(EZone::Slots, There, false, true);
	}
	else
	{
		MoveCursorTo(EZone::Backpack, There - NumSlotRows, true, true);
	}
}

void ULoadoutWidget::JumpTo(EZone NewZone)
{
	if (NewZone == EZone::Slots)
	{
		MoveCursorTo(EZone::Slots, Zone == EZone::Slots ? CursorIndex : TargetSlot, false, true);
	}
	else if (!ListRows.IsEmpty() && Zone != EZone::Backpack)
	{
		MoveCursorTo(EZone::Backpack, 0, true, true);
	}
}

void ULoadoutWidget::MoveCursorTo(EZone NewZone, int32 NewIndex, bool bScrollIntoView, bool bSound)
{
	if (NewZone == Zone && NewIndex == CursorIndex)
	{
		return;
	}
	Zone = NewZone;
	CursorIndex = NewIndex;
	MarkSeen();
	Restyle();
	RefreshCard();
	RefreshPrompts();
	RefreshShowcase();
	if (bScrollIntoView && Zone == EZone::Backpack && ListRows.IsValidIndex(CursorIndex) && ListBox)
	{
		ListBox->ScrollWidgetIntoView(ListRows[CursorIndex].Button, false, EDescendantScrollDestination::IntoView, 8.f);
	}
	if (bSound)
	{
		PlayCue(LooterSoundCue::Hover);
	}
}

void ULoadoutWidget::MarkSeen()
{
	if (const FWeaponInstanceData* Gun = CursorItem())
	{
		SeenGuns.Add(LoadoutRules::GunIdentity(*Gun));
	}
}

void ULoadoutWidget::FlashRow(EZone RowZone, int32 Index)
{
	FlashZone = RowZone;
	FlashIndex = Index;
	FlashAge = 0.f;
}

void ULoadoutWidget::PlayCue(const TCHAR* Cue) const
{
	LooterSound::Play2D(this, FName(Cue));
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------

void ULoadoutWidget::RunAction(EAction Action)
{
	switch (Action)
	{
	case EAction::Activate: Activate(); break;
	case EAction::Hold:     Hold(); break;
	case EAction::Move:     PickSlot(); break;
	case EAction::Stow:     Stow(); break;
	case EAction::Drop:     Drop(); break;
	case EAction::Inspect:  ToggleInspect(); break;
	case EAction::Sort:     CycleSort(); break;
	case EAction::Close:    Close(); break;
	case EAction::Cancel:
		if (bInspecting && !PickedSlot.IsSet())
		{
			ToggleInspect();
		}
		else
		{
			CancelPick();
		}
		break;
	default:
		break;
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
			if (From == SlotIndex)
			{
				PlayCue(LooterSoundCue::Back);
				Refresh();
				return;
			}
			const bool bSwap = SlotItem(SlotIndex) != nullptr;
			bool bMoved = false;
			{
				TGuardValue<bool> RefreshAfter(bApplyingAction, true);
				bMoved = bSwap ? Inventory->SwapSlots(From, SlotIndex) : Inventory->MoveSlot(From, SlotIndex);
			}
			CursorIndex = bMoved && !bSwap ? FMath::Max(Inventory->GetWeapons().Num() - 1, 0) : SlotIndex;
			TargetSlot = CursorIndex;
			Refresh();
			if (bMoved)
			{
				FlashRow(EZone::Slots, CursorIndex);
			}
			PlayCue(bMoved ? Sounds::Equip : LooterSoundCue::Denied);
			return;
		}
		// A slot is the swap target: choose its replacement from the backpack, sorted best for it.
		if (ListOrder.IsEmpty())
		{
			PlayCue(LooterSoundCue::Denied);
			return;
		}
		TargetSlot = SlotIndex;
		RebuildList();
		PlayCue(LooterSoundCue::Click);
		MoveCursorTo(EZone::Backpack, 0, true, false);
		RefreshCounts();
		return;
	}

	// A backpack gun goes into the target slot, trading places with the gun there (or into the first free slot).
	if (!ListOrder.IsValidIndex(CursorIndex))
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	const int32 BackpackIndex = ListOrder[CursorIndex];
	const bool bSlotFilled = SlotItem(TargetSlot) != nullptr;
	PickedSlot.Reset();
	bool bEquipped = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bEquipped = bSlotFilled ? Inventory->SwapSlotWithBackpack(TargetSlot, BackpackIndex) : Inventory->MoveBackpackToSlot(BackpackIndex);
	}
	if (bEquipped)
	{
		// The cursor follows the gun to its slot: its card and the showcase greet it there.
		Zone = EZone::Slots;
		CursorIndex = TargetSlot = bSlotFilled ? TargetSlot : FMath::Max(Inventory->GetWeapons().Num() - 1, 0);
	}
	Refresh();
	if (bEquipped)
	{
		FlashRow(EZone::Slots, CursorIndex);
	}
	PlayCue(bEquipped ? Sounds::Equip : LooterSoundCue::Denied);
}

void ULoadoutWidget::Hold()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory || PickedSlot.IsSet())
	{
		return;
	}
	bool bDone = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		if (Zone == EZone::Slots)
		{
			if (SlotItem(CursorIndex) && CursorIndex != Inventory->GetActiveSlot())
			{
				Inventory->EquipSlot(CursorIndex);
				bDone = true;
			}
		}
		else if (ListOrder.IsValidIndex(CursorIndex) && Inventory->EquipFromBackpack(ListOrder[CursorIndex]))
		{
			// It's in a slot now, in hand: follow it there.
			Zone = EZone::Slots;
			CursorIndex = TargetSlot = FMath::Max(Inventory->GetActiveSlot(), 0);
			bDone = true;
		}
	}
	Refresh();
	if (bDone)
	{
		FlashRow(EZone::Slots, CursorIndex);
	}
	// The gun's own equip sound plays as it comes up; the screen only clicks.
	PlayCue(bDone ? LooterSoundCue::Click : LooterSoundCue::Denied);
}

void ULoadoutWidget::PickSlot()
{
	if (Zone != EZone::Slots || !SlotItem(CursorIndex) || NumWeapons() < 2 || PickedSlot.IsSet())
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	PickedSlot = CursorIndex;
	Restyle();
	RefreshPrompts();
	PlayCue(LooterSoundCue::Click);
}

void ULoadoutWidget::Stow()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory || Zone != EZone::Slots || !SlotItem(CursorIndex) || PickedSlot.IsSet())
	{
		return;
	}
	if (!HasBackpackRoom())
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	bool bStowed = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bStowed = Inventory->StashSlot(CursorIndex);
	}
	Refresh();
	PlayCue(bStowed ? Sounds::Stow : LooterSoundCue::Denied);
}

void ULoadoutWidget::Drop()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory || PickedSlot.IsSet())
	{
		return;
	}
	bool bDropped = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		if (Zone == EZone::Slots)
		{
			bDropped = SlotItem(CursorIndex) && Inventory->DropSlot(CursorIndex);
		}
		else if (ListOrder.IsValidIndex(CursorIndex))
		{
			bDropped = Inventory->DropFromBackpack(ListOrder[CursorIndex]);
		}
	}
	Refresh();
	PlayCue(bDropped ? Sounds::Drop : LooterSoundCue::Denied);
}

void ULoadoutWidget::ToggleInspect()
{
	if (!bInspecting && !CursorItem())
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	bInspecting = !bInspecting;
	ApplyMode();
	CardIntroAge = 0.f;
	if (CardScroll)
	{
		CardScroll->ScrollToStart();
	}
	RefreshCard();
	RefreshPrompts();
	PlayCue(bInspecting ? Sounds::Inspect : LooterSoundCue::Back);
}

void ULoadoutWidget::CycleSort()
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory || Inventory->GetBackpack().Num() < 2)
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	// The cursor stays on the same gun as the list re-sorts under it.
	const int32 BackpackIndex = Zone == EZone::Backpack && ListOrder.IsValidIndex(CursorIndex) ? ListOrder[CursorIndex] : INDEX_NONE;
	Sort = LoadoutRules::NextSort(Sort);
	RebuildList();
	if (BackpackIndex != INDEX_NONE)
	{
		CursorIndex = FMath::Max(ListOrder.IndexOfByKey(BackpackIndex), 0);
		if (ListRows.IsValidIndex(CursorIndex) && ListBox)
		{
			ListBox->ScrollWidgetIntoView(ListRows[CursorIndex].Button, false, EDescendantScrollDestination::IntoView, 8.f);
		}
	}
	Restyle();
	RefreshCounts();
	RefreshPrompts();
	PlayCue(LooterSoundCue::Tab);
}

void ULoadoutWidget::CancelPick()
{
	if (PickedSlot.IsSet())
	{
		PickedSlot.Reset();
		Restyle();
		RefreshPrompts();
		PlayCue(LooterSoundCue::Back);
	}
}

// ---------------------------------------------------------------------------
// Keys
// ---------------------------------------------------------------------------

FReply ULoadoutWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (bPressPending && (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right))
	{
		// Esc lets go of a dragged gun where it was.
		CancelItemDrag();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return HandleKey(Key) ? FReply::Handled() : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool ULoadoutWidget::HandleKey(const FKey& Key)
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;

	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		// Esc backs out one step at a time: a picked-up slot, then Inspect, then the screen.
		if (PickedSlot.IsSet())
		{
			CancelPick();
		}
		else if (bInspecting)
		{
			ToggleInspect();
		}
		else
		{
			Close();
		}
		return true;
	}
	if (Bindings ? Bindings->IsInventoryKey(Key) : (Key == EKeys::Tab || Key == EKeys::I))
	{
		Close();
		return true;
	}
	if ((Key == EKeys::Two || Key == EKeys::Gamepad_RightShoulder) && !PickedSlot.IsSet())
	{
		if (ALooterHUD* HUD = OwningHUD.Get())
		{
			HUD->ShowInventoryPage(EInventoryPage::Bestiary);
		}
		return true;
	}
	if (Key == EKeys::Three && !PickedSlot.IsSet())
	{
		if (ALooterHUD* HUD = OwningHUD.Get())
		{
			HUD->ShowInventoryPage(EInventoryPage::Missions);
		}
		return true;
	}

	if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up)
	{
		MoveCursor(-1);
		return true;
	}
	if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down)
	{
		MoveCursor(1);
		return true;
	}
	const bool bLeft = Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left;
	const bool bRight = Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right;
	if (bLeft || bRight)
	{
		// On Inspect they turn the gun; otherwise they jump between the slots and the backpack.
		if (bInspecting)
		{
			if (ALoadoutGunStage* StagePtr = Stage.Get())
			{
				StagePtr->AddTurn(bLeft ? KeyTurnDegrees : -KeyTurnDegrees);
			}
		}
		else
		{
			JumpTo(bLeft ? EZone::Slots : EZone::Backpack);
		}
		return true;
	}

	const bool bInteractKey = Bindings && Bindings->GetKey(TEXT("Interact")) == Key;
	if (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom || bInteractKey)
	{
		RunAction(EAction::Activate);
		return true;
	}
	struct FBinding { FKey Keys[2]; EAction Action; };
	const FBinding Actions[] = {
		{ { EKeys::F, EKeys::Gamepad_FaceButton_Top }, EAction::Hold },
		{ { EKeys::Q, EKeys::Gamepad_FaceButton_Left }, EAction::Drop },
		{ { EKeys::M, EKeys::Gamepad_LeftShoulder }, EAction::Move },
		{ { EKeys::C, EKeys::Gamepad_LeftThumbstick }, EAction::Stow },
		{ { EKeys::X, EKeys::Gamepad_RightThumbstick }, EAction::Inspect },
		{ { EKeys::R, EKeys::Gamepad_Special_Left }, EAction::Sort },
	};
	for (const FBinding& Binding : Actions)
	{
		if (Key == Binding.Keys[0] || Key == Binding.Keys[1])
		{
			RunAction(Binding.Action);
			return true;
		}
	}
	return false;
}

