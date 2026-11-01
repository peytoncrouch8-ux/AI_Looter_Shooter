#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "LandfallWinch.generated.h"

class ALamplighterShip;
class UAudioComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * The anchor winch on the Mooring Ledge (Main 1 step 4, "Drop the anchor": Interact held ~2.5 s, tag Landfall_AnchorWinch).
 * Down hard on the ledge, the Lamplighter groans and shifts toward the drop now and then: without her anchor she'd slide
 * off into the canyon. Holding Interact on the winch cranks it (its handle turning, the crank's ratchet); at the hold's end
 * the anchor chain runs out and snaps taut to her hull with a clank, and she's held (ALamplighterShip::SetAnchored).
 * Placed on the ledge by WORLD's build script (or spawned by the ship beside her gangway when the level has none); it
 * finds the ship itself. Stand-in look: a timber block, an iron drum and a crank arm built from engine shapes, the chain a
 * stretched iron cylinder (paths in the .cpp; the Tripo anchor winch replaces them).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALandfallWinch : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ALandfallWinch();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	virtual void Tick(float DeltaSeconds) override;

	/** The ship it moors (the level's Lamplighter; she tells it as she lands). Shows her chain if she's anchored already. */
	void SetShip(ALamplighterShip* InShip);

	/** Winds it as the player's hold does: the chain out, the ship held. False unless she's moored and not yet anchored. */
	bool Wind();

	/** It can be wound now: her down on the ledge, not anchored. */
	bool CanWind() const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The timber block it's bolted to (what the player bumps into). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Drum;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Crank;

	/** The anchor chain to her hull, shown once she's anchored. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Chain;

	/** How long Interact is held to wind it (seconds). */
	UPROPERTY(EditAnywhere, Category = "Winch", meta = (ClampMin = "0.1"))
	float HoldSeconds = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Winch")
	FText WindPrompt;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The player is holding Interact on it now (the crank turns and ratchets while they do). */
	bool IsBeingWound() const;

	/** Stretches the chain from the drum to her hull, or hides it. */
	void PlaceChain(bool bShown);

	TWeakObjectPtr<ALamplighterShip> Ship;
	TWeakObjectPtr<UAudioComponent> CrankSound;
	float CrankAngle = 0.f;
	bool bChainShown = false;
};
