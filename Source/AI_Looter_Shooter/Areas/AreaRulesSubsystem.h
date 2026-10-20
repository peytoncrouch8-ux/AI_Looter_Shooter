#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Math/RandomStream.h"
#include "Subsystems/WorldSubsystem.h"
#include "AreaRulesSubsystem.generated.h"

class ACreatureBase;
class UAreaDefinition;
struct FCampaignRecord;

/**
 * The rules of the area being played for the creatures in it (UAreaDefinition, found by the level as play begins):
 *  - levels: a creature's level is where it stands, never the player's (the user's call, 2026-10-12): its kind's set
 *    level here (the area's boss, a Legendary monster), else its zone's levels, rolled for a level of variety; one placed
 *    or spawned at a level of its own keeps it. Its rank's levels come on top (ACreatureBase, as it begins play and each
 *    time it comes back). So a player who comes back strong finds the bluff's spiders still level 1-2, worth little.
 *  - promotions: as the player arrives, each placed Basic creature may be one of the four tiers above it for one life
 *    (Restless, Gravebound, Hallowed, Ethereal), by the tiers' chances at the area's place in the campaign, under the
 *    tiers' caps of living ones (KeepUnderCaps). In a session a map rolls at most once per 20 minutes of play
 *    (USessionSubsystem::ClaimPromotionRoll), so reloading doesn't reroll them; without a session every start rolls.
 *  - practice (Skyreach): its kills give no experience (UPlayerProgressionSubsystem::KillXP), and once the player has
 *    left it for the first time its creatures drop only ammo (ULootDropComponent).
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

	// --- Levels ---

	/**
	 * Creature's own level here before its rank's (it asks as it begins play, and again each time it comes back): RollLevelIn
	 * with this level's area and rolls, at the spot it stands on (its home, once it has one), a level of its own when it was
	 * placed at another than its class's. Never the player's.
	 */
	int32 RollLevelFor(const ACreatureBase& Creature, int32 OwnLevel);

	/**
	 * A creature's own level in InArea before its rank's, standing at Spot: OwnLevel (kept inside the band) when it was
	 * chosen for it (bOwnLevelChosen: placed at a level other than its class's); else Kind's set level, or Spot's levels
	 * rolled from Random (UAreaDefinition::PickLevel). OwnLevel as it is without an area or a band.
	 */
	static int32 RollLevelIn(const UAreaDefinition* InArea, const FVector& Spot, const UClass* Kind, int32 OwnLevel, bool bOwnLevelChosen,
		const FRandomStream& Random);

	/**
	 * The rank a placed Basic creature (Asking) starts this arrival with: a promotion when this arrival rolls them, kept
	 * under the tiers' caps (KeepUnderCaps), else Basic. OutRoll, when given, gets the roll's bits (its traits' seed).
	 */
	ECreatureRank RollPromotion(const AActor* Asking = nullptr, int32* OutRoll = nullptr);

	/**
	 * A placed Basic creature's rank when InArea's promotions roll, drawn from Random against the tiers' chances at its
	 * place in the campaign (UAreaDefinition::PickPromotion). OutRoll, when given, gets the roll (0 to 1).
	 */
	static ECreatureRank RollPromotionIn(const UAreaDefinition* InArea, const FRandomStream& Random, float* OutRoll = nullptr);

	/**
	 * Rolled kept under the tiers' caps in this level (FCreatureRankInfo::MaxAlivePerArea: Gravebound 4, Hallowed 2,
	 * Ethereal 1): a capped tier with that many alive (Except aside, Pending more already rolled to come) steps down a
	 * tier until one has room. Placed ranks count, though nothing caps them.
	 */
	ECreatureRank KeepUnderCaps(ECreatureRank Rolled, const AActor* Except = nullptr, TConstArrayView<ECreatureRank> Pending = {}) const;

	/** How many living creatures of Rank are in this level (Except aside): its rank in play, or the one it will begin play with. */
	int32 CountAlive(ECreatureRank Rank, const AActor* Except = nullptr) const;

	// --- Practice (Skyreach) ---

	/** Kills here give experience: not in a practice area. */
	bool GivesKillExperience() const;

	/** Kills here may drop guns: not in a practice area once the player has left it for the first time. */
	bool DropsGuns() const;

	/** InArea's rule; a level that is no area's gives experience. */
	static bool GivesKillExperienceIn(const UAreaDefinition* InArea);

	/** InArea's rule as the story stands (Campaign; none: the player hasn't left anywhere yet). */
	static bool DropsGunsIn(const UAreaDefinition* InArea, const FCampaignRecord* Campaign);

	/**
	 * The rules of WorldContextObject's level: its rules subsystem's answers, or yes where there is none (a test level,
	 * the editor, a class default).
	 */
	static bool GivesKillExperienceAt(const UObject* WorldContextObject);
	static bool DropsGunsAt(const UObject* WorldContextObject);

private:
	/** Every actor has begun play (and the session has put the level back): logs what this arrival made of the creatures. */
	void HandleLevelBegun();

	UPROPERTY(Transient)
	TObjectPtr<UAreaDefinition> Area;

	/** This level's rolls, seeded as play begins, so each start differs. */
	FRandomStream Rolls;

	bool bPromotionsRoll = false;

	/** What this arrival made of its creatures, for the log: levels given by where they stand, and promotions. */
	int32 LevelsGiven = 0;
	int32 PromotionsAsked = 0;
	/** By tier (LooterRanks::TierIndex); Basic's are the ones left as they were. */
	int32 Promoted[LooterRanks::NumTiers] = {};
	/** Rolls a cap stepped down (KeepUnderCaps), for the log. */
	int32 PromotionsCapped = 0;
};
