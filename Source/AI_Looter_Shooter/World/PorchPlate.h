#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "PorchPlate.generated.h"

class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;

/**
 * The plate on Delia's porch table (Main 1, "Take what's on the porch plate"): cold biscuits Ellis can't taste and two jars
 * of grave salt with a slip in Delia's hand (SALT FOR THE ROAD. THEY DON'T CARE FOR IT. — D.). A tap of Interact takes the
 * jars once: two grave-salt grenades (UPlayerThrowComponent), the slip as a caption (TakeLines), the missions hear
 * PorchPlate.Taken. The biscuits stay until Main 7's optional ("Eat what's on the porch plate"): while EatWhen holds,
 * Interact eats them, Delia answers through the door (EatLines) and the missions hear PorchPlate.Eaten.
 *
 * Both are kept by the campaign's story flags (TakenFlag, EatenFlag: the same names as the events), so the jars and the
 * biscuits stay gone in every later session. Its three models (plate, jars, biscuits) are set by the build script (stand-ins
 * until the art exists: nothing shows but the prompt). Tagged PorchPlate. Never ticks.
 *
 * From a build script (Python):
 *     plate = actors.spawn_actor_from_class(unreal.PorchPlate, porch_table_top)
 *     plate.plate.set_static_mesh(unreal.load_asset('/Game/Art/Props/SM_PorchPlate'))   # jars, biscuits the same
 *     plate.set_editor_property('take_lines', [story_line("Delia's slip", "SALT FOR THE ROAD. THEY DON'T CARE FOR IT. — D.")])
 *     plate.set_editor_property('take_when', during('Main1', from_step=4))
 *     plate.set_editor_property('eat_when', during('Main7'))
 */
UCLASS()
class AI_LOOTER_SHOOTER_API APorchPlate : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	static const FName PlateTag;

	APorchPlate();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual bool IsUsedUp() const override;

	/** Takes the jars for Player (their pawn): the grenades, the slip, the event. False when it can't now. */
	UFUNCTION(BlueprintCallable, Category = "Porch Plate")
	bool Take(AActor* Player);

	/** Eats the biscuits: Delia's answer, the event. False when it can't now. */
	UFUNCTION(BlueprintCallable, Category = "Porch Plate")
	bool Eat(AActor* Player);

	/** The jars can be taken now: not taken, and TakeWhen holds. */
	bool CanTake() const;

	/** The biscuits can be eaten now: the jars taken, not eaten, bCanEat and EatWhen holds. */
	bool CanEat() const;

	bool IsTaken() const;
	bool IsEaten() const;

	/** Shows the jars and biscuits as the campaign keeps them (the runner's changes call it; so can tests). */
	void RefreshLook();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The tin plate: what the Interact key's line finds. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Plate;

	/** The two jars of grave salt, gone once taken. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Jars;

	/** The biscuits, gone once eaten. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Biscuits;

	/** How many grave-salt grenades the jars are. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porch Plate", meta = (ClampMin = "0"))
	int32 Grenades = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porch Plate")
	FText TakePrompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porch Plate")
	FText EatPrompt;

	/** Said as the jars are taken (Delia's slip, as a caption). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porch Plate")
	TArray<FStoryLine> TakeLines;

	/** Said as the biscuits are eaten (Delia, through the door: "...Thank you, Ellis."). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porch Plate")
	TArray<FStoryLine> EatLines;

	/** When the jars can be taken (Main 1 at the farmhouse). Empty: whenever they're there. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porch Plate")
	FStoryCondition TakeWhen;

	/** The biscuits can be eaten at all (Main 7's optional), when EatWhen holds. Off: never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porch Plate")
	bool bCanEat = true;

	/** When the biscuits can be eaten (during Main 7). Empty: once the jars are taken. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porch Plate", meta = (EditCondition = "bCanEat"))
	FStoryCondition EatWhen;

	/** The story flags (and mission events) for the jars taken and the biscuits eaten. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porch Plate")
	FName TakenFlag = TEXT("PorchPlate.Taken");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porch Plate")
	FName EatenFlag = TEXT("PorchPlate.Eaten");

	/** How far from the player's eyes it can be used (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porch Plate", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 250.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleRunnerChanged();
	bool IsMet(const FStoryCondition& When) const;
	/** Plays Said as captions in the plate's own words. */
	void Say(const TArray<FStoryLine>& Said) const;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
};
