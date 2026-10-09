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
// Inventory
// ---------------------------------------------------------------------------

TArray<AWeaponBase*> UWeaponManagerComponent::GetWeapons() const
{
	return ObjectPtrDecay(Weapons);
}

AWeaponBase* UWeaponManagerComponent::GetActiveWeapon() const
{
	return Weapons.IsValidIndex(ActiveSlot) ? Weapons[ActiveSlot].Get() : nullptr;
}

bool UWeaponManagerComponent::AddWeapon(AWeaponBase* Weapon)
{
	if (!IsValid(Weapon) || Weapons.Contains(Weapon))
	{
		return false;
	}

	int32 Slot = INDEX_NONE;
	if (Weapons.Num() >= MaxWeapons)
	{
		// Full: swap out the weapon in hand.
		Slot = FMath::Max(ActiveSlot, 0);
		if (AWeaponBase* Replaced = Weapons[Slot])
		{
			Replaced->OnDropped();
			TossWeaponAway(Replaced);
		}
		Weapons[Slot] = Weapon;
		ActiveSlot = INDEX_NONE; // force SetActiveSlot to re-equip
	}
	else
	{
		Slot = Weapons.Add(Weapon);
	}

	AttachHolstered(Weapon);
	OnInventoryChanged.Broadcast();

	if (ActiveSlot == INDEX_NONE)
	{
		SetActiveSlot(Slot);
	}
	return true;
}

AWeaponBase* UWeaponManagerComponent::GiveWeapon(const FWeaponInstanceData& Instance)
{
	AActor* Owner = GetOwner();
	AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(this, Instance, Owner ? Owner->GetActorTransform() : FTransform::Identity);
	if (Weapon && !AddWeapon(Weapon))
	{
		Weapon->Destroy();
		return nullptr;
	}
	return Weapon;
}

void UWeaponManagerComponent::EquipSlot(int32 SlotIndex)
{
	if (Weapons.IsValidIndex(SlotIndex) && SlotIndex != ActiveSlot)
	{
		SetActiveSlot(SlotIndex);
	}
}

void UWeaponManagerComponent::NextWeapon()
{
	if (Weapons.Num() > 1)
	{
		EquipSlot((ActiveSlot + 1) % Weapons.Num());
	}
}

void UWeaponManagerComponent::PreviousWeapon()
{
	if (Weapons.Num() > 1)
	{
		EquipSlot((ActiveSlot - 1 + Weapons.Num()) % Weapons.Num());
	}
}

AWeaponBase* UWeaponManagerComponent::DropActiveWeapon()
{
	AWeaponBase* Weapon = RemoveFromSlots(ActiveSlot);
	if (Weapon)
	{
		Weapon->OnDropped();
		TossWeaponAway(Weapon);
	}
	return Weapon;
}

