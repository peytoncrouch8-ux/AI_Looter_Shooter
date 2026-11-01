#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "ReliquaryLantern.generated.h"

class AChapelReliquary;
class UBoxComponent;
class UMaterialInterface;
class UMissionRunner;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Where Abel's lantern is set on Saint Ada's Reliquary (Main 7 step 5, MAINS_2_7.md section 11: "Put Abel's lantern on the
 * Reliquary. [Interact, held 3 s, Reliquary_Lantern] Scene SaintAdaWakes"). It stands in the lid's recess of the chapel's
 * AChapelReliquary (found by its tag, seated on its ember: GetEmberStart). While the lantern is carried (UCarriedLantern-
 * Subsystem) and the step is on (SetWhen), holding Interact three seconds ("Set Abel's lantern on the Reliquary") lowers
 * it into the recess: SM_KeepersLantern, lit with his light; the player's interaction tells the missions (an Interact on
 * the actor tagged Reliquary_Lantern), the lantern is set down (no more lines on it), and SaintAdaWakes plays.
 *
 * After, it stays there for good, lit (ShownWhen: Saint Ada awake), its light warm and shadowless: the keeper keeping his
 * saint. Before, nothing shows; the prompt only offers while it can be set. Solid to nothing; the Interact line finds a box
 * over the recess.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AReliquaryLantern : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AReliquaryLantern();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;
	virtual bool IsUsedUp() const override { return bSet; }

	/** Sets the lantern on her as a held Interact does (the console: bForce, whatever the story). Plays the scene unless bQuiet. */
	bool SetLantern(AActor* ByWhom, bool bForce = false, bool bQuiet = false);
	bool CanSet() const;
	bool IsSet() const { return bSet; }

	/** Shows the lantern on her, lit (the scene's moment and its end state, a load after it). */
	void ShowLantern(bool bShown, float LitShare = 1.f);

	/** Reads the story again. */
	void RefreshStory();

	/** The Reliquary it sits on (the level's tagged Reliquary_Chapel nearest it). */
	AChapelReliquary* GetReliquary() const;

	/** Where the lantern's globe is (world): the scene's light pours from it. */
	FVector GetGlobe() const;

	/** It can be set while this holds (Main 7 step 5). Empty: whenever the lantern is carried. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary Lantern")
	FStoryCondition SetWhen;

	/** It shows set there, lit, while this holds (Saint Ada awake, for good). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary Lantern")
	FStoryCondition ShownWhen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary Lantern", meta = (ClampMin = "0.1", Units = "s"))
	float HoldSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary Lantern", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 350.f;

	/** The lantern's glass slot and its dark look (SM_KeepersLantern's LanternGlow; the trim's glass when dark). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary Lantern")
	FName GlassSlot = TEXT("LanternGlow");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lantern;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> Light;

	/** Over the recess: what the Interact line finds. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Grip;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool StoryHolds(const FStoryCondition& Condition) const;
	void HandleMissionsChanged();
	/** Onto the Reliquary's ember (its recess), facing as it faces. */
	void SeatOnReliquary();

	bool bSet = false;
	TWeakObjectPtr<AChapelReliquary> Reliquary;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DarkGlass;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LitGlass;
};
