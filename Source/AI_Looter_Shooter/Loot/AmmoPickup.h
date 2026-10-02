#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/AmmoTypes.h"
#include "AmmoPickup.generated.h"

class ULootTossComponent;
class UPrimitiveComponent;
class URotatingMovementComponent;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Ammo of one class dropped as loot: a spinning bundle of its rounds, the class's HUD icon modeled in 3D with the same
 * ink line (SM_Ammo<Type>, from Art/Models/Loot/Ammo.py), under a small white light beam so it can be found in the
 * grass from a distance. Run past it to collect (anything within CollectRadius of it): it goes into the player's shared
 * pool for that class, up to the carry limit. If you can't carry all of it, the rest stays on the ground. The rounds
 * themselves tell the classes apart (brass rifle rounds, oxblood shells, long sniper rounds...), never colors, which
 * belong to rarity: hence the plain white beam, shorter and fainter than any gun's. Despawns after a while.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AAmmoPickup : public AActor
{
	GENERATED_BODY()

public:
	AAmmoPickup();

	/** Spawns Amount rounds of Type at Location (not yet tossed). */
	static AAmmoPickup* SpawnAmmo(UWorld* World, EAmmoType Type, int32 Amount, const FVector& Location);

	/** Throws the pickup with the given velocity; it bounces and settles on whatever is below. */
	void Toss(const FVector& Velocity);

	EAmmoType GetAmmoType() const { return AmmoType; }
	int32 GetAmount() const { return Amount; }

	/** The spinning bundle of rounds, and the beam standing over it. */
	UStaticMeshComponent* GetModel() const { return Model; }
	UStaticMeshComponent* GetBeam() const { return Beam; }

	/** Seconds before an uncollected pickup disappears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	float LifeSeconds = 300.f;

	/** Seconds after spawning before it can be collected (so it visibly pops out of the body first). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	float CollectDelay = 0.5f;

	/**
	 * How close the player must come to the pickup to collect it (cm, from the pickup to the edge of the player's capsule):
	 * running past within about two meters is enough, so nobody has to stop and step onto it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	float CollectRadius = 180.f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTossStopped(const FHitResult& ImpactResult);

	/** Gives ammo to any player standing in the pickup radius. Runs on overlap and every so often while someone is inside. */
	void TryCollect();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Model;

	/** A thin white light beam on the root, so it stands still while the bundle spins. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Beam;

	/** The bundle of each ammo type (SM_Ammo<Type>, from Art/Models/Loot/Ammo.py), in EAmmoType order. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> TypeModels;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<ULootTossComponent> TossMovement;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<URotatingMovementComponent> Spin;

	EAmmoType AmmoType = EAmmoType::AssaultRifle;
	int32 Amount = 0;
	double SpawnTime = 0.0;
	/** Players already told "full" during their current visit, so standing on a pickup doesn't spam the message. */
	TSet<TWeakObjectPtr<AActor>> ToldFull;
	FTimerHandle RetryTimer;
};
