#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActorIndexSubsystem.generated.h"

class AActor;
class AAmmoPickup;
class ACharacter;
class ACreatureBase;
class AGrenadePickup;
class APlayableArea;
class AShieldPickup;
class ASilverPickup;
class AVolume;
class AWeaponBase;
class UWorld;

/** A creature in the index, with the one thing the minimap asks of its kind (answered once per look, not per frame). */
struct FIndexedCreature
{
	TWeakObjectPtr<ACreatureBase> Creature;
	/** A Stone Sentinel: statues are left off the map while dormant (AStoneSentinelCreature::IsDormant). */
	bool bSentinel = false;
};

/**
 * The world's loot, creatures, characters and volumes as lists, so the HUD, the loot labels, the interaction component and
 * the enemy shots read a few dozen entries instead of walking the world's actors with a TActorIterator every frame (the
 * audit of 2026-11-03 counted six of those a frame in normal play). Lists:
 *  - guns (every AWeaponBase: the ones lying as loot and the ones carried; callers keep IsPickup() ones),
 *  - ammo, grenade, silver and ward pickups,
 *  - creatures (living at the last look) and characters that are not creatures (the players, friends),
 *  - volumes and playable areas (what an enemy shot's trace leaves out).
 * Interactables (IInteractable props) already have their own registry, UInteractionSubsystem.
 *
 * How it stays right: a full look through the world (one TActorIterator pass per list) once at the start of play and then
 * every Looter.ActorIndex.Interval seconds (0.1) on a timer, never per frame; between looks an actor that spawns joins its
 * list at once (the world's spawn hook, as UGroundIgnoreSubsystem and UEncounterSubsystem do), so a gun dropped this frame
 * is offered this frame, and one destroyed goes null in the list (weak pointers) and leaves it at the next look. A reader
 * checks what can change between looks itself (IsPickup, IsDead, IsHidden) and skips nulls.
 *
 * Played worlds, and the editor preview worlds the automated tests build (which never tick: the spawn hook and the
 * first read's look keep them right, and RefreshNow is there for a test that needs the world as it stands).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UActorIndexSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The world's index; null for the editor's own world (and no world). */
	static UActorIndexSubsystem* Get(const UWorld* World);

	/** Seconds between full looks (Looter.ActorIndex.Interval's default), and the least and most a tuned value may be. */
	static constexpr float DefaultInterval = 0.1f;
	static constexpr float MinInterval = 0.02f;
	static constexpr float MaxInterval = 2.f;
	/** A tuned interval, held between MinInterval and MaxInterval. */
	static float ClampInterval(float Wanted);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// The lists. The first read of a world looks through it once if play never began there (a test world); entries may be
	// null (destroyed since the last look). Not const for that first look.
	const TArray<TWeakObjectPtr<AWeaponBase>>& GetGuns() { EnsureLooked(); return Guns; }
	const TArray<TWeakObjectPtr<AAmmoPickup>>& GetAmmoPickups() { EnsureLooked(); return AmmoPickups; }
	const TArray<TWeakObjectPtr<AGrenadePickup>>& GetGrenadePickups() { EnsureLooked(); return GrenadePickups; }
	const TArray<TWeakObjectPtr<ASilverPickup>>& GetSilverPickups() { EnsureLooked(); return SilverPickups; }
	const TArray<TWeakObjectPtr<AShieldPickup>>& GetShieldPickups() { EnsureLooked(); return ShieldPickups; }
	const TArray<FIndexedCreature>& GetCreatures() { EnsureLooked(); return Creatures; }
	const TArray<TWeakObjectPtr<ACharacter>>& GetCharacters() { EnsureLooked(); return Characters; }
	const TArray<TWeakObjectPtr<AVolume>>& GetVolumes() { EnsureLooked(); return Volumes; }
	const TArray<TWeakObjectPtr<APlayableArea>>& GetPlayableAreas() { EnsureLooked(); return PlayableAreas; }

	/**
	 * Looks through the world now and rebuilds every list (the timer's job; a test calls it for the world as it stands). Also
	 * takes up a changed Looter.ActorIndex.Interval, when the timer is running.
	 */
	void RefreshNow();

	/** Starts the timer at the interval the console variable holds (play begins with it; a test arms it by hand). */
	void ArmTimer();

	/** The seconds between the timer's looks as it runs; -1 when it isn't running. */
	float GetTimerRate() const;

	/** How many full looks have been made (a test checks that reading never makes one). */
	int32 GetRefreshCount() const { return RefreshCount; }

private:
	void EnsureLooked();
	void HandleActorSpawned(AActor* Actor);
	/** Puts a new actor in the list its kind belongs to (none: it isn't indexed). */
	void Note(AActor& Actor);

	TArray<TWeakObjectPtr<AWeaponBase>> Guns;
	TArray<TWeakObjectPtr<AAmmoPickup>> AmmoPickups;
	TArray<TWeakObjectPtr<AGrenadePickup>> GrenadePickups;
	TArray<TWeakObjectPtr<ASilverPickup>> SilverPickups;
	TArray<TWeakObjectPtr<AShieldPickup>> ShieldPickups;
	TArray<FIndexedCreature> Creatures;
	TArray<TWeakObjectPtr<ACharacter>> Characters;
	TArray<TWeakObjectPtr<AVolume>> Volumes;
	TArray<TWeakObjectPtr<APlayableArea>> PlayableAreas;

	FTimerHandle RefreshTimer;
	FDelegateHandle SpawnHandle;
	/** The interval the timer was armed with: a tuned console variable re-arms it at the next look. */
	float ArmedSeconds = 0.f;
	int32 RefreshCount = 0;
	/** A full look has been made: spawned actors join the lists from then on (before it, the look finds them). */
	bool bLooked = false;
};
