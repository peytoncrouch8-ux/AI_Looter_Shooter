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
