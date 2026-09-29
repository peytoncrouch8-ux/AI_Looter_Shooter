#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BuildCameraPawn.generated.h"

class UCameraComponent;

/** Free-fly camera possessed during Build Mode. Movement is driven by UBuildModeComponent. */
UCLASS(NotBlueprintable)
class AI_LOOTER_SHOOTER_API ABuildCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ABuildCameraPawn();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;
};
