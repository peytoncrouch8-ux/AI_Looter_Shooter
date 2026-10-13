#pragma once

#include "CoreMinimal.h"
#include "Creatures/DeadMinerRules.h"
#include "Creatures/EncounterSpawner.h"
#include "Story/StoryCondition.h"
#include "DeadMinerShiftSpawner.generated.h"

class UMissionRunner;

/**
 * An encounter of the Providence's shift (Docs/Areas/RansomsRest_Expansion.md, Back Pay and "Encounters"): a plain encounter
 * spawner (AEncounterSpawner) whose dead come back on an arrival only while the shift still works: Back Pay isn't finished,
 * or it was turned in keeping the payroll (the story flag BackPay.Kept, FCampaignRecord::HasFlag). Paid, the eleven rest:
 * the encounter goes off by the story, as any encounter whose condition stops holding does, and stays off. Its own
 * condition (ActiveWhen, FromStep) still applies on top: the shift adds "before Back Pay" to it once Back Pay is paid.
 *
 * Used for the mine's fights (MineUpper, MineLower), the Diggings' ambush out of the spoil heap, and the Foreman's lair
 * (LegendaryId Foreman): paid, he too stops coming back on his own, while his vigil stone still calls him (it forces his
 * lair's wave). The build script places it as it places any encounter, with this class.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ADeadMinerShiftSpawner : public AEncounterSpawner
{
	GENERATED_BODY()

public:
	/** The mission whose turn-in ends the shift (Back Pay), and the flag its choice sets to keep it working. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Shift")
	FName ShiftMission = DeadMinerRules::BackPayMission;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Shift")
	FName KeptFlag = DeadMinerRules::KeptFlag;

	/** Whether the shift still worked when it last looked. */
	bool DoesShiftWork() const { return bShiftWorks; }

	/** Reads the campaign again (the missions' changes call it) and applies the shift's rule. */
	void RefreshShift();

	/**
	 * Applies the shift's rule as given: working, its condition is the one it was placed with; paid, "before Back Pay" is
	 * added to it, so it's off. Then reads its story again. The tests call it (a test level has no campaign).
	 */
	void ApplyShift(bool bWorks);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleMissionsChanged();

	/** Its condition as placed, before the shift's rule. */
	FStoryCondition PlacedWhen;
	bool bPlacedCaptured = false;
	bool bShiftWorks = true;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
};
