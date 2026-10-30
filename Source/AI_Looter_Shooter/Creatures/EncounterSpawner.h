#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/HuntingGround.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "Story/StoryCondition.h"
#include "EncounterSpawner.generated.h"

class AController;
class ACreatureBase;
class APawn;
class UAreaDefinition;
class UEncounterSubsystem;
class ULineBatchComponent;
class UStoryLineSet;

/** Where an encounter spawner stands. */
UENUM(BlueprintType)
enum class EEncounterState : uint8
{
	/** Its story condition doesn't hold: nothing spawns, and what it had out goes once nobody sees it. */
	Off,
	/** On, waiting for the player to come near or for a trigger. Creatures taken away while the player was far are owed back. */
	Waiting,
	/** Its creatures are out, and its waves come on their timing. */
	Engaged,
	/** Every wave came and every creature it spawned is dead: it's done for this visit. */
	Cleared,
};

/** One look for its creatures' spots (AEncounterSpawner::SpawnSpotTries): how far from the player, and how far apart (cm). */
struct FEncounterSpotTry
{
	float ClearOfPlayer = 0.f;
	float Spacing = 0.f;
};

/**
 * An encounter (Docs/Areas/RansomsRest.md, "Spawners"): creatures spawned in play when the player comes near, all on one
 * level of ground, switched on and off by the story. Placed where a group fights (the town gate, the chapel yard, the
 * barn yard, boot hill's roaming Unpaid, the Gravemother's den: a Legendary monster's lair, LegendaryId), with:
 *  - groups (FEncounterGroup): any creature class, how many, their ranks (the area's promotions, chances of their own, or
 *    one rank: "4, one of them Restless" is three of a group and one of a Restless group), level and size;
 *  - where they stand: spots round it within SpawnRadius, or its SpawnPoints. A spot whose ground is more than
 *    MaxGroundStep above or below the spawner's is skipped, so no group is split by a change of level (under a roof, such
 *    as Tilly's cellar's vault, only ground under it is looked for: the yard over a cellar is another storey, though less
 *    than a step up); so are spots in a
 *    safe zone that's on, outside the playable area, off its hunting ground, on top of an obstacle (the floor inside the
 *    obstacle it stands in, such as the den in Den Rock, is ground), with no room at all (in a building's hull, a rock, a
 *    tree), or too near the player. Spots with room for the largest body its groups bring come first; with too few of
 *    those, the roomiest of the rest (FEncounterGroundProbe);
 *  - its hunting ground (FHuntingGround): its creatures hunt only players within GiveUpRadius of it, or inside
 *    GroundCorners (a fence's line), and give up and go home when their target leaves it; in a fight that comes for the
 *    player (bHuntOnSpawn), one out of the fight while the player is back on its ground is recalled, and brought back on
 *    its ground out of sight if it doesn't come (UpdateStragglers), so none holds a mission's count short;
 *  - waves, if it has more than one: WaveInterval apart, or each once the last is dead (bWaitForClear), at most MaxAlive at
 *    once and MaxTotal in all;
 *  - when: ActiveWhen (FStoryCondition, with FromStep for a mission's step), read again whenever the missions change. A
 *    trigger starts a wave too: an encounter event named WaveEvent (UEncounterSubsystem::SendEvent), TriggerWave from code,
 *    or Looter.Encounter.Wave.
 *
 * Its first wave comes when the player comes within ActivationRadius (or on a trigger). Once the player is past
 * DespawnRadius, with none of its creatures in view or fighting, it takes away what's left alive: those are owed, not
 * killed, and come back when the player does. A killed one never comes back (ACreatureBase::SpawnAtRuntime): waves only
 * ever add new ones, up to their count. Every spawn keeps to the level's caps (UEncounterSubsystem): 20 creatures within
 * 80 m of the player, and a kind's own cap (the Unpaid's 16); what a cap holds back is owed, and comes as soon as there's
 * room. It never ticks: a timer looks twice a second while it's on, and an idle spawner has none.
 *
 * What it has done isn't saved: a level loaded again starts every encounter over, as a mission's objectives start their
 * step over. Only a Legendary lair's monster keeps its death in the session: it's away, the lair cleared, on every
 * arrival until 20 minutes of play have passed since.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AEncounterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AEncounterSpawner();

	// --- What it spawns ---

	/** Its name in logs and console commands (Looter.Encounter.Wave TownGate). None: the actor's name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	FName SpawnerId;

	/** The kinds of creature it brings: any creature class, how many each wave, their ranks, level and size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TArray<FEncounterGroup> Groups;

	/** Tags every creature it spawns carries: a mission counts their kills by tag (Unpaid_TownGate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TArray<FName> CreatureTags;

	/**
	 * A Legendary monster's lair (the Gravemother's den): the id the monster is known by in the session ("Gravemother").
	 * Its death is noted with the map's world (USessionSubsystem::NoteLegendaryDefeat), and it's back only on an arrival
	 * at least 20 minutes of play after it (USessionSubsystem::IsLegendaryBack): until then each visit starts cleared,
	 * whatever its story says. None: an ordinary encounter.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	FName LegendaryId;

	/**
	 * Its creatures go for the player as they appear (a wave in a fight), if the player is on their hunting ground. Off:
	 * they stand until they notice someone.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	bool bHuntOnSpawn = false;

	// --- When ---

	/** It's on only while this holds (an empty condition: always), read again whenever the missions change. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|When")
	FStoryCondition ActiveWhen;

	/**
	 * With ActiveWhen.DuringMission: only from this step of it on, counted from 1 as the Missions page counts them
	 * ("Fight off the Unpaid at the town gate" is Main 3's step 2). 0: any step.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|When", meta = (ClampMin = "0"))
	int32 FromStep = 0;

	/** An encounter event that starts a wave here (UEncounterSubsystem::SendEvent): a boss's call, an egg sac bursting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|When")
	FName WaveEvent;

	/** The first wave comes when the player comes within ActivationRadius. Off: only a trigger brings it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|When")
	bool bSpawnOnApproach = true;

	/** How near (cm) the player comes before the first wave appears, and before creatures taken away come back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|When", meta = (ClampMin = "0", Units = "cm"))
	float ActivationRadius = 4500.f;

	/**
	 * With the player farther than this (cm), and none of its creatures in view, fighting or near the player, it takes away
	 * what's left alive (owed back for the player's return). 0: never.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|When", meta = (ClampMin = "0", Units = "cm"))
	float DespawnRadius = 9000.f;

	// --- Where they stand ---

	/** Without SpawnPoints, its creatures appear round it within this (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Where", meta = (ClampMin = "0", Units = "cm"))
	float SpawnRadius = 800.f;

	/** Spots of their own, relative to the spawner: each taken in turn, with more creatures than spots spread round them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Where", meta = (MakeEditWidget = true))
	TArray<FVector> SpawnPoints;

	/** Its creatures appear at least this far apart (cm), and as far from ones of its standing there already. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Where", meta = (ClampMin = "50", Units = "cm"))
	float Spacing = 250.f;

	/**
	 * A spot whose ground is more than this (cm) above or below the spawner's is on another level, and skipped: creatures
	 * never step off a drop of more than about 4 m, so a group on two levels would fight as two.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Where", meta = (ClampMin = "50", Units = "cm"))
	float MaxGroundStep = 400.f;

	// --- Its hunting ground (FHuntingGround) ---

	/**
	 * Its creatures hunt only players within this (cm, seen from above) of the spawner, and give up and go home when their
	 * target goes farther. 0, with no GroundCorners: they hunt anywhere, as far as their LoseInterestRadius.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Hunting Ground", meta = (ClampMin = "0", Units = "cm"))
	float GiveUpRadius = 3000.f;

	/** Its hunting ground as a polygon instead (world X and Y, cm; three corners or more): a fence's line. Spots lie inside it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Hunting Ground")
	TArray<FVector> GroundCorners;

	/** Past the polygon's edge this far (cm) still counts as on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Hunting Ground", meta = (ClampMin = "0", Units = "cm"))
	float GroundMargin = 200.f;

	/** A player more than this (cm) above or below the spawner isn't on its creatures' ground (a pit's rim). 0: any height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Hunting Ground", meta = (ClampMin = "0", Units = "cm"))
	float GroundMaxRise = 600.f;

	// --- Waves ---

	/** How many waves it brings (each spawns every group that joins it). 0: as many as MaxTotal allows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Waves", meta = (ClampMin = "0"))
	int32 NumWaves = 1;

	/**
	 * Seconds from one wave to the next: from the last one's start, or with bWaitForClear from the moment its creatures are
	 * all dead. 0 without bWaitForClear: later waves come only on a trigger.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Waves", meta = (ClampMin = "0", Units = "s"))
	float WaveInterval = 0.f;

	/** The next wave waits until the last one's creatures are all dead (then WaveInterval more). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Waves")
	bool bWaitForClear = false;

	/** At most this many of its creatures alive at once: a wave tops up to it, the rest wait their turn. 0: no cap of its own. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Waves", meta = (ClampMin = "0"))
	int32 MaxAlive = 0;

	/** At most this many creatures from its waves in all. 0: as many as its waves bring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Waves", meta = (ClampMin = "0"))
	int32 MaxTotal = 0;

	/** Seconds between its looks at the player and its waves while it's on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Waves", meta = (ClampMin = "0.1", Units = "s"))
	float CheckSeconds = 0.5f;

	/**
	 * Said once, on the move, as the last of its creatures falls (a fight won: not one sent away), queued after whatever
	 * is being said: a line set of /Game/Data/Story (Tools/Unreal/create_story_lines.py). None: nothing. Unfinished
	 * Business's twist is its first (Amos naming his old hands as they go down; the user's call, 2026-10-21: story said
	 * on the move, after the fight's noise, not at a giver).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Story")
	TObjectPtr<UStoryLineSet> ClearedLines;

	// --- Playing it ---

	/**
	 * Starts its next wave now (a trigger: its WaveEvent, a boss's code, the console), whether the player is near or not;
	 * false when it's off, cleared or out of waves. bForce turns it on whatever its story says, and starts a cleared one
	 * over, until the level is loaded again (the console's way to try an encounter).
	 */
	bool TriggerWave(bool bForce = false);

	/**
	 * Looks at the player and moves its waves on by DeltaSeconds: its timer calls it every CheckSeconds. A test level never
	 * runs timers, so the tests call it themselves.
	 */
	void UpdateEncounter(float DeltaSeconds);

	/** Reads its story condition again and switches it on or off (UEncounterSubsystem calls it whenever the missions change). */
	void RefreshStory();

	/** Takes away every creature it has out: owed, not killed, they come back when the player does. How many it took. */
	int32 Despawn();

	/** Seeds its rolls (ranks, where its creatures stand), for tests; play seeds them anew as it begins. */
	void SetRandomSeed(int32 Seed);

	/**
	 * Its Legendary monster is away this visit (beaten too lately): the encounter is cleared until the level loads again,
	 * and what it had out goes. Play asks the session as it begins; the tests and the console say so themselves. A forced
	 * TriggerWave (Looter.Encounter.Wave <id> force, Looter.Legendary.Return) brings it back.
	 */
	void SendLegendaryAway();

	/** Its Legendary monster is away this visit. */
	bool IsLegendaryAway() const { return bLegendaryAway; }

	// --- What's going on ---

	/** SpawnerId, or the actor's name. */
	FName GetSpawnerId() const;

	EEncounterState GetState() const { return State; }

	/** Its story condition held when it last looked (or the console forced it on). */
	bool IsStoryActive() const { return bStoryActive; }

	/** Its creatures alive now. */
	int32 NumAlive() const;
	TArray<ACreatureBase*> GetAliveCreatures() const;

	/** Creatures still to come: held back by a cap, or taken away while the player was far. */
	int32 NumOwed() const { return Owed.Num(); }

	int32 GetWavesStarted() const { return WavesStarted; }

	/** How many waves it plays (EncounterRules::PlannedWaves; a very large number for "as many as its total allows"). */
	int32 GetPlannedWaves() const;

	/** Creatures its waves have brought in all (one taken away and brought back counts once). */
	int32 GetTotalSpawned() const { return TotalQueued; }

	/** Its creatures killed (or lost off the world): none of them comes back. */
	int32 GetKilled() const { return Killed; }

	/** The hunting ground it gives its creatures: GiveUpRadius round it, or GroundCorners. */
	FHuntingGround MakeHuntingGround() const;

	/** Its state for people: "Engaged, on (during Main3, from step 2): waves 1/1, 3 alive, 1 owed, 0 killed, 4 spawned". */
	FString Describe() const;

	/** Draws its hunting ground, where its creatures appear and how near the player comes before they do, as lines in BatchID. */
	void DrawEncounter(ULineBatchComponent& Lines, uint32 BatchID) const;

	static FString GetStateName(EEncounterState InState);

	/**
	 * The looks it makes for its creatures' spots, in turn, until one finds any, so a fight never spawns nothing while its
	 * ground has room: its rules' (ClearOfPlayer from the player, Spacing apart); then 3.5 m from the player, the farthest
	 * spots first (a room the player stands in or over: the depot's waiting room lies within 8 m of Mr. Sloane's door
	 * upstairs, and the waiting room's fights spawned nothing, 2026-10-23); then as close together as BodyRadius allows
	 * (the baggage room's rats on a 1.4 m ring); then 2 m from the player. Repeats are left out.
	 */
	static TArray<FEncounterSpotTry> SpawnSpotTries(float ClearOfPlayer, float Spacing, float BodyRadius);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// --- What a kind of encounter does its own way (APatrolSpawner, AAmbushSpawner); the defaults are a plain encounter's ---

	/**
	 * Looks at the player while it waits (every CheckSeconds): whether their approach brings its creatures now. Plain: the
	 * player within ActivationRadius of GetSpawnCenter. An ambush springs on the player walking onto its ground instead.
	 */
	virtual bool CheckApproach();

	/** A creature it just spawned, before it's set on the player (bHuntOnSpawn): an ambush's entrance (rising, dropping). */
	virtual void OnCreatureSpawned(ACreatureBase& Creature) {}

	/**
	 * Whether its creatures arrive their own way (OnCreatureSpawned: an ambush's rising or dropping). Otherwise one that
	 * appears where a player can see it comes its kind's way (ACreatureBase::PlayEntranceIfSeen) rather than popping in.
	 */
	virtual bool HasOwnEntrance() const { return false; }

	/** Whether its creatures may roll Touched (ACreatureBase::bMayBeTouched): a hand-built fight (a set piece, a bounty) says no. */
	virtual bool LetsCreaturesBeTouched() const { return true; }

	/** Moves its pack on by DeltaSeconds at each look while it's on (waiting or engaged): a patrol's walk. */
	virtual void UpdatePack(float DeltaSeconds) {}

	/** Where its creatures appear round and the player's distance is measured from: its own spot, or a patrol's point on its route. */
	virtual FVector GetSpawnCenter() const;

	/** Its state just changed (a set piece's beats and its end, ASetPieceSpawner); nothing by default. */
	virtual void OnStateChanged(EEncounterState From, EEncounterState To) {}

	/** Wave Wave (counted from 1) is starting, before its creatures are queued and spawned: a set piece moves its spots. */
	virtual void OnWaveStarted(int32 Wave) {}

	/** The player within Distance (cm) of GetSpawnCenter. */
	bool IsPlayerWithin(float Distance) const;
	UEncounterSubsystem* GetEncounters() const;

