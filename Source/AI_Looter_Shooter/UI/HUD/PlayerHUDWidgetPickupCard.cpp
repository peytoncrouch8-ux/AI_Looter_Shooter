#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/Cards/GunCardRules.h"
#include "UI/Cards/GunCardWidget.h"
#include "UI/HUD/HudInteractPromptWidget.h"
#include "UI/HUD/HudMissionTrackerWidget.h"
#include "Interaction/InteractionComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Widget.h"
#include "Engine/LocalPlayer.h"
#include "Layout/Geometry.h"

// What the interact key would do to the thing looked at. Loot shows its own card over the gun (its label,
// UWeaponLabelWidget), with the take keys; here, on the right edge, the gun in hand's card under "IN HAND" to hold it
// against (the user's call, 2026-10-28), and where that card stands on screen, so the mission tracker can keep clear of it.
// Anything else gets the interaction prompt under the crosshair.

void UPlayerHUDWidget::UpdatePickupCard(UWeaponManagerComponent* Manager, const UInteractionComponent* Interaction, float DeltaTime)
{
	if (!PickupCard || !InHandCard)
	{
		return;
	}

	// The interaction component knows what's looked at; loot (offered by the weapon manager) gets the cards.
	const AWeaponBase* Pickup = Interaction ? Cast<AWeaponBase>(Interaction->GetFocusedActor()) : nullptr;
	if (Pickup && (!Pickup->IsPickup() || !Manager))
	{
		Pickup = nullptr;
	}
	// The key's name is only needed while something is looked at (a fading prompt keeps its words).
	const FString Key = Interaction && Interaction->GetFocusedActor() ? BoundKeyName(TEXT("Interact"), TEXT("E")) : FString();
	if (InteractPrompt)
	{
		InteractPrompt->Update(Pickup ? nullptr : Interaction, Key, DeltaTime);
	}

	// The gun in hand, while loot is looked at; unarmed there is nothing to hold the loot against, so no card.
	const AWeaponBase* InHand = Pickup ? Manager->GetActiveWeapon() : nullptr;
	if (!InHand)
	{
		PickupCard->SetVisibility(ESlateVisibility::Collapsed);
		InHandShown = nullptr;
		return;
	}
	const bool bAppearing = PickupCard->GetVisibility() == ESlateVisibility::Collapsed;
	if (bAppearing)
	{
		PickupCard->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// Clear of the minimap's frame: a tall card on the right edge's middle reached up over the place's name under a large
	// minimap (the 2026-10-23 play-check). Its height is the last layout's, so a new card settles a frame after it shows.
	if (UCanvasPanelSlot* CardSlot = Cast<UCanvasPanelSlot>(PickupCard->Slot))
	{
		const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
		const UGraphicsSettingsSubsystem* Settings = LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
		const float ClearTop = UHudMissionTrackerWidget::TopFor(Settings ? Settings->GetMinimapScale() : 1.f, !Settings || Settings->IsMinimapShown());
		const float Drop = LootCardDrop(static_cast<float>(GetCachedGeometry().GetLocalSize().Y), static_cast<float>(PickupCard->GetDesiredSize().Y),
			ClearTop);
		const FVector2D Wanted(CardSlot->GetPosition().X, Drop);
		if (!CardSlot->GetPosition().Equals(Wanted, 0.5f))
		{
			CardSlot->SetPosition(Wanted);
		}
	}

	// Set only when another gun comes into hand or the card shows again (setting it rewrites the card); it compares
	// nothing, the looked-at gun's card over it does. It unfolds as it shows, quietly: the world card's open is the sound.
	if (bAppearing || InHandShown.Get() != InHand)
	{
		InHandShown = InHand;
		const bool bNewGun = InHandCard->SetGun(InHand->GetInstance(), nullptr, EGunCardMode::InHand);
		if (bAppearing || bNewGun)
		{
			InHandCard->PlayIntro(/*bWithSound*/ false);
		}
	}
}

bool UPlayerHUDWidget::IsInHandCardShown() const
{
	return PickupCard && InHandCard && PickupCard->GetVisibility() != ESlateVisibility::Collapsed;
}

bool UPlayerHUDWidget::GetLootCardBounds(FVector2D& OutMin, FVector2D& OutMax) const
{
	if (!PickupCard || PickupCard->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return false;
	}
	// Laid out a frame after it's shown: until then its geometry has no size, and there's nothing to keep clear of yet.
	const FGeometry& CardGeometry = PickupCard->GetCachedGeometry();
	const FVector2D CardSize = CardGeometry.GetAbsoluteSize();
	if (CardSize.X <= 0.f || CardSize.Y <= 0.f)
	{
		return false;
	}
	OutMin = CardGeometry.GetAbsolutePosition();
	OutMax = OutMin + CardSize;
	return true;
}

float UPlayerHUDWidget::LootCardDrop(float ScreenHeight, float CardHeight, float ClearTop, float BottomKeep)
{
	if (ScreenHeight <= 0.f || CardHeight <= 0.f)
	{
		return 0.f;
	}
	// Centred on the right edge's middle, its top is half the spare height down; it drops only as far as the minimap needs,
	// and only as far as the screen's foot allows.
	const float CentredTop = (ScreenHeight - CardHeight) * 0.5f;
	const float Wanted = FMath::Max(0.f, ClearTop - CentredTop);
	const float Room = FMath::Max(0.f, ScreenHeight - BottomKeep - (CentredTop + CardHeight));
	return FMath::Min(Wanted, Room);
}
