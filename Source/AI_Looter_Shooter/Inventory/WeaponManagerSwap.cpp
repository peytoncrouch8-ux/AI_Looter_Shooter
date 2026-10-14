// UWeaponManagerComponent: taking a slot's gun in hand, and the swap that shows it (WeaponSwapMotion has its look and
// timing). In a local player's view the gun in hand first goes down out of view, locked (no shots, no reload; a reload under
// way is cut, and starts over if the swap is called off), and the guns change places out of sight; the next comes up on its
// own ready time, which its first shot waits for. Anywhere else (tests, a menu open over the game, no gun in hand) the
// change is at once, as before. The gun in hand stays the active one until the change, so everything that follows the gun
// in hand (the HUD, the first-person hands, the body) follows the one the player sees.

#include "Inventory/WeaponManagerComponent.h"
#include "Audio/LooterSound.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/WeaponSwapMotion.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponSounds.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarSwapMotion(
		TEXT("Looter.Swap.Motion"),
		1,
		TEXT("1: a swap puts the gun in hand away out of view before the next one comes up (the player's, in play). 0: guns change at once."),
		ECVF_Default);
}

// ---------------------------------------------------------------------------
// Taking a slot in hand
// ---------------------------------------------------------------------------

void UWeaponManagerComponent::EquipSlot(int32 SlotIndex)
{
	if (!Weapons.IsValidIndex(SlotIndex))
	{
		return;
	}
	const bool bAnimate = ShouldAnimateSwap();
	if (SlotIndex == ActiveSlot)
	{
		// The gun in hand: a swap putting it away is called off, and it comes back up from wherever it got to (at once
		// where swaps don't play out: a menu opened over one).
		if (SwapClock.IsLowering())
		{
			if (bAnimate)
			{
				CallOffSwap();
			}
			else
			{
				FinishRaise();
			}
		}
		return;
	}
	if (!bAnimate)
	{
		SetActiveSlot(SlotIndex);
		return;
	}
	BeginSwap(Weapons[SlotIndex], nullptr);
}

void UWeaponManagerComponent::NextWeapon()
{
	if (Weapons.Num() > 1)
	{
		EquipSlot((GetHeadingSlot() + 1) % Weapons.Num());
	}
}

void UWeaponManagerComponent::PreviousWeapon()
{
	if (Weapons.Num() > 1)
	{
		EquipSlot((GetHeadingSlot() - 1 + Weapons.Num()) % Weapons.Num());
	}
}

int32 UWeaponManagerComponent::GetHeadingSlot() const
{
	const int32 Target = GetSwapTargetSlot();
	return Target != INDEX_NONE ? Target : ActiveSlot;
}

// ---------------------------------------------------------------------------
// What the swap shows
// ---------------------------------------------------------------------------

int32 UWeaponManagerComponent::GetSwapTargetSlot() const
{
	AWeaponBase* Target = SwapClock.IsLowering() ? SwapTarget.Get() : nullptr;
	return Target ? Weapons.IndexOfByKey(Target) : INDEX_NONE;
}

float UWeaponManagerComponent::GetGunLowered() const
{
	const AWeaponBase* InHand = GetActiveWeapon();
	const UWorld* World = GetWorld();
	if (!InHand || !World)
	{
		return 0.f;
	}
	if (SwapClock.IsRunning())
	{
		return SwapClock.GetLowered(World->GetTimeSeconds());
	}
	// No swap: the gun's own draw (every way a gun comes into hand draws it: a swap's change, a pickup, the bench, a load).
	return WeaponSwapMotion::DrawLowered(WeaponSwapMotion::KindOf(InHand), InHand->GetReadyAlpha());
}

bool UWeaponManagerComponent::ShouldAnimateSwap() const
{
	const AWeaponBase* InHand = GetActiveWeapon();
	if (!InHand || InHand->IsHidden() || CVarSwapMotion.GetValueOnGameThread() <= 0)
	{
		return false;
	}
	if (bSwapMotionAnywhere)
	{
		return true;
	}
	// Only where someone sees it: the editor's test worlds, other holders and a paused game change guns at once, as before.
	const UWorld* World = GetWorld();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!World || !World->IsGameWorld() || World->IsPaused() || !Pawn || !Pawn->IsPlayerControlled() || !Pawn->IsLocallyControlled())
	{
		return false;
	}
	// A menu over the game (the inventory's picks): the gun is lowered out of view under it already.
	const APlayerController* Player = Cast<APlayerController>(Pawn->GetController());
	const ALooterHUD* Hud = Player ? Player->GetHUD<ALooterHUD>() : nullptr;
	return !(Hud && Hud->IsMenuOpen());
}

// ---------------------------------------------------------------------------
// The swap's steps
// ---------------------------------------------------------------------------

