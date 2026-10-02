#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractionTypes.h"
#include "UObject/Interface.h"
#include "InteractionSource.generated.h"

class AActor;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UInteractionSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * A component on the player that offers things to use whose rules are its own rather than the things': the weapon
 * manager offers the loot guns lying in the world, since what a tap and a hold do to loot depends on the player's slots
 * and backpack. The interaction component asks each source on its owner for what it offers as it looks around, focuses
 * those like any interactable, and hands a use back to the source that offered it.
 */
class AI_LOOTER_SHOOTER_API IInteractionSource
{
	GENERATED_BODY()

public:
	/**
	 * Adds what it offers near the player (in reach of View.ReachOrigin): each one's actor, point, options and reach. The
	 * interaction component fills in Source.
	 */
	virtual void GatherInteractions(const FInteractionView& View, TArray<FInteractionCandidate>& OutCandidates) const = 0;

	/** How Offer (one it offered) can be used now. */
	virtual FInteractionOptions GetOfferOptions(const AActor& Offer) const = 0;

	/** Uses Offer: a tap, or a hold that ran its full time (bHeld). True when something happened. */
	virtual bool UseOffer(AActor& Offer, bool bHeld) = 0;
};
