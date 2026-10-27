#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "MissionItem.generated.h"

class UMissionDefinition;
class UMissionRunner;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FMissionItemInfo;

/**
 * A thing in the world that belongs to a mission (FMissionItemInfo: the winch crank in the sheriff's lockup, the cellar key
 * on its hook, Fenn's coins under the agent's door, the Gravemother's fang where she fell). Interact takes it ("Take the
 * winch crank"), or running over it does (bPickUpOnTouch, for a drop in a fight); it goes into the campaign's pocket
 * (UMissionRunner::GiveItem), the missions hear a Collect event and its UMissionCollectItemObjective counts it, and the
 * Missions page lists it under Carried until the mission finishes.
 *
 * Placed by a build script, it's there while its mission is taken and not finished (or while ShownWhen holds), and never
 * once carried; it's taken only from its step (TakeWhen: the mission gates, 2026-10-27), with a short hold of Interact
 * (MissionGates::HoldSeconds) so a mashed key never takes it. One a creature drops as it dies (FMissionItemInfo::DroppedBy, UMissionRunner) falls at the body with a
 * gold beam over it and lies there whatever its mission's state, until it's picked up or the level is left (then the
 * runner gives it, so the mission never sticks). Its model is its own Mesh, else its item's (by data), else a small stand-in.
 * Tagged MissionItem and Item_<Id> (TagFor) for missions and filters to find; never ticks but while it falls.
 *
 * From a build script (Python):
 *     item = actors.spawn_actor_from_class(unreal.MissionItem, unreal.Vector(x, y, z))
 *     item.set_editor_property('item_id', 'WinchCrank')        # the mission's FMissionItemInfo.Id
 *     item.mesh.set_static_mesh(unreal.load_asset('/Game/Art/Props/SM_WinchCrank'))   # or leave it to the item's own
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AMissionItem : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag every mission item carries. */
	static const FName ItemTag;

	AMissionItem();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	/** Its model's bounds, grown to at least a hand's size round it (small things lying about are still easy to look at). */
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	virtual void Tick(float DeltaSeconds) override;

	/** Picks it up for Player, as Interact or a touch does: into the pocket, heard and gone. False when it can't be now. */
	UFUNCTION(BlueprintCallable, Category = "Mission Item")
	bool Take(AActor* Player);

	/** It's in the world to take now (its mission's state, ShownWhen, not carried). */
	bool ShouldShow() const;

	/** Its step has come (TakeWhen holds; a dropped one always): shown, it can be taken. */
	bool CanTake() const;

	bool IsShown() const { return bShownNow; }

	/** Shows or hides it as its mission stands (the runner's changes call it; so can tests). */
	void Refresh();

	/** Its item's mission, or null when no mission has ItemId. */
	const UMissionDefinition* GetMission() const;

	/** Its item as its mission describes it, or null. */
	const FMissionItemInfo* GetInfo() const;

	/** Its name for people: DisplayName, else its item's, else its id as words. */
	FText GetItemName() const;

	/** It fell from a creature that carried it (spawned by the runner), rather than being placed. */
	bool IsDropped() const { return bDropped; }

	/**
	 * Drops ItemId at Where (a body): it falls to the ground under it and lies there, beam up, taken with Interact or a touch.
	 * Null when it couldn't be spawned.
	 */
	static AMissionItem* SpawnDrop(UWorld* World, FName ItemId, const FVector& Where);

	/** One lying in World for ItemId (shown or not), or null. */
	static AMissionItem* FindIn(const UWorld* World, FName ItemId);

	/** "Item_WinchCrank": the tag an item's pickups carry. */
	static FName TagFor(FName ItemId);

	/** Moves its fall on by DeltaSeconds (the tick does while it falls; tests call it). */
	void AdvanceFall(float DeltaSeconds);

	bool IsFalling() const { return FallAge >= 0.f; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** What it is (set by the build script, else its item's model): seen, and found by the Interact key's line. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** The soft gold pillar over a dropped one (or a placed one with bShowBeam), so it's found after a fight. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Beam;

	/** The mission item it is (FMissionItemInfo::Id in its mission's Items). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Item")
	FName ItemId;

	/** Its name here, when it should read otherwise than its item's ("the crank on the wall"). Empty: its item's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Item")
	FText DisplayName;

	/** The prompt's words. Empty: "Take <name>". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Item")
	FText Prompt;

	/** Running over it takes it, as ammo is taken (a drop in a fight); Interact always does. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Item")
	bool bPickUpOnTouch = false;

	/** How near the player's middle comes for a touch to take it (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Item", meta = (ClampMin = "0", Units = "cm"))
	float TouchRadius = 130.f;

	/** How far from the player's eyes it can be taken (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Item", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 250.f;

	/**
	 * When it's there to take (Fenn's coins: from Old Fare's fifth step). Empty: while its mission is taken and not finished.
	 * A dropped one ignores it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Item")
	FStoryCondition ShownWhen;

	/**
	 * When it can be taken, while it's shown: its step (the mission gates, 2026-10-27: Back Pay's payroll is seen in the
	 * stope through the Foreman's fight and taken once he's down, its own step). Empty: whenever it's shown. A dropped one
	 * ignores it. Python: set_editor_property('take_when', condition).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Item")
	FStoryCondition TakeWhen;

	/** The gold beam over it even when placed (an item out in the open). A dropped one always has it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Item")
	bool bShowBeam = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleRunnerChanged();
	void SetShown(bool bShown);
	/** Starts or stops the look for a touch, as it's shown and takes one. */
	void UpdateTouchTimer();
	/** Gives the mesh its model (its own, its item's, else the stand-in) and the beam its look. */
	void ApplyLook();
	void PollTouch();

	/** The stand-in model while an item has none of its own yet (the art comes later). */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> StandIn;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
	FTimerHandle TouchTimer;
	bool bShownNow = true;
	bool bDropped = false;
	/** Seconds into its fall (a drop), or below 0 when it isn't falling; where it fell from and lands. */
	float FallAge = -1.f;
	FVector FallFrom = FVector::ZeroVector;
	FVector FallTo = FVector::ZeroVector;
};
