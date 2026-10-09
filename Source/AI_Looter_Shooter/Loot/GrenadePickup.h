#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrenadePickup.generated.h"

class ULootTossComponent;
class UPrimitiveComponent;
class URotatingMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Grave-salt grenades dropped as loot: the tin itself (SM_SaltGrenade, oversized to read in the grass like the ammo
 * bundles) spinning under the small white beam every ammo drop has, collected by running past it (within CollectRadius),
 * into the player's grenades (UPlayerThrowComponent::AddGrenades), up to the three they can carry; what doesn't fit stays
 * on the ground, and the feed says "Grave Salt full" once a visit. The orchestrator's loot hooks spawn it (SpawnGrenades,
 * then Toss) where a kill or a chest drops it (FThrowRules::DropChance, ChestChance). Despawns after a while.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AGrenadePickup : public AActor
{
	GENERATED_BODY()

public:
	AGrenadePickup();

	/** Spawns Count grenades at Location (not yet tossed). */
	static AGrenadePickup* SpawnGrenades(UWorld* World, int32 Count, const FVector& Location);

	/** Throws the pickup with the given velocity; it bounces and settles on whatever is below. */
	void Toss(const FVector& Velocity);

	int32 GetAmount() const { return Amount; }

	/** Gives the grenades to any player in reach now (the overlap and a timer call it; tests call it). */
	void TryCollect();

	/** Seconds before an uncollected pickup disappears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade", meta = (ClampMin = "0"))
	float LifeSeconds = 300.f;

	/** Seconds after spawning before it can be collected (so it visibly pops out of the body first). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade", meta = (ClampMin = "0"))
	float CollectDelay = 0.5f;

	/** How close the player must come (cm, to the edge of their capsule): the ammo pickups' two metres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade", meta = (ClampMin = "0"))
	float CollectRadius = 180.f;

	/** The tin shows this much bigger than it's held, so it reads at a distance (the ammo bundles are about 20 cm). */
	static constexpr float ModelScale = 1.8f;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTossStopped(const FHitResult& ImpactResult);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Model;

	/** The thin white beam every ammo drop has, on the root so it stands still while the tin spins. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Beam;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<ULootTossComponent> TossMovement;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<URotatingMovementComponent> Spin;

	int32 Amount = 1;
	double SpawnTime = 0.0;
	/** Players already told "full" during their current visit. */
	TSet<TWeakObjectPtr<AActor>> ToldFull;
	FTimerHandle RetryTimer;
};
