#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractionTypes.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class UInteractionComponent;
class UPrimitiveComponent;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Something in the level the player uses with the Interact key: a door or window, a headboard, the bell, a lantern post,
 * a hay bale, a poster, the skiff's gangplank, a station board (AInteractableProp is the generic one). It says how it
 * can be used right now (its prompt words, a tap, a hold and how long, whether it can be used at all) and what happens
 * when it is. The player's UInteractionComponent does the rest: what's looked at, the key's tap and hold, the HUD's
 * prompt and the mission event.
 *
 * An actor implementing it joins UInteractionSubsystem as its play begins and leaves as its play ends, so the
 * component's cone finds it (a little help up close, narrowing with distance); one that forgot is still found when the
 * crosshair is right on its part to use.
 */
class AI_LOOTER_SHOOTER_API IInteractable
{
	GENERATED_BODY()

public:
	/** How User can use it now. */
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const = 0;

	/**
	 * It's used: a tap, or a hold that ran its full time (bHeld). True when something happened; only then do missions
	 * hear of it (the component tells them, once per use).
	 */
	virtual bool Interact(UInteractionComponent& User, bool bHeld) = 0;

	/** The point the player looks at to use it (the middle of a door's leaf); unset: the actor's location. */
	virtual TOptional<FVector> GetInteractionLocation() const { return TOptional<FVector>(); }

	/**
	 * The part of it the player may look at anywhere to use it (FInteractionBox: the bell's whole rope, a lantern with its
	 * web, coins with room round them): the cone and the sight are judged by the box's point nearest the crosshair's line,
	 * and the crosshair through the box is straight at it. Unset: just its point (GetInteractionLocation).
	 */
	virtual TOptional<FInteractionBox> GetInteractionBox() const { return TOptional<FInteractionBox>(); }

	/**
	 * Part (one of its own components, or one on an actor fixed to it) is the thing to use: the crosshair on it offers it,
	 * and it never hides its own point from the eyes. False for what it only stands in or carries (the depot's walls round a
	 * station's board, the jetty's deck and the moored skiff's hull round the gangplank): those are looked at as any wall
	 * is, so nothing is offered from beside it, behind it or through it. Every part counts unless it says otherwise.
	 */
	virtual bool IsInteractionPart(const UPrimitiveComponent* Part) const { return true; }

	/**
	 * Used for good, and the world keeps it so (a wanted poster torn down, a hay bale loaded): lasting interact objectives
	 * (UMissionLastingInteractObjective) count it whenever they look, so their progress survives a session's reload.
	 */
	virtual bool IsUsedUp() const { return false; }
};
