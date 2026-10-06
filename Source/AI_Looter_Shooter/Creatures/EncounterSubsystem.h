#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EncounterSubsystem.generated.h"

class AActor;
class ACreatureBase;
class AEncounterSpawner;
class APawn;
class ASafeGround;
class UMissionRunner;
struct FMissionEvent;
struct FStoryCondition;

/**
 * The level's encounters: its spawners (AEncounterSpawner) and safe zones (ASafeGround), which join as their play begins,
 * and a list of its creatures (the placed ones found once as play begins, spawned ones as they appear), so the caps are
 * counted without looking through the level's actors:
 *  - at most 16 creatures of any kind alive within 80 m of the player (UEncounterSettings), which spawners keep to before
 *    spawning;
 *  - at most so many of a kind at once (the Unpaid's 12), from the settings or a spawner's group.
 * The story switches spawners and zones on and off: whenever the missions change (UMissionRunner::OnChanged) it has them
 * read their conditions again. Encounter events (SendEvent) start waves at the spawners listening for them: a boss's call,
 * an egg sac bursting, a script. Played worlds, and the editor preview worlds the automated tests build.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEncounterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UEncounterSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitialize() override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// --- Spawners and safe zones (each joins as its play begins and leaves as it ends) ---

	void RegisterSpawner(AEncounterSpawner* Spawner);
	void UnregisterSpawner(AEncounterSpawner* Spawner);
	void RegisterSafeZone(ASafeGround* Zone);
	void UnregisterSafeZone(ASafeGround* Zone);

	/** The spawners and safe zones that joined, still in the level. */
	TArray<AEncounterSpawner*> GetSpawners() const;
	TArray<ASafeGround*> GetSafeZones() const;

	/** The spawner with this id (AEncounterSpawner::GetSpawnerId, any case), or null. */
	AEncounterSpawner* FindSpawner(FName SpawnerId) const;

	/** Whether a point lies in a safe zone that's on now. */
	bool IsInSafeZone(const FVector& Point) const;

	/** The same in WorldContextObject's level; no in a level without encounters (or for a class default, which has none). */
	static bool IsSheltered(const UObject* WorldContextObject, const FVector& Point);

	// --- The story ---

	/**
	 * Whether a condition holds now (an empty one always does), read from the mission runner's campaign record: with
	 * FromStep (counted from 1), its DuringMission must also be on that step or past it. Without a mission runner in the
	 * level only an empty condition holds.
	 */
	bool IsStoryMet(const FStoryCondition& When, int32 FromStep = 0) const;

	/** Every spawner and safe zone reads its condition again. The missions' changes call it; so can tests. */
	void RefreshStory();

	// --- Encounter events ---

	/** Starts a wave at every spawner listening for Event (AEncounterSpawner::WaveEvent); how many started one. */
	int32 SendEvent(FName Event);

	/** The same in WorldContextObject's level (a boss's code, an egg sac); 0 in a level without encounters. */
	static int32 SendEventAt(const UObject* WorldContextObject, FName Event);

	// --- The level's creatures, for the caps ---

	/** A creature joins the count (spawned ones join as they appear; spawners add theirs as well). Twice changes nothing. */
	void TrackCreature(ACreatureBase* Creature);

	/** Living creatures of Class or a child of it (null: of any kind) in the level. */
	int32 CountAlive(const UClass* Class = nullptr) const;

	/** Living creatures within Radius (cm) of Point. */
	int32 CountAliveNear(const FVector& Point, float Radius) const;

	// --- The player ---

	/** The player's pawn (the first player's, or a test's stand-in); null behind the menu, or before one is spawned. */
	APawn* GetPlayer() const;

	/** A test's stand-in for the player: a test level has no player controller. */
	void SetTestPlayer(APawn* StandIn);

	/**
	 * Whether a body (a sphere Radius cm about Point) is in a local player's view, as creatures measure it for their update
	 * rates (FCreatureUpdateRate::IsInView): walls don't hide it, since it could step out any moment.
	 */
	bool IsInPlayersView(const FVector& Point, float Radius) const;

private:
	void HandleActorSpawned(AActor* Actor);
	void HandleMissionsChanged();
	void HandleMissionEvent(const FMissionEvent& Event);

	TArray<TWeakObjectPtr<ACreatureBase>> Creatures;
	TArray<TWeakObjectPtr<AEncounterSpawner>> Spawners;
	TArray<TWeakObjectPtr<ASafeGround>> SafeZones;
	TWeakObjectPtr<APawn> TestPlayer;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle SpawnHandle;
	FDelegateHandle MissionsHandle;
	FDelegateHandle MissionEventHandle;
};