AWeaponBase* UWeaponManagerComponent::RemoveFromSlots(int32 SlotIndex)
{
	if (!Weapons.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	AWeaponBase* Weapon = Weapons[SlotIndex];
	const bool bWasActive = SlotIndex == ActiveSlot;
	if (bWasActive && Weapon)
	{
		Weapon->OnHolstered();
	}
	Weapons.RemoveAt(SlotIndex);

	if (bWasActive)
	{
		ActiveSlot = INDEX_NONE;
		if (Weapons.Num() > 0)
		{
			SetActiveSlot(FMath::Min(SlotIndex, Weapons.Num() - 1));
		}
		else
		{
			OnActiveWeaponChanged.Broadcast(nullptr, Weapon);
		}
	}
	else if (SlotIndex < ActiveSlot)
	{
		--ActiveSlot;
	}

	OnInventoryChanged.Broadcast();
	return Weapon;
}

bool UWeaponManagerComponent::DropSlot(int32 SlotIndex)
{
	AWeaponBase* Weapon = RemoveFromSlots(SlotIndex);
	if (!Weapon)
	{
		return false;
	}
	Weapon->OnDropped();
	TossWeaponAway(Weapon);
	return true;
}

bool UWeaponManagerComponent::StashSlot(int32 SlotIndex)
{
	if (!Weapons.IsValidIndex(SlotIndex))
	{
		return false;
	}
	if (Backpack.Num() >= BackpackCapacity)
	{
		SendMessage(TEXT("Backpack full"));
		return false;
	}

	AWeaponBase* Weapon = RemoveFromSlots(SlotIndex);
	Backpack.Add(Weapon->GetInstanceForStorage());
	Weapon->Destroy();
	OnInventoryChanged.Broadcast();
	return true;
}

bool UWeaponManagerComponent::EquipFromBackpack(int32 BackpackIndex)
{
	if (!Backpack.IsValidIndex(BackpackIndex))
	{
		return false;
	}

	const FWeaponInstanceData Incoming = Backpack[BackpackIndex];
	Backpack.RemoveAt(BackpackIndex);

	if (Weapons.Num() >= MaxWeapons)
	{
		// Swap: the weapon in hand goes into the backpack where the new one came from.
		const int32 Slot = FMath::Max(ActiveSlot, 0);
		AWeaponBase* Outgoing = RemoveFromSlots(Slot);
		Backpack.Insert(Outgoing->GetInstanceForStorage(), BackpackIndex);
		Outgoing->Destroy();
	}

	AWeaponBase* Weapon = GiveWeapon(Incoming);
	if (Weapon)
	{
		EquipSlot(Weapons.IndexOfByKey(Weapon));
	}
	OnInventoryChanged.Broadcast();
	return Weapon != nullptr;
}

bool UWeaponManagerComponent::DropFromBackpack(int32 BackpackIndex)
{
	if (!Backpack.IsValidIndex(BackpackIndex))
	{
		return false;
	}

	const FWeaponInstanceData Instance = Backpack[BackpackIndex];
	Backpack.RemoveAt(BackpackIndex);

	AActor* Owner = GetOwner();
	if (AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(this, Instance, Owner ? Owner->GetActorTransform() : FTransform::Identity))
	{
		TossWeaponAway(Weapon);
	}
	OnInventoryChanged.Broadcast();
	return true;
}

bool UWeaponManagerComponent::SwapSlots(int32 SlotA, int32 SlotB)
{
	if (!Weapons.IsValidIndex(SlotA) || !Weapons.IsValidIndex(SlotB) || SlotA == SlotB)
	{
		return false;
	}
	Weapons.Swap(SlotA, SlotB);
	// The weapon in hand moved with its slot.
	if (ActiveSlot == SlotA)
	{
		ActiveSlot = SlotB;
	}
	else if (ActiveSlot == SlotB)
	{
		ActiveSlot = SlotA;
	}
	OnInventoryChanged.Broadcast();
	return true;
}

bool UWeaponManagerComponent::SwapSlotWithBackpack(int32 SlotIndex, int32 BackpackIndex)
{
	if (!Weapons.IsValidIndex(SlotIndex) || !Backpack.IsValidIndex(BackpackIndex))
	{
		return false;
	}

	// Make the incoming weapon first, so a failure leaves everything as it was.
	AActor* Owner = GetOwner();
	AWeaponBase* Incoming = UWeaponRollLibrary::SpawnWeapon(this, Backpack[BackpackIndex], Owner ? Owner->GetActorTransform() : FTransform::Identity);
	if (!Incoming)
	{
		return false;
	}

	AWeaponBase* Outgoing = Weapons[SlotIndex];
	const bool bWasInHand = SlotIndex == ActiveSlot;
	if (Outgoing)
	{
		Outgoing->OnHolstered();
		Backpack[BackpackIndex] = Outgoing->GetInstanceForStorage();
		Outgoing->Destroy();
	}
	else
	{
		Backpack.RemoveAt(BackpackIndex);
	}

	Weapons[SlotIndex] = Incoming;
	AttachHolstered(Incoming);
	if (bWasInHand)
	{
		ActiveSlot = INDEX_NONE; // force SetActiveSlot to take out the new weapon
		SetActiveSlot(SlotIndex);
	}
	OnInventoryChanged.Broadcast();
	return true;
}

bool UWeaponManagerComponent::MoveBackpackToSlot(int32 BackpackIndex)
{
	if (!Backpack.IsValidIndex(BackpackIndex) || Weapons.Num() >= MaxWeapons)
	{
		return false;
	}
	const FWeaponInstanceData Instance = Backpack[BackpackIndex];
	Backpack.RemoveAt(BackpackIndex);
	if (!GiveWeapon(Instance))
	{
		Backpack.Insert(Instance, BackpackIndex);
		return false;
	}
	OnInventoryChanged.Broadcast();
	return true;
}

bool UWeaponManagerComponent::MoveSlot(int32 FromSlot, int32 ToSlot)
{
	ToSlot = FMath::Min(ToSlot, Weapons.Num() - 1);
	if (!Weapons.IsValidIndex(FromSlot) || ToSlot < 0 || FromSlot == ToSlot)
	{
		return false;
	}
	AWeaponBase* InHand = GetActiveWeapon();
	TObjectPtr<AWeaponBase> Weapon = Weapons[FromSlot];
	Weapons.RemoveAt(FromSlot);
	Weapons.Insert(Weapon, ToSlot);
	// The weapon in hand stays in hand; only its slot number changes.
	if (InHand)
	{
		ActiveSlot = Weapons.IndexOfByKey(InHand);
	}
	OnInventoryChanged.Broadcast();
	return true;
}

bool UWeaponManagerComponent::AddToBackpack(const FWeaponInstanceData& Instance)
{
	if (!Instance.Definition)
	{
		return false;
	}
	if (Backpack.Num() >= BackpackCapacity)
	{
		SendMessage(TEXT("Backpack full"));
		return false;
	}
	Backpack.Add(Instance);
	OnInventoryChanged.Broadcast();
	return true;
}

void UWeaponManagerComponent::SendMessage(const FString& Message)
{
	OnMessage.Broadcast(FText::FromString(Message));
}

void UWeaponManagerComponent::SetActiveSlot(int32 NewSlot)
{
	AWeaponBase* OldWeapon = GetActiveWeapon();
	if (OldWeapon)
	{
		OldWeapon->OnHolstered();
	}

	ActiveSlot = NewSlot;

	AWeaponBase* NewWeapon = GetActiveWeapon();
	if (NewWeapon)
	{
		USceneComponent* HoldParent = nullptr;
		FName HoldSocket;
		FTransform HoldOffset;
		GetHold(NewWeapon, HoldParent, HoldSocket, HoldOffset);
		NewWeapon->OnEquipped(Cast<APawn>(GetOwner()), HoldParent, HoldSocket, HoldOffset);
	}

	OnActiveWeaponChanged.Broadcast(NewWeapon, OldWeapon);
}

void UWeaponManagerComponent::AttachHolstered(AWeaponBase* Weapon)
{
	// Attached now so it follows the owner even while put away.
	USceneComponent* HoldParent = nullptr;
	FName HoldSocket;
	FTransform HoldOffset;
	GetHold(Weapon, HoldParent, HoldSocket, HoldOffset);
	Weapon->OnEquipped(Cast<APawn>(GetOwner()), HoldParent, HoldSocket, HoldOffset);
	Weapon->OnHolstered();
}

void UWeaponManagerComponent::TossWeaponAway(AWeaponBase* Weapon) const
{
	const AActor* Owner = GetOwner();
	if (!Weapon || !Owner)
	{
		return;
	}

	const FVector Forward = Owner->GetActorForwardVector();
	Weapon->SetActorLocationAndRotation(Owner->GetActorLocation() + Forward * 60.f + FVector(0.f, 0.f, 40.f), Owner->GetActorRotation());
	Weapon->Toss(Forward * 250.f + FVector(0.f, 0.f, 250.f));
}

// ---------------------------------------------------------------------------
// Where weapons are held
// ---------------------------------------------------------------------------

void UWeaponManagerComponent::SetThirdPersonHold(bool bThirdPerson)
{
	if (bThirdPersonHold == bThirdPerson)
	{
		return;
	}
	bThirdPersonHold = bThirdPerson;
	for (AWeaponBase* Weapon : Weapons)
	{
		if (IsValid(Weapon))
		{
			USceneComponent* HoldParent = nullptr;
			FName HoldSocket;
			FTransform HoldOffset;
			GetHold(Weapon, HoldParent, HoldSocket, HoldOffset);
			Weapon->AttachToHolder(HoldParent, HoldSocket, HoldOffset);
		}
	}
}

void UWeaponManagerComponent::GetHold(const AWeaponBase* Weapon, USceneComponent*& OutParent, FName& OutSocket, FTransform& OutOffset) const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (bThirdPersonHold && Character && Character->GetMesh())
	{
		// Put the gun's grip in the right hand; the rifle animations aim the hand.
		OutParent = Character->GetMesh();
		OutSocket = ThirdPersonAttachSocket;
		OutOffset = FTransform(Weapon ? -Weapon->GetGripPoint() : FVector::ZeroVector);
		return;
	}
	OutParent = FindAttachComponent();
	OutSocket = AttachSocket;
	OutOffset = GetFirstPersonHold(Weapon);
}

