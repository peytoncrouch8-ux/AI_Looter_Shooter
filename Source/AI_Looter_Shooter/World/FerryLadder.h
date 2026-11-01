#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "FerryLadder.generated.h"

class APawn;
class USceneComponent;
class UStaticMeshComponent;

/**
 * The rope ladder over the burial deck's open west end, down to the Ferryman's landing (ASextonsFerry; Main 2 step 2,
 * "Climb down to the Ferryman's landing and hear Mister Sexton's price"). Two iron stanchions on the deck's edge, two
 * ropes and their rungs hanging Drop cm down the drop to the landing's planks.
 *
 * Used from either end with a tap (Interact): from the deck "Climb down to the ferry", from the landing "Climb up". The
 * climb is a short scene (FerryLadderClimb: about 2.6 s, look only, skippable): the view turns to the ladder, goes hand over
 * hand down (or up) its rungs, and steps off facing the way on; the player's body waits at the far end from the start,
 * so a skip, a save or a quit mid-climb finds them there. With scenes off (tours, perf runs) it simply puts them there.
 *
 * Its actor is the ladder's top on the deck's edge: +X out over the canyon, Z up. Stand-in look: engine shapes in iron and
 * the game's canvas (rope) and timber (rungs), paths in the .cpp; no Tripo model asked for (TRIPO_MODELS.md "Mains 2-3"
 * lists it with the ferry). Hob perches on the stanchions' crossbar (GetPerch) for Main 2's talk.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AFerryLadder : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AFerryLadder();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	/** Climbs Climber down (bDown) or up: the scene, or straight there with scenes off. False without a climber. */
	bool Climb(APawn* Climber, bool bDown);

	/** Whether a pawn standing at Location is at the ladder's top end (on the deck) rather than its foot (on the landing). */
	bool IsAtTop(const FVector& Location) const;

	/** Where the climber's feet go at each end: on the deck behind its edge, on the landing out from its foot. */
	FVector GetTopFeet() const;
	FVector GetBottomFeet() const;

	/** The ladder's face at its top and at its foot (the ropes' middle, where the rungs are). */
	FVector GetTopRung() const;
	FVector GetBottomRung() const;

	/** Which way a climber faces stepping off at the top (inland, over the deck) and at the foot (out, toward the ferry). */
	float GetTopYaw() const;
	float GetBottomYaw() const;

	/** Hob's spot on the stanchions' crossbar, facing out over the ferry: (x, y, z) and yaw. */
	FTransform GetPerch() const;

	/** How far down the ladder hangs to the landing's planks (cm): the ferry's ClimbDrop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder", meta = (ClampMin = "100", Units = "cm"))
	float Drop = 380.f;

	/** How far back from the edge the climber stands on the deck, and out from the ladder's foot on the landing (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder", meta = (ClampMin = "30", Units = "cm"))
	float TopStandBack = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder", meta = (ClampMin = "30", Units = "cm"))
	float BottomStandOut = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder")
	FText DownPrompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder")
	FText UpPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StanchionLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StanchionRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Crossbar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RopeLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RopeRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Rungs;

	/** The rungs' count (one every ~40 cm of a 3.8 m drop). */
	static constexpr int32 RungCount = 9;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Hangs the ropes and rungs down Drop (the editor and play both). */
	void Hang();

	/** The first player's pawn, for which end its prompt shows. */
	const APawn* FindPlayer() const;
};
