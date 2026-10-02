#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Math/RandomStream.h"
#include "Subsystems/WorldSubsystem.h"
#include "AreaRulesSubsystem.generated.h"

class UAreaDefinition;

/**
 * The rules of the area being played for the creatures in it (UAreaDefinition, found by the level as play begins):
 *  - the level band: a creature's level follows the player's inside it, give or take one (a boss's doesn't vary), and its
 *    rank's levels come on top (ACreatureBase, as it begins play and each time it comes back);
 *  - promotions: as the player arrives, each placed Basic creature may be Restless or Gravebound for one life, by the
 *    area's chances. In a session a map rolls at most once per 20 minutes of play (USessionSubsystem::ClaimPromotionRoll),
 *    so reloading doesn't reroll them; without a session every start rolls.
 * A level that is no area's, or an area asset made before bands, leaves its creatures as they were placed.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UAreaRulesSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** The area played in this level, or null. */
	const UAreaDefinition* GetArea() const;

	/** Whether this arrival promotes placed creatures (the area has chances, and the session's cooldown allowed it). */
	bool ArePromotionsRolling() const { return bPromotionsRoll; }

	/** A creature's own level here, before its rank's: RollLevelIn with this level's area, the player's level and its rolls. */
	int32 RollLevel(int32 OwnLevel, ECreatureRank Rank);

	/** The rank a placed Basic creature starts this arrival with: a promotion when this arrival rolls them, else Basic. */
	ECreatureRank RollPromotion();

	/** The local player's level (1 when there's no player). */
	int32 GetPlayerLevel() const;

	/**
	 * A creature's own level in InArea, before its rank's: the player's level plus -1, 0 or +1 from Random (a boss: plus 0),
	 * kept inside the area's band; OwnLevel (as placed or spawned) without an area or a band.
	 */
	static int32 RollLevelIn(const UAreaDefinition* InArea, int32 PlayerLevel, int32 OwnLevel, ECreatureRank Rank, const FRandomStream& Random);

	/** A placed Basic creature's rank when InArea's promotions roll, drawn from Random against its chances. */
	static ECreatureRank RollPromotionIn(const UAreaDefinition* InArea, const FRandomStream& Random);

private:
	/** Every actor has begun play (and the session has put the level back): logs what this arrival made of the creatures. */
	void HandleLevelBegun();

	UPROPERTY(Transient)
	TObjectPtr<UAreaDefinition> Area;

	/** This level's rolls, seeded as play begins, so each start differs. */
	FRandomStream Rolls;

	bool bPromotionsRoll = false;

	/** What this arrival's promotions made, for the log. */
	int32 PromotionsAsked = 0;
	int32 PromotedRare = 0;
	int32 PromotedEpic = 0;
};
