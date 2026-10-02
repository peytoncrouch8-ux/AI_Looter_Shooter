#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractionTypes.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class UInteractionComponent;

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
 * component's forgiving cone finds it; one that forgot is still found when the crosshair is right on it.
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
};
