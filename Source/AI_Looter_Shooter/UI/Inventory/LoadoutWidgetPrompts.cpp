// ULoadoutWidget: the words around the lists: the counts and the sort over them, the ammo line under them, and the prompt
// bar at the bottom saying what the keys do right now (each hint a button too).

#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"

using namespace LooterUI;
using namespace LoadoutParts;

// ---------------------------------------------------------------------------
// The counts and the ammo
// ---------------------------------------------------------------------------

void ULoadoutWidget::RefreshCounts()
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory || !EquippedCount)
	{
		return;
	}
	EquippedCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), NumWeapons(), NumSlots())));
	BackpackCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Inventory->GetBackpack().Num(), Inventory->BackpackCapacity)));
	SortText->SetText(FText::FromString(FString::Printf(TEXT("[R] Sort: %s"), LoadoutRules::SortName(Sort)).ToUpper()));
	// Nothing to sort with fewer than two guns.
	SortButton->SetVisibility(Inventory->GetBackpack().Num() > 1 ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

void ULoadoutWidget::RefreshAmmo()
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!AmmoRow || !Inventory)
	{
		return;
	}
	AmmoRow->ClearChildren();
	const TConstArrayView<EAmmoType> Types = LooterAmmo::AllTypes();
	for (int32 Index = 0; Index < Types.Num(); ++Index)
	{
		const EAmmoType Type = Types[Index];
		const int32 Carried = Inventory->GetAmmo(Type);
		const int32 Max = FMath::Max(Inventory->GetMaxAmmo(Type), 1);
		// The ammo's icon and its count; faded while none is carried. (The five share one view box sized for the tall
		// sniper round, so the small rounds read at 22 px.)
		UHorizontalBox* Gauge = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Gauge->AddChildToHorizontalBox(MakeImage(WidgetTree, InkedIconBrush(AmmoIconName(Type), AmmoIcon(Type), FVector2D(22.f, 22.f),
			Carried > 0 ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.4f))))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* CountSlot = Gauge->AddChildToHorizontalBox(MakeText(WidgetTree, FString::FromInt(Carried), 10,
			Carried > 0 ? Color::Text() : Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.6f)));
		CountSlot->SetVerticalAlignment(VAlign_Center);
		CountSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));
		Gauge->SetToolTipText(FText::FromString(FString::Printf(TEXT("%s: %d / %d"), LooterAmmo::GetInfo(Type).Name, Carried, Max)));
		AmmoRow->AddChildToHorizontalBox(Gauge)->SetPadding(FMargin(Index > 0 ? 16.f : 0.f, 0.f, 0.f, 0.f));
	}
}

// ---------------------------------------------------------------------------
// The prompt bar
// ---------------------------------------------------------------------------

TArray<ULoadoutWidget::FPrompt> ULoadoutWidget::CurrentPrompts() const
{
	TArray<FPrompt> Prompts;
	if (bItemDrag)
	{
		// Dragging: what letting go here does.
		const FString Action = DropActionText(DropTarget);
		Prompts.Add({ TEXT("Let go"), Action.IsEmpty() ? FString(TEXT("Put back")) : Action, EAction::None });
		Prompts.Add({ TEXT("Esc"), TEXT("Cancel"), EAction::Cancel });
		return Prompts;
	}
	if (PickedSlot.IsSet())
	{
		const bool bSame = PickedSlot.GetValue() == CursorIndex && Zone == EZone::Slots;
		const bool bItem = Zone == EZone::Slots && SlotItem(CursorIndex) != nullptr;
		Prompts.Add({ TEXT("E"), bSame ? TEXT("Put back") : (bItem ? TEXT("Swap here") : TEXT("Move here")), EAction::Activate });
		Prompts.Add({ TEXT("Esc"), TEXT("Cancel"), EAction::Cancel });
		return Prompts;
	}

	// The main action first (its cap lit), then the others the cursor's gun allows, then closing.
	const bool bBackpackGuns = !ListOrder.IsEmpty();
	if (bInspecting)
	{
		Prompts.Add({ TEXT("X"), TEXT("Back"), EAction::Inspect });
		Prompts.Add({ TEXT("W / S"), TEXT("Next gun"), EAction::None });
		Prompts.Add({ TEXT("Drag"), TEXT("Turn"), EAction::None });
	}
	else if (Zone == EZone::Slots)
	{
		if (SlotItem(CursorIndex))
		{
			if (bBackpackGuns)
			{
				Prompts.Add({ TEXT("E"), TEXT("Swap from backpack"), EAction::Activate });
			}
			if (CursorIndex != GetActiveSlot())
			{
				Prompts.Add({ TEXT("F"), TEXT("Hold"), EAction::Hold });
			}
			Prompts.Add({ TEXT("X"), TEXT("Inspect"), EAction::Inspect });
			if (NumWeapons() > 1)
			{
				Prompts.Add({ TEXT("M"), TEXT("Move"), EAction::Move });
			}
			if (HasBackpackRoom())
			{
				Prompts.Add({ TEXT("C"), TEXT("Stow"), EAction::Stow });
			}
			Prompts.Add({ TEXT("Q"), TEXT("Drop"), EAction::Drop });
		}
		else if (bBackpackGuns)
		{
			Prompts.Add({ TEXT("E"), TEXT("Choose from backpack"), EAction::Activate });
		}
	}
	else if (ListItem(CursorIndex))
	{
		const FString Equip = SlotItem(TargetSlot) ? FString::Printf(TEXT("Swap into slot %d"), TargetSlot + 1) : FString(TEXT("Equip"));
		Prompts.Add({ TEXT("E"), Equip, EAction::Activate });
		Prompts.Add({ TEXT("F"), TEXT("Hold"), EAction::Hold });
		Prompts.Add({ TEXT("X"), TEXT("Inspect"), EAction::Inspect });
		Prompts.Add({ TEXT("Q"), TEXT("Drop"), EAction::Drop });
	}
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	Prompts.Add({ Bindings ? Bindings->GetKey(TEXT("Inventory")).GetDisplayName().ToString() : FString(TEXT("Tab")), TEXT("Close"), EAction::Close });
	return Prompts;
}

void ULoadoutWidget::RefreshPrompts()
{
	if (!PromptBar)
	{
		return;
	}
	PromptBar->ClearChildren();
	const TArray<FPrompt> Prompts = CurrentPrompts();
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		const FPrompt& Prompt = Prompts[Index];
		ULooterButton* Hint = MakeKeyHintButton(WidgetTree, Prompt.Key, Prompt.Text, Index == 0, static_cast<int32>(Prompt.Action));
		Hint->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandlePromptClicked);
		if (Prompt.Action == EAction::None)
		{
			Hint->SetCursor(EMouseCursor::Default);
		}
		PromptBar->AddChildToHorizontalBox(Hint)->SetPadding(FMargin(Index > 0 ? 24.f : 0.f, 0.f, 0.f, 0.f));
	}
}
