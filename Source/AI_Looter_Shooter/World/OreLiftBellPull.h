#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "World/OreLift.h"
#include "OreLiftBellPull.generated.h"

class USphereComponent;

/**
 * A landing's bell pull on the ore lift (AOreLift): the bronze signal bell and its cord on the winch house's landing and
 * on the lift head, the model's own (their SOCKET_BellPull). This is the cord's handle to use; the build script places
 * one on each socket and points it at its lift and landing.
 *  - Closed (before Dead Lift's ride): a tap tugs it, the bell gives a dull clunk, and a caption says why it won't run.
 *  - Open, hold Interact (a second, the round ring): the cage waiting here rides; at the other landing it's called here.
 *  - While the cage is on its way, or ridden, there's nothing to do.
 * The missions hear each use (an Interact event with it), and the lift's own Board.Lift and Lift.Arrived.<End>.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AOreLiftBellPull : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AOreLiftBellPull();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	/** Pulls it as the player would (a tap, or a hold run to its end); True when something happened. */
	bool Pull(AActor* User, bool bHeld);

	/** The words a pull would say now (the hold's or the tap's), for logs and tests. */
	FText GetPromptNow() const;

	/** Rings this landing's bell (a pull's signal, the answer to a call from the other landing, the cage's arrival). */
	void Ring(FName Cue);

	/** The cord's handle: what the crosshair finds, a little in front of the rope. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> Handle;

	/** Its lift, and the landing it's at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lift")
	TObjectPtr<AOreLift> Lift;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lift")
	EOreLiftEnd End = EOreLiftEnd::Bottom;

	/** What a hold does: ride up (the bottom's), ride down (the top's), call the cage from the other landing; and the tap
	 *  on a closed lift's cord. */
	UPROPERTY(EditAnywhere, Category = "Lift|Prompt")
	FText RideUpPrompt;

	UPROPERTY(EditAnywhere, Category = "Lift|Prompt")
	FText RideDownPrompt;

	UPROPERTY(EditAnywhere, Category = "Lift|Prompt")
	FText CallPrompt;

	UPROPERTY(EditAnywhere, Category = "Lift|Prompt")
	FText ClosedPrompt;

	/** How long Interact is held to ride or call (a second, as every hold). */
	UPROPERTY(EditAnywhere, Category = "Lift|Prompt", meta = (ClampMin = "0.05"))
	float HoldSeconds = 1.f;

	/** How far from the player's eyes it can be used (cm); 0: the player's reach. */
	UPROPERTY(EditAnywhere, Category = "Lift|Prompt", meta = (ClampMin = "0"))
	float Reach = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