void UWeaponManagerComponent::BeginSwap(AWeaponBase* Target, AWeaponBase* Pickup)
{
	AWeaponBase* InHand = GetActiveWeapon();
	const UWorld* World = GetWorld();
	if (!InHand || !World)
	{
		return;
	}
	SwapTarget = Target;
	SwapPickup = Pickup;
	// Already on its way down: only what comes up changes. A second press never starts the holster over.
	if (SwapClock.IsLowering())
	{
		return;
	}

	// From wherever it is: up, part way through its own draw, or coming back up from a swap called off.
	const float From = GetGunLowered();
	if (!SwapClock.IsRunning())
	{
		// Locked as it goes, with the lock a strike uses (BeginMeleeSwing): no shot leaves a gun on its way out of view, a
		// burst's rest is dropped, and a reload under way is cut (to start over if the swap is called off). A strike
		// swinging it holds it locked already, and lets go itself.
		InHand->StopFire();
		bSwapLocked = !InHand->IsMeleeSwinging();
		if (bSwapLocked)
		{
			InHand->BeginMeleeSwing();
		}
	}
	const EWeaponKind Kind = WeaponSwapMotion::KindOf(InHand);
	SwapClock.Lower(World->GetTimeSeconds(), From, WeaponSwapMotion::HolsterSeconds(Kind, InHand->GetStats().Handling));
	// A gun still below the view (just drawn) goes without a sound: the next one's draw is heard at once.
	if (From < WeaponSwapMotion::QuietFrom)
	{
		PlaySwapCue(WeaponSounds::Holster(Kind));
	}
	UpdateSwap();
}

void UWeaponManagerComponent::UpdateSwap()
{
	UWorld* World = GetWorld();
	if (!SwapClock.IsRunning() || !World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	if (!SwapClock.IsDone(Now))
	{
		World->GetTimerManager().SetTimer(SwapTimer, this, &UWeaponManagerComponent::UpdateSwap, SwapClock.SecondsLeft(Now), false);
		return;
	}
	World->GetTimerManager().ClearTimer(SwapTimer);
	if (SwapClock.IsLowering())
	{
		FinishHolster();
	}
	else
	{
		FinishRaise();
	}
}

void UWeaponManagerComponent::FinishHolster()
{
	AWeaponBase* Pickup = SwapPickup.Get();
	AWeaponBase* Target = SwapTarget.Get();
	const int32 TargetSlot = Target ? Weapons.IndexOfByKey(Target) : INDEX_NONE;
	if (Pickup && Pickup->IsPickup() && GetActiveWeapon())
	{
		// Loot taken in hand in this gun's place (EquipPickup): they change places now, out of sight.
		StopSwap();
		PutLootInHand(Pickup);
		return;
	}
	if (TargetSlot != INDEX_NONE && TargetSlot != ActiveSlot)
	{
		// The change: this gun is put away (its lock let go) and the next is drawn (SetActiveSlot ends the swap).
		SetActiveSlot(TargetSlot);
		return;
	}
	// What it was for is gone (dropped from the inventory, the loot taken some other way): this gun comes back up.
	CallOffSwap();
}

void UWeaponManagerComponent::CallOffSwap()
{
	const UWorld* World = GetWorld();
	if (!SwapClock.IsLowering() || !World)
	{
		return;
	}
	SwapTarget.Reset();
	SwapPickup.Reset();
	const double Now = World->GetTimeSeconds();
	const float From = SwapClock.GetLowered(Now);
	SwapClock.Raise(Now);
	// Back up from well down is a draw, and sounds like one; from barely started, it never really went.
	const AWeaponBase* InHand = GetActiveWeapon();
	if (InHand && From >= WeaponSwapMotion::ReturnHeardFrom)
	{
		PlaySwapCue(WeaponSounds::Equip(WeaponSwapMotion::KindOf(InHand)));
	}
	UpdateSwap();
}

void UWeaponManagerComponent::FinishRaise()
{
	AWeaponBase* InHand = GetActiveWeapon();
	const bool bUnlock = bSwapLocked;
	StopSwap();
	// Free to fire again, and a reload the swap cut short starts over, as after a strike. A strike swinging it now keeps it
	// locked: its own end lets go.
	const UPlayerMeleeComponent* Melee = GetOwner() ? GetOwner()->FindComponentByClass<UPlayerMeleeComponent>() : nullptr;
	if (InHand && bUnlock && InHand->IsMeleeSwinging() && !(Melee && Melee->IsSwinging()))
	{
		InHand->EndMeleeSwing(true);
	}
}

void UWeaponManagerComponent::StopSwap()
{
	SwapClock.Reset();
	SwapTarget.Reset();
	SwapPickup.Reset();
	bSwapLocked = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SwapTimer);
	}
}

void UWeaponManagerComponent::PlaySwapCue(FName Cue) const
{
	const AActor* Owner = GetOwner();
	LooterSound::PlayAttached(Cue, Owner ? Owner->GetRootComponent() : nullptr);
}
