#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldTypes.h"
#include "PlayerShieldComponent.generated.h"

class AShieldPickup;
class UHealthComponent;
class UPlayerProgressionSubsystem;
class UWeaponManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShieldWardsChanged);

/**
 * The player's ward (the user's pick, 2026-10-22: a recharging shield over health, found as loot; Docs/Work/2026-10-22-Next/
 * SHIELD.md). It wears one ward and carries up to MaxSpares more, and stands in front of the player's health through the
 * health component's shield hook (UHealthComponent::Shield, the creature wards' way): every hit is soaked by its charge
 * first, the hit that empties it breaks it (only part of the overflow gets through), and after RechargeDelay unhurt it
 * fills again at RechargeRate. Health's own regen (FWoundsClose) holds its ramp until the ward is full again
 * (UPlayerVitalsSubsystem). Its traits act here (PlayerShieldTraits.cpp); its wards are carried, swapped, dropped and
 * saved in PlayerShieldWards.cpp. The HUD's ward bar (UHudShieldBarWidget) and the loadout read it.
 * On the player's character (ALooterCharacter makes it; the weapon manager adds one at play's start if it's missing).
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UPlayerShieldComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerShieldComponent();

	static UPlayerShieldComponent* Find(const AActor* Owner);
	/** Find, or a new one added to Owner (registered, and begun if Owner's play has). */
	static UPlayerShieldComponent* FindOrAdd(AActor& Owner);

	/** Spare wards carried besides the one worn. */
	static constexpr int32 MaxSpares = 3;

	// --- The ward worn, as it stands ---

	bool HasWard() const { return Ward.IsSet(); }
	/** The ward worn (Definition null: none). */
	const FShieldInstanceData& GetWard() const { return Ward; }
	/** Its charge now, and the most it holds (live: its stats times Looter.Shield.CapacityScale); 0 without a ward. */
	float GetCharge() const { return Charge.Charge; }
	float GetCapacity() const;
	float GetChargeShare() const;
	/** Empty after a hit broke it, until it starts to fill again. */
	bool IsBroken() const { return HasWard() && bBroken; }
	/** Filling now (its wait met, not full). */
	bool IsRefilling() const { return HasWard() && Charge.bFilling; }
	bool IsFull() const;
	/** Seconds left of the wait before it fills (0 while filling or full). */
	float GetWaitLeft() const;
	/**
	 * Health's regen should hold its ramp now: a ward is worn, it isn't full, and Looter.Shield.HealthWaits is on. The wait
	 * of the wounds that close still counts (UPlayerVitalsSubsystem holds only their ramp).
	 */
	bool HoldsHealthBack() const;

	/** Counts that only go up, so a screen that polls sees every one (as UHealthComponent::GetHurtCount). */
	uint32 GetHitCount() const { return HitCount; }
	uint32 GetBreakCount() const { return BreakCount; }
	uint32 GetFillCount() const { return FillCount; }
	uint32 GetRefillStartCount() const { return RefillStartCount; }
	/** Saint's Grace answered (the HUD flashes the bar warm). */
	uint32 GetGraceCount() const { return GraceCount; }

	// --- The wards carried (PlayerShieldWards.cpp) ---

	const TArray<FShieldInstanceData>& GetSpares() const { return Spares; }
	bool HasSpareRoom() const { return Spares.Num() < MaxSpares; }

	/**
	 * Wears Ward now: the one worn goes to the spares (or onto the ground in front of the player when they're full). It
	 * comes on empty and starts its wait, unless bFull (a load, the starting ward). Returns false for a ward with no definition.
	 */
	bool Equip(const FShieldInstanceData& NewWard, bool bFull = false);
	/** A ward picked up with a tap: worn when none is, else into the spares; false when the spares are full. */
	bool Take(const FShieldInstanceData& NewWard);
	/** The spare at Index is worn, the one worn takes its place among the spares. */
	bool EquipSpare(int32 Index);
	/** The ward worn into the spares (none worn after). False when the spares are full. */
	bool StowWard();
	/** Puts the ward worn, or a spare, on the ground in front of the player. */
	bool DropWard();
	bool DropSpare(int32 Index);

	/** What it carries, into a session's save and back (the ward worn comes back full). */
	void SaveWards(FShieldInventorySave& OutSave) const;
	void RestoreWards(const FShieldInventorySave& Save);
	/** Nothing worn or carried (a load's, before restoring). */
	void EmptyWards();
	bool WasStarterGiven() const { return bStarterGiven; }

	/** The worn ward or the spares changed: the loadout rebuilds. */
	UPROPERTY(BlueprintAssignable, Category = "Shield")
	FOnShieldWardsChanged OnWardsChanged;

	// --- For tests and the dev commands ---

	/** Sets the charge outright (clamped to the capacity), as if it had soaked or filled to there; no events. */
	void SetChargeForTest(float NewCharge);
	/** Moves the ward on DeltaSeconds as its tick would (a test level's components don't tick). */
	void Advance(float DeltaSeconds);
	/** The health component's hook, as a hit hands it to the ward: returns what reaches health. Public for the tests. */
	float SoakHit(float Damage, bool bCritical, const FVector& HitLocation);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Binds the health component's shield hook to this (once; a health component made later is bound by the tick). */
	void BindHealth();
	/** Binds the weapon manager (the starting ward comes with the first gun). */
	void BindWeaponManager();
	UFUNCTION()
	void HandleInventoryChanged();
	/** Gives the first gun's starting ward, once. */
	bool GiveStarter();
	/** Binds the player's progression (Harvest hears kills), once there is a local player. */
	void BindProgression();
	void HandleKillXP(int64 XP, const AActor* Victim);

	// --- The traits (PlayerShieldTraits.cpp) ---
	/** A hit on a charged ward: Barbwire's cut is queued, Lodestone's rounds given. */
	void TraitsOnHit(float ChargeBefore);
	/** The ward broke: Last Breath's burst is queued. */
	void TraitsOnBreak();
	/** Each tick: the queued cuts and burst dealt, Saint's Grace looked at. */
	void TickTraits();
	void LooseNova();
	float Strength() const;

	void PlayCue(const TCHAR* Cue, float Volume = 1.f, float Pitch = 1.f) const;
	void NotifyWardsChanged();
	/** A ward was put on (worn or loaded): the trait's readiness and the break state start over. */
	void ResetWardState(bool bFull);
	FVector DropSpot() const;

	UPROPERTY(Transient)
	FShieldInstanceData Ward;

	UPROPERTY(Transient)
	TArray<FShieldInstanceData> Spares;

	ShieldRules::FShieldCharge Charge;
	bool bBroken = false;
	/** The player was dead last tick: alive again (a respawn) wakes the ward full. */
	bool bWasDead = false;
	/** The share the current refill started from (a top-up's start and end make no sound). */
	float RefillFrom = 1.f;
	bool bStarterGiven = false;
	/** Last Breath and Saint's Grace come ready again once the ward is full. */
	bool bNovaReady = true;
	bool bGraceReady = true;
	/** Last Breath waits for the tick, so a creature's death never happens inside its own attack's damage call. */
	bool bNovaPending = false;
	/** Barbwire's cuts, waiting for the tick for the same reason. */
	TArray<TPair<TWeakObjectPtr<AActor>, float>> PendingBarbs;

	uint32 HitCount = 0;
	uint32 BreakCount = 0;
	uint32 FillCount = 0;
	uint32 RefillStartCount = 0;
	uint32 GraceCount = 0;
	/** World seconds of the last Shield.Hit, so a burst of pellets crackles once. */
	double LastHitSound = -1000.0;

	TWeakObjectPtr<UHealthComponent> Health;
	TWeakObjectPtr<UWeaponManagerComponent> BoundManager;
	TWeakObjectPtr<UPlayerProgressionSubsystem> Progression;
	FDelegateHandle KillHandle;
};
