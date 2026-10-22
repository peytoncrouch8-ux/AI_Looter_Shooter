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
/** A shield in front of the health (UHealthComponent::Shield): handed a hit's Damage, returns what's left for health. */
DECLARE_DELEGATE_RetVal_ThreeParams(float, FHealthShield, float /*Damage*/, bool /*bCritical*/, const FVector& /*HitLocation*/);

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

	/** Restores full health and clears the dead state. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	/** Puts health at NewHealth, alive (a saved session's health); at least 1, so it never kills. */
	void SetHealth(float NewHealth);

	/**
	 * Changes the most health it can have (the player's level reward), before MaxHealthScale. A living one gains or loses
	 * as much health as the maximum moved, never dying of it; before play it starts full anyway, and a dead one stays dead
	 * until it's reset.
	 */
	void SetMaxHealth(float NewMaxHealth);

	/**
	 * Scales the most health it can have on top of SetMaxHealth's (a cursed iron in hand: Grasping's 0.85; 1 without). A
	 * living one keeps its health's share as the maximum moves, so swapping guns back and forth never heals or hurts, and
	 * never kills.
	 */
	void SetMaxHealthScale(float Scale);
	float GetMaxHealthScale() const { return MaxHealthScale; }

	/** Gives back up to Amount, never past the maximum; nothing for a dead one. Returns what it healed. */
	float Heal(float Amount);

	/**
	 * Takes up to Amount without it being damage (no numbers, no hit, no killer): never the last point of health, so it
	 * can't kill. Returns what it took. A cursed iron's toll (Hungry's reload).
	 */
	float Drain(float Amount);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bDead; }

	/**
	 * How many times it has been hurt: each damage that landed and each Drain that took something. It only goes up, so a
	 * watcher that sees it change knows something hurt since it last looked, however small (the player's wounds-that-close
	 * restart their wait on it: FWoundsClose). Healing and a smaller maximum are not hurts.
	 */
	uint32 GetHurtCount() const { return HurtCount; }

	/**
	 * Scales every damage it takes from now on, by who asks (an ember power's: Spyglass's mark 1.3, Iron Hide's plates 0.4);
	 * 1 takes that one's away. Each source holds one scale; they multiply. The number shown and every listener get the
	 * damage as scaled.
	 */
	void SetDamageTakenScale(FName Source, float Scale);

	/** Every source's scale together (1 with none). */
	float GetDamageTakenScale() const;

	/**
	 * The share of each hit it shrugs off, 0 to 0.9 (a creature's tier: FCreatureRankInfo::Defense, set as its rank is
	 * applied), after every scale; a critical hit pierces half of it (AfterDefense). The number shown and every listener
	 * get what's left.
	 */
	void SetDefense(float InDefense);
	float GetDefense() const { return Defense; }

	/** What a hit for Damage leaves through Defense: Damage x (1 - Defense x (bCritical ? 0.5 : 1)) (the user's "crits pierce half"). */
	static float AfterDefense(float Damage, float Defense, bool bCritical);

	/**
	 * A shield in front of its health (a creature's soul ward: UCreatureTraitComponent's Warded): handed each hit after every
	 * scale and the defense, it takes what it can and returns what's left for health. The number shown and every listener
	 * still get the whole hit (a ward hit is a hit). None bound: all of it comes off health.
	 */
	FHealthShield Shield;

	/** What dealt the latest damage (the gun, for shots), so death handlers can tell what made the kill. */
	UFUNCTION(BlueprintPure, Category = "Health")
	AActor* GetLastDamageCauser() const { return LastDamageCauser.Get(); }

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

	/** DamageType: an element status's number takes the element's colour (ElementCallout::LookFor). */
	void SpawnDamageNumber(float Damage, bool bCritical, const FVector& Location, AController* InstigatedBy, const UDamageType* DamageType) const;

	float Health = 0.f;
	bool bDead = false;
	/** MaxHealth is SetMaxHealth's times this (SetMaxHealthScale). */
	float MaxHealthScale = 1.f;
	/** GetHurtCount's. */
	uint32 HurtCount = 0;
	/** SetDamageTakenScale's, by source. */
	TMap<FName, float> DamageTakenScales;
	/** SetDefense's. */
	float Defense = 0.f;
	TWeakObjectPtr<AActor> LastDamageCauser;

	// The engine broadcasts point damage right before "any damage" in the same TakeDamage call,
	// so the point handler stashes the hit location for the any-damage handler to use.
	bool bHasPendingHitLocation = false;
	FVector PendingHitLocation = FVector::ZeroVector;
};
