#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Story/StoryCondition.h"
#include "StoryLighting.generated.h"

class UMissionRunner;
struct FCampaignRecord;

/** One of the story's lights: while When holds, the level is lit as State (a state of its ALightingStates: Dusk). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FStoryLightingRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Lighting")
	FStoryCondition When;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Lighting")
	FName State;
};

/**
 * The level's light following the story (Docs/Areas/RansomsRest.md, Main 6: "Starting the mission fades the Rest to
 * dusk"): the first of its Rules whose condition holds names the lighting state the level should be in, and
 * ULightingStateSubsystem switches to it, behind the camera's fade as the story moves (a mission started at Delia's
 * door) or at once as the level begins (a session loaded during Main 6). When no rule holds, the light stays as it is: a
 * rule's light lasts until another rule's comes, or the level loads again in its own light (Day), so the end of a mission
 * never snaps the sky back under the player. Placed by the area's build script (Tools/Unreal/build_area_deck.py); it
 * never ticks.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AStoryLighting : public AInfo
{
	GENERATED_BODY()

public:
	AStoryLighting();

	/** The story's lights, the first that holds winning (Main 6 at dusk). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Lighting")
	TArray<FStoryLightingRule> Rules;

	/** The state the first rule that holds names, or None when none holds. Runner adds the missions running in a level. */
	static FName PickState(const TArray<FStoryLightingRule>& InRules, const FCampaignRecord& Campaign, const UMissionRunner* Runner = nullptr);

	/** Reads the story again and switches the light if a rule asks for a new one (at once with bInstant, else behind a fade). */
	void Refresh(bool bInstant);

	/** The state the story last asked for here (None before any). */
	FName GetAskedState() const { return Asked; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleMissionsChanged();

	FName Asked;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
};
