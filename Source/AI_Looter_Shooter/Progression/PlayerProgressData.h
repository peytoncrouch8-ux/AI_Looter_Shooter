#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "PlayerProgressData.generated.h"

/**
 * How many of one kind of actor the player defeated at each enemy tier (Regular, Restless, Gravebound, Hallowed,
 * Ethereal), as the creature stood when it died. A Legendary monster or a boss is no tier and is not counted here; the
 * kind's total (FPlayerProgressData::Defeated) counts it as it counts everything.
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FTierDefeats
{
	GENERATED_BODY()

	/** By rank, saved by the rank's name so the enum can grow. Only tiers defeated have an entry. */
	UPROPERTY()
	TMap<ECreatureRank, int32> ByTier;

	int32 Count(ECreatureRank Tier) const
	{
		const int32* Found = ByTier.Find(Tier);
		return Found ? *Found : 0;
	}

	/** Every tier's count, summed with Other's. */
	void Add(const FTierDefeats& Other)
	{
		for (const TPair<ECreatureRank, int32>& Pair : Other.ByTier)
		{
			ByTier.FindOrAdd(Pair.Key) += Pair.Value;
		}
	}
};

/**
 * The player's progress as a session saves it (ULooterSessionSave): level and experience, the tutorial, and the
 * bestiary's kinds met and defeat counts. The defaults are a new game: level 1 with no experience, nothing met. It grows
 * with the player save (rewards, skill points, unlocks).
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FPlayerProgressData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Level = 1;

	/** Experience earned into the current level (not in total), so a change to the curve never changes the level. */
	UPROPERTY()
	int64 XP = 0;

	/** The tutorial island's prompts have been followed to the end (or skipped). */
	UPROPERTY()
	bool bTutorialDone = false;

	/** How many of each kind of actor the player has defeated, by class path (the bestiary's counts). */
	UPROPERTY()
	TMap<FString, int32> Defeated;

	/**
	 * Of those, how many at each enemy tier (2026-10-20), by the same class path. Defeated stays the total, so it is right
	 * for every save. New within session version 3: a save from before the tiers has no split and reads as tiers unknown
	 * (its defeats were never told apart, and some were Restless or Gravebound, so none is called Regular): the split
	 * counts only the defeats made since, and the total still holds all. No upgrade, and no version change.
	 */
	UPROPERTY()
	TMap<FString, FTierDefeats> DefeatedByTier;

	/** Every kind of actor the player has met (been hunted by, hurt or defeated), by class path: its bestiary page is open. */
	UPROPERTY()
	TSet<FString> Encountered;
};
