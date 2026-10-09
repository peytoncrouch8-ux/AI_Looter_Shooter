#pragma once

#include "CoreMinimal.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/PackRules.h"
#include "PatrolSpawner.generated.h"

/**
 * A roaming pack on a patrol loop (Docs/Polish/BorderlandsComparison.md, item 7): an encounter whose creatures walk a route
 * together, the strongest leading and the rest in a staggered file behind, resting a moment at each end (or each time a
 * loop comes round). Everything else is a plain encounter's (AEncounterSpawner): its story, its groups, the caps, its
 * hunting ground (a corridor along the route, GroundCorners, so the chase keeps to the road's stretch), and taking them
 * away while the player is far.
 *
 *  - The pack's point walks the route at PaceShare of its slowest member's walking speed, only while every one of them is
 *    calm and keeping up (within CatchUpDistance of its place in the file); a fight holds it, and the survivors walk back
 *    to their places in the file afterwards (UCreaturePackComponent's anchor).
 *  - With none of its creatures out (before the player first comes, or taken away while they're far) the point walks on
 *    alone, so the pack is somewhere else along its road each time the player comes back. Its creatures appear round
 *    the point (GetSpawnCenter), and the player's distance is measured from it.
 *
 * Built by Tools/Unreal/build_area_camps.py from layout.json's gameplay.encounters.patrols.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API APatrolSpawner : public AEncounterSpawner
{
	GENERATED_BODY()

public:
	APatrolSpawner();

	/** Its route, relative to the spawner (as SpawnPoints are), in walking order: two points or more. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Patrol", meta = (MakeEditWidget = true))
	TArray<FVector> PatrolRoute;

	/** Round and round (back to its first point after its last); off, there and back along it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Patrol")
	bool bLoop = false;

	/** Seconds it rests at each end (a loop: at its first point). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Patrol", meta = (ClampMin = "0", Units = "s"))
	float PauseSeconds = 3.f;

	/** Its pace against its slowest member's walking speed: a stroll they keep up with. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Patrol", meta = (ClampMin = "0.1", ClampMax = "1"))
	float PaceShare = 0.7f;

	/** How far apart (cm) its creatures walk in their file. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Patrol", meta = (ClampMin = "100", Units = "cm"))
	float FileSpacing = 260.f;

	/** It waits while any calm member is farther than this (cm) from its place in the file. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Patrol", meta = (ClampMin = "100", Units = "cm"))
	float CatchUpDistance = 600.f;

	/** Its route in the world. */
	TArray<FVector> GetWorldRoute() const;

	/** Where its pack's point is now (world), and how far along the route. */
	FVector GetPatrolPoint() const;
	const PackRules::FPatrolWalk& GetWalk() const { return Walk; }

	/** Whether its point moved at its last look (it rests at an end, or waits for a fight or a straggler). */
	bool IsWalking() const { return bWalking; }

	/** Its pace (cm/s): PaceShare of its slowest member's walking speed (of its groups' kinds while none is out). */
	float GetPace() const;

	/** Walks it on by DeltaSeconds as its looks do (the tests call it; play's timer calls UpdateEncounter, which does). */
	void WalkFor(float DeltaSeconds) { UpdatePack(DeltaSeconds); }

protected:
	virtual void BeginPlay() override;
	virtual void UpdatePack(float DeltaSeconds) override;
	virtual FVector GetSpawnCenter() const override;

private:
	/** Each living member's place in the file now: the strongest first. */
	void PlaceFile(const TArray<FVector>& WorldRoute);

	PackRules::FPatrolWalk Walk;
	bool bWalking = false;
};
