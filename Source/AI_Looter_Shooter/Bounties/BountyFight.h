#pragma once

#include "CoreMinimal.h"
#include "Bounties/BountyTypes.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/SetPieceSpawner.h"
#include "BountyFight.generated.h"

class ABountyArena;
class ABountyFight;
class AController;

/** A bounty fight's moment: its mark down, its fight over. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBountyFightEvent, ABountyFight& /*Fight*/);

/**
 * A taken bounty's fight (Docs/Work/2026-10-22-Next/BOUNTY.md, "The fight"): a set piece (ASetPieceSpawner: pushes on top
 * of the last's stragglers, each with its beat) made at the bounty's arena from its posting (BountyRules::MakeGroups),
 * its ground and spots the arena's. The mark comes with the last push: named (its tag reads "<rank> <name>") and given
 * exactly the posting's traits; a Swift posse runs Fleet. Its mark's death is the bounty won (OnMarkDown); the fight over
 * (every push's creatures down), OnDone. No story flag (it repeats) and no loot shower (the board drops the strongbox).
 *
 * Spawned in play by UBountyBoardSubsystem (SpawnFor) when a bounty is taken, and as each level begins while one is
 * taken; never saved, as no encounter is. Destroyed with what it has out when the bounty is given up or paid.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API ABountyFight : public ASetPieceSpawner
{
	GENERATED_BODY()

public:
	ABountyFight();

	/** A fight for Posting at Arena, set up and begun (as a level's spawners begin). Null without a world or a kind. */
	static ABountyFight* SpawnFor(UWorld* World, const ABountyArena& Arena, const FBountyPosting& Posting);

	/** The posting it's for. */
	const FBountyPosting& GetPosting() const { return Posting; }
	int32 GetSerial() const { return Posting.Serial; }

	/** The mark while it's out (null before its push, once it's dead). */
	ACreatureBase* GetMark() const { return Mark.Get(); }

	/** Its mark has died this visit. */
	bool IsMarkDown() const { return bMarkDown; }

	/** It has sprung (its first push came). */
	bool HasSprung() const { return GetWavesStarted() > 0; }

	/** Creatures it brings in all, the mark among them, and how many are down. */
	int32 GetPlannedTotal() const { return PlannedTotal; }

	FOnBountyFightEvent OnMarkDown;
	FOnBountyFightEvent OnDone;

	/** The mark's tag (on every mark, for the console and the tests). */
	static const FName MarkTag;

protected:
	virtual void OnCreatureSpawned(ACreatureBase& Creature) override;
	virtual void OnStateChanged(EEncounterState From, EEncounterState To) override;

private:
	/** Its groups, beats, ground and spring from the posting and the arena (before its play begins). */
	void Setup(const ABountyArena& Arena, const FBountyPosting& InPosting);

	/** A creature it spawned is its mark: the mark's rank and class, none found yet. */
	bool IsMarkCandidate(const ACreatureBase& Creature) const;

	UFUNCTION()
	void HandleMarkDeath(AController* Killer);

	FBountyPosting Posting;
	TWeakObjectPtr<ACreatureBase> Mark;
	TSubclassOf<ACreatureBase> MarkClass;
	ECreatureRank MarkRank = ECreatureRank::Epic;
	int32 PlannedTotal = 0;
	bool bMarkDown = false;
	bool bDoneTold = false;
};
