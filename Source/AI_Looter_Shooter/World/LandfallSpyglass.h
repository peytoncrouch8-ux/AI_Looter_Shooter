#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "LandfallSpyglass.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * The brass spyglass on its tripod at the lookout on Ransom's Point (Main 1 step 6, "Look over the Rest": Interact, tag
 * Landfall_Spyglass). Holloway wanted eyes on the valley: through it the player sees what Vane has done to Ransom's Rest
 * (the scene SmogReveal): his dredge barge hanging over the Sink (the actor tagged Landfall_Barge), its hose sunk into the
 * ground, the soul smog pouring off it, while Vane gloats over the valley's loudspeakers; then Finch on the tube: the
 * Lantern has Abel's trail. The first look plays all of it (story flag Landfall.Revealed); a later look is a quiet one
 * through the glass. It turns its tube toward the barge as play begins.
 *
 * Placed by WORLD's build script at the lookout. Stand-in look: a brass tube on three legs built from engine shapes
 * (paths in the .cpp).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALandfallSpyglass : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ALandfallSpyglass();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	/** Looks through it as the player's use does: the reveal (or the quiet look). False when no scene can play. */
	bool Look();

	/** The glass's front end: where the scene's camera looks from, past the tube. */
	FVector GetLens() const;

	/** The barge it looks at (the actor tagged BargeTag), or null. */
	AActor* FindBarge() const;

	/** Turns the tube toward Target. */
	void AimAt(const FVector& Target);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The tube's pivot on top of the legs (it turns toward the barge). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Mount;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Tube;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Eyepiece;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Legs;

	/** What it looks at: Vane's dredge barge over the Sink. */
	UPROPERTY(EditAnywhere, Category = "Spyglass")
	FName BargeTag;

	UPROPERTY(EditAnywhere, Category = "Spyglass")
	FText LookPrompt;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
