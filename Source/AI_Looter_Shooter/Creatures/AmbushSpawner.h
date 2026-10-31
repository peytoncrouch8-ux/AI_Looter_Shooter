#pragma once

#include "CoreMinimal.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/SpiderPerch.h"
#include "AmbushSpawner.generated.h"

class ASpiderCreature;

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
	/**
	 * Spiders lurking on walls, ledges and trees (Perches; the user's pick, 2026-10-31): brought out onto their perches as the
	 * player comes within ActivationRadius (or walks onto its ground, with AmbushCorners), still and waiting; each springs
	 * as the player passes near or under it, walks on past, stares at it, lingers or shoots it (ASpiderCreature::BeginLurk),
	 * its mates a moment after. They don't come at once (bHuntOnSpawn is off). A creature that isn't a spider appears.
	 */
	Lurk,
	/**
	 * The dead rise round the player who sprang it, staggered, surrounding them: the first where they look, the next behind
	 * them (AUnpaidCreature::RiseAmbush, round them; 2026-10-31). Other kinds come up their own way, where they stand.
	 */
	RiseAround,
	/**
	 * Spiders crawling out of dens in the walls (the Sink's floor; the user's ask, 2026-10-31): each waits unseen in a den's
	 * throat (Dens, in turn), then, staggered (SpiderAmbushRules::DenDelay), its legs reach out and grip the lip, it squeezes
	 * out, tips head down onto the face, climbs down, drops the last meter and hunts (ASpiderCreature::BeginDenEmerge). One
	 * that can't (no wall at its den), or a creature that isn't a spider, appears where it was spawned (its floor spot).
	 */
	Den,
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
 * A lurk (Entrance Lurk: spiders waiting on walls, ledges and trees) by Tools/Unreal/build_area_lurks.py from gameplay.encounters.lurks.
 * A den ambush (Entrance Den: the Sink floor's spiders crawling out of burrows in the pit walls) by Tools/Unreal/build_area_sink.py.
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

	/** Rising round the player (RiseAround): this far from them (cm at size 1); 0: Looter.Unpaid.Rise.Radius (480). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Ambush", meta = (ClampMin = "0", Units = "cm"))
	float RiseRadius = 0.f;

	/**
	 * Lurk: its spiders' perches (world), each held by one spider, the nearest free one to where it was spawned; spiders past
	 * their number lurk crouched on the ground where they stand. Written by Tools/Unreal/build_area_lurks.py from the layout's
	 * gameplay.encounters.lurks; the spot of each is the ground under it (its SpawnPoints), so they spawn there and climb up.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Ambush")
	TArray<FSpiderPerchSpot> Perches;

	/**
	 * Den: its dens' mouths (Kind Wall): Spot the middle of the mouth's opening on the wall plane (world), FaceYaw the way the
	 * mouth faces out of the wall (degrees, 0 = +X). Spider i of a spawn comes out of den i mod their number. Its SpawnPoints
	 * stay floor spots (one at the wall's foot under each den), where a spider that can't come out appears instead.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Ambush")
	TArray<FSpiderPerchSpot> Dens;

	/** A lurker sprang at Prey: its mates still waiting who can reach them spring too, each a moment after (SpiderAmbushRules::MateDelay). */
	void NoteLurkerSprang(const ASpiderCreature& Spider, APawn* Prey);

	/** Whether a point is on its ground: inside its corners, and within AmbushMaxRise of the spawner's height. */
	bool IsOnAmbushGround(const FVector& Point) const;

protected:
	virtual void BeginPlay() override;
	virtual bool CheckApproach() override;
	virtual void OnCreatureSpawned(ACreatureBase& Creature) override;
	virtual bool HasOwnEntrance() const override { return Entrance != EAmbushEntrance::Appear; }

private:
	/** Drops a creature from above its feet: lifted as far as there's room, falling. */
	void Drop(ACreatureBase& Creature) const;
	/** Sets a spider on its perch (Lurk), the nearest free one; anything else appears, and with the lurk off it drops and comes. */
	void SetOnPerch(ACreatureBase& Creature);
	/** Sends a spider into its den to crawl out (Den), its turn in the spawn by DenTurn; anything else appears. */
	void SendIntoDen(ACreatureBase& Creature);

	/** Den: the spider's turn in the spawn coming out now (spawns this close together are one: a wave's all come at once), and when the last came. */
	int32 DenTurn = 0;
	double LastDenSpawn = -1000.0;

	/** Who holds each perch (by index into Perches); a dead or gone one's is free. */
	TArray<TWeakObjectPtr<ACreatureBase>> PerchHolders;

	/** Its last look at the player: where they were, on its ground or not, and when (a long gap starts it afresh). */
	FVector LastPlayerSpot = FVector::ZeroVector;
	bool bLastOnGround = false;
	bool bSeenPlayer = false;
	double LastLookTime = 0.0;
};
