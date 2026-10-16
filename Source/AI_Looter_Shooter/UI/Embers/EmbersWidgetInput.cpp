// UEmbersWidget: the keys, the mouse and equipping a power. The layout and the cards are in EmbersWidget.cpp.

#include "UI/Embers/EmbersWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Embers/EmberComponent.h"
#include "Embers/EmberDefinition.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"

using namespace LooterUI;

// ---------------------------------------------------------------------------
// Choosing and equipping
// ---------------------------------------------------------------------------

void UEmbersWidget::Select(int32 Index)
{
	if (Cards.IsEmpty())
	{
		return;
	}
	const int32 Wanted = FMath::Clamp(Index, 0, Cards.Num() - 1);
	if (Wanted == Selected)
	{
		return;
	}
	Selected = Wanted;
	LooterSound::Play2D(this, LooterSoundCue::Hover);
	Restyle();
}

bool UEmbersWidget::EquipSelected()
{
	UEmberComponent* Component = GetEmbers();
	if (!Component || !Powers.IsValidIndex(Selected) || !Powers[Selected])
	{
		return false;
	}
	const UEmberDefinition& Power = *Powers[Selected];
	if (Component->GetEquippedIndex() == Selected)
	{
		ShowMessage(FString::Printf(TEXT("%s is already equipped"), *Power.DisplayName.ToString()), Color::TextDim());
		LooterSound::Play2D(this, LooterSoundCue::Click);
		return false;
	}
	if (!Component->EquipIndex(Selected))
	{
		// A power that's running finishes first (the component refuses the swap): say which.
		const UEmberDefinition* Running = Component->GetEquipped();
		ShowMessage(Component->IsActive() && Running
			? FString::Printf(TEXT("%s is running: it finishes before you can swap"), *Running->DisplayName.ToString())
			: FString(TEXT("Can't swap right now")), Color::Worse());
		LooterSound::Play2D(this, LooterSoundCue::Denied);
		return false;
	}
	// Seated: the loadout's clack, the cards restyled, and a word in the power's colour.
	LooterSound::Play2D(this, LoadoutParts::Sounds::Equip);
	ShowMessage(FString::Printf(TEXT("%s equipped"), *Power.DisplayName.ToString()), Power.Color);
	Restyle();
	RefreshStates();
	return true;
}

void UEmbersWidget::HandleCardClicked(ULooterButton* Button)
{
	if (Button)
	{
		Select(Button->Index);
		EquipSelected();
	}
	// Clicking handed keyboard focus to the game viewport (LooterButton); take it back for the page's keys.
	SetKeyboardFocus();
}

void UEmbersWidget::HandleCardHovered(ULooterButton* Button)
{
	if (Button)
	{
		Select(Button->Index);
	}
}

// ---------------------------------------------------------------------------
// The other pages
// ---------------------------------------------------------------------------

void UEmbersWidget::ShowPage(EInventoryPage Page)
{
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->ShowInventoryPage(Page);
	}
}

void UEmbersWidget::HandleTabClicked(ULooterButton* Button)
{
	if (Button && Button->Index != static_cast<int32>(EInventoryPage::Embers))
	{
		ShowPage(static_cast<EInventoryPage>(Button->Index));
	}
	else
	{
		SetKeyboardFocus();
	}
}

// ---------------------------------------------------------------------------
// Keys
// ---------------------------------------------------------------------------

FReply UEmbersWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;

	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || (Bindings ? Bindings->IsInventoryKey(Key) : (Key == EKeys::Tab || Key == EKeys::I)))
	{
		Close();
		return FReply::Handled();
	}
	// The other pages: their number keys, or the shoulders to the pages either side (the loadout, the bestiary).
	const int32 PageKey = Key == EKeys::Gamepad_LeftShoulder ? static_cast<int32>(EInventoryPage::Loadout)
		: Key == EKeys::Gamepad_RightShoulder ? static_cast<int32>(EInventoryPage::Bestiary)
		: LoadoutParts::PageForNumberKey(Key);
	if (PageKey != INDEX_NONE)
	{
		if (PageKey != static_cast<int32>(EInventoryPage::Embers))
		{
			ShowPage(static_cast<EInventoryPage>(PageKey));
		}
		return FReply::Handled();
	}
	// The cards stand side by side: left and right choose (up and down too, so W / S work as on the other pages).
	if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Up || Key == EKeys::W
		|| Key == EKeys::Gamepad_DPad_Up)
	{
		Select(Selected - 1);
		return FReply::Handled();
	}
	if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Down || Key == EKeys::S
		|| Key == EKeys::Gamepad_DPad_Down)
	{
		Select(Selected + 1);
		return FReply::Handled();
	}
	const bool bInteractKey = Bindings && Bindings->GetKey(TEXT("Interact")) == Key;
	if (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom || bInteractKey)
	{
		EquipSelected();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
