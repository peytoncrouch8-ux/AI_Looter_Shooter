#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/AmmoTypes.h"
#include "AmmoPickup.generated.h"

class UDynamicMeshComponent;
class ULootTossComponent;
class UPointLightComponent;
class UPrimitiveComponent;
class URotatingMovementComponent;
class USphereComponent;

/**
 * A box of one ammo class dropped as loot. Walk over it to collect: it goes into the player's shared pool for that
 * class, up to the carry limit. If you can't carry all of it, the rest stays in the box. Every class is the same olive
 * ammo can (no rarity-like colors); the cartridges on top tell them apart (fat red shells, long sniper rounds, ...).
 * Despawns after a while.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AAmmoPickup : public AActor
{
	GENERATED_BODY()

public:
	AAmmoPickup();

	/** Spawns a box of Amount rounds of Type at Location (not yet tossed). */
	static AAmmoPickup* SpawnAmmo(UWorld* World, EAmmoType Type, int32 Amount, const FVector& Location);

	/** Throws the box with the given velocity; it bounces and settles on whatever is below. */
	void Toss(const FVector& Velocity);

	EAmmoType GetAmmoType() const { return AmmoType; }
	int32 GetAmount() const { return Amount; }

	/** Seconds before an uncollected box disappears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	float LifeSeconds = 300.f;

	/** Seconds after spawning before it can be collected (so it visibly pops out of the body first). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	float CollectDelay = 0.5f;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTossStopped(const FHitResult& ImpactResult);

	/** Gives ammo to any player standing in the pickup radius. Runs on overlap and every so often while someone is inside. */
	void TryCollect();
	void BuildModel();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UDynamicMeshComponent> Model;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<ULootTossComponent> TossMovement;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<URotatingMovementComponent> Spin;

	EAmmoType AmmoType = EAmmoType::AssaultRifle;
	int32 Amount = 0;
	double SpawnTime = 0.0;
	/** Players already told "full" during their current visit, so standing on a box doesn't spam the message. */
	TSet<TWeakObjectPtr<AActor>> ToldFull;
	FTimerHandle RetryTimer;
};
