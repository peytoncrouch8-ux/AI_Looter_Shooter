#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Math/RandomStream.h"
#include "CreaturePackComponent.generated.h"

class ACreatureBase;
class APawn;
enum class ECreatureState : uint8;

/**
 * A creature's place in its pack (every ACreatureBase has one; the rules are PackRules, the numbers UEncounterSettings'
 * "Packs"). Its brain asks it three things, and a patrol tells it one:
 *  - Flanking: while it chases, it looks at its pack now and then (PackRadius): of those chasing the same player, the
 *    ones on the ends of the line swing out to either side and spiral in, the middle comes straight (FlankStepDegrees a
 *    member, at most FlankMaxDegrees; straight in within FlankReleaseDistance). A boss or a Legendary monster holds the
 *    middle and is no one's flank.
 *  - Breaking off: a Basic creature of RetreatKinds that had a pack, once its packmates are gone and it's hurt to
 *    RetreatHealthShare or less, rolls once a life (RetreatChance): it runs RetreatSeconds away from the player (bent
 *    toward home rather than off its hunting ground), crying out, then turns and comes again.
 *  - The rank sting: a Restless or better creature turning on a player for the first time this life sounds
 *    Creature.RankSting (2D, a cue to the player), unless another just did (RankStingRest), and keeps the moment
 *    (GetRankStingAge) for its tag to flash its rank's word.
 *  - A patrol's anchor (APatrolSpawner): while it has one, standing about and walking home mean keeping to that point as
 *    it moves along the route, at the anchor's pace.
 * It never ticks: the brain calls it.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UCreaturePackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreaturePackComponent();

	// --- While it chases (ACreatureBase's Chase) ---

	/** Moves its looks at its pack and its breaking off on by DeltaSeconds. */
	void TickChase(float DeltaSeconds);

	/** Where it runs now: away while it breaks off, else at its target along its flank (the target itself with none). */
	FVector GetChaseGoal(const APawn& Victim) const;

	/** It's breaking off: it runs, and starts no attack. */
	bool IsRetreating() const { return RetreatLeft > 0.f; }

	/** Its speed against its chase speed while it breaks off. */
	float GetRetreatSpeedShare() const;

	/** Its flank now (degrees; 0: straight at its target). */
	float GetFlankDegrees() const { return FlankDegrees; }

	/** It rolled for breaking off this life (and whether it had a pack to lose). */
	bool HasRolledRetreat() const { return bRetreatRolled; }
	bool HadPack() const { return bHadPack; }

	// --- Its brain's changes ---

	/** Its brain changed state (ACreatureBase::SetState): a new hunt, the rank sting, letting go of a flank. */
	void HandleStateChanged(ECreatureState OldState, ECreatureState NewState);

	/** It came back after a death: a new life, which may sting and break off again. */
	void ResetLife();

	// --- The rank sting ---

	/** It stung (or a packmate's sting stood for it) this life. */
	bool HasRankStung() const { return bRankStung; }

	/** How many times it sounded the sting (once a life at most). */
	int32 GetRankStings() const { return RankStings; }

	/** Seconds since it sounded the sting (a large number when it hasn't): its tag can flash its rank's word while this is short. */
	float GetRankStingAge() const;

	// --- A patrol's anchor (APatrolSpawner) ---

	/** The point it keeps to while calm, and whether that point is walking on (its patrol isn't resting). */
	void SetRoamAnchor(const FVector& Anchor, bool bMoving);
	void ClearRoamAnchor();
	bool HasRoamAnchor() const { return bHasRoamAnchor; }
	const FVector& GetRoamAnchor() const { return RoamAnchor; }
	bool IsRoamAnchorMoving() const { return bHasRoamAnchor && bRoamAnchorMoving; }

	/** Standing about, it should set off after its anchor now (it walked on more than a step's slack away). */
	bool WantsToRoam(const FVector& Here) const;

	/**
	 * Where it walks while it keeps to its anchor, and how fast: full walking pace well behind, slower as it closes, so it
	 * walks on with the patrol rather than running up and stopping. False without an anchor (it strolls round home).
	 */
	bool GetRoamMove(const FVector& Here, float WalkSpeed, FVector& OutGoal, float& OutSpeed) const;

	// --- Tests ---

	/** Seeds its one roll a life for breaking off (play seeds it from the creature's id). */
	void SetSeed(int32 Seed);

	/** Looks at its pack now, as its next look would (the tests don't wait for the interval). */
	void LookAtPackNow();

protected:
	virtual void BeginPlay() override;

private:
	ACreatureBase* GetCreature() const;
	/** Its flank among the pack chasing its target, and whether its pack is gone and it breaks off. */
	void LookAtPack();
	/** The rank sting, if it's due (PackRules::ShouldRankSting) and no other creature just sounded one. */
	void TryRankSting();
	void StartRetreat();

	/** Its flank now (degrees), and seconds to its next look at the pack. */
	float FlankDegrees = 0.f;
	float NextLook = 0.f;

	/** Breaking off: seconds left of it and where it runs. */
	float RetreatLeft = 0.f;
	FVector RetreatTo = FVector::ZeroVector;
	/** It had packmates near it this life, and it rolled for breaking off (once a life). */
	bool bHadPack = false;
	bool bRetreatRolled = false;
	FRandomStream Rolls;

	bool bRankStung = false;
	int32 RankStings = 0;
	double LastRankStingTime = -1.0e9;

	bool bHasRoamAnchor = false;
	bool bRoamAnchorMoving = false;
	FVector RoamAnchor = FVector::ZeroVector;
};
