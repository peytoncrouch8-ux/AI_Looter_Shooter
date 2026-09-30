#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/CriticalSpotTarget.h"
#include "CreatureBase.generated.h"

class UHealthComponent;
class ULootDropComponent;
class UWidgetComponent;

UENUM(BlueprintType)
enum class ECreatureState : uint8
{
	Idle,     // standing around near home
	Wander,   // strolling to a random spot near home
	Chase,    // closing in on its target
	Attack,   // wind-up, strike, recovery
	Return,   // lost its target, walking home
	Dead
};

/**
 * Shared brain and life cycle for hostile creatures: senses the player, chases, telegraphs and lands melee
 * attacks, dies (dropping loot), and respawns at home. Movement is local steering that avoids obstacles and
 * never walks off an island edge, so it needs no navmesh. Subclasses provide the body and animation through
 * the On* hooks. The mesh's physics asset holds the hit zones, and critical spots are data (CriticalSpotBones) matched
 * against the bone a shot hit.
 */
UCLASS(Abstract)
class AI_LOOTER_SHOOTER_API ACreatureBase : public ACharacter, public ICriticalSpotTarget
{
	GENERATED_BODY()

public:
	ACreatureBase();

	virtual void Tick(float DeltaSeconds) override;

	// ICriticalSpotTarget: hits on the mesh's CriticalSpotBones are critical (x1.5, see LooterCombat).
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	UFUNCTION(BlueprintPure, Category = "Creature")
	ECreatureState GetCreatureState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Creature")
	bool IsDead() const { return State == ECreatureState::Dead; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
	FText DisplayName;

	/** Hits on these bones' hit zones are critical. Everything else takes base damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
	TArray<FName> CriticalSpotBones;

	/** Notices a visible player within this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Senses", meta = (ClampMin = "0"))
	float AggroRadius = 2600.f;

	/** Gives up and walks home when its target gets this far away. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Senses", meta = (ClampMin = "0"))
	float LoseInterestRadius = 5500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Movement", meta = (ClampMin = "0"))
	float WanderRadius = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Movement", meta = (ClampMin = "0"))
	float WalkSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Movement", meta = (ClampMin = "0"))
	float ChaseSpeed = 520.f;

	/** Distance (capsule center to target center, flat) from which it starts an attack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Attack", meta = (ClampMin = "0"))
	float AttackRange = 220.f;

	/** Average damage of one attack; each hit rolls within LooterCombat::DamageVariance of it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Attack", meta = (ClampMin = "0"))
	float AttackDamage = 12.f;

	/** Telegraph time before the strike lands; long enough to dodge on reaction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Attack", meta = (ClampMin = "0.05"))
	float AttackWindup = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Attack", meta = (ClampMin = "0.05"))
	float AttackRecovery = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Attack", meta = (ClampMin = "0"))
	float AttackCooldown = 1.1f;

	/** Seconds the body stays before sinking away. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Life", meta = (ClampMin = "0"))
	float CorpseTime = 6.f;

	/** Seconds after the corpse is gone before it respawns at home. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Life", meta = (ClampMin = "0"))
	float RespawnDelay = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Life")
	bool bRespawns = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// --- Hooks for the subclass's body and animation ---
	virtual void OnAttackStarted() {}
	virtual void OnAttackStrike(bool bConnected) {}
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) {}
	virtual void OnDied() {}
	virtual void OnRespawned() {}

	/** Turn the creature's shootable shapes on or off (off while dead). */
	virtual void SetHitVolumesEnabled(bool bEnabled) {}

	/** Seconds spent in the current state; drives attack and death animation. */
	float GetStateTime() const { return StateTime; }

	/** Ground height under a point (world-static geometry only), or false if there is none nearby. */
	bool FindGround(const FVector& Point, float Above, float Below, FVector& OutGround) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULootDropComponent> Loot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> HealthBar;

	/** Height above the capsule center where the health bar floats. */
	float HealthBarHeight = 130.f;

private:
	void SetState(ECreatureState NewState);
	void TickBrain(float DeltaSeconds);
	void TickAttack(float DeltaSeconds);
	void Strike();
	void UpdatePerception();
	APawn* FindVisibleTarget() const;
	bool IsValidTarget(const APawn* Pawn) const;
	bool HasLineOfSight(const AActor* Other) const;
	void MoveToward(const FVector& Goal, float Speed, float DeltaSeconds);
	FVector ChooseDirection(const FVector& Desired);
	bool IsDirectionClear(const FVector& Direction) const;
	void FaceToward(const FVector& Point, float DeltaSeconds);
	bool PickWanderGoal();
	void SnapToGround();
	void Respawn();
	void UpdateHealthBar(float DeltaSeconds);

	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(AController* Killer);

	ECreatureState State = ECreatureState::Idle;
	TWeakObjectPtr<APawn> Target;
	FTransform Home;
	FVector WanderGoal = FVector::ZeroVector;
	float StateTime = 0.f;
	float IdleDuration = 2.f;
	float PerceptionTimer = 0.f;
	float CooldownRemaining = 0.f;
	bool bStruck = false;

	// Steering
	FVector SteerDirection = FVector::ZeroVector;
	float SteerTimer = 0.f;
	float PreferredSide = 1.f;
	float StuckTime = 0.f;
	FVector EscapeDirection = FVector::ZeroVector;
	float EscapeTime = 0.f;

	float HealthBarTime = 0.f;
	FTimerHandle RespawnTimer;
	FTimerHandle HideTimer;
};