FTransform UWeaponManagerComponent::GetFirstPersonHold(const AWeaponBase* Weapon) const
{
	// Guns are modeled around different origins (the bullpup's is its butt), so line them up by where the hand holds them.
	FTransform Hold = AttachOffset;
	if (Weapon && !Weapon->GetGripPoint().IsZero())
	{
		// A six-gun (the Drover) is held a little forward along its barrel, half its HoldReach (the third-person arms
		// reach the rest of the way, LooterStancePose); a long gun's is 0, so it sits where it did.
		const UWeaponDefinition* Def = Weapon->GetInstance().Definition.Get();
		Hold.AddToTranslation(Hold.TransformVector(AttachGrip - Weapon->GetGripPoint() + FVector(Def ? Def->HoldReach * 0.5f : 0.f, 0.f, 0.f)));
	}
	return Hold;
}

USceneComponent* UWeaponManagerComponent::FindAttachComponent() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (!AttachComponentName.IsNone())
	{
		TInlineComponentArray<USceneComponent*> Components(Owner);
		for (USceneComponent* Component : Components)
		{
			if (Component->GetFName() == AttachComponentName)
			{
				return Component;
			}
		}
	}

	if (const ACharacter* Character = Cast<ACharacter>(Owner))
	{
		return Character->GetMesh();
	}
	return Owner->GetRootComponent();
}
