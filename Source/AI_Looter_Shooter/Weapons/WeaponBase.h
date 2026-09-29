#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponRecoil.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponBase.generated.h"

class UDynamicMeshComponent;
class UInstancedStaticMeshComponent;
class USphereComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UPointLightComponent;
class ULootTossComponent;
class URotatingMovementComponent;
class UWidgetComponent;
class UWeaponManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAmmoChanged, int32, CurrentMagazine, int32, ReserveAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReload, float, ReloadDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponReloadFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponHit, const FHitResult&, Hit, float, Damage, bool, bCritical);

/**
 * A single rolled weapon. Fires real bullets (UBulletSubsystem) and supports semi/full-auto/burst and multi-pellet shots.
 * Aims from the owning pawn's view point, so it works for both players and AI. Each shot flashes at the muzzle and
 * kicks (the holder's view and animation read the recoil profile).
 * When unowned it acts as a loot pickup: it spins, glows in its rarity color, and shows a label.
 */
UCLASS(Blueprintable)
class AI_LOOTER_SHOOTER_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	/** Applies a rolled instance. Call before BeginPlay (SpawnWeapon does this for you). */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void InitializeFromInstance(const FWeaponInstanceData& InInstance);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();

	/** Called by the weapon manager when the weapon goes in or out of the player's hands. */
	void OnEquipped(APawn* NewOwner, USceneComponent* AttachTo, FName Socket, const FTransform& AttachOffset);

	/** Moves the weapon to a new holder (camera for first person, the hand for third person) without touching its state. */
	void AttachToHolder(USceneComponent* AttachTo, FName Socket, const FTransform& AttachOffset);

	/** Where the right hand holds the gun, in the weapon's own space. */
	FVector GetGripPoint() const { return ModelGrip; }

	/** Where the left hand holds the gun, in the weapon's own space. */
	FVector GetForegripPoint() const { return ModelForegrip; }
	void OnHolstered();
	void OnDropped();

	/** Puts the weapon in the world as loot and throws it with the given velocity. */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Loot")
	void Toss(const FVector& Velocity);

	/** True while lying in the world waiting to be picked up. */
	UFUNCTION(BlueprintPure, Category = "Weapon|Loot")
	bool IsPickup() const { return bIsPickup; }

	/** Shows/hides the floating label. Focused adds stats and the pickup prompt. */
	void SetLabelState(bool bVisible, bool bFocused);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	const FWeaponInstanceData& GetInstance() const { return Instance; }

	/** Instance data including current ammo, for moving the weapon into the backpack. */
	FWeaponInstanceData GetInstanceForStorage() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	const FWeaponStats& GetStats() const { return Instance.Stats; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	EWeaponRarity GetRarity() const { return Instance.Rarity; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FText GetDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentMagazine() const { return CurrentMagazine; }

	/** Rounds available to reload with: the holder's shared pool for this weapon's ammo class (0 for loot on the ground). */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetReserveAmmo() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	EAmmoType GetAmmoType() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsReloading() const { return bReloading; }

	/** 0..1 through the current reload, or -1 when not reloading. */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetReloadProgress() const;

	/** How strongly reload animation should show right now: eases in at the start of a reload and out at the end. */
	float GetReloadBlend() const;

	/** What a reload visibly works on (magazine or pump), for animating it. None for guns without a code-built model. */
	EWeaponReloadPart GetReloadPart() const { return bUsingModel ? ReloadPart : EWeaponReloadPart::None; }

	/** True while the trigger is held (even between shots or during a reload). */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool WantsToFire() const { return bWantsToFire; }

	/** Spread half-angle (degrees) right now, including the holder's stance (crouching tightens it). */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetEffectiveSpread() const;

	/** How this gun kicks when it fires (from its definition). */
	const FWeaponRecoilProfile& GetRecoilProfile() const;

	/** The muzzle where the player sees it: first-person guns are drawn with their own field of view and scale. */
	FVector GetVisibleMuzzleLocation() const;

	/** A bullet from this weapon hit something (the bullet system calls this when it lands). Broadcasts OnHit. */
	void NotifyBulletHit(const FHitResult& Hit, float Damage, bool bCritical);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanReload() const;

	/** Magazine changed (shots, reloads, equip). Reserve lives in the holder's pool: see UWeaponManagerComponent::OnAmmoChanged. */
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponAmmoChanged OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponFired OnFired;

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponReload OnReloadStarted;

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponReloadFinished OnReloadFinished;

	/** Fires once per bullet that hits something, when it lands. Use for hit markers. */
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponHit OnHit;

	/** "Weapon" trace channel (DefaultEngine.ini): pawn capsules ignore it so shots reach the body parts that hold critical spots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel2;

	/** Draw every bullet's path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Debug")
	bool bDrawDebugTraces = false;

	/** Only ticks while it has something to animate: the magazine or pump during a reload, or the muzzle flash. */
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Root. Collides with the world only while the weapon is loot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	/** Code-built model, used when the definition asks for a procedural model. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDynamicMeshComponent> ModelMesh;

	/** The code-built model's moving part (magazine or pump), which reloads animate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDynamicMeshComponent> ModelPartMesh;

	/** Built-in muzzle flash: a hot core, a spiky star and three flame tongues at the muzzle, shown for a moment after each shot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> MuzzleFlash;

	/** The muzzle flash's light on its surroundings. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> MuzzleLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> RarityLight;

	/** Rarity-colored light pillar shown while the weapon lies in the world as loot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LootBeam;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Label;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULootTossComponent> TossMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URotatingMovementComponent> SpinMovement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ExposeOnSpawn = "true"))
	FWeaponInstanceData Instance;

private:
	void ApplyDefinitionVisuals();
	UPrimitiveComponent* GetActiveMesh() const;
	FVector GetMuzzleLocation() const;
	void GetAimViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	void HandleFiring();
	void FireShot();
	void FinishReload();
	void CancelReload();
	/** Moves the magazine or pump to where the reload has got to (back in place when not reloading). */
	void UpdateReloadPart();
	/** Places the muzzle flash on the current model's muzzle and sets up its quads. */
	void SetupMuzzleFlash();
	void PlayMuzzleFlash();
	void UpdateMuzzleFlash(float DeltaSeconds);
	/** Ticks only while there's something to animate. */
	void RefreshTick();
	void BroadcastAmmo();
	void SetPickupState(bool bPickup);
	void RefreshLootBeam();

	UFUNCTION()
	void HandleTossStopped(const FHitResult& ImpactResult);

	/** The inventory of whoever holds this weapon (where its reserve ammo lives), if anyone. */
	UWeaponManagerComponent* GetHolderInventory() const;

	int32 CurrentMagazine = 0;
	int32 BurstShotsRemaining = 0;
	bool bWantsToFire = false;
	bool bReloading = false;
	bool bAmmoInitialized = false;
	bool bIsPickup = false;
	bool bUsingModel = false;

	/** Key points of the procedural model, in ModelMesh space (which is weapon space while held). */
	FVector ModelMuzzle = FVector::ZeroVector;
	FVector ModelGrip = FVector::ZeroVector;
	FVector ModelForegrip = FVector(30.f, 0.f, -3.f);
	EWeaponReloadPart ReloadPart = EWeaponReloadPart::None;
	/** Direction the part slides out (magazine) or back (pump), in ModelMesh space. */
	FVector ReloadPartAxis = FVector::ZeroVector;
	double LastFireTime = -1000.0;

	/** Seconds the muzzle flash still shows, and how strong this shot's flash is. */
	float FlashTimeLeft = 0.f;
	float FlashStrength = 1.f;
	/** Full glow of each muzzle flash quad, which fades with the flash. */
	TArray<float, TInlineAllocator<8>> FlashGlow;

	FTimerHandle FireTimer;
	FTimerHandle ReloadTimer;
};
