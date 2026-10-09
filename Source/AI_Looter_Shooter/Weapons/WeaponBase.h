#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponRecoil.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponBase.generated.h"

class UInstancedStaticMeshComponent;
class USphereComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UPointLightComponent;
class ULootTossComponent;
class URotatingMovementComponent;
class USoundBase;
class UWeaponModelComponent;
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
 * kicks (the holder's view and animation read the recoil profile), and every shot, reload step, draw and dry click
 * plays its sound cue (LooterSound) on the gun.
 * When unowned it acts as a loot pickup: it spins, glows in its rarity color, and shows a label.
 *
 * It counts its kills as notches (WeaponBaseNotches.cpp; WeaponNotches has the rules), and a cursed iron's drawbacks act
 * where they bite: misfires and rounds per shot as it fires, the health a reload costs, the holder's max health while
 * it's in hand (WeaponCurseEffects). Its loot beam gutters like a dying flame while it's cursed.
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
	FVector GetGripPoint() const;

	/** Where the left hand holds the gun, in the weapon's own space. */
	FVector GetForegripPoint() const;
	void OnHolstered();
	void OnDropped();

	/** Puts the weapon in the world as loot and throws it with the given velocity. */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Loot")
	void Toss(const FVector& Velocity);

	/** True while lying in the world waiting to be picked up. */
	UFUNCTION(BlueprintPure, Category = "Weapon|Loot")
	bool IsPickup() const { return bIsPickup; }

	/** The rarity-colored pillar over it while it lies as loot (a landing drop's beam flares: ULootFanfareSubsystem). */
	UStaticMeshComponent* GetLootBeam() const { return LootBeam; }

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

	/** What a reload visibly works on (magazine or pump), for animating it. None for guns that aren't built from parts. */
	EWeaponReloadPart GetReloadPart() const;

	/** True while the trigger is held (even between shots or during a reload). */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool WantsToFire() const { return bWantsToFire; }

	/** Spread half-angle (degrees) right now, including the holder's stance (crouching tightens it). */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetEffectiveSpread() const;

	/** How this gun kicks when it fires: its definition's profile, scaled by its Recoil stat. */
	FWeaponRecoilProfile GetRecoilProfile() const;

	/**
	 * 0 when it was just drawn, 1 once it's up and can fire: a swap takes BaseReadySeconds divided by the gun's Handling
	 * (the first-person view raises the gun over that time).
	 */
	float GetReadyAlpha() const;

	/** Seconds to bring a gun of Handling 1 up after drawing it. */
	static constexpr float BaseReadySeconds = 0.35f;

	/** Where the eye lines up when aiming down the sights, in the weapon's own space. */
	FVector GetAimPoint() const;

	/** The muzzle where the player sees it: first-person guns are drawn with their own field of view and scale. */
	FVector GetVisibleMuzzleLocation() const;

	/** A bullet from this weapon hit something (the bullet system calls this when it lands). Broadcasts OnHit. */
	void NotifyBulletHit(const FHitResult& Hit, float Damage, bool bCritical);

	/**
	 * A creature this gun killed (UPlayerProgressionSubsystem::CreditKillWeapon): one more notch. At a milestone its
	 * stats are rebuilt (its damage), the holder's HUD says so and a chime plays; at 100 notches a curse lifts the same way.
	 * The tally cut in its stock follows.
	 */
	void AddKill();

	/**
	 * The gun behind Victim's latest damage (UHealthComponent::GetLastDamageCauser): the gun itself, or the gun in the hand
	 * of whoever dealt it (or owns what did). Null when no gun was behind it.
	 */
	static AWeaponBase* FindKillWeapon(const AActor* Victim);

	/** The sights come up: its holder's view calls this as aiming starts, for the aim-in sound. */
	void PlayAimIn() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanReload() const;

	// --- A melee strike with the gun in hand (UPlayerMeleeComponent; WeaponBaseMelee.cpp) ---

	/**
	 * The stock swings: no shots and no reload until EndMeleeSwing (a trigger held or a reload asked for meanwhile waits
	 * for it). A reload under way is cut short and starts over once the swing is done, Borderlands' way. True if it cut
	 * a reload short.
	 */
	bool BeginMeleeSwing();

	/**
	 * The swing is over: the gun back in its hold and, with bResume, the reload it cut short or one asked for meanwhile,
	 * else auto fire if the trigger is still held. Without bResume (the gun left the hand mid-swing) nothing follows.
	 */
	void EndMeleeSwing(bool bResume = true);

	bool IsMeleeSwinging() const { return bMeleeSwinging; }

	/**
	 * Where the swing has the gun the player sees: Offset (cm) and Rotation in the gun's own frame, turned about the middle
	 * of its hands. Only a gun drawn in first person moves; in third person it stays in the body's hands. Zero puts it back.
	 */
	void SetMeleePose(const FVector& Offset, const FRotator& Rotation);

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

	/** Only ticks while it has something to animate: the magazine, pump or cylinder during a reload, a revolver's cylinder
	 *  turning after a shot, or the muzzle flash. */
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

	/** The gun assembled from its definition's parts, used when the definition has parts. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWeaponModelComponent> Model;

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
	USceneComponent* GetActiveMesh() const;
	/** Whether the gun is drawn like first-person arms (own field of view and scale). */
	bool IsDrawnFirstPerson() const;
	FVector GetMuzzleLocation() const;
	void GetAimViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	void HandleFiring();
	void FireShot();
	/** A cursed iron's dud (Unlucky): the hammer falls, a dull pop and a wisp of smoke at the muzzle, no bullet. */
	void Misfire();
	/** A sound cue on the gun, following it in hand; the definition's own sound in its place when it sets one. */
	void PlayCue(FName Cue, USoundBase* Override = nullptr) const;
	void FinishReload();
	void CancelReload();
	/** The reload's sounds (LooterReload::Steps) whose moments it has passed since it last looked. */
	void PlayReloadSteps(float Progress);
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
	/** Lying as loot with a curse on it: its beam gutters (LightBeams::Gutter), so it ticks. */
	bool IsBeamGuttering() const;
	void UpdateBeamGutter();

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
	/** A melee strike's swing is on (BeginMeleeSwing), and whether a reload waits for its end. */
	bool bMeleeSwinging = false;
	bool bReloadAfterMelee = false;
	/** The visible gun is off its hold by a swing's pose (SetMeleePose), to put back. */
	bool bMeleePoseApplied = false;

	double LastFireTime = -1000.0;
	/** When it was last drawn, and how long it takes to come up (see GetReadyAlpha). */
	double DrawnTime = -1000.0;
	float ReadySeconds = 0.f;

	/** Seconds the muzzle flash still shows, and how strong this shot's flash is. */
	float FlashTimeLeft = 0.f;
	float FlashStrength = 1.f;
	/** Full glow of each muzzle flash quad, which fades with the flash. */
	TArray<float, TInlineAllocator<8>> FlashGlow;

	/** The curses' rolls (Unlucky's misfires): a stream of its own, seeded afresh for each gun. */
	FRandomStream ShotRolls;

	/** How far through the current reload its sounds have played. */
	float ReloadSoundProgress = 0.f;

	/** The loot beam's steady numbers (RefreshLootBeam), which a cursed gun's beam gutters about. */
	float BeamGlow = 0.f;
	float BeamHeight = 0.f;
	float BeamRadius = 0.f;

	FTimerHandle FireTimer;
	FTimerHandle ReloadTimer;
};
