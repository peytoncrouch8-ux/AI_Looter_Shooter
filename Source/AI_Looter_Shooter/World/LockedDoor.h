#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractableProp.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "LockedDoor.generated.h"

class UInteractionComponent;

/**
 * A door that stays shut until the story opens it: Bright & Daughter's cellar door (Knocking Below, "Find the cellar key":
 * Docs/Areas/RansomsRest_Expansion.md, section 4). It's an AInteractableProp door (Toggle, Swing about its hinge) that opens
 * only while the player carries its key (KeyItem, a mission item in the campaign's pocket: FCampaignRecord::CarriedItems) or
 * once UnlockedWhen holds (the mission finished, when the key has gone from the pocket). Locked, a tap of Interact rattles
 * its latch (the town's latch sound, LockedLines as captions) and nothing opens; the missions hear nothing of it. Open, it
 * can always be shut again.
 *
 * Tools/Unreal/build_area_missionprops_valley.py hangs one on the workshop's SOCKET_CellarDoor (SM_RR_CellarDoor: its pivot
 * the hinge at the floor, the leaf along -X, swinging open toward the socket's front).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALockedDoor : public AInteractableProp
{
	GENERATED_BODY()

public:
	ALockedDoor();

	// --- IInteractable: locked, the key rattles the latch instead of opening it ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;

	/** It has a lock at all: a key, or a story condition that opens it. Without either it's a plain door. */
	bool HasLock() const { return !KeyItem.IsNone() || !UnlockedWhen.IsEmpty(); }

	/** It opens now: no lock, its key carried, or UnlockedWhen holds. Without the level's mission runner, only a plain door. */
	UFUNCTION(BlueprintPure, Category = "Locked Door")
	bool IsUnlocked() const;

	/**
	 * Tries it while it's locked, as a tap of Interact does: the latch's rattle and LockedLines (once per RattleRest). False
	 * when it isn't locked (a use opens it instead).
	 */
	bool TryLocked();

	/** How many times it has been tried locked this play (tests, logs). */
	int32 GetRattles() const { return Rattles; }

	/** The mission item whose carrying opens it (CellarKey). None: only UnlockedWhen opens it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locked Door")
	FName KeyItem;

	/** It's open to anyone once this holds (after Knocking Below). Empty: only the key opens it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locked Door")
	FStoryCondition UnlockedWhen;

	/** The prompt while it's locked ("Locked"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locked Door")
	FText LockedPrompt;

	/** What's said trying it locked (Ellis, Hob), as captions after whatever is being said. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locked Door")
	TArray<FStoryLine> LockedLines;

	/** Seconds before a locked try rattles again (a held-down key, a player hammering it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locked Door", meta = (ClampMin = "0", Units = "s"))
	float RattleRest = 1.5f;

private:
	/** World time of the last rattle. */
	double LastRattle = -1.0e6;
	int32 Rattles = 0;
};
