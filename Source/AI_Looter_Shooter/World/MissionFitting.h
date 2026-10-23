#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractableProp.h"
#include "Story/StoryCondition.h"
#include "MissionFitting.generated.h"

class UMissionRunner;

/**
 * Where a mission has the player put something to work, with a held Interact (Docs/Areas/RansomsRest_Expansion.md, Dead
 * Lift's "Fit the crank, stoke the boiler, drive the pin", Back Pay's barred manway gate): the winch's crank fitted to its
 * shaft, the coal sack emptied into the boiler, the clamp pin driven home at the bottom sheave, the gate's bar lifted from
 * below (a Swing effect: it swings open on use). An AInteractableProp (a held use,
 * turned on for good once used: its tags, such as LiftFitting and LiftFitting_Crank, are what a mission's Interact
 * objective counts) that the story opens and closes:
 *  - it can be used only while UsableWhen holds (its mission's step; empty: always), and never once done; otherwise it's
 *    never focused, so no stray prompt waits in the winch house before or after the mission;
 *  - it's done once used here, or while any of DoneWhen holds (the mission's later steps and after it: a session loaded
 *    then finds the crank on its shaft); done, it counts as used up (IInteractable::IsUsedUp), so a lasting interact
 *    objective counts it whenever it looks;
 *  - with bHideUntilDone its model (Mesh: the crank, the sack, the pin) shows only once it's done; off, it's always seen
 *    (the gate), only its prompt coming and going.
 * With bHideUntilDone the prompt and the hand are at the actor's own spot (the build script stands it where the hand goes),
 * whichever way the model lies (the emptied sack lies by the coal bin, the prompt is at the firebox door); a model always
 * seen is used at its middle. A use plays UseCue at the actor.
 * Never ticks but while its parent's cooldown runs; it hears the mission runner's changes.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AMissionFitting : public AInteractableProp
{
	GENERATED_BODY()

public:
	AMissionFitting();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;
	virtual bool IsUsedUp() const override { return IsDone(); }

	/** It can be used now: its story lets it and it isn't done. */
	bool CanFit() const;

	/** Done: used here (or turned on), or its story says it's done by now. */
	bool IsDone() const;

	/** Shows its model as done or not, and closes its prompt once done (the runner's changes call it; so can tests). */
	void Refresh();

	/** When it can be used (Dead Lift's fittings step). Empty: always, until done. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Fitting")
	FStoryCondition UsableWhen;

	/** It's done while any of these holds, used here or not (the mission's later steps, and after it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Fitting")
	TArray<FStoryCondition> DoneWhen;

	/** Its model shows only once it's done (a crank fitted, a pin driven); off, it's always seen (a gate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Fitting")
	bool bHideUntilDone = true;

	/** The sound of the use, played at its spot (LooterSound cue name; none: silent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Fitting")
	FName UseCue;

	/**
	 * Half the size of what's looked at to use it where its model is hidden until done (cm): the firebox door, the clamp
	 * round the pin's hole, so it's used looked at anywhere there, from any spot the player stands (the 2026-10-23
	 * play-check: the boiler from one spot of three, the pin from one of four, at 71 degrees up). A model always seen (a
	 * gate) is looked at anywhere on it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Fitting")
	FVector LookHalfSize = FVector(35.0, 35.0, 35.0);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleRunnerChanged();
	void HandleUsed(AInteractableProp& Prop, AActor* User, bool bHeld);

	/** Whether any of DoneWhen holds now. */
	bool IsDoneByStory() const;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
	FDelegateHandle UsedHandle;
	bool bUsedHere = false;
};
