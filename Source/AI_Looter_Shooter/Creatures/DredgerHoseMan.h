#pragma once

#include "CoreMinimal.h"
#include "Creatures/DredgerCreature.h"
#include "DredgerHoseMan.generated.h"

class UDredgerHoseComponent;
class UStaticMeshComponent;

/**
 * A Dredger hose-man (EDredgerRole::HoseMan; the contract's numbers: 220 HP, latch at 8 m, about 5 HP/s at level 1, break at
 * 14 m, the tank 30% of his health as a crit spot). Bulkier than a rifleman, a brass and iron tank on his back feeding a
 * soul-hose to a brass nozzle in his hands. He walks in on the fight director's melee turns (EFightRole::Melee: the ring,
 * his turn), and at 8 m with a clear line throws the hose's line at the player: the tell first (he plants, raises the
 * nozzle, its mouth swells cyan, the tank's gauge flares, a muffled call), then the line flies (UDredgerHoseComponent).
 * Latched, he braces and reels: the player is pulled toward him and drained (a hiss, the screen's edges) until the line
 * breaks: 14 m away, the world cut across it, or he is staggered (a crit on his mask does it) or his tank bursts.
 *
 * The tank is his weak spot: every hit on it is a crit, and once tank hits have taken 30% of his health it bursts (a big
 * soul-light pop, a shock ring, the creatures round him hurt, him staggered and hurt again, the hose torn off). Burst, he
 * fights on swinging the nozzle as a club, angrier and quicker. Topics: DredgerHoseMan.cpp (kit, turns, the latch),
 * DredgerHoseManTank.cpp (the tank and its burst).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ADredgerHoseMan : public ADredgerCreature
{
	GENERATED_BODY()

public:
	ADredgerHoseMan();

	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	// --- Its place in a fight ---
	virtual EFightRole GetFightRole() const override { return EFightRole::Melee; }
	virtual EFightRole GetAttackTokenRole() const override { return EFightRole::Melee; }

	UDredgerHoseComponent* GetHose() const { return Hose; }
	UStaticMeshComponent* GetTank() const { return Tank; }
	UStaticMeshComponent* GetTankGauge() const { return TankGauge; }

	/** Whether Pawn is one he may drain now (alive, on his ground, out of every safe zone: his hunt's own rule). */
	bool CanDrain(const APawn* Pawn) const { return IsValidTarget(Pawn); }
	/** His drain a second now (grown with his level and rank: DredgerRules::DrainPerSecond). */
	float GetDrainPerSecond() const;
	/** Where the hose leaves the nozzle and where it leaves the tank (world). */
	FVector GetNozzleMouth() const { return GetMuzzle(); }
	FVector GetNozzleBack() const;
	FVector GetTankFeed() const;

	// --- The tank (DredgerHoseManTank.cpp) ---
	bool IsTankBurst() const { return bTankBurst; }
	float GetTankDamage() const { return TankDamage; }
	/** A hit on the tank took Lost of his health (By shot it): it adds up, and bursts the tank at its share. Returns the sum. */
	float NoteTankHit(float Lost, AController* By);
	/** Bursts the tank now (By gets the blame for what the burst does), whatever it has taken. */
	void BurstTank(AController* By);

	/** Seconds before he may throw again. */
	float GetLatchRestLeft() const { return LatchRestLeft; }
	/** The line let go: his rest before the next throw begins. */
	void NoteHoseLetGo();

protected:
	virtual void BeginPlay() override;
	virtual void TickBeforeBrain(float DeltaSeconds) override;
	virtual void TickJob(float DeltaSeconds) override;
	virtual void PlaceJobKit() override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual void OnAttackCalledOff() override;
	virtual float GetAttackStartRange() const override;
	virtual bool CanStartAttack() const override;
	virtual bool CanReachHeight(float Rise) const override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;

	virtual FVector GetHeldGrip() const override { return FVector::ZeroVector; }
	virtual FVector GetHeldForegrip() const override { return FVector(26.f, 0.f, -2.f); }
	virtual FVector GetHeldMuzzle() const override { return FVector(52.f, 0.f, 1.f); }

private:
	/** His line has a clear way to his target (looked at a few times a second). */
	bool IsLineClear() const;
	/** The tank's gauge glows brighter as he drains (and flares through his tell). */
	void TickGauge(float DeltaSeconds);
	/** The burst's world side: the pop, the ring, the hurt round him (DredgerHoseManTank.cpp). */
	void PlayTankBurst(AController* By);

	UPROPERTY(VisibleAnywhere, Category = "Dredger")
	TObjectPtr<UDredgerHoseComponent> Hose;

	UPROPERTY(VisibleAnywhere, Category = "Dredger|Kit")
	TObjectPtr<UStaticMeshComponent> Tank;
	UPROPERTY(VisibleAnywhere, Category = "Dredger|Kit")
	TObjectPtr<UStaticMeshComponent> TankGauge;
	UPROPERTY(VisibleAnywhere, Category = "Dredger|Kit")
	TObjectPtr<UStaticMeshComponent> NozzleBarrel;
	UPROPERTY(VisibleAnywhere, Category = "Dredger|Kit")
	TObjectPtr<UStaticMeshComponent> NozzleMouth;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GaugeLook;

	bool bTankBurst = false;
	float TankDamage = 0.f;
	float LatchRestLeft = 0.f;
	/** The latch attack under way is a throw (else a club swing, after the burst). */
	bool bThrowAttack = false;
	/** His class's ranges and speeds, put back when he comes back (the burst changes them). */
	float ClassAttackRange = 220.f;
	float ClassChaseSpeed = 300.f;
	float GaugeNow = 0.f;
	mutable double LineCheckedAt = -10.0;
	mutable bool bLineClear = false;
};
