#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/GunKindsTuning.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/VfxTypes.h"
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
class UHudScopeWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAmmoChanged, int32, CurrentMagazine, int32, ReserveAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReload, float, ReloadDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponReloadFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponHit, const FHitResult&, Hit, float, Damage, bool, bCritical);

/**
 * A single rolled weapon. Fires real bullets (UBulletSubsystem) and supports semi/full-auto/burst and multi-pellet shots.
 * Aims from the owning pawn's view point, so it works for both players and AI. Each shot flashes at the muzzle and
 * kicks (the holder's view and animation read the recoil profile), and every shot, reload step, draw and dry click
 * plays its sound cue (LooterSound) on the gun. Its brass flies (a rifle's case each shot, a shotgun's hull as it's
 * pumped, a revolver's empties on a reload: FWeaponBrass), and a barrel made hot by a burst or a heavy shot smokes a
 * moment once firing stops.
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

	/**
	 * The trigger goes down. A gun that's ready fires; one still cycling remembers the press only if it's about to be ready
	 * (WeaponTrigger::BufferSeconds) and only once, so spammed clicks never queue shots that fire after the clicking stops.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartFire();

	/** StartFire as of the world time Now (a test steps through a gun's cooldown with it, as the level never ticks). */
	void StartFireAt(double Now);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StopFire();

	/** A shot is on its way after the gun's cooldown: a remembered press, auto fire under a held trigger, or a burst's next round. */
	bool HasShotPending() const;

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

	/** How much bigger than modeled its kind is drawn (UWeaponModelComponent::ScaleFor); 1 for a gun not built from parts. */
	float GetModelScale() const;
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

	/** Shows/hides the floating label. Focused, it opens into the gun card over the gun (UWeaponLabelWidget, UGunCardWidget). */
	void SetLabelState(bool bVisible, bool bFocused);

	/** Its floating label (ULootLabelSubsystem keeps it clear of the other loot labels on screen). */
	UWidgetComponent* GetLabelComponent() const { return Label; }

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

	/**
	 * 0..1 through the current reload's choreography (LooterReload::ChoreographyProgress: a tactical magazine reload jumps
	 * its handle), or -1 when not reloading. It runs on a beat past the moment the gun is ready again
	 * (Looter.Gunfeel.Reload.ReadyEarly), so the motion ends smoothly, until the gun fires or is put away.
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetReloadProgress() const;

	/** The current reload as planned (from empty or not; a pump gun's shells, re-planned when a trigger press cuts it short). */
	const FReloadPlan& GetReloadPlan() const { return ReloadPlan; }

	/** The current reload (or its last beat) is from empty: slower, the handle, the bolt or the pump worked too. False when not reloading. */
	bool IsReloadEmpty() const { return (bReloading || bReloadTail) && ReloadPlan.bEmpty; }

	/** The current reload's steps for its plan (LooterReload::StepsFor), in its choreography progress. */
	TConstArrayView<FReloadStepAt> GetReloadSteps() const { return ReloadSteps; }

	/**
	 * A trigger press mid-reload (StartFireAt): a pump gun with Looter.Gunfeel.Reload.Shotgun.Interrupt finishes the shell
	 * in hand (and the pump if it was empty), then fires. True if a shot will follow; any other reload ignores the press.
	 */
	bool InterruptReload();

	/** How far through its rack between shots a pump gun's fore-end is (0 to 1), or -1 while it's still. WeaponBasePump.cpp. */
	float GetPumpCycle() const;

	/** How strongly reload animation should show right now: eases in at the start of a reload and out at the end. */
	float GetReloadBlend() const;

	/** What a reload visibly works on (magazine or pump), for animating it. None for guns that aren't built from parts. */
	EWeaponReloadPart GetReloadPart() const;

	/** True while the trigger is held (even between shots or during a reload). */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool WantsToFire() const { return bWantsToFire; }

	/**
	 * Spread half-angle (degrees) right now, including the holder's stance (crouching tightens it) and a rifle's bloom: each
	 * shot of a burst opens the cone wider (GunfeelKick::BloomAfterShot) and it closes again once firing stops.
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetEffectiveSpread() const;

	/** How this gun kicks when it fires: its definition's profile, scaled by its Recoil stat. */
	FWeaponRecoilProfile GetRecoilProfile() const;

	/**
	 * 0 when it was just drawn, 1 once it's up and can fire: a swap takes BaseReadySeconds divided by the gun's Handling
	 * (the first-person view raises the gun over that time).
	 */
	float GetReadyAlpha() const;

	/**
	 * How fast the gun's owner lives against the world (its CustomTimeDilation; 1 normally): the fire and reload timers run
	 * on the world's clock, so under Sundown (the world at a quarter speed, the player at full) they divide by this, or the
	 * gun cycled and reloaded at the world's crawl while the player moved at full speed.
	 */
	float GetOwnerTimeScale() const;

	/** Seconds to bring a gun of Handling 1 up after drawing it. */
	static constexpr float BaseReadySeconds = 0.35f;

	/** Where the eye lines up when aiming down the sights, in the weapon's own space. */
	FVector GetAimPoint() const;

	/** The muzzle where the player sees it: first-person guns are drawn with their own field of view and scale. */
	FVector GetVisibleMuzzleLocation() const;

	/** A point on the gun (world) where the player sees it: moved as a first-person gun is drawn; as it is in third person. */
	FVector GetVisibleLocation(const FVector& WorldLocation) const;

	/**
	 * Where its spent cases leave (the ejection port, as the player sees it: the visible gun's in first person) and which
	 * way they're flung (out to the gun's right, up and a little back). False for a gun that throws none from a port (a
	 * revolver keeps its empties until a reload) or isn't built from parts. WeaponBaseBrass.cpp.
	 */
	bool GetEjectPort(FVector& OutLocation, FVector& OutDirection) const;

	/** How long after a shot a pump gun's hull flies out: at its rack's back point (ReloadTuning::RackTiming). WeaponBasePump.cpp. */
	float GetHullDelay() const;

	/** Rounds fired since the cylinder was last emptied: the empties a revolver's reload tips out. */
	int32 GetSpentInCylinder() const { return SpentInCylinder; }

	/** How hot the barrel is now (0 cold; each shot heats it, it cools by itself): over WispHeat it smokes once firing stops. */
	float GetMuzzleHeat() const;

	/** Whether a wisp of smoke is curling from the muzzle now. */
	bool IsMuzzleSmoking() const { return SmokeTimeLeft > 0.f; }

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

	// --- The frontier-tech kinds' own ways (Phase 12.3; WeaponBaseKinds.cpp, numbers in GunKindsTuning) ---

	/** How far through its cycle between shots the sniper's bolt is (0 to 1), or -1 while it's home. */
	float GetBoltCycle() const;

	/** How open the sniper's bolt is now (0 home, 1 drawn back), by its cycle or its reload; 0 for any other gun. */
	float GetBoltOpen() const;

	/** How much of the trigger hand is on the bolt's knob now (0 on the grip, 1 on the knob); 0 for any other gun. */
	float GetBoltHand() const;

	/** The LMG's spin now (0 cold, 1 at its full rate). */
	float GetSpin() const;

	/** The LMG barrel's heat now (0 cold, 1 glowing its hottest). */
	float GetBarrelHeat() const;

	/** The sniper's shooter is holding their breath through the scope (the sprint key while aimed), and how much breath is left (0-1). */
	bool IsHoldingBreath() const { return Breath.bHolding; }
	float GetBreathLeft() const;

	/** The sniper's scope is up in first person: the scope picture shows (UHudScopeWidget) and the gun and hands are hidden. */
	bool IsScopedIn() const { return bScopeHidden; }

	/** Seconds between this gun's shots right now: its fire rate's, slower while an LMG is still spinning up. */
	float GetShotInterval() const;

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
	 *  turning after a shot, the muzzle flash, a pump racked after a shot, or the smoke curling from a hot muzzle. */
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

	/** Built-in muzzle flash (M_FX_Muzzle): a white-hot core, a soft bloom, a star of thin spikes and short tongues, about two frames a shot. */
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
	/** The gun is ready again (Looter.Gunfeel.Reload.ReadyEarly before its motion ends): rounds in if not yet, fire if asked. */
	void FinishReload();
	/** The reload's motion has run its last beat. */
	void EndReloadTail();
	void CancelReload();
	/** The reload's sounds (its plan's steps) whose moments it has passed since it last looked, and what they do. */
	void PlayReloadSteps(float Progress);
	/** The magazine (helix, box, rocket, speedloader) seated: its rounds in from the pool, once. */
	void CommitReloadRounds();
	/** A shell seated: one round in from the pool. */
	void SeatShell();
	/** The reload's motion share so far (0 to 1: time over its motion), before the choreography's jumps; -1 when there's none. */
	float GetReloadRawProgress() const;
	/** The motion and ready times (world seconds) for ReloadPlan, from the gun's stats and the live numbers. */
	void PlanReloadTimes();
	/** A Looter.Gunfeel.Reload.Debug line. */
	void LogReload(const TCHAR* What) const;
	/** Moves the magazine or pump to where the reload has got to (back in place when not reloading). */
	void UpdateReloadPart();
	/** How much wider than its own the rifle's cone is open right now (0 closed; 1 is twice the spread), after recovering since the last shot. */
	float GetSpreadBloom() const;
	/** A shot opens the cone (rifle only; a quarter as much through the sight). */
	void AddShotBloom();
	/** Places the muzzle flash on the current model's muzzle and sets up its quads. */
	void SetupMuzzleFlash();
	void PlayMuzzleFlash();
	void UpdateMuzzleFlash(float DeltaSeconds);
	// WeaponBaseBrass.cpp: spent cases.
	/** A shot's brass: a rifle's case flicked out now, a shotgun's hull once it's pumped (GetHullDelay), none from a revolver. */
	void EjectAfterShot();
	/** One case out of the ejection port. */
	void EjectCasing();
	/** The revolver's rod punched: its spent empties tumble out of the chambers' backs. */
	void DumpEmpties();
	/** Moves a pump gun's rack on (WeaponBasePump.cpp). */
	void UpdateBrass(float DeltaSeconds);
	// WeaponBasePump.cpp: the pump racked between shots.
	/** A shot from a pump gun: its rack starts (the hull flies as it bottoms). */
	void StartPumpCycle();
	/** A rack under way stops (a reload takes the gun); bThrowHull throws a hull still in the chamber. */
	void EndPumpCycle(bool bThrowHull);
	// WeaponBaseSmoke.cpp: the hot muzzle's smoke.
	/** A shot heats the barrel; once firing stops a hot one smokes. */
	void NoteShotHeat();
	void UpdateMuzzleSmoke(float DeltaSeconds);
	void StopMuzzleSmoke();
	// WeaponBaseKinds.cpp: the frontier-tech kinds' own ways.
	/** A shot (or a dud) went: the bolt's cycle starts, the LMG heats and spins on, the launcher's mouth is empty. */
	void NoteKindShot(bool bMisfire);
	/** The trigger went down: an LMG from cold spins up (heard). */
	void StartKindTrigger();
	/** Off the trigger (released, reloading, put away): the LMG spins down from here. */
	void StopKindTrigger();
	/** Each tick while a kind has something going: the bolt's cycle, the scope's sway, the glows. */
	void UpdateKinds(float DeltaSeconds);
	bool WantsKindTick() const;
	/** The SMG helix's and the LMG barrel's glows, as the magazine and the heat have them now. */
	void RefreshGauges();
	void UpdateBoltCycle();
	void UpdateScopeSway(float DeltaSeconds);
	/** The scope picture and the hidden gun as the sights come up (the sniper's, in a local player's first-person hands). */
	void UpdateScope();
	/** Puts the scope down: the picture gone, the gun shown again. */
	void HideScope();
	/** Gives back the aim the scope's sway still holds (put away, dropped). */
	void ClearScopeSway();
	/** The launcher's shot: a rocket from the eye along the aim, seen leaving the muzzle (AWeaponRocket). */
	void FireRocket(const FVector& ViewLocation, const FVector& Direction);
	/** The first-person gun turned by the bolt's work (about its grip, like a strike's pose; zero puts it back). */
	void ApplyCyclePose(const FVector& Offset, const FRotator& Rotation);
	/** The holder's sprint key is down (a player's). */
	bool IsSprintHeld() const;
	/** The holder's sights (0 at the hip, 1 aimed); 0 for a holder without a view. */
	float GetHolderAimAlpha() const;
	/** The LMG's spin as of Now. */
	float GetSpinAt(double Now) const;

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

	/** The rifle's bloom as of BloomTime (it closes by itself; GetSpreadBloom). */
	float SpreadBloom = 0.f;
	double BloomTime = -1000.0;

	/** Seconds the muzzle flash still shows, and how strong this shot's flash is. */
	float FlashTimeLeft = 0.f;
	float FlashStrength = 1.f;
	/** The flash's quads at scale 1 (VfxRules::MuzzleQuads), one instance each, in order; placed again each shot for its view. */
	TArray<FVfxFlashQuad> FlashQuads;
	/** The gun's flash scale (its definition's MuzzleFlashScale x its model's scale). */
	float FlashGunScale = 1.f;
	/** This kind's flash length (VfxRules::MuzzleLook; WeaponBaseMuzzle.cpp) and this shot's (times Looter.Vfx.Muzzle.Time). */
	float FlashBaseDuration = 0.04f;
	float FlashDuration = 0.04f;
	/** The frame the flash was fired on: it isn't aged that frame, so even a slow frame shows it once at full. */
	uint64 FlashFrame = 0;
	/** Shots in the burst so far (0 the first; a pause of MuzzleBurstGap starts over): a fast gun's smoke and sparks thin out. */
	int32 MuzzleShotsInBurst = 0;
	double LastMuzzleTime = -1000.0;

	/** Seconds left of a pump gun's rack after a shot (negative: none; it ticks while there are): its hull flies at the back point. */
	float PendingEjectSeconds = -1.f;
	/** That rack's start after the shot and its length (seconds), whether its back point has passed, and a spent hull still in the chamber. */
	float PumpRackStart = 0.f;
	float PumpRackSeconds = 0.f;
	bool bRackBeatDone = false;
	bool bHullInChamber = false;
	/** Rounds fired since a revolver's cylinder was last emptied (its next reload's empties). */
	int32 SpentInCylinder = 0;

	/** The barrel's heat as of HeatTime (it cools by itself). */
	float MuzzleHeat = 0.f;
	double HeatTime = -1000.0;
	/** Seconds after the last shot until the gun looks whether it's hot enough to smoke (negative: not waiting). */
	float WispCheckSeconds = -1.f;
	/** The wisp now: seconds left of it, how long it lasts, how thick, and its puffs' clock. */
	float SmokeTimeLeft = 0.f;
	float SmokeLength = 0.f;
	float SmokeStrength = 0.f;
	float SmokePuffIn = 0.f;

	/** The curses' rolls (Unlucky's misfires): a stream of its own, seeded afresh for each gun. */
	FRandomStream ShotRolls;

	/** How far through the current reload (its choreography) its sounds have played. */
	float ReloadSoundProgress = 0.f;

	/** The reload under way: its plan and steps, its motion and ready times (world seconds), the clock it ran before its timer was last set. */
	FReloadPlan ReloadPlan;
	TArray<FReloadStepAt> ReloadSteps;
	float ReloadMotionSeconds = 0.f;
	float ReloadReadySeconds = 0.f;
	float ReloadClockOffset = 0.f;
	/** Its rounds are in (CommitReloadRounds or a shell), a shot follows it (InterruptReload), its motion runs on past ready. */
	bool bReloadCommitted = false;
	bool bFireAfterReload = false;
	bool bReloadTail = false;
	/** When the gun was ready (world seconds): a shot after that ends the tail. */
	double ReloadReadyTime = -1000.0;

	/** The sniper's bolt: when the shot it cycles after went, its timing, whether it's still cycling, heard, and the case out. */
	double BoltShotTime = -1000.0;
	GunKinds::FBoltTiming BoltTiming;
	bool bBoltCycling = false;
	bool bBoltHeard = false;
	bool bBoltCaseOut = true;
	/** The first-person gun is off its hold by the bolt's work (ApplyCyclePose), to put back. */
	bool bCyclePoseApplied = false;

	/** The LMG's spin as of SpinTime, and whether it was firing then (spinning up) or not (down). */
	float Spin = 0.f;
	double SpinTime = -1000.0;
	bool bSpinFiring = false;

	/** The LMG barrel's heat as of its last shot. */
	float BarrelHeat = 0.f;
	double BarrelHeatTime = -1000.0;

	/** The scope's sway: seconds aimed, the turn it has put on the aim now, its scale (breath) eased, and the breath. */
	float SwayTime = 0.f;
	FRotator SwayApplied = FRotator::ZeroRotator;
	float SwayScale = 1.f;
	GunKinds::FScopeBreath Breath;
	/** The scope's picture over the view, and whether the gun is hidden behind it. */
	TWeakObjectPtr<UHudScopeWidget> ScopeWidget;
	bool bScopeHidden = false;

	/** The loot beam's steady numbers (RefreshLootBeam), which a cursed gun's beam gutters about. */
	float BeamGlow = 0.f;
	float BeamHeight = 0.f;
	float BeamRadius = 0.f;

	FTimerHandle FireTimer;
	FTimerHandle ReloadTimer;
};
