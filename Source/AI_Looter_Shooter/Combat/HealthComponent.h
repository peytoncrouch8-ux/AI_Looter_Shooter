#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class AController;
class ADamageNumberActor;
class UDamageType;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnHealthDamaged, float, Damage, bool, bCritical, FVector, HitLocation, AController*, InstigatedBy, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthDeath, AController*, Killer);

/**
 * Gives an actor health. Listens to the engine's damage events, so anything that calls
 * UGameplayStatics::ApplyDamage / ApplyPointDamage works. Spawns floating damage numbers
 * for damage dealt by the local player.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	/** Restores full health and clears the dead state. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bDead; }

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthDamaged OnDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthDeath OnDeath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	/** Takes hits (and shows numbers) but never loses health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bInvulnerable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Damage Numbers")
	bool bShowDamageNumbers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Damage Numbers")
	TSubclassOf<ADamageNumberActor> DamageNumberClass;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
		UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser);

	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	void SpawnDamageNumber(float Damage, bool bCritical, const FVector& Location, AController* InstigatedBy) const;

	float Health = 0.f;
	bool bDead = false;

	// The engine broadcasts point damage right before "any damage" in the same TakeDamage call,
	// so the point handler stashes the hit location for the any-damage handler to use.
	bool bHasPendingHitLocation = false;
	FVector PendingHitLocation = FVector::ZeroVector;
};
