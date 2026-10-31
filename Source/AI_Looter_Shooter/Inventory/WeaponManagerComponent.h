#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/InteractionSource.h"
#include "Inventory/BackpackOrder.h"
#include "Inventory/GunSlotRules.h"
#include "Inventory/WeaponManagerTypes.h"
#include "Player/PawnInputBinding.h"
#include "Player/WeaponSwapMotion.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponPartSwap.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponManagerComponent.generated.h"

class AController;
class APawn;
class AWeaponBase;
class UInputAction;
class UInputMappingContext;
class UInteractionComponent;
class UMissionDefinition;
class UMissionRunner;
class UPlayerProgressionSubsystem;
class USceneComponent;
struct FWeaponInventorySave;
enum class EXPSource : uint8;

/**
 * Add to any pawn to let it carry and use weapons. Handles slots, equipping, swapping, dropping,
 * and (for players) binds its own Enhanced Input actions so the character Blueprint needs no wiring.
 *
 * Loot lying in the world is offered to the player's interaction component (IInteractionSource): that finds the gun
 * looked at and owns the Interact key, and a tap or a hold on loot comes back here (TryPickup, EquipPickup).
 *
 * It also keeps the gunsmith's bench's parts box (AGunsmithBench): guns scrapped for one of their parts, and parts fitted
 * from the box onto guns of their kind.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UWeaponManagerComponent : public UActorComponent, public IInteractionSource
{
	GENERATED_BODY()

public:
	UWeaponManagerComponent();

	/** Adds a weapon. If every open slot is full, the active weapon is dropped and replaced. */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	bool AddWeapon(AWeaponBase* Weapon);

	/** Spawns a weapon from a rolled instance and adds it. */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	AWeaponBase* GiveWeapon(const FWeaponInstanceData& Instance);

	/**
	 * Takes a slot's gun in hand (number keys, wheel, pickups, inventory). In a local player's view the gun in hand goes down
	 * first and the change happens out of sight (a swap); elsewhere at once. The in-hand gun's own slot calls a swap off.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void EquipSlot(int32 SlotIndex);

	/** The next slot's gun in hand, counting on from the one a swap is bringing up (two turns of the wheel go two guns on). */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void NextWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void PreviousWeapon();

	// --- Swaps (WeaponManagerSwap.cpp; WeaponSwapMotion has their look) ---

	/**
	 * A swap is under way: the gun in hand on its way out of view (or, called off, back up). It neither fires nor reloads
	 * meanwhile, and stays the active weapon until the change, so the HUD, the hands and the body follow the gun seen.
	 */
	bool IsSwapping() const { return SwapClock.IsRunning(); }

	/** The slot whose gun a swap brings up once the one in hand is down; INDEX_NONE without one (or bringing up loot). */
	int32 GetSwapTargetSlot() const;

	/**
	 * How far the gun in hand is from its hold now (WeaponSwapMotion): 0 up, 1 out of view, below 0 a draw's overshoot; a
	 * swap's holster while one runs, else the gun's own draw. The first-person view poses the gun by it, the body its arm.
	 */
	float GetGunLowered() const;

	/** Moves a swap on to the world's time now: the change once the gun is down, the end of one called off. Its timer calls it. */
	void UpdateSwap();

	/** Tests: swaps play out in any world, held by anyone (else only a local player's, in play, with no menu open). */
	bool bSwapMotionAnywhere = false;

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

	/** Equips a backpack weapon. If all open slots are full, the weapon in hand takes its place in the backpack. */
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

	/** The backpack's guns (its spare wards are the shield component's: GetBackpackItems lists both, in order). */
	UFUNCTION(BlueprintPure, Category = "Weapons|Inventory")
	const TArray<FWeaponInstanceData>& GetBackpack() const { return Backpack; }

	// --- The backpack's shared room (WeaponManagerBackpack.cpp; the user, 2026-10-31: guns and wards share it) ---

	/** Its guns and the owner's spare wards (UPlayerShieldComponent::GetSpares), and the spares alone. */
	int32 GetBackpackUsed() const;
	int32 NumBackpackWards() const;
	/** Room for one more gun or ward: every "is there room" asks this. An old save may load over the room once (nothing is
	 *  thrown away); then nothing goes in until enough comes out. */
	bool HasBackpackRoom() const;
	/** The guns and wards in the order they went in (the save keeps it; the loadout lists by it). */
	TArray<FBackpackItem> GetBackpackItems() const;
	/** The spares changed (RemovedSpare: the one that came out; a new one goes in last): the order follows, listeners hear it. */
	void NoteWardsChanged(int32 RemovedSpare = INDEX_NONE);

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

	/** Empties it without dropping anything: the guns are gone, and the backpack, the ammo and the parts box. */
	void ClearInventory();

	// --- Guns never vanish unseen (WeaponManagerWatch.cpp; the vanished guns of 2026-10-14) ---

	/**
	 * A gun ends play destroyed (AWeaponBase::EndPlay). One still in the slots that this didn't destroy itself was taken
	 * from the player some other way: a warning names it, with the callstack, and its slot is let go.
	 */
	void NoteGunDestroyed(AWeaponBase& Gun);

	// --- The gunsmith's bench: the parts box, scrapping and fitting (WeaponManagerBench.cpp) ---

	/** The most parts the box holds: scrapping stops there until parts are fitted or thrown out. */
	static constexpr int32 MaxBoxedParts = 100;

	/** The parts box: parts kept from scrapped guns and taken off guns at the bench, oldest first. Saved with the session. */
	const TArray<FBoxedWeaponPart>& GetPartsBox() const { return PartsBox; }

	/** The gun the player carries there, or null. */
	const FWeaponInstanceData* FindCarriedGun(const FCarriedGun& Gun) const;

	/** How many guns the player carries, in the equip slots and the backpack. */
	int32 NumCarriedGuns() const;

	/**
	 * Whether a carried gun can be scrapped now; OutWhy says why not. A named gun can't (its parts are its own), nor the
	 * last gun carried, nor the last one in the equip slots (the player is never left with nothing to shoot), nor any while
	 * the parts box is full.
	 */
	bool CanScrap(const FCarriedGun& Gun, FText* OutWhy = nullptr) const;

	/**
	 * Scraps a carried gun, keeping its part in KeepSlot in the parts box; the rest of the gun is gone. False (nothing
	 * changed) unless CanScrap and that slot has a part.
	 */
	bool ScrapGun(const FCarriedGun& Gun, FName KeepSlot);

	/** Whether the box's part at BoxIndex fits a carried gun (WeaponPartSwap::CanFit; NoSuchPart when either is missing). */
	WeaponPartSwap::ECheck CheckFit(const FCarriedGun& Gun, int32 BoxIndex) const;

	/**
	 * Fits the box's part at BoxIndex onto a carried gun, and the part it replaces takes its place in the box (nothing for a
	 * slot that was empty). A gun in an equip slot is rebuilt with its new part, in hand if it was, keeping the rounds loaded
	 * up to its new magazine's size; any over go back to the ammo carried. False (nothing changed) unless it fits.
	 */
	bool FitPart(const FCarriedGun& Gun, int32 BoxIndex);

	/** Throws the box's part at BoxIndex away. */
	bool DiscardPart(int32 BoxIndex);

	/** Puts a part in the box (the console, tests). False when the box is full or the part names no kind of gun. */
	bool AddToPartsBox(const FBoxedWeaponPart& Part);

	/** Empties the parts box. */
	void ClearPartsBox();

	// --- The vendors: selling from the backpack and the junk mark (WeaponManagerVendor.cpp) ---

	/**
	 * Takes the backpack's gun at BackpackIndex out for good (sold to a vendor) into OutGun; the guns after it move up one.
	 * False, nothing changed, when there's none there. Equipped guns are never sold, so there's no slot version.
	 */
	bool TakeFromBackpack(int32 BackpackIndex, FWeaponInstanceData& OutGun);

	/**
	 * Marks the backpack's gun at BackpackIndex junk, or not (FWeaponInstanceData::bJunk), for a vendor's "Sell all junk".
	 * False when there's none there or it's a named gun (never sold, so never junk).
	 */
	bool SetBackpackJunk(int32 BackpackIndex, bool bJunk);

	/** Carried ammo of a class changed (pickups, reloads). */
	UPROPERTY(BlueprintAssignable, Category = "Weapons|Ammo")
	FOnAmmoPoolChanged OnAmmoChanged;

	/** An ammo pickup gave the player Amount rounds (the HUD's pickup feed); Amount is 0 when that class was already full. */
	UPROPERTY(BlueprintAssignable, Category = "Weapons|Ammo")
	FOnAmmoPickedUp OnAmmoPickedUp;

	/** The backpack's places, shared by its guns and the spare wards (HasBackpackRoom). */
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

	/** Every equip slot (GunSlotRules::NumSlots); the player's open with their progress (GetUnlockedSlots). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons", meta = (ClampMin = "1"))
	int32 MaxWeapons = GunSlotRules::NumSlots;

	// --- Slots opening with the player's progress (WeaponManagerSlotGate.cpp; GunSlotRules, the user's rule of 2026-10-31) ---

	/** The slots open now: all MaxWeapons for anyone but the player; the player's by GunSlotRules (2, 3 at level 3, 4 after the story). */
	int32 GetUnlockedSlots() const;
	/** Room in the open slots for one more gun: every "into a free slot" asks this, so a shut slot never takes a gun. */
	bool HasFreeSlot() const;
	/** What keeps slot SlotIndex shut; Open when it's open or holds a gun (an older save's gun keeps its slot until it leaves). */
	EGunSlotLock GetSlotLock(int32 SlotIndex) const;
	/** The level and story the player's slots open by now; unset for anyone else (every slot open). */
	TOptional<FGunSlotProgress> GetSlotProgress() const;
	/** Tests: the slots open by Progress, as a player's would (unset: back to the world's). */
	void SetSlotProgressForTesting(const TOptional<FGunSlotProgress>& Progress) { SlotProgressForTesting = Progress; }
	/** A slot opened for the player by something earned (a level, a mission; never a load): the HUD's banner and feed line. */
	FOnGunSlotUnlocked OnSlotUnlocked;
	/** Announces the slots open in Now but not in Before (each with what had shut it). The progress watch calls it; tests do. */
	void AnnounceSlotsOpened(const FGunSlotProgress& Before, const FGunSlotProgress& Now);

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

	/**
	 * The first-person hold for this gun: AttachOffset, moved so the gun's grip is where AttachGrip would be; or, for a gun
	 * with a hip hold of its own (its definition's FirstPersonGrip and FirstPersonRotation: the Drover), its grip there,
	 * turned its way.
	 */
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

	/**
	 * Takes a weapon out of the equip slots (fixing up the active slot) without destroying it. bHolster false: the one in
	 * hand isn't put away first (a gun already being destroyed).
	 */
	AWeaponBase* RemoveFromSlots(int32 SlotIndex, bool bHolster = true);
	void SendMessage(const FString& Message);

	/** Destroys a gun it carries or carried: only these go quietly (NoteGunDestroyed). */
	void ReleaseGun(AWeaponBase* Gun);

	/** After a change the player asked for (a drop, a sale, a load, a clear): when it left them no gun, a log line says how. */
	void NoteGunsGone(int32 CarriedBefore, const TCHAR* Path) const;

	/** Guns lost on a path that should never lose one: a warning naming the path and why, and the C++ callstack. */
	void WarnGunsLost(int32 Lost, const TCHAR* Path, const FString& Why) const;

	/** As it ends play: guns carried that go with it anywhere but the end of the level or of play are a loss, said aloud. */
	void ReportEndPlay(EEndPlayReason::Type EndPlayReason) const;

	// --- The slots' gate (WeaponManagerSlotGate.cpp) ---
	/** A slot's key: its gun in hand; a shut slot says how it opens. */
	void HandleSlotKey(int32 SlotIndex);
	/** Follows the player's level and the level's missions, to announce a slot as it opens (a local player's only). */
	void WatchSlotProgress(AController* Controller);
	void StopWatchingSlotProgress();
	void HandleSlotXPChanged(int64 Gained, EXPSource Source);
	void HandleSlotMissionFinished(const UMissionDefinition& Mission, bool bRewarded);
	TOptional<FGunSlotProgress> SlotProgressForTesting;
	TWeakObjectPtr<UPlayerProgressionSubsystem> SlotProgression;
	TWeakObjectPtr<UMissionRunner> SlotRunner;
	FDelegateHandle SlotXPHandle;
	FDelegateHandle SlotMissionHandle;
	/** The level the slots last saw (each change is checked against it). */
	int32 SlotLevelSeen = 1;
	/** A load is putting the saved guns back: every slot takes one (a shut slot keeps an older save's gun). */
	bool bRestoringSlots = false;

	/** Empties everything (ClearInventory, a load's RestoreInventory) without a word. */
	void EmptyInventory();

	/** How many guns are in the equip slots. */
	int32 NumEquipped() const;

	// --- The backpack's guns in and out, keeping their places in BackpackOrder (WeaponManagerBackpack.cpp) ---
	void BackpackAdd(const FWeaponInstanceData& Instance);
	/** Returns where its entry stood in the order, for BackpackInsert to put another back there. */
	int32 BackpackRemoveAt(int32 Index);
	void BackpackInsert(const FWeaponInstanceData& Instance, int32 Index, int32 Position = INDEX_NONE);
	/** The order brought into step with the guns and wards carried before a change counts on it. */
	void SyncBackpackOrder();

	/**
	 * Puts a gun made from Instance into an equip slot in place of the one there (WeaponManagerBench.cpp): taken in hand if
	 * that one was in hand. False (nothing changed) when the gun can't be made.
	 */
	bool ReplaceEquipped(int32 SlotIndex, const FWeaponInstanceData& Instance);

	// --- Swaps (WeaponManagerSwap.cpp) ---

	/** Whether a change of gun plays out as a swap: a local player's, in play, a gun in hand and in view, no menu open. */
	bool ShouldAnimateSwap() const;
	/** Sends the gun in hand down, locked, to bring up Target (a slot's gun) or Pickup (loot); one going down only retargets. */
	void BeginSwap(AWeaponBase* Target, AWeaponBase* Pickup);
	/** The gun in hand is down: the change, out of sight; or, what it was for gone, back up. */
	void FinishHolster();
	/** Called off: the gun in hand comes back up from wherever it got to. */
	void CallOffSwap();
	/** Back up after a swap called off: free to fire again, and a reload the swap cut short starts over. */
	void FinishRaise();
	/** Forgets a swap under way (the caller puts the gun in hand away or takes another, which lets go of its lock). */
	void StopSwap();
	/** A swap's sound, on the body: the gun going away may be gone before its sound is. */
	void PlaySwapCue(FName Cue) const;
	/** The slot the wheel counts on from: the one a swap is bringing up, else the one in hand. */
	int32 GetHeadingSlot() const;
	/** Loot taken in hand at once: a free slot, else in place of the gun in hand (EquipPickup's change, without a swap). */
	void PutLootInHand(AWeaponBase* Pickup);

	/** The swap under way: the holster's clock, what it brings up, and whether it locked the gun in hand itself. */
	FWeaponSwapClock SwapClock;
	TWeakObjectPtr<AWeaponBase> SwapTarget;
	TWeakObjectPtr<AWeaponBase> SwapPickup;
	bool bSwapLocked = false;
	FTimerHandle SwapTimer;

	UPROPERTY(Transient)
	TArray<FWeaponInstanceData> Backpack;

	/** How the backpack's guns and the spare wards interleave (GetBackpackItems). */
	FBackpackOrder BackpackOrder;

	/** The gunsmith's bench's parts box (GetPartsBox). */
	UPROPERTY(Transient)
	TArray<FBoxedWeaponPart> PartsBox;

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

	/** While it destroys a gun itself (ReleaseGun): that one's end isn't a loss. */
	bool bReleasingGun = false;

	/** A local player has held it: its guns are the player's, whose pawn should never be destroyed mid-play. */
	bool bHeldByPlayer = false;
};
