// ULoadoutWidget: what the keys (and clicks, through RunAction) do on a ward's row: on the WARD row, E goes to the
// backpack's wards to choose a replacement, C stows the worn one in the backpack, Q drops it; on a backpack ward's row, E
// or F wears it (the worn one takes its place in the backpack), Q drops it. The rows are LoadoutWidgetWard.cpp.

#include "UI/Inventory/LoadoutWidget.h"
#include "Audio/LooterSound.h"
#include "Shields/PlayerShieldComponent.h"
#include "UI/Inventory/LoadoutBackpackRules.h"
#include "UI/Inventory/LoadoutParts.h"
#include "Templates/UnrealTemplate.h"

using namespace LoadoutParts;

TArray<ULoadoutWidget::FPrompt> ULoadoutWidget::WardPrompts() const
{
	TArray<FPrompt> Prompts;
	const UPlayerShieldComponent* Shield = GetShield();
	if (!Shield)
	{
		return Prompts;
	}
	if (Zone == EZone::Wards)
	{
		// The WARD row, as a slot: choose its replacement from the backpack, stow, drop.
		if (!Shield->GetSpares().IsEmpty())
		{
			Prompts.Add({ TEXT("E"), Shield->HasWard() ? TEXT("Swap from backpack") : TEXT("Choose from backpack"), EAction::Activate });
		}
		if (Shield->HasWard())
		{
			if (Shield->HasBackpackRoom())
			{
				Prompts.Add({ TEXT("C"), TEXT("Stow"), EAction::Stow });
			}
			Prompts.Add({ TEXT("Q"), TEXT("Drop"), EAction::Drop });
		}
	}
	else if (ListWard(CursorIndex))
	{
		// A backpack ward: wear it (the worn one takes its place), or drop it. Vendors buy guns only, so no junk mark.
		Prompts.Add({ TEXT("E"), Shield->HasWard() ? TEXT("Swap with worn") : TEXT("Wear"), EAction::Activate });
		Prompts.Add({ TEXT("Q"), TEXT("Drop"), EAction::Drop });
	}
	return Prompts;
}

bool ULoadoutWidget::RunWardAction(EAction Action)
{
	switch (Action)
	{
	case EAction::Activate:
	case EAction::Hold:
		WardActivate();
		return true;
	case EAction::Stow:
		WardStow();
		return true;
	case EAction::Drop:
		WardDrop();
		return true;
	case EAction::Move:
	case EAction::Inspect:
		// A ward isn't moved between slots or turned on the stand.
		PlayCue(LooterSoundCue::Denied);
		return true;
	default:
		return false;
	}
}

void ULoadoutWidget::WardActivate()
{
	UPlayerShieldComponent* Shield = GetShield();
	if (!Shield)
	{
		return;
	}
	if (Zone == EZone::Wards)
	{
		// The WARD row: off to the backpack's first ward to choose its replacement, as a slot's E goes to the backpack.
		const int32 First = LoadoutBackpack::FirstWardRow(ListOrder);
		if (First == INDEX_NONE)
		{
			PlayCue(LooterSoundCue::Denied);
			return;
		}
		PlayCue(LooterSoundCue::Click);
		MoveCursorTo(EZone::Backpack, First, true, false);
		return;
	}
	int32 Spare = INDEX_NONE;
	if (!ListWard(CursorIndex, &Spare))
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	bool bWorn = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bWorn = Shield->EquipSpare(Spare);
	}
	if (bWorn)
	{
		// The cursor follows the ward to the WARD row: its card greets it there.
		Zone = EZone::Wards;
		CursorIndex = 0;
	}
	Refresh();
	if (bWorn)
	{
		FlashRow(EZone::Wards, 0);
	}
	PlayCue(bWorn ? Sounds::Equip : LooterSoundCue::Denied);
}

void ULoadoutWidget::WardStow()
{
	UPlayerShieldComponent* Shield = GetShield();
	if (!Shield || Zone != EZone::Wards)
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	bool bStowed = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bStowed = Shield->StowWard();
	}
	Refresh();
	PlayCue(bStowed ? Sounds::Stow : LooterSoundCue::Denied);
}

void ULoadoutWidget::WardDrop()
{
	UPlayerShieldComponent* Shield = GetShield();
	if (!Shield)
	{
		return;
	}
	int32 Spare = INDEX_NONE;
	const bool bWornRow = Zone == EZone::Wards;
	if (!bWornRow)
	{
		ListWard(CursorIndex, &Spare);
	}
	bool bDropped = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bDropped = bWornRow ? Shield->DropWard() : (Spare != INDEX_NONE && Shield->DropSpare(Spare));
	}
	Refresh();
	PlayCue(bDropped ? Sounds::Drop : LooterSoundCue::Denied);
}
