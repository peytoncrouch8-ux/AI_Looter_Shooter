#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Bosses/BossTypes.h"
#include "Engine/TimerHandle.h"
#include "BossComponent.generated.h"

class ABossSeal;
class AController;
class ACreatureBase;
class APawn;
class UHealthComponent;
class UHudBossBarWidget;

DECLARE_MULTICAST_DELEGATE(FOnBossFight);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBossPhaseChanged, int32 /*NewPhase*/, int32 /*OldPhase*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossCustomEvent, FName /*EventName*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossUntargetableChanged, bool /*bUntargetable*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossStaggered, bool /*bStaggered*/);
DECLARE_DELEGATE_RetVal(bool, FBossCanStagger);

/**
 * Makes a creature a boss: put it on any ACreatureBase (a default subobject of a boss class, or added in play as the test
 * boss does). The creature keeps its own body, brain and attacks; this adds the fight around them, and a creature without
 * one behaves exactly as before. A component rather than a base class, so a boss can be any kind of creature (Abel is
 * an Unpaid, the test boss a spider) without a second creature hierarchy.
 *
 * The fight:
 *  - starts when StartFight is called (a mission's moment), when a player hurts the boss, or when one comes within
 *    EngageRadius of its spot. Its spot is the creature's home. Until then, and again after a reset, the boss waits there
 *    and hunts nobody (its creature is held passive): it never fights without its bar and its wall.
 *  - phases (Phases, by share of health; BossTypes.h) start one at a time as its health falls, each running its events:
 *    waves of adds, untargetable spells, volleys of pellets, and custom moments for the boss's own code.
 *  - adds rise round it with ACreatureBase::SpawnAtRuntime, so they never come back once killed; at most MaxAliveAdds
 *    (never more than 10) are alive at once.
 *  - untargetable: the boss takes hits and loses nothing (no damage numbers either), its bar greys, and it may withdraw to
 *    its spot; until the spell's time is up, its adds are dead, or the boss's code ends it.
 *  - a fog wall (ABossSeal) closes the arena once the player and the boss are both inside: a placed one (Seal), or a ring
 *    of SealRadius round its spot. Until it closes, a boss that strays past LeashRadius goes home and the fight starts over.
 *  - the boss bar (UHudBossBarWidget) shows at the top of the screen for the fight; the creature's own tag steps aside.
 *  - the player's death (their health component's OnDeath) resets it all: the boss heals and goes back to its spot, the
 *    adds go, the wall drops, the bar goes. Nothing in the player's death or respawn changes.
 *  - the boss's death wins it: the adds go, the wall drops, the bar empties and fades, and a boss with a BossId is
 *    recorded as beaten in the campaign. Its rank (Boss) keeps it from coming back, and its loot is its rank's.
 *  - the show (Show, BossComponentShow.cpp): the bar sweeps in with the title and a sting as the fight starts, a later
 *    phase flashes it with a sting, the boss's cry and a camera shake; the death slows the world a moment, shakes it hard
 *    and the bar says it's beaten. Its loot is thrown out as a shower (LootShower, ABossLootShower) in place of the toss.
 *  - its weak spot (Stagger): enough critical damage close together staggers it a moment; its creature does the reeling.
 *  - a boss that lives as its kind does until it turns on someone (bWaitsPassive off, bStartWhenHunting: the Gravemother)
 *    starts its fight as it turns on a player, and stands down when it has had nobody to fight a while (StandDownSeconds).
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UBossComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- What the fight is ---

	/** The campaign's name for it ("Abel"): beating it is recorded once (FCampaignRecord::RecordBossDefeat). None: not recorded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	FName BossId;

	/** The name on its bar ("Abel Ransom, the Keeper"). Empty: the creature's DisplayName. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	FText BossName;

	/** Its phases, by the share of health at which each starts (the first at full health). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	TArray<FBossPhase> Phases;

	/** A player this close to its spot (cm) starts the fight. 0: only StartFight or a hit does. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Fight", meta = (ClampMin = "0", Units = "cm"))
	float EngageRadius = 0.f;

	/** A player's hit starts the fight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Fight")
	bool bStartWhenHurt = true;

	/** Before its wall closes, the boss chasing this far (cm) from its spot goes home and the fight starts over. 0: never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Fight", meta = (ClampMin = "0", Units = "cm"))
	float LeashRadius = 2500.f;

	/** The most adds alive at once (never more than 10: the area's budget, Docs/Areas/RansomsRest.md). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Adds", meta = (ClampMin = "0", ClampMax = "10"))
	int32 MaxAliveAdds = 10;

	/** A fog wall placed in the level for this fight (the Keeper's Gate). None: a ring of SealRadius, if that's set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Seal")
	TObjectPtr<ABossSeal> Seal;

	/** With no placed wall, a ring of this radius (cm) round its spot closes the arena. 0: no wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Seal", meta = (ClampMin = "0", Units = "cm"))
	float SealRadius = 0.f;

	/** Shows the boss bar at the top of the screen during the fight (and hides the creature's own tag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Bar")
	bool bShowBar = true;

	/** How its fight is shown: the title, the cries, the camera's shakes, the slow beat at its death. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Show")
	FBossShow Show;

	/** Its weak spot's stagger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Fight")
	FBossStagger Stagger;

	/** Its loot, thrown out as a shower at its death (with its loot drop component's table, level and luck). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Loot")
	FBossLootShowerSettings LootShower;

	/**
	 * Until its fight starts it waits at its spot, hunting nobody (Abel, the test boss). Off: it lives as its kind does until
	 * the fight starts, and a reset leaves it free too (the Gravemother wandering her den).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Fight")
	bool bWaitsPassive = true;

	/** The fight starts as the boss itself turns on a living player (its own senses, a pack's call). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Fight")
	bool bStartWhenHunting = false;

	/**
	 * With no wall closed, a fight whose boss has had nobody to fight this long (s) stands down: its bar goes and the fight
	 * ends where it is, with no healing and no trip home (the creature walks back on its own). 0: never.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Fight", meta = (ClampMin = "0", Units = "s"))
	float StandDownSeconds = 0.f;

	/** Its death kills its adds (the Gravemother's brood dies with her), rather than their fading with the fight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Adds")
	bool bAddsDieWithBoss = false;

	// --- The fight ---

	/** Starts the fight against Player (none: the first player). Nothing happens if it's on, won, or the boss is dead. */
	void StartFight(APawn* Player = nullptr);

	/** Starts it over: the boss heals and goes back to its spot, its adds go, its wall drops, the bar goes. */
	void ResetFight();

	bool IsFighting() const { return bFighting; }
	bool IsWon() const { return bWon; }
	APawn* GetFightPlayer() const;

	/** The middle of its arena: the creature's home as the fight began. */
	FVector GetSpot() const { return Spot; }

	// --- Phases ---

	/** The phase it's in (0 = the first), as ordered for the fight (GetFightPhases). */
	int32 GetPhase() const { return Phase; }
	int32 NumPhases() const { return FightPhases.Num(); }
	/** The phases as the fight plays them: highest share first (BossRules::Ordered). */
	const TArray<FBossPhase>& GetFightPhases() const { return FightPhases; }
	/** Each phase's share of health, highest first: where the bar's ticks go. */
	TArray<float> GetPhaseShares() const;

	// --- Untargetable ---

	/** It can't be hurt until the spell ends (its time, its adds, or EndUntargetable). A second spell replaces the first. */
	void BeginUntargetable(const FBossUntargetable& Spell);
	/** Ends the spell now (Abel's lanterns relit): it can be hurt, and a boss that withdrew joins the fight again. */
	void EndUntargetable();
	bool IsUntargetable() const { return bUntargetable; }

	// --- Adds ---

	/** Raises a wave round the boss (as many as its caps allow), sets them on the player; returns how many rose. */
	int32 SpawnWave(const FBossAddWave& Wave);
	int32 NumAliveAdds() const;
	TArray<ACreatureBase*> GetAliveAdds() const;
	/** Removes every add still standing (a reset or a win: they fade with the fight). */
	void DespawnAdds();

	/**
	 * Takes a creature the boss's own code raised (the Gravemother's brood) as one of its adds: counted toward its caps, its
	 * pellets put out, gone with the fight (or killed with the boss, bAddsDieWithBoss) like any of them.
	 */
	void AdoptAdd(ACreatureBase* Add);

	// --- The weak spot (BossComponentShow.cpp) ---

	/** Staggered now: its creature reels (OnStaggered), open to punishment. */
	bool IsStaggered() const { return bStaggered; }

	/** How near a stagger its critical hits have brought it: 0 nothing, 1 it staggers. */
	float GetStaggerBuildUp() const { return StaggerBuildUp; }

	/**
	 * Staggers it now for Stagger.Seconds (its build-up full, or the console's say): false while it can't (not fighting,
	 * dead, untargetable, staggered already, or its creature's CanStagger says no).
	 */
	bool BeginStagger();

	/** Ends a stagger now (its creature's code, a reset); it builds nothing up for Stagger.Cooldown after. */
	void EndStagger();

	/** Its creature's say on whether a stagger may start now (bound by a boss class; unbound: yes). */
	FBossCanStagger CanStagger;

	/** Its death's slow beat is running (the world runs at the slowest any dying boss asks for; EndPlay lets its claim go). */
	bool IsDeathSlowOn() const { return bDeathSlowOn; }

	// --- Volleys ---

	/** A volley at the player after its tell (UEnemyProjectileSubsystem). One winds up at a time. */
	void FireVolley(const FBossVolley& Volley);
	bool IsVolleyWindingUp() const { return bVolleyPending; }

	// --- The wall and the bar ---

	/** The wall closing this fight's arena: the placed one, or the ring it made. Null until a ring is first needed. */
	ABossSeal* GetActiveSeal() const;
	bool IsSealRaised() const;

	/** The bar is wanted on screen (whether or not there's a player to show it to: a test level has none). */
	bool IsBarShown() const { return bBarWanted; }
	UHudBossBarWidget* GetBar() const;

	/** Plays the fight on by DeltaSeconds: its tick. A test level doesn't tick, so the tests call it themselves. */
	void TickFight(float DeltaSeconds);

	ACreatureBase* GetCreature() const;
	UHealthComponent* GetBossHealth() const;

	/** The fight started, started over (the player died), or was won (the boss died). */
	FOnBossFight OnFightStarted;
	FOnBossFight OnFightReset;
	FOnBossFight OnFightWon;
	/** A phase started (the first too, from INDEX_NONE). */
	FOnBossPhaseChanged OnPhaseChanged;
	/** A Custom event's moment came, for the boss's own code ("Bell", "Grieve"). */
	FOnBossCustomEvent OnCustomEvent;
	FOnBossUntargetableChanged OnUntargetableChanged;
	/** A stagger started, or ended. */
	FOnBossStaggered OnStaggered;

	/** After a win the empty bar stays this long (seconds) before it fades. */
	static constexpr float BarHideDelay = 3.f;
	/** The player must be this far (cm) inside the wall's line for it to close behind them. */
	static constexpr float SealMargin = 150.f;
	/** Adds rise only on the boss's own level of ground: no more than this (cm) above or below its feet. */
	static constexpr float MaxAddRise = 300.f;
	/** A player outside the closed wall this long (seconds) starts the fight over, rather than being shut out of it. */
	static constexpr float ShutOutSeconds = 3.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// --- The fight's course (BossComponent.cpp) ---
	UFUNCTION()
	void HandleBossDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);
	UFUNCTION()
	void HandleBossHealthChanged(float NewHealth, float MaxHealth);
	UFUNCTION()
	void HandleBossDeath(AController* Killer);
	UFUNCTION()
	void HandlePlayerDeath(AController* Killer);

	/** Its name for the log: its bar's name, or the actor's. */
	FString GetLabel() const;
	/**
	 * What a reset, a stand-down and a win all do: the pellets, wall, spell, stagger and the player's death hook go; the adds
	 * go too (bWon with bAddsDieWithBoss: they die) unless bKeepAdds.
	 */
	void ClearFight(bool bFightWon = false, bool bKeepAdds = false);
	/** No one to fight a while with no wall closed: the fight ends where it is (no healing, no trip home). */
	void StandDown();
	void BindPlayer(APawn* Player);
	void UnbindPlayer();
	APawn* FindPlayerNear(const FVector& Center, float Radius) const;

	// --- The wall and the bar (BossComponentArena.cpp) ---
	/** The wall for this fight, making the ring round its spot the first time one is needed. */
	ABossSeal* EnsureSeal();
	/** Closes the wall once the player and the boss are both inside it. */
	void TryRaiseSeal();
	void DropSeal();
	void ShowBar();
	void HideBar();
	void UpdateBar();

	// --- Phases, spells and volleys (BossComponentPhases.cpp) ---
	/** Starts every phase its health has fallen to, one at a time. */
	void EvaluatePhase();
	void EnterPhase(int32 NewPhase);
	/** Runs the current phase's events that are due. */
	void RunDueEvents();
	void RunEvent(const FBossPhaseEvent& Event);
	/** A wave of the current phase hasn't risen yet (an untargetable spell waiting for the adds waits for it too). */
	bool HasPendingWaves() const;
	void TickUntargetable(float DeltaSeconds);
	/** Ends the spell; bRejoin sets a withdrawn boss on the player again (not when the fight itself is ending). */
	void StopUntargetable(bool bRejoin);
	void TickVolley(float DeltaSeconds);
	void ReleaseVolley();
	/** Puts out every pellet and tell the boss and its adds have going. */
	void ClearShots();

	// --- The show, the weak spot and the loot (BossComponentShow.cpp) ---
	/** The fight starts: the bar sweeps in with its title, the sting, the boss's cry, the shake. */
	void PlayIntro();
	/** A later phase starts: the sting, the boss's cry, the shake (the bar flashes by itself). */
	void PlayPhaseTell();
	/** It died: the slow beat, the hard shake, the sting and its cry, the bar's last word (bWasFighting), and its loot. */
	void PlayDeath(bool bWasFighting);
	/** Its loot drop component's toss turned off for the shower, if the shower is on (BeginPlay). */
	void TakeOverLoot();
	void ThrowLoot();
	/** A critical hit's damage toward a stagger. */
	void AddCritDamage(float Damage);
	void TickStagger(float DeltaSeconds);
	void PlayCue(FName Cue) const;
	void Shake(float Strength, float Seconds) const;
	void StartDeathSlow();
	void EaseDeathSlow();
	void EndDeathSlow();
	/** Kills every add still standing (bAddsDieWithBoss at the win). */
	void KillAdds();

	// --- Adds (BossComponentAdds.cpp) ---
	/** Forgets adds that are dead or gone: they never come back. */
	void PruneAdds();
	/** The ground under Point on the boss's level (FeetZ), or false (off an edge, up a rock). Without ground anywhere near the boss (a test level), level with its feet. */
	bool FindAddSpot(const FVector& Point, float FeetZ, bool bGroundUnderBoss, FVector& OutFeet) const;
	bool TraceGround(const FVector& Point, FVector& OutGround) const;

	/** One event of the current phase waiting for its time. */
	struct FScheduledEvent
	{
		int32 Event = INDEX_NONE;
		float NextTime = 0.f;
		/** It has happened at least once. */
		bool bFired = false;
		/** A once-only event that has happened. */
		bool bDone = false;
	};

	TArray<FBossPhase> FightPhases;
	TArray<FScheduledEvent> Scheduled;
	int32 Phase = INDEX_NONE;
	float PhaseTime = 0.f;
	bool bFighting = false;
	bool bWon = false;
	FVector Spot = FVector::ZeroVector;
	TWeakObjectPtr<APawn> FightPlayer;
	TWeakObjectPtr<UHealthComponent> BoundPlayerHealth;
	/** The creature's own tag setting, given back when the fight ends. */
	bool bSavedShowsTag = true;
	/** Seconds the player has been outside the closed wall. */
	float PlayerOutsideTime = 0.f;

	bool bUntargetable = false;
	FBossUntargetable ActiveSpell;
	float SpellTime = 0.f;
	/** The health component's own settings while no spell holds, and whether the creature was held back already. */
	bool bSavedInvulnerable = false;
	bool bSavedShowNumbers = true;
	bool bSavedPassive = false;
	bool bWithdrawn = false;

	bool bVolleyPending = false;
	FBossVolley PendingVolley;
	float VolleyTimeLeft = 0.f;

	TArray<TWeakObjectPtr<ACreatureBase>> Adds;

	/** The ring it made round its spot, kept for later fights. */
	UPROPERTY(Transient)
	TObjectPtr<ABossSeal> SpawnedSeal;

	UPROPERTY(Transient)
	TObjectPtr<UHudBossBarWidget> Bar;
	bool bBarWanted = false;
	/** Seconds left before the bar fades after a win. */
	float BarHideTime = 0.f;

	/** Seconds its boss has had nobody to fight (StandDownSeconds). */
	float NoTargetTime = 0.f;

	/** Its loot drop component's own toss was turned off: the shower throws its loot. */
	bool bLootTakenOver = false;

	bool bStaggered = false;
	float StaggerBuildUp = 0.f;
	float StaggerTimeLeft = 0.f;
	float StaggerCooldownLeft = 0.f;

	/** Its death's slow beat: on, and the beat's timer. */
	bool bDeathSlowOn = false;
	FTimerHandle DeathSlowTimer;
};
