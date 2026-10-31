// UPlayerHUDWidget: the soul flask's row under the player frame (UHudFlaskWidget): following the pawn's flask component,
// its changes (a drink pours a pip, a filled charge pops one and says so in the pickup feed, a refusal shakes the row or
// says "Health full") and the bound key.

#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/HudFlaskWidget.h"
#include "UI/HUD/HudPickupFeedWidget.h"
#include "Player/PlayerFlaskComponent.h"
#include "Player/PlayerFlaskRules.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"

namespace
{
	/** The Flask key's binding id (the contract's; UKeyBindingSubsystem holds it) and its default, shown until it's bound. */
	const FName FlaskBindingId(TEXT("Flask"));
	const TCHAR* const FlaskDefaultKey = TEXT("H");
}

FString UPlayerHUDWidget::FlaskChargeLine()
{
	// In the feed's own words, as an ammo pickup's ("+36 AR Ammo") and a grenade's are.
	return TEXT("+1 Flask Charge");
}

FString UPlayerHUDWidget::FlaskKeyName() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const FKey Key = Bindings ? Bindings->GetKey(FlaskBindingId) : EKeys::Invalid;
	// A binding the subsystem doesn't know yet gives no key: the tab says the default rather than "NONE".
	return Key.IsValid() ? Key.GetDisplayName().ToString().ToUpper() : FString(FlaskDefaultKey);
}

void UPlayerHUDWidget::BindFlask(UPlayerFlaskComponent* Flask)
{
	if (BoundFlask.Get() == Flask)
	{
		return;
	}
	if (UPlayerFlaskComponent* Old = BoundFlask.Get())
	{
		Old->OnFlaskChanged.Remove(FlaskChangedHandle);
	}
	FlaskChangedHandle.Reset();
	if (Flask)
	{
		FlaskChangedHandle = Flask->OnFlaskChanged.AddUObject(this, &UPlayerHUDWidget::HandleFlaskChanged);
	}
	BoundFlask = Flask;
	// A new pawn's flask (a respawn, a load) is shown as it is: nothing poured or filled on the way.
	if (FlaskRow)
	{
		FlaskRow->SnapNext();
	}
}

void UPlayerHUDWidget::UpdateFlask(float DeltaTime)
{
	if (FlaskRow)
	{
		FlaskRow->Update(BoundFlask.Get(), FlaskKeyName(), DeltaTime);
	}
}

void UPlayerHUDWidget::HandleFlaskChanged(int32 Charges, int32 Delta, EFlaskChange Why)
{
	if (!FlaskRow)
	{
		return;
	}
	if (Why == EFlaskChange::Restored)
	{
		// A load puts the flask back as it was saved: shown as it is.
		FlaskRow->SnapNext();
		return;
	}
	EHudFlaskFlash Kind = EHudFlaskFlash::Drunk;
	if (UHudFlaskWidget::FlashFor(Why, Kind))
	{
		FlaskRow->Flash(Kind, Charges);
	}
	// A charge filled by soul is a find worth telling where the eyes are; a respawn's top-up is quiet.
	if (Why == EFlaskChange::Filled && Delta > 0 && PickupFeed)
	{
		PickupFeed->AddLine(FlaskChargeLine());
	}
}
