#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GroundIgnoreSubsystem.generated.h"

struct FCollisionQueryParams;
class AActor;
class ULevel;
class UWorld;

/**
 * The actors LooterWorld::StaticGeometryParams leaves out of ground traces (every volume, and the playable area), found
 * once per world rather than by walking all of the world's actors on every call: a creature builds those params ten times
 * a second. The list is looked up again on the next call after a volume or playable area is spawned or destroyed, or a
 * level streams in or out. Played worlds, and the editor preview worlds the automated tests build; the editor's own world
 * isn't cached (undo and the level tools add actors behind these events' backs), so it looks every time.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGroundIgnoreSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Adds the volumes and the playable area to Params' ignored actors, looking them up again first if the level changed. */
	void AddIgnoredTo(FCollisionQueryParams& Params);

	/** Adds the same to Params, for a world without a subsystem (the editor's): walks every actor, as the cache saves doing. */
	static void AddIgnoredByWalking(const UWorld& World, FCollisionQueryParams& Params);

	/** How many times the list was looked up (a test checks that a call without a change doesn't look again). */
	int32 GetLookupCount() const { return LookupCount; }

private:
	void HandleActorSpawned(AActor* Actor);
	void HandleActorDestroyed(AActor* Actor);
	void HandleLevelChanged(ULevel* Level, UWorld* World);
	static bool IsIgnoredKind(const AActor* Actor);
	static void FindIgnored(const UWorld& World, TArray<TWeakObjectPtr<const AActor>>& OutFound);

	TArray<TWeakObjectPtr<const AActor>> Ignored;
	/** The level count the list was found for: a backstop should a level join or leave without telling. */
	int32 LookedUpLevels = INDEX_NONE;
	int32 LookupCount = 0;
	bool bDirty = true;
	FDelegateHandle SpawnHandle;
	FDelegateHandle DestroyHandle;
	FDelegateHandle LevelAddedHandle;
	FDelegateHandle LevelRemovedHandle;
};
