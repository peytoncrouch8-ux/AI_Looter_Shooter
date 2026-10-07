#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "StoryCharacter.generated.h"

class UMissionRunner;
class USpeakerPointComponent;
class UStaticMeshComponent;

/**
 * A character of the story who isn't an enemy: Mister Sexton on the lookout rail, Hob, Amos at his fence, Abel on his board
 * after his fight (Abel the boss is a creature of his own). Not a creature: it never hunts, fights or takes damage. It
 * stands where it's placed, posed by code (for now a placeholder body that breathes and turns to whoever it's talking
 * to), and carries a speaker point the player talks to: tag it (Speaker_Sexton) for missions to find it.
 *
 * The story decides whether it's there: ShownWhen is read from the campaign record as play begins and whenever the
 * missions change (one finished, one started). Hidden, it can't be seen, bumped into or talked to.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AStoryCharacter : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AStoryCharacter();

	// --- IInteractable: talking to it is its speaker point's ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	virtual void Tick(float DeltaSeconds) override;

	/** It's in the world now: the story's condition held when it last looked. */
	UFUNCTION(BlueprintPure, Category = "Story")
	bool IsShown() const { return bShown; }

	/**
	 * Reads the story again and shows or hides it. The missions' changes call it; so can tests. A character with more to
	 * follow (Hob's perches) adds to it.
	 */
	virtual void RefreshShown();

	/** Moves its pose on by DeltaSeconds (the tick does while it's shown): breathing, and turning to whoever it talks to. */
	virtual void UpdatePose(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Its body, turned and breathing by code: a person-sized placeholder until its model comes (set another mesh here). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	/** Who it is and what it says; where the player looks to talk (about head height). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpeakerPointComponent> SpeakerPoint;

	/** When it's in the world; an empty condition: always. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FStoryCondition ShownWhen;

	/** How far its body rises and settles with each breath (cm), and how long a breath takes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose", meta = (ClampMin = "0", Units = "cm"))
	float BreathHeight = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose", meta = (ClampMin = "0.5"))
	float BreathSeconds = 4.f;

	/** How fast it turns to whoever it talks to and back after (degrees a second); 0: it never turns (a seated Sexton). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose", meta = (ClampMin = "0"))
	float TurnSpeed = 150.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** In the world or out of it: seen, solid, talked to and posed, or none of those. */
	void SetShown(bool bInShown);

	/** Whether its story condition holds now (ShownWhen; an empty condition always does). */
	bool IsStoryShown() const;

private:
	void HandleMissionsChanged();
	void HandleTalked(USpeakerPointComponent& Point, AActor* Listener);

	/** Remembers the body as placed (its spot and facing), which the pose works from, the first time it's needed. */
	void CapturePlacedBody();

	FTransform PlacedBody;
	bool bPlacedBodyCaptured = false;

	/** Who it's talking to, turned toward while its lines play. */
	TWeakObjectPtr<AActor> TalkingTo;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;

	float PoseClock = 0.f;
	/** How far the body has turned from its placed facing (degrees). */
	float TurnYaw = 0.f;
	bool bShown = true;
	/** Shown or hidden at least once (play has begun). */
	bool bShownApplied = false;
};
