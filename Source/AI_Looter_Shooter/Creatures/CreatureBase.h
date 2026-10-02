#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/CriticalSpotTarget.h"
#include "Components/SkinnedMeshComponent.h"
#include "Creatures/CreaturePoseAnimInstance.h"
#include "Creatures/CreatureUpdateRate.h"
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
 * against the bone a shot hit. Distant creatures update less often, and stop posing their bodies while off screen
 * (UpdateRate); ones busy with a player always update every frame.
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

	/** This frame's pose of the bones the code moves (component space), for UCreaturePoseAnimInstance. */
	const TArray<FCreatureBonePose>& GetBonePose() const { return BonePose; }

	/** Turns on Attacker as if it had been hurt by it (its pack heard the fight). Nothing changes if it's busy already. */
	void AlertTo(APawn* Attacker);

	/** Seconds between its updates right now (0 = every frame); see UpdateRate. */
	float GetUpdateInterval() const { return UpdateInterval; }

	/** Whether it has stopped posing its body for now (far away and out of every player's view); see UpdateRate. */
	bool IsPoseFrozen() const { return bPoseFrozen; }

	/**
	 * Whether a creature must update every frame wherever it is: it's after a player (chasing, attacking, or alerted, which
	 * gives it a target) or was hurt a moment ago. Slow updates there would show in the fight and in its timing.
	 */
	static bool NeedsFullRate(ECreatureState CreatureState, bool bHasTarget, bool bRecentlyHurt);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
	FText DisplayName;

	/**
	 * The creature's level, shown on its health bar. Tutorial island creatures are level 1; later areas set their own.
	 * Nothing scales with it yet (health, damage and loot will).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Progression", meta = (ClampMin = "1"))
	int32 Level = 1;

	/**
	 * Experience the player who kills it earns, once per kill (UPlayerProgressionSubsystem::AwardKill). 10 on the
	 * tutorial island for now; it drops to 0 there once later areas have their own creatures.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Progression", meta = (ClampMin = "0"))
	int32 XPReward = 10;

	/** Hits on these bones' hit zones are critical. Everything else takes base damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
	TArray<FName> CriticalSpotBones;

	/** Notices a visible player within this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Senses", meta = (ClampMin = "0"))
	float AggroRadius = 2600.f;

	/** When it's hurt, every creature of its kind within this distance turns on the attacker too (0 = only itself). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Senses", meta = (ClampMin = "0"))
	float PackAlertRadius = 0.f;

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

	/** How often it updates by its distance from the players and the camera: every frame up close, less often far away. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Update Rate")
	FCreatureUpdateRate UpdateRate;

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

	/**
	 * Its body is posed again after a spell frozen far away out of view (IsPoseFrozen), during which it kept moving: set it
	 * up where it stands now, changing as little of the pose it was last drawn in as possible (a quick turn can show it).
	 * Subclasses skip their pose work while it's frozen.
	 */
	virtual void OnPoseThawed() {}

	/**
	 * The attack lands, AttackWindup into it: bites whatever is in reach in front (and calls OnAttackStrike). A creature
	 * whose attack works differently (the slime's leap) overrides it.
	 */
	virtual void Strike();

	/** Deals one attack's damage to Victim (rolled like a weapon hit) and shoves it along Push. */
	void HitWithAttack(APawn* Victim, const FVector& Push);

	/** Moving toward a goal at Speed but not getting anywhere (it then takes a detour). Hoppers stand still between hops. */
	virtual bool IsStuck(float Speed) const;

	/** Whether it can begin an attack right now (in range and off cooldown); a hopper waits until it's on the ground. */
	virtual bool CanStartAttack() const { return true; }

	/** The pawn it's after, if any. */
	APawn* GetTarget() const { return Target.Get(); }

	/** A living player character it could go after. */
	bool IsValidTarget(const APawn* Pawn) const;

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

	/** The bones the code moves, set by the subclass every frame. */
	TArray<FCreatureBonePose> BonePose;

private:
	void SetState(ECreatureState NewState);
	void TickBrain(float DeltaSeconds);
	void TickAttack(float DeltaSeconds);
	void UpdatePerception();
	APawn* FindVisibleTarget() const;
	bool HasLineOfSight(const AActor* Other) const;
	void MoveToward(const FVector& Goal, float Speed, float DeltaSeconds);
	FVector ChooseDirection(const FVector& Desired);
	bool IsDirectionClear(const FVector& Direction) const;
	void FaceToward(const FVector& Point, float DeltaSeconds);
	bool PickWanderGoal();
	void SnapToGround();
	void Respawn();
	void UpdateHealthBar(float DeltaSeconds);

	// --- Update rate (CreatureBaseUpdateRate.cpp) ---
	void TickUpdateRate(float DeltaSeconds);
	/** Picks the update rate for where it is and what it's doing, and applies it. */
	void RefreshUpdateRate();
	/** Picks the rate again at once if it's updating slowly or frozen (it just got busy with a player). */
	void WakeUpdateRate();
	void ApplyUpdateInterval(float Interval);
	void SetPoseFrozen(bool bFrozen);

	/** How the local players see it, for picking its update rate. */
	struct FViewerMeasure
	{
		/**
		 * Distance (cm) to the nearest local player's pawn or camera, divided by the camera's zoom when it's in that camera's
		 * zoomed view (a scope shows it that much nearer); the largest float when there is no player.
		 */
		float Distance = TNumericLimits<float>::Max();
		/** In a local player's view, or drawn on screen lately. Behind a wall or a rock still counts. */
		bool bOnScreen = false;
	};
	FViewerMeasure MeasureViewers() const;

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

	// Update rate
	float UpdateInterval = 0.f;
	float UpdateRateCheckTime = 0.f;
	/** Seconds left of updating every frame after a hurt. */
	float FullRateTime = 0.f;
	bool bPoseFrozen = false;
	/** The mesh's own settings, for while its pose isn't frozen. */
	EVisibilityBasedAnimTickOption AwakeAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	bool bAwakeUsesScreenRenderState = false;
};
