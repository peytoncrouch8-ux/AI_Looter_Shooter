// UBenchWidget: the cursor, choosing a gun and a slot, fitting, starting a scrap, and the mouse on the rows and buttons
// (BenchWidgetKeys.cpp has the keys).

#include "UI/Bench/BenchWidget.h"
#include "Audio/LooterSound.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/Bench/BenchRules.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponPartSwap.h"
#include "Components/ScrollBox.h"
#include "Templates/UnrealTemplate.h"

#define LOCTEXT_NAMESPACE "LooterBench"

using namespace LooterUI;

// ---------------------------------------------------------------------------
// The cursor
// ---------------------------------------------------------------------------

void UBenchWidget::MoveCursor(int32 Columns, int32 Rows)
{
	const EColumn WasColumn = Column;
	const int32 WasIndex = CursorIndex;
	if (Columns != 0 && !bScrapping)
	{
		// To the next column that has something in it; each opens on its choice (the chosen gun, the chosen slot, the
		// first part that fits).
		const int32 Target = FMath::Clamp(static_cast<int32>(Column) + Columns, 0, static_cast<int32>(EColumn::Parts));
		const EColumn NewColumn = static_cast<EColumn>(Target);
		if (NewColumn != Column && !RowsOf(NewColumn).IsEmpty())
		{
			const int32 NewIndex = NewColumn == EColumn::Guns ? ChosenGun : (NewColumn == EColumn::Slots ? ChosenSlot : 0);
			MoveCursorTo(NewColumn, FMath::Clamp(NewIndex, 0, RowsOf(NewColumn).Num() - 1), true);
		}
	}
	else if (Rows != 0)
	{
		const int32 Count = RowsOf(Column).Num();
		MoveCursorTo(Column, FMath::Clamp(CursorIndex + Rows, 0, FMath::Max(Count - 1, 0)), true);
	}
	if (Column != WasColumn || CursorIndex != WasIndex)
	{
		PlayCue(LooterSoundCue::Hover, 0.6f);
	}
}

void UBenchWidget::MoveCursorTo(EColumn NewColumn, int32 NewIndex, bool bScrollIntoView)
{
	if (NewColumn == Column && NewIndex == CursorIndex)
	{
		return;
	}
	Column = NewColumn;
	CursorIndex = NewIndex;
	// On the slots the cursor chooses the slot (the parts box shows what fits it), unless a part is being chosen to keep.
	if (Column == EColumn::Slots && !bScrapping && ChosenSlot != CursorIndex)
	{
		ChosenSlot = CursorIndex;
		RebuildParts();
	}
	Restyle();
	RefreshGun();
	RefreshStage(false);
	RefreshPrompts();
	if (bScrollIntoView)
	{
		const TArray<FRow>& Rows = RowsOf(Column);
		UScrollBox* List = Column == EColumn::Guns ? GunList.Get() : (Column == EColumn::Parts ? PartList.Get() : nullptr);
		if (List && Rows.IsValidIndex(CursorIndex))
		{
			List->ScrollWidgetIntoView(Rows[CursorIndex].Button, false, EDescendantScrollDestination::IntoView, 8.f);
		}
	}
}

// ---------------------------------------------------------------------------
// Choosing
// ---------------------------------------------------------------------------

void UBenchWidget::Activate()
{
	switch (Column)
	{
	case EColumn::Guns:
		if (Guns.IsValidIndex(CursorIndex))
		{
			// The gun to work on; the cursor goes on to its slots.
			ChooseGun(CursorIndex);
			PlayCue(LooterSoundCue::Click);
			if (!SlotRows.IsEmpty())
			{
				MoveCursorTo(EColumn::Slots, ChosenSlot, false);
			}
		}
		break;
	case EColumn::Slots:
		if (bScrapping)
		{
			AskScrap(CursorIndex);
		}
		else if (!CanChange())
		{
			Deny(LOCTEXT("FixedPartsActivate", "A named gun keeps its own parts"));
		}
		else
		{
			// The slot's parts in the box: the cursor goes on to them, the first that fits.
			ChooseSlot(CursorIndex);
			PlayCue(LooterSoundCue::Click);
			if (!PartRows.IsEmpty())
			{
				MoveCursorTo(EColumn::Parts, 0, true);
			}
		}
		break;
	case EColumn::Parts:
		TryFit(CursorIndex);
		break;
	}
}

