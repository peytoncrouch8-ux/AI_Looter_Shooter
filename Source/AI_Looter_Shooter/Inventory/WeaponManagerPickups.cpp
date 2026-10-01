#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Settings/KeyBindingSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

// ---------------------------------------------------------------------------
// Pickups
// ---------------------------------------------------------------------------

bool UWeaponManagerComponent::TryPickup()
{
	AWeaponBase* Pickup = FocusedPickup.Get();
	if (!Pickup || !Pickup->IsPickup())
	{
		return false;
	}

	FocusedPickup.Reset();
	OnFocusedPickupChanged.Broadcast(nullptr);

	// Slots full but backpack has room: stash it like Borderlands does.
	if (Weapons.Num() >= MaxWeapons && Backpack.Num() < BackpackCapacity)
	{
		Backpack.Add(Pickup->GetInstanceForStorage());
		SendMessage(FString::Printf(TEXT("%s sent to backpack"), *Pickup->GetDisplayName().ToString()));
		Pickup->Destroy();
		OnInventoryChanged.Broadcast();
		return true;
	}

	if (!AddWeapon(Pickup))
	{
		return false;
	}

	// Picking something up means you want to use it.
	EquipSlot(Weapons.IndexOfByKey(Pickup));
	return true;
}

bool UWeaponManagerComponent::EquipPickup(AWeaponBase* Pickup)
{
	if (!IsValid(Pickup) || !Pickup->IsPickup())
	{
		return false;
	}
	if (FocusedPickup.Get() == Pickup)
	{
		FocusedPickup.Reset();
		OnFocusedPickupChanged.Broadcast(nullptr);
	}

	// A free slot takes it, the same as a tap.
	if (Weapons.Num() < MaxWeapons)
	{
		if (!AddWeapon(Pickup))
		{
			return false;
		}
		EquipSlot(Weapons.IndexOfByKey(Pickup));
		return true;
	}

	// Every slot is full: it takes the slot in use, and the weapon that was in it goes to the backpack, or onto the
	// ground when the backpack is full too.
	const int32 Slot = FMath::Max(ActiveSlot, 0);
	if (AWeaponBase* Outgoing = Weapons[Slot])
	{
		const FString OutgoingName = Outgoing->GetDisplayName().ToString();
		if (Backpack.Num() < BackpackCapacity)
		{
			Outgoing->OnHolstered();
			Backpack.Add(Outgoing->GetInstanceForStorage());
			Outgoing->Destroy();
			SendMessage(FString::Printf(TEXT("%s sent to backpack"), *OutgoingName));
		}
		else
		{
			Outgoing->OnDropped();
			TossWeaponAway(Outgoing);
			SendMessage(FString::Printf(TEXT("Backpack full: %s dropped"), *OutgoingName));
		}
	}
	Weapons[Slot] = Pickup;
	AttachHolstered(Pickup);
	ActiveSlot = INDEX_NONE; // the weapon that was in hand is gone; SetActiveSlot takes out the new one
	SetActiveSlot(Slot);
	OnInventoryChanged.Broadcast();
	return true;
}

float UWeaponManagerComponent::GetPickupHoldProgress() const
{
	const UWorld* World = GetWorld();
	if (!World || !PressedPickup.IsValid() || !World->GetTimerManager().IsTimerActive(PickupHoldTimer))
	{
		return 0.f;
	}
	return FMath::Clamp(World->GetTimerManager().GetTimerElapsed(PickupHoldTimer) / PickupHoldSeconds, 0.f, 1.f);
}

void UWeaponManagerComponent::HandleInteractPressed()
{
	// Nothing happens until the key comes up (a tap) or has been held long enough (equip).
	PressedPickup = FocusedPickup;
	if (PressedPickup.IsValid())
	{
		GetWorld()->GetTimerManager().SetTimer(PickupHoldTimer, this, &UWeaponManagerComponent::HandleInteractHeld, PickupHoldSeconds, false);
	}
}

void UWeaponManagerComponent::HandleInteractReleased()
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	const bool bTap = Timers.IsTimerActive(PickupHoldTimer);
	Timers.ClearTimer(PickupHoldTimer);
	AWeaponBase* Pickup = PressedPickup.Get();
	PressedPickup.Reset();

	// Only if the player still looks at what they pressed on: a tap never grabs something else.
	if (bTap && Pickup && Pickup == FocusedPickup.Get())
	{
		TryPickup();
	}
}

void UWeaponManagerComponent::HandleInteractHeld()
{
	AWeaponBase* Pickup = PressedPickup.Get();
	PressedPickup.Reset();
	if (Pickup && Pickup == FocusedPickup.Get())
	{
		EquipPickup(Pickup);
	}
}

void UWeaponManagerComponent::UpdatePickupFocus()
{
	APlayerController* PC = InputBinding.GetController();
	UWorld* World = GetWorld();
	if (!PC || !World)
	{
		return;
	}

	// Safety net for a missed trigger release (focus changes, alt-tab): if fire isn't actually held, stop.
	if (const UEnhancedPlayerInput* PlayerInput = Cast<UEnhancedPlayerInput>(PC->PlayerInput))
	{
		if (FireAction && !PlayerInput->GetActionValue(FireAction).Get<bool>())
		{
			StopFire();
		}
	}

	// Aim like the gun does (the crosshair in first/third person, the eyes in the front view); reach is measured
	// from the character, since a third-person camera sits a couple of meters behind them.
	FVector ViewLocation;
	FRotator ViewRotation;
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (const UPlayerViewComponent* View = Pawn ? Pawn->FindComponentByClass<UPlayerViewComponent>() : nullptr)
	{
		View->GetAimViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	const FVector ViewDirection = ViewRotation.Vector();
	const FVector ReachOrigin = Pawn ? Pawn->GetPawnViewLocation() : ViewLocation;

	AWeaponBase* BestPickup = nullptr;
	float BestAim = PickupAimThreshold;
	TArray<TWeakObjectPtr<AWeaponBase>> NowLabeled;

	for (TActorIterator<AWeaponBase> It(World); It; ++It)
	{
		AWeaponBase* Weapon = *It;
		if (!Weapon->IsPickup())
		{
			continue;
		}

		const FVector ToWeapon = Weapon->GetActorLocation() - ViewLocation;
		const float Distance = FVector::Dist(Weapon->GetActorLocation(), ReachOrigin);
		if (Distance > LabelRange)
		{
			continue;
		}
		NowLabeled.Add(Weapon);

		const float Aim = FVector::DotProduct(ToWeapon.GetSafeNormal(), ViewDirection);
		if (Distance <= PickupRange && Aim > BestAim)
		{
			BestAim = Aim;
			BestPickup = Weapon;
		}
	}

	// Hide labels for loot that went out of range, then refresh the ones in range.
	for (const TWeakObjectPtr<AWeaponBase>& Old : LabeledPickups)
	{
		if (Old.IsValid() && !NowLabeled.Contains(Old))
		{
			Old->SetLabelState(false, false);
		}
	}
	for (const TWeakObjectPtr<AWeaponBase>& Weapon : NowLabeled)
	{
		Weapon->SetLabelState(true, Weapon.Get() == BestPickup);
	}
	LabeledPickups = MoveTemp(NowLabeled);

	if (FocusedPickup.Get() != BestPickup)
	{
		FocusedPickup = BestPickup;
		OnFocusedPickupChanged.Broadcast(BestPickup);
	}
}
