#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "SpeakerPoint.generated.h"

class UArrowComponent;
class USpeakerPointComponent;
class UStaticMeshComponent;

/**
 * Someone talking through a door or a window without coming out (Grandma Delia won't open her screen door to a corpse;
 * Tilly talks through her shop window; Father Aldana through the vestry door). Place it on the door's face, its arrow
 * pointing out to where the player stands. The building's own mesh is usually the door, so it has no mesh of its own;
 * Mesh can hold a separate leaf or shutter, which never opens. Its speaker point does the talking (who, what, the Talk
 * event); tag the actor (Speaker_Delia) for missions to find it.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASpeakerPoint : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASpeakerPoint();

	// --- IInteractable: talking here is the speaker point's ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** A door leaf or shutter of its own when the building has none (none by default); solid, and it never opens. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Who talks here and what they say; where the player looks to talk (about the middle of the door's face). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpeakerPointComponent> SpeakerPoint;

#if WITH_EDITORONLY_DATA
private:
	/** Out from the door, toward where the player stands to talk (the editor shows it; the game never does). */
	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