void UBenchWidget::ChooseGun(int32 Row)
{
	if (!Guns.IsValidIndex(Row))
	{
		return;
	}
	const bool bChanged = Row != ChosenGun;
	const FName WasSlot = SlotNameAt(ChosenSlot);
	ChosenGun = Row;
	bScrapping = false;
	// The same slot on the new gun when it has one (a gun of the same kind), so comparing guns part by part is quick.
	const FWeaponInstanceData* Item = ChosenItem();
	const int32 Same = Item ? BenchRules::SlotNames(*Item).IndexOfByKey(WasSlot) : INDEX_NONE;
	ChosenSlot = FMath::Max(Same, 0);
	RebuildSlots();
	RebuildParts();
	Restyle();
	RefreshGun();
	RefreshStage(bChanged);
	RefreshPrompts();
}

void UBenchWidget::ChooseSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= NumSlots())
	{
		return;
	}
	ChosenSlot = SlotIndex;
	RebuildParts();
	Restyle();
	RefreshGun();
	RefreshStage(false);
	RefreshPrompts();
}

// ---------------------------------------------------------------------------
// Fitting and scrapping
// ---------------------------------------------------------------------------

void UBenchWidget::TryFit(int32 Row)
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	const FBoxedWeaponPart* Part = RowPart(Row);
	const FWeaponInstanceData* Item = ChosenItem();
	if (!Inventory || !Part || !Item)
	{
		return;
	}
	if (!CanChange())
	{
		Deny(LOCTEXT("FixedPartsFit", "A named gun keeps its own parts"));
		return;
	}
	const int32 BoxIndex = PartEntries[Row].BoxIndex;
	const WeaponPartSwap::ECheck Check = Inventory->CheckFit(ChosenRef(), BoxIndex);
	if (Check != WeaponPartSwap::ECheck::Ok)
	{
		Deny(WeaponPartSwap::CheckText(Check));
		return;
	}
	const FText NewName = FText::FromString(BenchRules::PartName(*Part));
	const FText OldName = FText::FromString(BenchRules::SlotPartName(*Item, ChosenSlot));
	const bool bWasEmpty = BenchRules::CurrentPart(*Item, ChosenSlot) == nullptr;
	bool bFitted = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bFitted = Inventory->FitPart(ChosenRef(), BoxIndex);
	}
	if (!bFitted)
	{
		Deny(LOCTEXT("CantFitNow", "That part can't be fitted now"));
		Refresh();
		return;
	}
	PlayCue(LooterSoundCue::BenchFit);
	SetStatus(bWasEmpty ? FText::Format(LOCTEXT("FittedEmpty", "Fitted the {0}"), NewName)
		: FText::Format(LOCTEXT("Fitted", "Fitted the {0} · the {1} is in the box"), NewName, OldName), Color::Better());
	// Back on the slot, so the stand shows the gun as it is now rather than the part that came off (which sits where the
	// fitted one was in the box: fitting it again puts the gun back as it was).
	Column = EColumn::Slots;
	CursorIndex = ChosenSlot;
	Refresh();
}

void UBenchWidget::BeginScrap()
{
	if (bScrapping)
	{
		return;
	}
	if (Column == EColumn::Guns && Guns.IsValidIndex(CursorIndex) && CursorIndex != ChosenGun)
	{
		ChooseGun(CursorIndex);
	}
	UWeaponManagerComponent* Inventory = Manager.Get();
	const FWeaponInstanceData* Item = ChosenItem();
	if (!Inventory || !Item)
	{
		return;
	}
	FText Why;
	if (!Inventory->CanScrap(ChosenRef(), &Why))
	{
		Deny(Why);
		return;
	}
	bScrapping = true;
	// The cursor starts on the chosen slot's part, or the first part the gun has (it has one: CanScrap).
	int32 Keep = ChosenSlot;
	if (!BenchRules::CurrentPart(*Item, Keep))
	{
		Keep = 0;
		while (Keep < NumSlots() - 1 && !BenchRules::CurrentPart(*Item, Keep))
		{
			++Keep;
		}
	}
	Column = EColumn::Slots;
	CursorIndex = Keep;
	RebuildSlots();
	RebuildParts();
	Restyle();
	RefreshGun();
	RefreshStage(false);
	RefreshPrompts();
	PlayCue(LooterSoundCue::Click);
	SetStatus(FText::Format(LOCTEXT("ChooseKeep", "Scrapping the {0}: choose the part to keep"), FText::FromString(LooterWeaponText::Name(*Item))),
		Color::Accent());
}

