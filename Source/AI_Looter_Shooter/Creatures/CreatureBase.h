#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/CriticalSpotTarget.h"
#include "Combat/HitReaction.h"
#include "Components/SkinnedMeshComponent.h"
#include "Creatures/CreaturePoseAnimInstance.h"
#include "Creatures/CreaturePursuit.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureSteerPlanner.h"
#include "Creatures/CreatureTypes.h"
#include "Creatures/CreatureUnstick.h"
#include "Creatures/CreatureUpdateRate.h"
#include "Creatures/FightRules.h"
#include "Creatures/HuntingGround.h"
#include "Templates/Function.h"
#include "CreatureBase.generated.h"

class UCreatureHitReactionComponent;
class UCreatureKnockComponent;
class UCreaturePackComponent;
class UCreatureReactComponent;
class UCreatureTraitComponent;
class UCreatureVoiceComponent;
class UHealthComponent;
class ULootDropComponent;
class ULootTable;
class UWidgetComponent;

/**
 * Shared brain and life cycle for hostile creatures: senses the player, chases, telegraphs and lands melee attacks, dies
 * (dropping loot), and respawns at home. Movement is local steering that avoids obstacles and never walks off an island
 * edge, so it needs no navmesh. Subclasses provide the body and animation through the On* hooks. The mesh's physics asset
 * holds the hit zones; critical spots are data (CriticalSpotBones) matched against the bone a shot hit. Distant creatures
 * update less often and stop posing off screen (UpdateRate); ones busy with a player always update every frame.
 *
 * Each creature has a rank (ECreatureRank: tag word and color, size, stats, loot, pack call) and a size (BodyScale times its
 * rank's): the whole actor is scaled, so every distance the code works in (attack reach, steering probes, the subclass's gait
 * or hops) is multiplied by GetSizeScale(). Each topic has its own CreatureBase<Topic>.cpp (CODEMAP.md); the small types it
 * hands out are in CreatureTypes.h.
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

	/** The pawn it's after, if any. */
	APawn* GetTarget() const { return Target.Get(); }

	/**
	 * In a fight with Pawn: alive, after them, and chasing or attacking. One that lost them and only tracks, sniffs or prowls
	 * (Search) is not. Fast travel and the combat music both ask this, so they can't drift apart (CreatureBasePursuit.cpp).
	 */
	bool IsFightingWith(const APawn* Pawn) const;
	/** The state half of it: Chase and Attack. */
	static bool IsFightState(ECreatureState CreatureState);

	// --- Its turn in a fight (CreatureBaseFight.cpp; UFightDirectorSubsystem runs the turns, the ring and the ranged spots) ---

	/** Its place in a fight: Melee waits on the ring for its turn (the default), Ranged holds a spot in its band, Support keeps back its own way. */
	virtual EFightRole GetFightRole() const { return EFightRole::Melee; }
	/** The kind of turn the attack it would start now takes (Melee by default; the Spitter's spit is a Ranged one, its bite Melee). */
	virtual EFightRole GetAttackTokenRole() const { return EFightRole::Melee; }
	/** A ranged kind's band round its target (cm, near and far edge): where its spot may lie. False: it has none. */
	virtual bool GetFightBand(float& OutNear, float& OutFar) const { return false; }
	/** A boss or a Legendary monster: its own fight (no turns, no ring, its own speed and damage). */
	bool KeepsOwnFight() const;
	/** It takes turns and a place in fights: not its own fight, not held back. */
	bool UsesFightDirector() const;
	/** Its brain is held by a hit: staggered, knocked or dazed. */
	bool IsHeldByHit() const;
	/** How far past its ground's edge (cm) a hit can press it (NotePressed); 0: never (every kind but the Unpaid). */
	virtual float GetPressReach() const { return 0.f; }
	/** How long a press lasts once its target is out of that reach (s). */
	virtual float GetPressSeconds() const { return 0.f; }
	/** Seconds between the hit slows it takes (Looter.Gunfeel.Flinch.Slow); 0: every hit slows it (the Unpaid: once a half second). */
	virtual float GetHitSlowGap() const { return 0.f; }
	/** Its chase speed now: ChaseSpeed, quickened by Looter.Fight.ChaseScale unless it keeps its own fight. */
	float GetChaseSpeedNow() const;
	/** Waiting its turn on the ring, at its slot or on its way round to it (an Unpaid then never phase-steps in). */
	bool IsWaitingTurn() const { return bWaitingTurn; }

	UFUNCTION(BlueprintPure, Category = "Creature")
	bool IsDead() const { return State == ECreatureState::Dead; }

	/** This frame's pose of the bones the code moves (component space), for UCreaturePoseAnimInstance. */
	const TArray<FCreatureBonePose>& GetBonePose() const { return BonePose; }

	/** As if Attacker had hurt it (its pack heard): off its ground it watches from its edge; already after them, a search becomes the hunt; after someone else, nothing. */
	void AlertTo(APawn* Attacker);

	/**
	 * Pressed: hurt (or its pack hurt) by By from just past its ground's edge (within GetPressReach), it fights them there for
	 * GetPressSeconds, renewed while they stay within that reach (the Unpaid, 2026-10-28: shot at from the chapel's doorway,
	 * nine stood in a line at the edge). False (nothing changes) for a kind that never presses or one already out of reach.
	 */
	bool NotePressed(const APawn* By);
	/** How far past its hunting ground's edge (cm) it counts a player as on it now: its press reach while pressed, else 0. */
	float GetGroundReach() const;
	/** Whether Where is on its hunting ground, with the reach it has now (GetGroundReach). */
	bool IsOnItsGround(const FVector& Where) const;
	bool IsPressed() const { return PressedLeft > 0.f; }

	/** How its hunt is going (CreatureBasePursuit.cpp): its step, its last known spot, why it gave up; for tests and tools. */
	const FCreaturePursuit& GetPursuit() const { return Pursuit; }

	/** Its place in its pack: its flank, breaking off, its rank's sting, a patrol's anchor (CreaturePackComponent.h). */
	UCreaturePackComponent* GetPack() const { return Pack; }

	/** Developer tools and tests only (Looter.CastShots; CreatureBaseDev.cpp): its brain put in NewState now, after InTarget or toward Goal. */
	void DevPutInState(ECreatureState NewState, APawn* InTarget = nullptr, const FVector& Goal = FVector::ZeroVector);

	/** Developer tools and tests only: its senses find Seen now, as its own eyes would (a new hunt, its notice beat and all). */
	void DevSight(APawn* Seen);
	/** Developer tools only: its kind's entrance (rising, climbing out, swelling up) now, seen or not. */
	void DevPlayEntrance();

	// --- Reactions (CreatureBaseReact.cpp; UCreatureReactComponent) ---

	/** In its notice beat: it has just spotted its prey and shows it, facing them, before it comes (FCreatureNotice). */
	bool IsNoticing() const;
	/** How much of its notice pose shows now (0 to 1, eased in and out). */
	float GetNoticeStrength() const;
	/** Its notice beat, hit flash, dust and ash, and its dice for idle life. */
	UCreatureReactComponent* GetReact() const { return React; }
	/** Knocked off its feet or reeling (an ember power's fling or daze): its brain waits meanwhile (UCreatureKnockComponent). */
	UCreatureKnockComponent* GetKnock() const { return Knock; }
	/** A boss, or a creature a boss fight is built round: its fight's rules (no notice beat; its death is its show's). */
	bool IsBossCreature() const { return KeepsBossRules(); }
	/** Comes into the world its kind's way (the dead rise, a spider climbs out of the dirt, a slime swells) if a player can see it. */
	bool PlayEntranceIfSeen();
	/**
	 * Its body set up afresh where it stands, as after a frozen spell (OnPoseThawed: feet put down under it, a shroud
	 * hanging still, springs at rest): after a jump that moved it all at once (Phasing's blink, UCreatureTraitComponent).
	 */
	void SettleBody() { OnPoseThawed(); }

	// --- Boss fights (UBossComponent; CreatureBaseRank.cpp) ---

	/** Holds it back from fighting (a boss under a spell, withdrawn): passive, it hunts nobody, lets go and walks home, and no hit or pack call sets it on anyone. */
	void SetPassive(bool bInPassive);
	bool IsPassive() const { return bPassive; }

	/** Puts it back at its spot as a boss fight's reset wants it: alive at full health, calm, nobody targeted, at its rank and level (nothing rolled again). */
	void ResetToHome();

	/** Where it stands when calm: its spot as play began (a boss fight's arena centers on it). */
	const FTransform& GetHome() const { return Home; }

	/** Whether its tag shows over it when it's hurt or hunting. A boss turns it off while its bar shows at the top of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature")
	bool bShowsHealthTag = true;

	// --- Rank, level and size (CreatureBaseRank.cpp; the area being played gives the level and may promote: UAreaRulesSubsystem) ---

	/** Its rank now: StartingRank, or a promotion, which lasts until it dies. */
	UFUNCTION(BlueprintPure, Category = "Creature|Rank")
	ECreatureRank GetRank() const { return CurrentRank; }

	/**
	 * Promotes (or demotes) it until it dies: its tag, size, health, defense, damage, experience, level, loot table, traits
	 * and pack call follow the rank (UCreatureRankSettings), a hurt creature keeping its share of health. Before play, the
	 * rank it starts with.
	 */
	void SetRank(ECreatureRank NewRank);

	/** The share of each hit its tier shrugs off (FCreatureRankInfo::Defense; a crit pierces half): its health component's. */
	float GetDefense() const;

	/**
	 * The seed its traits are rolled from (UCreatureTraitComponent::RollTraits), the same at every load with no save data:
	 * a placed one's name in its level (a spawned one's object id) with the roll that promoted it this arrival (0 for a
	 * rank it was placed or spawned with).
	 */
	int32 GetTierSeed() const;

	/** Its tier's traits (none below Gravebound), set each time its rank is applied. */
	UCreatureTraitComponent* GetTraitComponent() const { return TraitComponent; }

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

	/** What its steering looks ahead with (FCreatureSteerProbe), at its size. */
	using FSteerProbes = FCreatureProbeSizes;
	FSteerProbes GetSteerProbes() const;

	/** Its steering's look along Direction (flat) for up to Distance (CreatureBaseSteering.cpp); public for the tests. */
	FCreatureSteerPlanner::FLookResult LookAlong(const FVector& Direction, float Distance) const;

	/** Its way round obstacles (the side it keeps, its counts); for the tests. */
	const FCreatureSteerPlanner& GetSteerPlanner() const { return SteerPlanner; }

	/** Gliding free of something it was stuck on (CreatureBaseUnstick.cpp). */
	bool IsUnsticking() const { return Unstick.IsGliding(); }

	/** Its movement was stopped by something: kept as a felt wall when its looks missed it (CreatureBaseSteering.cpp). */
	virtual void MoveBlockedBy(const FHitResult& Impact) override;

	/** The ground its body covers from above, at size 1, which other creatures keep clear of (CreatureBaseSpacing.cpp). */
	using FFootprint = FCreatureFootprint;
	virtual FFootprint GetFootprint() const;
	/** Its footprint now, at its size: its line's two ends (world) and its radius. */
	void GetFootprintInWorld(FVector& OutFront, FVector& OutBack, float& OutRadius) const;

	/** How a creature spawned in play (by a spawner, an egg sac or a command) starts. */
	using FRuntimeSpawn = FCreatureRuntimeSpawn;

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
	 * play gives it the levels where it stands (or its kind's set level); elsewhere it keeps the level it was placed at. Its
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

	/** The rank it starts with and comes back as (a promotion lasts one life). A Legendary monster or a boss doesn't come back on its own (UCreatureRankSettings). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Rank", meta = (DisplayName = "Rank"))
	ECreatureRank StartingRank = ECreatureRank::Basic;

	/**
	 * Its size against its model (0.45 makes a spiderling, 1.8 a giant), before its rank makes it a little bigger. The
	 * actor's own scale in the level is ignored: creatures always start from their model's size.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Rank", meta = (ClampMin = "0.1", ClampMax = "5"))
	float BodyScale = 1.f;

	/** Its name stands for its rank's word on its tag, in the rank's color: a named Legendary (the Gravemother), not "Soulfed". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Rank")
	bool bNameIsRankWord = false;

	/**
	 * Creatures with the same pack tag answer each other's pack calls (PackAlertRadius, and a rank's wider call): every
	 * spider, spiderlings and the Gravemother too. None: only its own class.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Senses")
	FName PackTag;

	/** Hits on these bones' hit zones are critical. Everything else takes base damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
	TArray<FName> CriticalSpotBones;

	/**
	 * Notices a visible player within this distance: its lock-on radius (26 m until the user's 20% more, 2026-10-12). Hunting,
	 * it also stands for the reach of its nose: an unseen target nearer than this is tracked again (FCreaturePursuit).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Senses", meta = (ClampMin = "0"))
	float AggroRadius = 3120.f;

	/** When it's hurt, every creature of its pack (PackTag) within this distance turns on the attacker too (0 = only itself); its rank can call farther (GetPackCallRadius). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Senses", meta = (ClampMin = "0"))
	float PackAlertRadius = 0.f;

	/**
	 * Its target farther than this is gone (a fast travel, a trip away): it gives up at once and walks home. Nearer, seen, it
	 * keeps after them; out of sight it searches (FCreaturePursuit). A boss lets go at 55 m at most, as creatures used to.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Senses", meta = (ClampMin = "0"))
	float LoseInterestRadius = 8000.f;

	/**
	 * Where it fights (FHuntingGround): it hunts only players standing on it; a target who leaves it is watched from its edge
	 * for a while, then given up. Spawners give their creatures their own (AEncounterSpawner). Unset, the default: anywhere,
	 * as far as LoseInterestRadius. Wherever it is, nothing hunts a player in a safe zone that's on (ASafeGround).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature|Senses")
	FHuntingGround HuntingGround;

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
	/** A hit from By has newly pressed it past its ground's edge (NotePressed): the Unpaid phase-steps in. */
	virtual void OnPressedByHit(const APawn& By) {}
	/**
	 * The hit being taken (its share of health and its burst's, the gun, crit, way, spot), stored by HandleDamaged before
	 * OnHurt: each kind sizes its flinch by GetLastHit().FlinchStrength (Gunfeel; fresh only during that OnHurt).
	 */
	const FCreatureHit& GetLastHit() const { return LastHit; }
	virtual void OnDied() {}
	virtual void OnRespawned() {}

	/** Turn the creature's shootable shapes on or off (off while dead). */
	virtual void SetHitVolumesEnabled(bool bEnabled) {}

	/**
	 * Its body is posed again after a spell frozen far away out of view (IsPoseFrozen), during which it kept moving: set it
	 * up where it stands now, changing as little of the pose it was last drawn in as possible. Subclasses skip pose work while frozen.
	 */
	virtual void OnPoseThawed() {}

	/** Its size changed in play (a promotion, or SetBodyScale): put the body's pose back under it at the new size. */
	virtual void OnSizeChanged() {}

	/** Its rank was applied (play begins, a promotion, coming back), after its stats and size: the subclass's own rank traits follow (the Unpaid's coal color). */
	virtual void OnRankChanged() {}

	/** Its notice beat begins (the brain holds it, facing its prey): its kind's sound and the start of its pose. */
	virtual void OnNoticed() {}
	/** Its kind's entrance as it comes into the world in a player's view (PlayEntranceIfSeen); none by default. */
	virtual void PlayEntrance() {}
	/** Holds its brain this long (an entrance), as a stagger does: it stands, its state's clock held. */
	void HoldBrain(float Seconds) { BrainHold = FMath::Max(BrainHold, Seconds); }

	/** The attack lands, AttackWindup into it: bites whatever is in reach in front (and calls OnAttackStrike). The slime's leap overrides it. */
	virtual void Strike();

	/** Deals one attack's damage to Victim (rolled like a weapon hit, times GetFightDamageScale) and shoves it along Push, both times Strength (a charge's 2). */
	void HitWithAttack(APawn* Victim, const FVector& Push, float Strength = 1.f);

	/** What every hit it deals is multiplied by now: Looter.Fight.DamageScale, or 1 for a boss or Legendary monster (CreatureBaseFight.cpp). */
	float GetFightDamageScale() const;

	/** Its attack was called off before its strike (hit off its turn, and no turn to be had when it came to): undo its kind's tell. */
	virtual void OnAttackCalledOff() {}

	/** Moving toward a goal at Speed but not getting anywhere (it then takes a detour). Hoppers stand still between hops. */
	virtual bool IsStuck(float Speed) const;

	/** Whether it can begin an attack right now (in range and off cooldown); a hopper waits until it's on the ground. */
	virtual bool CanStartAttack() const { return true; }

	/** How far off it may begin an attack now: its reach, or farther for an attack of its own that's ready (a charge). */
	virtual float GetAttackStartRange() const { return GetAttackRange(); }

	/**
	 * Whether it can hurt a target this far above (or below) it (cm): its strike's reach by default, and a ranged attack
	 * reaches higher (the Spitter's lob). Out of it while near, it starts no attack, and stays there a while out of its reach.
	 */
	virtual bool CanReachHeight(float Rise) const;

	/** Whether it keeps turning to its target through a wind-up: a charge that has taken its aim holds it (the Gravemother's). */
	virtual bool TracksTargetInWindup() const { return true; }

	/** A player it may hunt: alive, on its HuntingGround, out of every safe zone that's on (CreatureBaseHunting.cpp). Off its ground: watched from the edge. */
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCreatureVoiceComponent> Voice;

	/** Hit-stop, stagger and death burst (a stagger pauses its brain: TickBrain). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCreatureHitReactionComponent> HitReaction;

	/** Its pack: its chase's flank and breaking off, its rank's sting, a patrol's anchor (the brain asks it). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCreaturePackComponent> Pack;

	/** Its reactions: the notice beat, the hit flash, footfall dust and ash (its frame moves it on). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCreatureReactComponent> React;

	/** Knocked through the air and left dizzy (the ember powers): its brain waits as for a stagger (TickBrain). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCreatureKnockComponent> Knock;

	/** Its tier's traits (Fleet, Mending, Warded...; ApplyTraits sets them as its rank is applied). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCreatureTraitComponent> TraitComponent;

	/** Height above the capsule center where the health bar floats, at size 1 (it rises with the scaled actor). */
	float HealthBarHeight = 130.f;

	/** The bones the code moves, set by the subclass every frame. */
	TArray<FCreatureBonePose> BonePose;

	/** The latest hit taken (GetLastHit; CreatureBaseHurt.cpp). */
	FCreatureHit LastHit;

private:
	void SetState(ECreatureState NewState);
	void TickBrain(float DeltaSeconds);
	void TickAttack(float DeltaSeconds);
	void UpdatePerception();
	APawn* FindVisibleTarget() const;
	bool HasLineOfSight(const AActor* Other) const;
	/** It closes in to this (flat) and there stands squared up: three quarters of its attack range, at its size. */
	float GetChaseHold() const { return GetAttackRange() * 0.75f; }
	/** How far up or down its strike lands (at its size, at least size 1's). */
	float GetStrikeHeight() const { return 220.f * FMath::Max(SizeScale, 1.f); }

	// Each of these is documented at its definition, in the topic's CreatureBase<Topic>.cpp.

	// --- Whom it hunts (CreatureBaseHunting.cpp): a living player on its HuntingGround, out of every safe zone that's on (ASafeGround) ---
	using EHuntCheck = ECreatureHuntCheck;
	EHuntCheck CheckHunt(const APawn* Pawn) const;
	bool IsSceneHoldingPlayer() const;

	// --- Its hunt going on (CreatureBasePursuit.cpp; FCreaturePursuit has the rules): kept up until a real reason to stop ---
	bool KeepsBossRules() const;
	void BeginHunt(APawn* Prey);
	bool TakeUpHunt(APawn* Prey, bool bHit);
	void UpdatePursuit(float Interval);
	void GiveUpHunt();
	void ResumeHunt();
	void SetHuntState(ECreatureState NewState);
	void TickSearch(float DeltaSeconds);
	void TickSniff(float DeltaSeconds);
	void TickProwl(float DeltaSeconds, const APawn& Prey);

	// --- Its turn in a fight (CreatureBaseFight.cpp; UFightDirectorSubsystem) ---
	void TickChaseFight(const APawn& Victim, float DeltaSeconds);
	void SetWaitFacing(bool bFacing);
	void EndFightTurn();
	void LeaveFightTurn();
	void LoseTurnToHit();
	void ResumeAfterHit();

	// --- Its notice beat (CreatureBaseReact.cpp) ---
	void BeginNotice(ECreatureState From);
	bool HoldForNotice(const APawn& Prey, float DeltaSeconds);

	// --- Steering (CreatureBaseSteering.cpp) ---
	void MoveToward(const FVector& Goal, float Speed, float DeltaSeconds);
	FVector ChooseDirection(const FVector& Desired, const FVector& Goal, float Speed);
	FCreatureSteerPlanner::FRequest MakeSteerRequest(const FVector& Goal, const FVector& Desired, float Speed) const;
	bool IsDirectionClear(const FVector& Direction) const;
	float LookAboveUnderRoof(const FVector& From, float Above) const;

	// --- Getting free when it's stuck (CreatureBaseUnstick.cpp; FCreatureUnstick has the rules) ---
	void TickUnstick(float DeltaSeconds, const FVector& Goal, float Speed);
	bool TickNudge(float DeltaSeconds);
	bool NudgeFree(bool bHung);
	void FaceToward(const FVector& Point, float DeltaSeconds);
	bool PickWanderGoal();
	void SnapToGround();
	void Respawn();
	void UpdateHealthBar(float DeltaSeconds);

	// --- Room between creatures (CreatureBaseSpacing.cpp): the crowd's push, bending its way, a shuffle aside standing ---
	void ForEachCreature(TFunctionRef<void(ACreatureBase&)> Visit) const;
	FVector SpacingPushNow() const;
	FVector SpacedDirection(const FVector& Desired);
	void KeepSpacing(float DeltaSeconds);

	// --- Rank, level and size (CreatureBaseRank.cpp) ---
	void BeginRankAndLevel();
	void RespawnRankAndLevel();
	ECreatureRank RollArrivalRank();
	int32 RollOwnLevel(ECreatureRank Rank) const;
	void TakeRankAndLevel(ECreatureRank Rank, int32 OwnLevel);
	void ApplyRank();
	void ApplyStats();
	void ApplySize();
	void ApplyTraits();
	void CaptureBaseStats();

	// --- Update rate (CreatureBaseUpdateRate.cpp) ---
	void TickUpdateRate(float DeltaSeconds);
	void RefreshUpdateRate();
	void WakeUpdateRate();
	void ApplyUpdateInterval(float Interval);
	void SetPoseFrozen(bool bFrozen);

	using FViewerMeasure = FCreatureViewerMeasure;
	FViewerMeasure MeasureViewers() const;

	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(AController* Killer);

	ECreatureState State = ECreatureState::Idle;
	TWeakObjectPtr<APawn> Target;
	/** How its hunt is going: searching, waiting where it can't reach, why it gave up (CreatureBasePursuit.cpp). */
	FCreaturePursuit Pursuit;
	/** Seconds its press past its ground's edge has left (NotePressed). */
	float PressedLeft = 0.f;
	FTransform Home;
	FVector WanderGoal = FVector::ZeroVector;
	float StateTime = 0.f;
	float IdleDuration = 2.f;
	float PerceptionTimer = 0.f;
	float CooldownRemaining = 0.f;
	bool bStruck = false;
	/** Held back from fighting (SetPassive). */
	bool bPassive = false;
	/** Seconds its brain stays held for an entrance (HoldBrain). */
	float BrainHold = 0.f;

	// Its turn in a fight (CreatureBaseFight.cpp)
	/** Waiting its turn on the ring (IsWaitingTurn). */
	bool bWaitingTurn = false;
	/** It faces its target as it moves round the ring (its walking movement doesn't turn it while this is on). */
	bool bWaitFacing = false;
	/** Hit off its turn in its wind-up: it must have a turn again when it comes to, or its attack is called off. */
	bool bAttackNeedsTurn = false;

	// Steering
	FVector SteerDirection = FVector::ZeroVector;
	float SteerTimer = 0.f;
	/** Seconds it has been making its way in its current state: SetState zeroes it, which starts its steering afresh. */
	float StuckTime = 0.f;
	/** Its way round obstacles (FCreatureSteerPlanner) and its unstick's timers and glide (FCreatureUnstick). */
	FCreatureSteerPlanner SteerPlanner;
	FCreatureUnstick Unstick;
	/** The crowd's push (SpacingPushNow), looked at again when SpacingTimer runs out. */
	FVector SpacingPush = FVector::ZeroVector;
	float SpacingTimer = 0.f;

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
	/** The roll that promoted it this arrival (its bits), for GetTierSeed; 0 for a rank it was placed or spawned with. */
	int32 TierRoll = 0;
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
