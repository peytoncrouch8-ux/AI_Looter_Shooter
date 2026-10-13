#pragma once

#include "CoreMinimal.h"
#include "StoryCondition.generated.h"

class UMissionRunner;
struct FCampaignRecord;

/**
 * When something of the story applies, read from the campaign record: after some missions are finished, before others
 * are, while one is being played, once the story has settled something (a story flag set) or not. An empty condition
 * always applies. Story characters use it to be in the world or not (Abel on his board after his fight), and speaker
 * points to pick what they say (Delia's door after each mission). Missions are named by id, as the campaign record keeps
 * them; flags by name, as it keeps them (FCampaignRecord::StoryFlags).
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FStoryCondition
{
	GENERATED_BODY()

	/** Every one of these missions is finished. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	TArray<FName> AfterMissions;

	/** None of these missions is finished yet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	TArray<FName> BeforeMissions;

	/** Every one of these story flags is set (a turn-in's choice: BackPay.Paid, the porch plate: PorchPlate.Taken). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	TArray<FName> WithFlags;

	/** None of these story flags is set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	TArray<FName> WithoutFlags;

	/**
	 * This mission is being played: the campaign's main mission (even while the player is in another area), or a mission
	 * running in this level (a side mission). None: whatever is being played.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FName DuringMission;

	/**
	 * With DuringMission: it's on this step or a later one, counted from 0 as the campaign record keeps steps (Hob lands
	 * once Main 1 is past the claw-out, its step 2). 0: any step.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story", meta = (ClampMin = "0"))
	int32 FromStep = 0;

	/**
	 * With DuringMission: it's on a step before this one, counted from 0 as FromStep is (the town gate's fight is on
	 * during Main 3's second step only: from step 1, before step 2), so a fight already won doesn't come back when a
	 * session is loaded on a later step. 0: until the mission ends.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story", meta = (ClampMin = "0"))
	int32 BeforeStep = 0;

	/** It asks nothing, so it always applies. */
	bool IsEmpty() const;

	/** It applies in Campaign. Runner adds the missions running in this level; without it only the campaign's main mission counts as being played. */
	bool IsMet(const FCampaignRecord& Campaign, const UMissionRunner* Runner = nullptr) const;

	/** "after Main1; before Main6; with flags BackPay.Paid; without flags BackPay.Kept; during Main2 from step 2, before step 3", or "always", for logs and the console. */
	FString Describe() const;
};
