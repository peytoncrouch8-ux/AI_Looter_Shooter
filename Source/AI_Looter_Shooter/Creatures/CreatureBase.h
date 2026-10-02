#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/CriticalSpotTarget.h"
#include "Components/SkinnedMeshComponent.h"
#include "Creatures/CreaturePoseAnimInstance.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureUpdateRate.h"
#include "CreatureBase.generated.h"

class UHealthComponent;
class ULootDropComponent;
class ULootTable;
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
 *
 * Each creature has a rank (ECreatureRank: its tag's word and color, size, stats, loot and pack call) and a size
 * (BodyScale times its rank's): the whole actor is scaled, so the capsule, the model, its hit zones and the health bar
 * follow, and every distance the code works in (attack reach, steering probes, the subclass's gait or hops) is multiplied
 * by GetSizeScale(). CreatureBaseRank.cpp holds the rank, level and size, and which creatures come back after a death;
 * the area being played gives a creature its level and may promote it (UAreaRulesSubsystem).
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

	// --- Rank, level and size (CreatureBaseRank.cpp) ---

	/** Its rank now: StartingRank, or a promotion, which lasts until it dies. */
	UFUNCTION(BlueprintPure, Category = "Creature|Rank")
	ECreatureRank GetRank() const { return CurrentRank; }

	/**
	 * Promotes (or demotes) it until it dies: its tag, size, health, damage, experience, level, loot table and pack call
	 * follow the rank (UCreatureRankSettings), and a hurt creature keeps its share of health. Before play starts, this is
	 * the rank it starts with.
	 */
	void SetRank(ECreatureRank NewRank);

	/**
	 * Sets its level, which its tag shows and the guns it drops take (passed to its loot drop component). In play its
	 * health and damage follow at once (FLevelRules::EnemyScale, its rank's multipliers on top), a hurt one keeping its share.
	 */
	void SetLevel(int32 NewLevel);

	/** Its size against its model right now: BodyScale times its rank's size. Every distance it works in is multiplied by it. */
	UFUNCTION(BlueprintPure, Category = "Creature|Rank")
	float GetSizeScale() const { return SizeScale; }

	/** Sets BodyScale and resizes it at once, its feet staying where they stand. */
	void SetBodyScale(float NewBodyScale);

	/** Whether it comes back after a death: placed creatures do, unless placed as a Legendary monster or a boss. */
	bool WillRespawn() const;

	/** Whether it answers Other's pack call: the same PackTag, or without one, the same class. */
	bool SharesPackWith(const ACreatureBase& Other) const;

	/** How far its pack call reaches when it's hurt: its PackAlertRadius, or its rank's call if that's farther. */
	float GetPackCallRadius() const;

	/** Distance (capsule center to target center, flat) from which it starts an attack, at its size. */
	float GetAttackRange() const { return AttackRange * SizeScale; }

	/** How far its strike lands (flat): a little past where it starts the attack, at its size. */
	float GetStrikeReach() const;

	/** What its steering looks ahead with (IsDirectionClear), at its size. */
	struct FSteerProbes
	{
		/** A sphere this wide swept this far ahead finds anything too steep to walk up. */
		float SweepLength = 0.f;
		float SweepRadius = 0.f;
		/** It looks for ground this far ahead, at most this far below its middle: it never walks off a higher drop. */
		float LedgeDistance = 0.f;
		float LedgeDrop = 0.f;
	};
	FSteerProbes GetSteerProbes() const;

	/** How a creature spawned in play (by a spawner, an egg sac or a command) starts. */
	struct FRuntimeSpawn
	{
		ECreatureRank Rank = ECreatureRank::Basic;
		/**
		 * Its level before its rank's, kept whatever the area's band. 0: as a placed creature's, from the area's band and the
		 * player's level, or the class's where the area has no band.
		 */
		int32 Level = 0;
		/** Its BodyScale; 0 keeps the class's. */
		float BodyScale = 0.f;
	};

	/**
	 * Spawns a creature in play, standing on Feet (the ground) and facing Yaw. Only placed creatures come back: this one is
	 * gone for good once killed, and its body is removed after it sinks away.
	 */
	static ACreatureBase* SpawnAtRuntime(UWorld* World, TSubclassOf<ACreatureBase> Class, const FVector& Feet, float Yaw,
		const FRuntimeSpawn& Spawn);

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
	 * The creature's level, shown on its health bar; the guns it drops take it, and its health and damage grow with it
	 * (8% of their level 1 values a level, as guns' damage does). Placed in an area with a level band (UAreaDefinition),
	 * play gives it one from the band around the player's level; elsewhere it keeps the level it was placed at. Its
	 * rank's levels come on top (a Restless one placed at 3, with no band, is level 4). In play, set it with SetLevel.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Progression", meta = (ClampMin = "1"))
	int32 Level = 1;

	/**
	 * Experience for a kill at level 1, once per kill (UPlayerProgressionSubsystem::AwardKill): a kill gives it grown 8%
	 * for each level above 1, and less when the creature is below the player (FLevelRules::KillXP). 10 on the tutorial
	 * island for now; it drops to 0 there once later areas have their own creatures. In play it includes its rank's
	 * multiplier (a Restless one's is twice this), so the kill counts the rank once.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Progression", meta = (ClampMin = "0"))
	int32 XPReward = 10;

	/**
	 * The rank it starts with, and comes back as after a death (a promotion lasts one life). A Legendary monster or a boss
	 * doesn't come back on its own (UCreatureRankSettings).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Rank", meta = (DisplayName = "Rank"))
	ECreatureRank StartingRank = ECreatureRank::Basic;

	/**
	 * Its size against its model (0.45 makes a spiderling, 1.8 a giant), before its rank makes it a little bigger. The
	 * actor's own scale in the level is ignored: creatures always start from their model's size.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Rank", meta = (ClampMin = "0.1", ClampMax = "5"))
	float BodyScale = 1.f;

	/**
	 * Creatures with the same pack tag answer each other's pack calls (PackAlertRadius, and a rank's wider call): every
	 * spider, spiderlings and the Gravemother too. None: only its own class.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Senses")
	FName PackTag;

	/** Hits on these bones' hit zones are critical. Everything else takes base damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
	TArray<FName> CriticalSpotBones;

	/** Notices a visible player within this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Senses", meta = (ClampMin = "0"))
	float AggroRadius = 2600.f;

	/**
	 * When it's hurt, every creature of its pack (PackTag) within this distance turns on the attacker too (0 = only itself).
	 * Its rank can call farther (GetPackCallRadius).
	 */
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

	/**
	 * Whether it comes back at home after a death, as its StartingRank. Off for anything spawned in play (SpawnAtRuntime);
	 * a Legendary monster or a boss doesn't come back either way (WillRespawn).
	 */
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

	/** Its size changed in play (a promotion, or SetBodyScale): put the body's pose back under it at the new size. */
	virtual void OnSizeChanged() {}

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

	/** Height above the capsule center where the health bar floats, at size 1 (it rises with the scaled actor). */
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

	// --- Rank, level and size (CreatureBaseRank.cpp) ---
	/**
	 * As play begins: a promotion its area may give a placed Basic creature for this arrival, a level from the area's band
	 * (or the one it was placed or spawned at), and the stats, loot and size of both.
	 */
	void BeginRankAndLevel();
	/** Coming back: as its StartingRank (a promotion lasts one life), at a level its area rolls again for the player's now. */
	void RespawnRankAndLevel();
	/** The rank it begins play with: StartingRank, or for a placed Basic creature its area's promotion this arrival. */
	ECreatureRank RollArrivalRank() const;
	/** Its own level before its rank's: from its area's band and the player's level, or GivenLevel (no band, or spawned at one). */
	int32 RollOwnLevel(ECreatureRank Rank) const;
	/** Takes Rank with OwnLevel plus the rank's levels, and applies them (ApplyRank). */
	void TakeRankAndLevel(ECreatureRank Rank, int32 OwnLevel);
	/** Applies CurrentRank: stats from the base ones, loot table, and size. */
	void ApplyRank();
	/** Health, damage and experience from the base ones: grown for its level, then times its rank's multipliers. */
	void ApplyStats();
	/** Scales the actor to BodyScale times its rank's size, its feet staying put, and what the scale doesn't reach. */
	void ApplySize();
	/** Keeps what the class or the level gave it, once, before any rank changes it. */
	void CaptureBaseStats();

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

	// Rank, level and size
	ECreatureRank CurrentRank = ECreatureRank::Basic;
	float SizeScale = 1.f;
	/** The level it was placed or spawned at, before any band or rank: what it keeps where its area has no band. */
	int32 GivenLevel = 1;
	/** Spawned at a level of its own (FRuntimeSpawn::Level): it keeps it whatever the area's band. */
	bool bKeepsGivenLevel = false;
	/** Spawned in play (SpawnAtRuntime): never comes back, and its body is removed once it has sunk away. */
	bool bSpawnedAtRuntime = false;
	/** What the class or the level gave it, which ranks multiply (CaptureBaseStats). */
	bool bBaseCaptured = false;
	float BaseMaxHealth = 100.f;
	float BaseAttackDamage = 0.f;
	int32 BaseXPReward = 0;
	float BaseStepHeight = 0.f;
	/** Its own loot table, for Basic (none: the default one). */
	UPROPERTY(Transient)
	TObjectPtr<ULootTable> BaseLootTable;

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
