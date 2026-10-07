#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "HayBale.generated.h"

class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;
class UStoryLineSet;

/** Lines said as the level's LoadedCount-th hay bale goes in (Amos's, Hob's), after whatever is being said. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FHayBaleRemark
{
	GENERATED_BODY()

	/** Said when this many of the level's bales are loaded, this one included. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Remark", meta = (ClampMin = "1"))
	int32 LoadedCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Remark")
	TArray<FStoryLine> Lines;

	/** The lines from an asset instead, when set (create_story_lines.py's). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Remark")
	TObjectPtr<UStoryLineSet> LineSet;
};

/**
 * One of Amos Whitlock's hay bales (Docs/Areas/RansomsRest.md, Side 2 "Unfinished Business": "Amos Whitlock died last harvest
 * with his hay half in ... Load 6 hay bales into his barn (hold Interact)"): a small square bale (FarmProps.py's
 * SM_HayBale_Square) lying out in the hayfield, and its place in the stack under the hoist by his barn's big doors.
 * Holding Interact on it while LoadWhen holds (from Side 2's second step) loads it: it's gone from the field and stands in
 * the stack, and Amos (or Hob) remarks on some of them (LoadRemarks). The player's interaction component tells the
 * missions.
 *
 * Loaded stays loaded: the session keeps the loaded bales by name with the map's world (FSavedMapWorld::LoadedBales), and
 * Side 2's lasting objective (UMissionLastingInteractObjective) counts the loaded ones it finds, so a reload, which starts a
 * side mission over, loses none. Once Side 2 is done (LoadedWhen) every bale is in the stack whatever the session kept.
 *
 * Two meshes in one actor: the bale in the field (Bale, at the actor) and the same bale in the stack (Stacked, placed by the
 * build script on its spot by the barn); only one is in the game at a time. Neither is tagged an obstacle: the actor's
 * bounds reach from the field to the barn, and the scatter keeps grass out of an obstacle's bounds. It never ticks.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AHayBale : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag missions find the bales by (Side 2's "Load the hay bales"). */
	static const FName BaleTag;

	/** FarmProps.py's small square bale. */
	static const TCHAR* const ModelPath;

	AHayBale();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	/** Loaded for good: the world keeps it so, and Side 2's lasting objective counts it. */
	virtual bool IsUsedUp() const override { return bLoaded; }

	virtual void OnConstruction(const FTransform& Transform) override;

	/**
	 * Loads it as a held Interact does: gone from the field, standing in the stack by the barn, a remark for this many
	 * loaded, and a save soon (it's progress). bForce loads it whatever LoadWhen says (the console). Missions hear of it
	 * from the player's interaction component, not from here. False when it's loaded already or can't be now.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hay")
	bool Load(AActor* ByWhom, bool bForce = false);

	/** Loaded as the session or the story keeps it: in the stack at once, with no remark or save. */
	void RestoreLoaded();

	/** Back out in the field (the console, after a step started over). */
	void PutBack();

	/** It lies in the field and LoadWhen holds (empty: any time). */
	UFUNCTION(BlueprintPure, Category = "Hay")
	bool CanLoad() const;

	UFUNCTION(BlueprintPure, Category = "Hay")
	bool IsLoaded() const { return bLoaded; }

	/** How many of the level's bales are loaded. */
	static int32 CountLoaded(const UWorld* World);

	/** Reads its story again: in the stack once LoadedWhen holds. */
	void RefreshStory();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The bale lying in the field, at the actor's spot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Bale;

	/** The same bale in the stack by the barn, where the build script puts it; in the game only once it's loaded. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Stacked;

	/** The prompt's words. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hay")
	FText Prompt;

	/** How long Interact is held to load it: a heave, longer than tearing a poster down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hay", meta = (ClampMin = "0.1", Units = "s"))
	float LoadHoldSeconds = 1.2f;

	/** How far from the player's eyes it can be loaded (cm); 0: the player's reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hay", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 0.f;

	/** It can be loaded only while this holds: once Amos has asked (Side 2 from its second step; from 0: 1). Empty: any time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hay")
	FStoryCondition LoadWhen;

	/** It's in the stack from the start once this holds: Side 2 done. Empty: never by the story alone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hay")
	FStoryCondition LoadedWhen;

	/** What's said as the level's bales go in (Amos at the first and the last, Hob at the third). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hay|Lines")
	TArray<FHayBaleRemark> LoadRemarks;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** In the field (findable, solid) or in the stack. */
	void ShowLoaded(bool bInStack);

	void SayRemark(int32 LoadedNow);
	void HandleMissionsChanged();

	bool bLoaded = false;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
};
