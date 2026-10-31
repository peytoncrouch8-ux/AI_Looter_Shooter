// ULoadoutWidget: the cursor in the page's two columns (2026-10-28). W / S move within a column: on the left the equip
// slots then the WARD row, on the right the backpack's guns and spare wards. A / D switch columns to the row nearest the
// cursor's on the page, as the eye goes across, so the cursor never jumps to the top of a long list.

#include "UI/Inventory/LoadoutWidget.h"
#include "Audio/LooterSound.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldTypes.h"
#include "UI/Style/LooterButton.h"
#include "Components/ScrollBox.h"

using namespace LoadoutParts;

void ULoadoutWidget::MoveCursor(int32 Rows)
{
	if (Zone == EZone::Backpack)
	{
		if (!ListRows.IsEmpty())
		{
			MoveCursorTo(EZone::Backpack, FMath::Clamp(CursorIndex + Rows, 0, ListRows.Num() - 1), true, true);
		}
		return;
	}
	// The left column, top to bottom: the slots, then the wards.
	const int32 NumSlotRows = SlotRows.Num();
	const int32 Total = NumSlotRows + WardRows.Num();
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
		MoveCursorTo(EZone::Wards, There - NumSlotRows, false, true);
	}
}

void ULoadoutWidget::MoveColumn(bool bRight)
{
	const bool bOnLeft = Zone != EZone::Backpack;
	if (bRight != bOnLeft)
	{
		// Already in that column.
		return;
	}
	const float Here = RowCenterY(Zone, CursorIndex);
	if (bRight)
	{
		if (ListRows.IsEmpty())
		{
			PlayCue(LooterSoundCue::Denied);
			return;
		}
		int32 Best = 0;
		for (int32 Row = 1; Row < ListRows.Num(); ++Row)
		{
			if (FMath::Abs(RowCenterY(EZone::Backpack, Row) - Here) < FMath::Abs(RowCenterY(EZone::Backpack, Best) - Here))
			{
				Best = Row;
			}
		}
		MoveCursorTo(EZone::Backpack, Best, true, true);
		return;
	}
	// The left column's nearest row, a slot or a ward (the upper one on a tie).
	EZone BestZone = EZone::Slots;
	int32 Best = INDEX_NONE;
	float BestDistance = TNumericLimits<float>::Max();
	auto Consider = [&](EZone RowZone, int32 Count)
	{
		for (int32 Row = 0; Row < Count; ++Row)
		{
			const float Distance = FMath::Abs(RowCenterY(RowZone, Row) - Here);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				BestZone = RowZone;
				Best = Row;
			}
		}
	};
	Consider(EZone::Slots, SlotRows.Num());
	Consider(EZone::Wards, WardRows.Num());
	if (Best != INDEX_NONE)
	{
		MoveCursorTo(BestZone, Best, false, true);
	}
}

float ULoadoutWidget::RowCenterY(EZone RowZone, int32 Index) const
{
	// From the layout's own numbers (LoadoutLayout, as LoadoutWidgetLayout.cpp and the rows build it), so it holds before
	// the page has ever been drawn.
	using namespace LoadoutLayout;
	switch (RowZone)
	{
	case EZone::Slots:
		return FirstRowTop + Index * (SlotRowHeight + RowGap) + SlotRowHeight * 0.5f;
	case EZone::Wards:
	{
		const int32 NumSlotRows = SlotRows.Num();
		const float SlotsEnd = FirstRowTop + NumSlotRows * SlotRowHeight + FMath::Max(NumSlotRows - 1, 0) * RowGap;
		// One row since 2026-10-31: the ward worn (the spares are backpack rows).
		const float WardTop = SlotsEnd + SectionGap + HeaderHeight + HeaderGap;
		return WardTop + WardRowHeight * 0.5f;
	}
	default:
		return FirstRowTop + Index * (BackpackRowHeight + RowGap) + BackpackRowHeight * 0.5f - (ListBox ? ListBox->GetScrollOffset() : 0.f);
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
	else if (const FShieldInstanceData* Ward = CursorWard())
	{
		SeenGuns.Add(ShieldRules::Identity(*Ward));
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
