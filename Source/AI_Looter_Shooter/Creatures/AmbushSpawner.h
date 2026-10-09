#pragma once

#include "CoreMinimal.h"
#include "Creatures/EncounterSpawner.h"
#include "AmbushSpawner.generated.h"

/** How an ambush's creatures arrive (AAmbushSpawner::Entrance). */
UENUM(BlueprintType)
enum class EAmbushEntrance : uint8
{
	/** They're simply there, as any encounter's are. */
	Appear,
	/** The dead rise where they stand: an Unpaid fades in from its shroud's ends (AUnpaidCreature::RiseIn), as Abel's adds do. */
	Rise,
	/** They drop from above (the Webwood's trees) and land on their feet. */
	Drop,
};

/**
 * A landmark ambush (Docs/Polish/BorderlandsComparison.md, item 7): an encounter that springs when the player walks onto
 * its ground (AmbushCorners: the churchyard inside its iron fence, the Webwood's band of dead trees), rather than when
 * they come near. A player set down inside (a respawn grave, a load, the story turning it on while they stand there) has
 * to walk out and back in (PackRules::IsWalkIn): nobody wakes into a fight. Its creatures arrive their own way
 * (Entrance) and come for the player at once (bHuntOnSpawn). Cleared, it's done until the level loads again; taken away
 * while the player was far, they're owed, and spring again the next time the player walks in.
 *
 * Built by Tools/Unreal/build_area_camps.py from layout.json's gameplay.encounters.ambushes.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AAmbushSpawner : public AEncounterSpawner
{
	GENERATED_BODY()

public:
	AAmbushSpawner();

	/** The ground that springs it (world X and Y, cm; three corners or more). Fewer: it springs on approach, as any encounter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Ambush")
	TArray<FVector> AmbushCorners;

	/** A player more than this (cm) above or below the spawner isn't on its ground (the Sink's floor under the Webwood). 0: any height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Ambush", meta = (ClampMin = "0", Units = "cm"))
	float AmbushMaxRise = 600.f;

	/**
	 * Between two looks the player moved no farther than this (cm) to count as walking in: a sprint covers about 4 m in
	 * half a second, a respawn or a load much more.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Ambush", meta = (ClampMin = "100", Units = "cm"))
	float WalkInStep = 1200.f;

	/** How its creatures arrive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Ambush")
	EAmbushEntrance Entrance = EAmbushEntrance::Appear;

	/** A dropping creature starts this far over its feet (cm), less if something overhead is nearer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Ambush", meta = (ClampMin = "0", Units = "cm"))
	float DropHeight = 350.f;

	/** Whether a point is on its ground: inside its corners, and within AmbushMaxRise of the spawner's height. */
	bool IsOnAmbushGround(const FVector& Point) const;

protected:
	virtual bool CheckApproach() override;
	virtual void OnCreatureSpawned(ACreatureBase& Creature) override;

private:
	/** Drops a creature from above its feet: lifted as far as there's room, falling. */
	void Drop(ACreatureBase& Creature) const;

	/** Its last look at the player: where they were, on its ground or not, and when (a long gap starts it afresh). */
	FVector LastPlayerSpot = FVector::ZeroVector;
	bool bLastOnGround = false;
	bool bSeenPlayer = false;
	double LastLookTime = 0.0;
};
