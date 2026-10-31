// UPlayerHUDWidget: an equip slot opening for the player (the user, 2026-10-31: the third at level 3, the fourth after
// Ransom's Rest's story; GunSlotRules). The weapon manager tells the moment (OnSlotUnlocked); here it becomes the "NEW GUN
// SLOT" banner, a gold line in the pickup feed, and the weapon column brought forward, where the slot flashes open itself
// (UHudWeaponSlotsWidget).

#include "UI/HUD/PlayerHUDWidget.h"
#include "Inventory/GunSlotRules.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/HUD/HudGunSlotBannerWidget.h"
#include "UI/HUD/HudPickupFeedWidget.h"
#include "UI/Style/LooterUIStyle.h"

namespace
{
	/** Seconds the weapon column stays forward after a slot opens: a beat longer than a shot's, to see the new slot. */
	constexpr float SlotUnlockActivity = 4.f;
}

void UPlayerHUDWidget::BindSlotUnlocks(UWeaponManagerComponent* Manager)
{
	if (SlotUnlockManager.Get() == Manager)
	{
		return;
	}
	if (UWeaponManagerComponent* Old = SlotUnlockManager.Get())
	{
		Old->OnSlotUnlocked.Remove(SlotUnlockHandle);
	}
	SlotUnlockHandle = Manager ? Manager->OnSlotUnlocked.AddUObject(this, &UPlayerHUDWidget::HandleSlotUnlocked) : FDelegateHandle();
	SlotUnlockManager = Manager;
}

void UPlayerHUDWidget::HandleSlotUnlocked(int32 SlotIndex, EGunSlotLock Was)
{
	if (GunSlotBanner)
	{
		GunSlotBanner->Announce(SlotIndex, Was);
	}
	// The feed says it at once, where the eyes are; the banner takes its turn up top.
	if (PickupFeed)
	{
		PickupFeed->AddFlashLine(GunSlotRules::FeedLine(), LooterUI::Color::SoulGold());
	}
	WeaponActivity = FMath::Max(WeaponActivity, SlotUnlockActivity);
}
