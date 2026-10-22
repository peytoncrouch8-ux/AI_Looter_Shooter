#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponRocket.generated.h"

class AController;
class AWeaponBase;
class UAudioComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A launcher's rocket in flight (AWeaponBase fires it for a Launcher kind; numbers in GunKindsTuning): the rocket model
 * (the definition's ProjectileMesh, SM_PowderkegRocket) flying from the shooter's eye along the aim at RocketSpeed with a
 * slight drop, its motor's flame and a smoke trail drawn through FWeaponFX. It's seen leaving the gun's muzzle as the
 * player sees it (a first-person gun is drawn with its own view) and closes onto its true path within a few metres, as a
 * tracer does. It moves itself, sweeping a small ball along the weapons' trace channel (what bullets meet: walls, the
 * ground, a creature's body parts), never its shooter or the gun; whatever it meets, or the end of its life, bursts it
 * (WeaponRocketBlast::Detonate). After the burst it lingers a moment, then goes.
 *
 * Nothing collides with it: it never blocks the player, a creature or a shot.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AWeaponRocket : public AActor
{
	GENERATED_BODY()

public:
	AWeaponRocket();

	/**
	 * Fires a rocket from Start (the shooter's eye) along Direction for Gun, seen leaving from VisualStart (the muzzle as
	 * the player sees it). Its damage and shove are the gun's now. Null without a world.
	 */
	static AWeaponRocket* Launch(UWorld* World, const FVector& Start, const FVector& Direction, const FVector& VisualStart, AWeaponBase* Gun);

	/** One step of its life: flight and the sweep, or after the burst the wait to go (Tick runs it; tests call it). */
	void Step(float DeltaSeconds);

	/** Bursts it at Location (Normal: the way the burst throws; Hit: what it met, null in the air). Nothing if it already has. */
	void Burst(const FVector& Location, const FVector& Normal, const FHitResult* Hit);

	bool HasBurst() const { return bBurst; }
	virtual FVector GetVelocity() const override { return Velocity; }
	/** Where it really is (the burst's and the sweep's place), whatever its drawn model's offset. */
	FVector GetFlightLocation() const { return Position; }
	float GetDamage() const { return Damage; }
	AWeaponBase* GetWeapon() const { return Weapon.Get(); }

	/** The sweep's ball (cm): a rocket is fat, so it clips a creature a bullet would just miss. */
	static constexpr float CollisionRadius = 7.f;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void DrawFlight();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	TWeakObjectPtr<AWeaponBase> Weapon;
	TWeakObjectPtr<AActor> Shooter;
	TWeakObjectPtr<AController> InstigatorController;
	/** The motor's roar and the fins' whistle while it flies (Weapon.Launcher.Flight), stopped as it bursts. */
	TWeakObjectPtr<UAudioComponent> FlightSound;
	TEnumAsByte<ECollisionChannel> Channel = ECC_Visibility;

	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	/** The drawn model's offset from its true place (the muzzle's), closing as it flies. */
	FVector VisualOffset = FVector::ZeroVector;
	float Damage = 0.f;
	float Impulse = 0.f;
	float Age = 0.f;
	float SinceBurst = 0.f;
	float SmokeIn = 0.f;
	bool bLaunched = false;
	bool bBurst = false;
	FRandomStream Random{ 0x50c4e7 };
};
