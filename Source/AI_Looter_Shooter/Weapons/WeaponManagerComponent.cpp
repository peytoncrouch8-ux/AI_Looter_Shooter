#include "Weapons/WeaponManagerComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponRollLibrary.h"
#include "Settings/KeyBindingSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

UWeaponManagerComponent::UWeaponManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UWeaponManagerComponent::HandleControllerChanged);
		SetupInput(Pawn->GetController());
	}

	for (const FStartingWeapon& Starting : StartingWeapons)
	{
		if (!Starting.Definition)
		{
			continue;
		}
		const FWeaponInstanceData Instance = Starting.bRandomRarity
			? UWeaponRollLibrary::RollWeapon(Starting.Definition, Starting.Level)
			: UWeaponRollLibrary::RollWeaponWithRarity(Starting.Definition, Starting.Rarity, Starting.Level);
		if (GiveWeapon(Instance))
		{
			// Starting weapons come with a few magazines; everything after that comes from ammo drops.
			AddAmmo(Starting.Definition->AmmoType, Instance.Stats.MagazineSize * Starting.Definition->StartingReserveMagazines);
		}
	}
}

// ---------------------------------------------------------------------------
// Ammo
// ---------------------------------------------------------------------------

int32 UWeaponManagerComponent::GetAmmo(EAmmoType Type) const
{
	return LooterAmmo::IsValid(Type) ? AmmoPool[static_cast<int32>(Type)] : 0;
}

int32 UWeaponManagerComponent::GetMaxAmmo(EAmmoType Type) const
{
	return LooterAmmo::IsValid(Type) ? LooterAmmo::GetInfo(Type).MaxCarried : 0;
}

int32 UWeaponManagerComponent::AddAmmo(EAmmoType Type, int32 Amount)
{
	if (!LooterAmmo::IsValid(Type) || Amount <= 0)
	{
		return 0;
	}
	int32& Carried = AmmoPool[static_cast<int32>(Type)];
	const int32 Added = FMath::Min(Amount, FMath::Max(GetMaxAmmo(Type) - Carried, 0));
	if (Added > 0)
	{
		Carried += Added;
		OnAmmoChanged.Broadcast(Type, Carried);
	}
	return Added;
}

int32 UWeaponManagerComponent::TakeAmmo(EAmmoType Type, int32 Amount)
{
	if (!LooterAmmo::IsValid(Type) || Amount <= 0)
	{
		return 0;
	}
	int32& Carried = AmmoPool[static_cast<int32>(Type)];
	const int32 Taken = FMath::Min(Amount, Carried);
	if (Taken > 0)
	{
		Carried -= Taken;
		OnAmmoChanged.Broadcast(Type, Carried);
	}
	return Taken;
}

void UWeaponManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TeardownInput();

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UWeaponManagerComponent::HandleControllerChanged);
	}

	for (AWeaponBase* Weapon : Weapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->Destroy();
		}
	}
	Weapons.Reset();

	Super::EndPlay(EndPlayReason);
}

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

	APawn* Pawn = Cast<APawn>(GetOwner());
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

	// Attach now so the weapon follows the owner even while holstered.
	USceneComponent* HoldParent = nullptr;
	FName HoldSocket;
	FTransform HoldOffset;
	GetHold(Weapon, HoldParent, HoldSocket, HoldOffset);
	Weapon->OnEquipped(Pawn, HoldParent, HoldSocket, HoldOffset);
	Weapon->OnHolstered();

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
	USceneComponent* HoldParent = nullptr;
	FName HoldSocket;
	FTransform HoldOffset;
	GetHold(Incoming, HoldParent, HoldSocket, HoldOffset);
	Incoming->OnEquipped(Cast<APawn>(Owner), HoldParent, HoldSocket, HoldOffset);
	Incoming->OnHolstered();
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
	OutOffset = AttachOffset;
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

// ---------------------------------------------------------------------------
// Firing passthrough
// ---------------------------------------------------------------------------

void UWeaponManagerComponent::StartFire()
{
	if (AWeaponBase* Weapon = GetActiveWeapon())
	{
		Weapon->StartFire();
	}
}

void UWeaponManagerComponent::StopFire()
{
	if (AWeaponBase* Weapon = GetActiveWeapon())
	{
		Weapon->StopFire();
	}
}

void UWeaponManagerComponent::Reload()
{
	if (AWeaponBase* Weapon = GetActiveWeapon())
	{
		Weapon->Reload();
	}
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void UWeaponManagerComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	SetupInput(NewController);
}

void UWeaponManagerComponent::SetupInput(AController* Controller)
{
	TeardownInput();

	// Use the player's rebound copy of the weapon controls when available.
	UKeyBindingSubsystem* Bindings = FPawnInputBinding::GetBindings(Controller);
	UInputMappingContext* Context = Bindings ? Bindings->GetRuntimeContext(InputMappingContext) : InputMappingContext.Get();
	UEnhancedInputComponent* Input = InputBinding.Setup(GetOwner(), Controller, Context, InputMappingPriority);
	if (!Input)
	{
		return;
	}

	if (FireAction)
	{
		Input->BindAction(FireAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::StartFire);
		Input->BindAction(FireAction, ETriggerEvent::Completed, this, &UWeaponManagerComponent::StopFire);
		Input->BindAction(FireAction, ETriggerEvent::Canceled, this, &UWeaponManagerComponent::StopFire);
	}
	if (ReloadAction)
	{
		Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::Reload);
	}
	if (NextWeaponAction)
	{
		Input->BindAction(NextWeaponAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::NextWeapon);
	}
	if (PreviousWeaponAction)
	{
		Input->BindAction(PreviousWeaponAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::PreviousWeapon);
	}
	if (DropWeaponAction)
	{
		Input->BindAction(DropWeaponAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::HandleDropInput);
	}
	if (InteractAction)
	{
		Input->BindAction(InteractAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::HandleInteractInput);
	}

	// Loot labels and pickup focus only matter to the local player.
	GetWorld()->GetTimerManager().SetTimer(PickupFocusTimer, this, &UWeaponManagerComponent::UpdatePickupFocus, 0.1f, true);
}

void UWeaponManagerComponent::TeardownInput()
{
	// Losing input (Build Mode, unpossess) means the release event may never arrive.
	StopFire();
	InputBinding.Teardown();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PickupFocusTimer);
	}
	for (const TWeakObjectPtr<AWeaponBase>& Weapon : LabeledPickups)
	{
		if (Weapon.IsValid())
		{
			Weapon->SetLabelState(false, false);
		}
	}
	LabeledPickups.Reset();
	FocusedPickup.Reset();
}

void UWeaponManagerComponent::HandleDropInput()
{
	DropActiveWeapon();
}

void UWeaponManagerComponent::HandleInteractInput()
{
	TryPickup();
}