private:
	/** One creature still to come: its group, and the rank it was rolled (one taken away keeps its own). */
	struct FOwedCreature
	{
		int32 Group = INDEX_NONE;
		ECreatureRank Rank = ECreatureRank::Basic;
	};

	/** One of its creatures out in the level, and its group. */
	struct FLivingCreature
	{
		TWeakObjectPtr<ACreatureBase> Creature;
		int32 Group = INDEX_NONE;
		/** Seconds out of the fight while the player stands on its ground, and since it was last recalled (UpdateStragglers). */
		float OutOfFightSeconds = 0.f;
		float SinceRecallSeconds = 0.f;
	};

	// --- Its state, the story and its timer (EncounterSpawner.cpp) ---
	void SetState(EEncounterState NewState);
	/** Its story ended: nothing more spawns, and a later start is a fresh encounter. */
	void GoOff();
	/** Starts or stops its timer for what its state needs. */
	void UpdateTimer();
	void HandleCheckTimer();
	bool WantsChecks() const;
	/** Whether the player coming near would bring anything: owed creatures, its first wave, or more timed waves. */
	bool WantsApproach() const;

	// --- Waves, spawning and taking away (EncounterSpawnerWaves.cpp) ---
	void Engage();
	void StartWave();
	/** Spawns what's owed, as many as the caps and its ground allow; how many. */
	int32 SpawnOwed();
	ACreatureBase* SpawnOne(const FOwedCreature& Entry, const FVector& Feet, APawn* Player);
	/** Drops the dead from its list, counting them killed. */
	void PruneLiving();
	/** Removes every living creature it has out (owed back when bOweThem); how many. */
	int32 RemoveLiving(bool bOweThem);
	bool HasWavesLeft() const;
	bool HasTimedWaves() const;
	bool ShouldDespawn() const;
	bool IsAnyInView() const;
	bool IsAnyFighting() const;
	/** Creature is in the player's view, or was drawn on screen a moment ago. */
	bool IsInView(const ACreatureBase& Creature) const;
	const UAreaDefinition* FindArea() const;

	// --- Sent away (EncounterSpawnerLeaving.cpp) ---
	/**
	 * Its story moved on while its creatures were seen or fighting: each breaks off (ACreatureBase::SetPassive: it lets go
	 * of the player and walks home) and leaves its fight for Leaving, so a later start of its story is a fresh encounter.
	 */
	void SendLivingAway();
	/** Those sent away go once nobody sees them, or LeaveAtMostSeconds after they were sent. */
	void UpdateLeaving(float DeltaSeconds);

	// --- Stragglers (EncounterSpawnerStragglers.cpp) ---
	/**
	 * A fight that came for the player (bHuntOnSpawn), while the player is on its ground: each creature out of the fight is
	 * recalled, and one that stays out of it out of sight is taken away and owed, to come again at once (EncounterRules::
	 * StragglerCall). A creature never holds its fight's count short by standing somewhere out of the way.
	 */
	void UpdateStragglers(float DeltaSeconds);
	/** The player can be fought here: alive, out of a safe zone, within ActivationRadius and on its hunting ground. */
	bool IsPlayerOnFightGround(const APawn& Player) const;

	// --- Where they stand (EncounterSpawnerSpots.cpp) ---
	/**
	 * Spots for Wanted creatures on its ground (EncounterRules::ChooseSpots): those with room for the largest body its
	 * groups bring (EncounterRules::LargestBody) first, then the roomiest of the rest; fewer when its ground has fewer.
	 * Looked for by SpawnSpotTries in turn: OutTry gets the one that found them (0: its own rules), OutWhy, when none
	 * did, what ruled its candidates out.
	 */
	TArray<FVector> ChooseSpawnSpots(int32 Wanted, const APawn* Player, int32* OutTry = nullptr, FString* OutWhy = nullptr) const;

	/** One of its creatures died: a Legendary lair's monster is noted beaten in the session. */
	UFUNCTION()
	void HandleCreatureDeath(AController* Killer);

	EEncounterState State = EEncounterState::Off;
	bool bStoryActive = false;
	bool bStoryApplied = false;
	/** The console turned it on (TriggerWave with bForce), whatever its story says. */
	bool bForced = false;
	/** Behind the main menu: it stays off. */
	bool bMenuWorld = false;
	/** It warned once that its creatures found nowhere to stand. */
	bool bWarnedNoRoom = false;
	/** It said once that its creatures came nearer the player, or closer together, than its rules ask (SpawnSpotTries). */
	bool bNotedCloseSpots = false;
	/** Its Legendary monster is away this visit (SendLegendaryAway). */
	bool bLegendaryAway = false;

	int32 WavesStarted = 0;
	/** Creatures its waves have brought, against MaxTotal. */
	int32 TotalQueued = 0;
	int32 Killed = 0;
	/** Seconds since the last wave started (or, waiting for a clear, since its creatures were all gone). */
	float WaveClock = 0.f;

	TArray<FOwedCreature> Owed;
	TArray<FLivingCreature> Living;
	/** Creatures sent away when its story moved on (SendLivingAway), and how long since. */
	TArray<TWeakObjectPtr<ACreatureBase>> Leaving;
	float LeavingSeconds = 0.f;
	FRandomStream Rolls;
	FTimerHandle CheckTimer;
};
