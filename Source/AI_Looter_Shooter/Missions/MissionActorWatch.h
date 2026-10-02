#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MissionActorWatch.generated.h"

class AActor;
class AController;
class UHealthComponent;
class UMissionRunner;

/**
 * Listens to one actor's health for the mission runner: its deaths and hits, passed on with the actor they happened to
 * (the health component's own events don't say whose they are, and kill objectives count by class, tag and place).
 * The runner makes one per actor with health in its level, ones spawned later too.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMissionActorWatch : public UObject
{
	GENERATED_BODY()

public:
	void Watch(UMissionRunner* InRunner, AActor* InActor, UHealthComponent* Health);

	/** The actor watched, or null once it's gone. */
	AActor* GetActor() const { return Watched.Get(); }

private:
	UFUNCTION()
	void HandleDeath(AController* Killer);

	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	TWeakObjectPtr<UMissionRunner> Runner;
	TWeakObjectPtr<AActor> Watched;
};
