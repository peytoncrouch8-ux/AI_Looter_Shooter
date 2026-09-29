#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LayoutPlaceable.generated.h"

UINTERFACE(MinimalAPI)
class ULayoutPlaceable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Layout objects that move on their own (creatures) implement this so saving the layout records where
 * they belong, not wherever they happened to wander to.
 */
class AI_LOOTER_SHOOTER_API ILayoutPlaceable
{
	GENERATED_BODY()

public:
	/** Where this object should be placed when the layout is loaded (ground point, facing, scale). */
	virtual FTransform GetLayoutTransform() const = 0;
};
