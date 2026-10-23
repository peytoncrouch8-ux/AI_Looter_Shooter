#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "MissionArtifact.generated.h"

class UMissionDefinition;
class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Something found in the world that hands the player a mission, Borderlands' way: a letter nailed to a door, a miner's tag
 * in the dirt, a journal by a dead man. A placed prop whose mesh is set by data (the build scripts place it, labelled and
 * tagged) carrying the tag its missions name as their giver (FMissionOffer::GiverTag: Artifact_MinersTag). While one of
 * them is offered a ! floats over it, and Interact picks it up: it's heard (Giver::ArtifactRead) and the offer card opens
 * with its own words (Text, else the mission's Briefing) as a found document.
 *
 * With bTakeOnAccept it's gone once its mission is taken, and stays gone (a tag pocketed); without, it stays where it lies
 * (a notice nailed up). A mission can also be turned in at it (its turn-in's speaker tag on it): Interact then turns it
 * in, as a talk does. It never ticks: it hears the mission runner's changes.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AMissionArtifact : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AMissionArtifact();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	/** What it is, grown to at least a hand's size round it (InteractionFocus::GrownBox). */
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	/**
	 * Picks it up for Player (their pawn or controller), as a tap of Interact does: a mission waiting to be turned in here is
	 * turned in; else the offer card opens on the first mission it offers. False when it has nothing for them now.
	 */
	UFUNCTION(BlueprintCallable, Category = "Artifact")
	bool Use(AActor* Player);

	/** The mission it offers now (the first available, a main one first), or null. */
	const UMissionDefinition* GetOffer() const;

	/** Taken for good: bTakeOnAccept and every mission it offers taken (or finished). Hidden then, and off the Interact key. */
	bool IsTaken() const;

	/** Hides it or shows it as its missions stand (the runner's changes call it; so can tests). */
	void Refresh();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** What it is (set by the build script): a letter, a tag, a journal. Only the Interact key's line sees it; nothing collides. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** What it is, for people ("A miner's tag", "Abel's letter"): the card's "Found:" line when its mission names no giver. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact")
	FText Title;

	/** Its own words, the card's story (a letter's lines, what's scratched on a tag). Empty: the mission's Briefing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact", meta = (MultiLine = true))
	FText Text;

	/** The prompt's words ("Read the letter", "Pick up the tag"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact")
	FText Prompt;

	/** Gone once its mission is taken (a tag pocketed); off, it stays where it lies (a notice nailed up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact")
	bool bTakeOnAccept = true;

	/**
	 * How far from the player's eyes it can be picked up (cm): a notice board's 3 m. Beyond its use, the reach weighs in the
	 * Interact key's choice (InteractionFocus::ScoreOf: distance counts against a candidate as a share of its reach), so a
	 * card laid over a paper (Ruth's note on the Rangers' board, the gate's poster) must reach farther than the paper's 2.5 m
	 * (the player's own reach) to win the focus over it wherever on the paper the player looks.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 300.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleRunnerChanged();
	/** Shows it in the world and among the interactables, or takes it out of both. */
	void SetShown(bool bShown);

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
	bool bShownNow = true;
};
