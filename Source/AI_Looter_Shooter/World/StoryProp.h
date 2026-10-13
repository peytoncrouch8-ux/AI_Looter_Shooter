#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "StoryProp.generated.h"

class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;
struct FCampaignRecord;

/**
 * A model the story shows and hides (Docs/Areas/RansomsRest_Expansion.md, Dead Lift, The Last Watch and Back Pay): the
 * eleven's brass tags on the tag board's IN hooks until they're paid and on OUT after, their tags on the barricade's nail
 * until Ellis takes them, smoke from the winch house's stack once the boiler is stoked, Ruth's code book on its shelf while
 * its answer isn't being read. Nothing to use and nothing collides with it: its model is its Mesh (set by the build script,
 * Tools/Unreal/build_area_mountain.py and build_area_missionprops_mountain.py).
 *
 * It's there while any of ShownWhen holds (none: always) and none of HiddenWhen does, read again whenever the missions
 * change (a step, a turn-in, a story flag); hidden otherwise. Never ticks. Without a campaign (no mission runner) only one
 * with no ShownWhen shows.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AStoryProp : public AActor
{
	GENERATED_BODY()

public:
	AStoryProp();

	/** It's there now, as its story stands. */
	bool ShouldShow() const;

	bool IsShown() const { return bShownNow; }

	/** Shows or hides it as its story stands (the runner's changes call it; so can tests). */
	void Refresh();

	/**
	 * The rule on its own: shown while any of Shown holds (an empty list: always) and none of Hidden does, read in Campaign
	 * (Runner adds the missions running in the level). No campaign: shown only when Shown is empty.
	 */
	static bool Applies(const TArray<FStoryCondition>& Shown, const TArray<FStoryCondition>& Hidden, const FCampaignRecord* Campaign,
		const UMissionRunner* Runner);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** What it is: the tags on their hooks, a plume of smoke, a book. Seen only: nothing collides with it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** It's there while any of these holds. Empty: always, unless HiddenWhen says otherwise. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Prop")
	TArray<FStoryCondition> ShownWhen;

	/** It's gone while any of these holds, whatever ShownWhen says (the barricade's tags once taken). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Prop")
	TArray<FStoryCondition> HiddenWhen;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleRunnerChanged();
	void SetShown(bool bShown);

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
	bool bShownNow = true;
};
