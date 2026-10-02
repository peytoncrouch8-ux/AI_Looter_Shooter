#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/InteractionSource.h"
#include "Player/PawnInputBinding.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponManagerComponent.generated.h"

class AController;
class APawn;
class AWeaponBase;
class UInputAction;
class UInputMappingContext;
class UInteractionComponent;
class USceneComponent;
class UWeaponDefinition;
struct FWeaponInventorySave;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveWeaponChanged, AWeaponBase*, NewWeapon, AWeaponBase*, OldWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponMessage, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoPoolChanged, EAmmoType, Type, int32, Carried);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoPickedUp, EAmmoType, Type, int32, Amount);

USTRUCT(BlueprintType)
struct FStartingWeapon
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> Definition = nullptr;

	/** If false, the rarity below is used instead of a random roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bRandomRarity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (EditCondition = "!bRandomRarity"))
	EWeaponRarity Rarity = EWeaponRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	int32 Level = 1;
};

/**
 * Add to any pawn to let it carry and use weapons. Handles slots, equipping, swapping, dropping,
 * and (for players) binds its own Enhanced Input actions so the character Blueprint needs no wiring.
 *
 * Loot lying in the world is offered to the player's interaction component (IInteractionSource): that finds the gun
 * looked at and owns the Interact key, and a tap or a hold on loot comes back here (TryPickup, EquipPickup).
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UWeaponManagerComponent : public UActorComponent, public IInteractionSource
{
	GENERATED_BODY()

public:
	UWeaponManagerComponent();

	/** Adds a weapon. If every slot is full, the active weapon is dropped and replaced. */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	bool AddWeapon(AWeaponBase* Weapon);

	/** Spawns a weapon from a rolled instance and adds it. */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	AWeaponBase* GiveWeapon(const FWeaponInstanceData& Instance);

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void EquipSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void NextWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void PreviousWeapon();

	/** Drops the active weapon in front of the owner and returns it. */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	AWeaponBase* DropActiveWeapon();

	/**
	 * Picks up a loot weapon (a tap of the interact key on it): into a free equip slot, else the backpack, else it swaps
	 * with the weapon in hand.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Pickup")
	bool TryPickup(AWeaponBase* Pickup);

	/**
	 * Picks up a loot weapon and takes it in hand (holding the interact key): into a free equip slot when there is one,
	 * else in place of the weapon in hand, which goes to the backpack, or onto the ground when the backpack is full.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Pickup")
	bool EquipPickup(AWeaponBase* Pickup);

	// --- Loot as things to use (IInteractionSource; WeaponManagerPickups.cpp) ---

	/** The loot lying within PickupRange of the player's eyes. */
	virtual void GatherInteractions(const FInteractionView& View, TArray<FInteractionCandidate>& OutCandidates) const override;

	/** Loot takes a tap (TryPickup) and a hold of PickupHoldSeconds (EquipPickup); the HUD's loot card spells out what each does. */
	virtual FInteractionOptions GetOfferOptions(const AActor& Offer) const override;

	virtual bool UseOffer(AActor& Offer, bool bHeld) override;

	// --- Inventory ---

	/** Moves an equipped weapon into the backpack. */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Inventory")
	bool StashSlot(int32 SlotIndex);

	/** Equips a backpack weapon. If all slots are full, the weapon in hand takes its place in the backpack. */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Inventory")
	bool EquipFromBackpack(int32 BackpackIndex);

	UFUNCTION(BlueprintCallable, Category = "Weapons|Inventory")
	bool DropSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Weapons|Inventory")
	bool DropFromBackpack(int32 BackpackIndex);

	/** Swaps two equipped slots (reorders them). The weapon in hand stays in hand. */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Inventory")
	bool SwapSlots(int32 SlotA, int32 SlotB);

	/** An equipped weapon and a backpack item trade places. If the slot's weapon was in hand, the new one is now. */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Inventory")
	bool SwapSlotWithBackpack(int32 SlotIndex, int32 BackpackIndex);

	/** Moves a backpack item into a free equipped slot, leaving the weapon in hand as it is. */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Inventory")
	bool MoveBackpackToSlot(int32 BackpackIndex);

	/** Moves an equipped weapon to another slot, shifting the ones between; past the last weapon means last. The weapon in hand stays in hand. */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Inventory")
	bool MoveSlot(int32 FromSlot, int32 ToSlot);

	/** Puts a weapon straight into the backpack, if there's room. */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Inventory")
	bool AddToBackpack(const FWeaponInstanceData& Instance);

	UFUNCTION(BlueprintPure, Category = "Weapons|Inventory")
	const TArray<FWeaponInstanceData>& GetBackpack() const { return Backpack; }

	// --- Ammo: one shared pool per ammo class, used by every weapon of that class ---

	/** Adds up to Amount rounds (capped at the class's carry limit). Returns how many were actually taken. */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Ammo")
	int32 AddAmmo(EAmmoType Type, int32 Amount);

	/** Removes up to Amount rounds and returns how many were removed. */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Ammo")
	int32 TakeAmmo(EAmmoType Type, int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Weapons|Ammo")
	int32 GetAmmo(EAmmoType Type) const;

	UFUNCTION(BlueprintPure, Category = "Weapons|Ammo")
	int32 GetMaxAmmo(EAmmoType Type) const;

	// --- Saved sessions (WeaponManagerSave.cpp) ---

	/** What it carries, for a saved session: the guns in their slots with their magazines, the one in hand, the backpack and the ammo. */
	void SaveInventory(FWeaponInventorySave& OutSave) const;

	/** Carries what a saved session had, in place of whatever it carries now. */
	void RestoreInventory(const FWeaponInventorySave& Save);

	/** Empties it without dropping anything: the guns are gone, and the backpack and the ammo. */
	void ClearInventory();

	/** Carried ammo of a class changed (pickups, reloads). */
	UPROPERTY(BlueprintAssignable, Category = "Weapons|Ammo")
	FOnAmmoPoolChanged OnAmmoChanged;

	/** An ammo pickup gave the player Amount rounds (the HUD's pickup feed); Amount is 0 when that class was already full. */
	UPROPERTY(BlueprintAssignable, Category = "Weapons|Ammo")
	FOnAmmoPickedUp OnAmmoPickedUp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Inventory", meta = (ClampMin = "0"))
	int32 BackpackCapacity = 12;

	/** Short feedback for the HUD ("Backpack full", "Sent to backpack", ...). */
	UPROPERTY(BlueprintAssignable, Category = "Weapons")
	FOnWeaponMessage OnMessage;

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void Reload();

	UFUNCTION(BlueprintPure, Category = "Weapons")
	AWeaponBase* GetActiveWeapon() const;

	UFUNCTION(BlueprintPure, Category = "Weapons")
	TArray<AWeaponBase*> GetWeapons() const;

	UFUNCTION(BlueprintPure, Category = "Weapons")
	int32 GetActiveSlot() const { return ActiveSlot; }

	UPROPERTY(BlueprintAssignable, Category = "Weapons")
	FOnActiveWeaponChanged OnActiveWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapons")
	FOnWeaponInventoryChanged OnInventoryChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons", meta = (ClampMin = "1"))
	int32 MaxWeapons = 3;

	/** Weapons rolled and given on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	TArray<FStartingWeapon> StartingWeapons;

	/** Name of the owner's mesh component to hold weapons with. If not found, the character's main mesh is used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Attachment")
	FName AttachComponentName = TEXT("FirstPersonMesh");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Attachment")
	FName AttachSocket = TEXT("HandGrip_R");

	/**
	 * Offset from the attach point, e.g. to place a camera-attached gun in the lower right of the view. It places a gun
	 * whose grip is at AttachGrip; guns with their grip elsewhere are shifted so every grip lands in the same spot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Attachment")
	FTransform AttachOffset;

	/** The grip point (in the gun's own space) AttachOffset was set for: the first rifle's. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Attachment")
	FVector AttachGrip = FVector(5.f, 0.f, -5.f);

	/** The first-person hold for this gun: AttachOffset, moved so the gun's grip is where AttachGrip would be. */
	FTransform GetFirstPersonHold(const AWeaponBase* Weapon) const;

	/** Socket on the character's body mesh (in the right hand) that holds the gun by its grip in third person. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Attachment")
	FName ThirdPersonAttachSocket = TEXT("HandGrip_R");

	/**
	 * Holds weapons in the body's hand (third person) instead of the first-person attach point. The gun's grip point
	 * is placed on ThirdPersonAttachSocket. Re-attaches every carried weapon without interrupting reloads.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapons|Attachment")
	void SetThirdPersonHold(bool bThirdPerson);

	UFUNCTION(BlueprintPure, Category = "Weapons|Attachment")
	bool IsThirdPersonHold() const { return bThirdPersonHold; }

	/**
	 * How close (cm) loot must be to the player's eyes to pick it up (the loot's reach in the interaction component, whose
	 * cone, AimThreshold, says how directly it must be looked at).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Pickup", meta = (ClampMin = "0"))
	float PickupRange = 250.f;

	/** Loot labels are shown within this distance (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Pickup", meta = (ClampMin = "0"))
	float LabelRange = 1500.f;

	/** Seconds the interact key is held on loot to equip it; a shorter press picks it up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Pickup", meta = (ClampMin = "0.1"))
	float PickupHoldSeconds = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Input")
	int32 InputMappingPriority = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Input")
	TObjectPtr<UInputAction> NextWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Input")
	TObjectPtr<UInputAction> PreviousWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons|Input")
	TObjectPtr<UInputAction> DropWeaponAction;

	// The interact key belongs to the interaction component (UInteractionComponent), which hands loot back here.

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	/** The interaction component's focus moved: the loot labels follow at once. */
	UFUNCTION()
	void HandleInteractionFocusChanged(AActor* Focused);

	void SetupInput(AController* Controller);
	void TeardownInput();
	void HandleDropInput();

	/**
	 * Runs a few times a second for the local player: loot labels within LabelRange (the one the interaction component
	 * focuses shows its stats), and the fire key's safety net.
	 */
	void UpdateLootLabels();

	USceneComponent* FindAttachComponent() const;
	/** Where a weapon is held right now: the first-person attach point, or the body's hand in third person. */
	void GetHold(const AWeaponBase* Weapon, USceneComponent*& OutParent, FName& OutSocket, FTransform& OutOffset) const;
	void SetActiveSlot(int32 NewSlot);
	void TossWeaponAway(AWeaponBase* Weapon) const;

	/** Attaches a weapon that just joined the slots where it's held, put away until it's taken in hand. */
	void AttachHolstered(AWeaponBase* Weapon);

	/** Takes a weapon out of the equip slots (fixing up the active slot) without destroying it. */
	AWeaponBase* RemoveFromSlots(int32 SlotIndex);
	void SendMessage(const FString& Message);

	UPROPERTY(Transient)
	TArray<FWeaponInstanceData> Backpack;

	/** Rounds carried per ammo class, indexed by EAmmoType. */
	int32 AmmoPool[LooterAmmo::NumTypes] = {};

	UPROPERTY(Transient)
	TArray<TObjectPtr<AWeaponBase>> Weapons;

	FPawnInputBinding InputBinding;
	/** The number keys that take a slot in hand (UKeyBindingSubsystem's weapon slot context). */
	FPawnInputBinding SlotInputBinding;
	TArray<TWeakObjectPtr<AWeaponBase>> LabeledPickups;
	FTimerHandle LootLabelTimer;

	/** The owner's interaction component, whose focus the labels follow. */
	TWeakObjectPtr<UInteractionComponent> LabelFocus;

	int32 ActiveSlot = INDEX_NONE;
	bool bThirdPersonHold = false;
};