void UBenchWidget::StopScrapping()
{
	if (!bScrapping)
	{
		return;
	}
	bScrapping = false;
	Column = EColumn::Slots;
	CursorIndex = ChosenSlot;
	RebuildSlots();
	RebuildParts();
	Restyle();
	RefreshGun();
	RefreshStage(false);
	RefreshPrompts();
	PlayCue(LooterSoundCue::Back);
	SetStatus(FText::GetEmpty(), Color::TextDim());
}

void UBenchWidget::AskScrap(int32 SlotIndex)
{
	const FWeaponInstanceData* Item = ChosenItem();
	if (!Item)
	{
		return;
	}
	if (!BenchRules::CurrentPart(*Item, SlotIndex))
	{
		Deny(LOCTEXT("EmptySlot", "There's no part in that slot to keep"));
		return;
	}
	ConfirmSlot = SlotIndex;
	PlayCue(LooterSoundCue::Click);
	OpenConfirm(EConfirm::Scrap, FString::Printf(TEXT("Scrap %s?"), *LooterWeaponText::Name(*Item)),
		FString::Printf(TEXT("Keep the %s. The rest of the gun is gone for good."), *BenchRules::SlotPartName(*Item, SlotIndex)),
		LOCTEXT("ScrapButton", "Scrap"));
}

void UBenchWidget::AskDiscard(int32 Row)
{
	const FBoxedWeaponPart* Part = RowPart(Row);
	if (!Part)
	{
		return;
	}
	ConfirmBoxIndex = PartEntries[Row].BoxIndex;
	PlayCue(LooterSoundCue::Click);
	OpenConfirm(EConfirm::Discard, FString::Printf(TEXT("Throw out the %s?"), *BenchRules::PartName(*Part)),
		TEXT("It leaves the parts box for good."), LOCTEXT("ThrowOutButton", "Throw out"));
}

void UBenchWidget::Deny(const FText& Why)
{
	PlayCue(LooterSoundCue::Denied);
	SetStatus(Why, Color::Worse());
}

// ---------------------------------------------------------------------------
// The mouse on the rows and buttons
// ---------------------------------------------------------------------------

void UBenchWidget::HandleRowHovered(ULooterButton* Button)
{
	if (!Button || Confirm != EConfirm::None)
	{
		return;
	}
	const EColumn Hovered = Button->Action == BenchActions::Gun ? EColumn::Guns : (Button->Action == BenchActions::Slot ? EColumn::Slots : EColumn::Parts);
	// Scrapping holds the cursor to the gun's slots, where the part to keep is chosen.
	if (!bScrapping || Hovered == EColumn::Slots)
	{
		MoveCursorTo(Hovered, Button->Index, false);
	}
}

void UBenchWidget::HandleRowClicked(ULooterButton* Button)
{
	if (Button && Confirm == EConfirm::None)
	{
		HandleRowHovered(Button);
		if (Button->Action == BenchActions::Gun && !bScrapping)
		{
			ChooseGun(Button->Index);
			PlayCue(LooterSoundCue::Click);
		}
		else if (Button->Action == BenchActions::Slot)
		{
			if (bScrapping)
			{
				AskScrap(Button->Index);
			}
			else
			{
				ChooseSlot(Button->Index);
				PlayCue(LooterSoundCue::Click);
			}
		}
		else if (Button->Action == BenchActions::Part && !bScrapping)
		{
			TryFit(Button->Index);
		}
	}
	// Clicking handed keyboard focus to the game viewport (LooterButton); take it back for the screen's keys.
	SetKeyboardFocus();
}

void UBenchWidget::HandleButton(ULooterButton* Button)
{
	if (Button)
	{
		if (Button->Action == BenchActions::Scrap && Confirm == EConfirm::None)
		{
			BeginScrap();
		}
		else if (Button->Action == BenchActions::StopScrapping && Confirm == EConfirm::None)
		{
			StopScrapping();
		}
		else if (Button->Action == BenchActions::Accept)
		{
			AcceptConfirm();
		}
		else if (Button->Action == BenchActions::Back)
		{
			CloseConfirm();
			PlayCue(LooterSoundCue::Back);
		}
	}
	SetKeyboardFocus();
}

#undef LOCTEXT_NAMESPACE
