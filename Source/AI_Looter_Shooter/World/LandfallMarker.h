#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LandfallMarker.generated.h"

class USceneComponent;
class UWorld;

/**
 * A tiny tagged point for a Reach objective that has to move with something (Main 1 step 0, "Go to the Great Lantern at
 * the prow": tag Lamplighter_Prow, radius 350). The Lamplighter spawns it on her prow and carries it; a Reach objective
 * looks its tag up every update, so it follows her across the sky. Nothing to see, nothing to hit.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALandfallMarker : public AActor
{
	GENERATED_BODY()

public:
	ALandfallMarker();

	/** Spawns one tagged Tag at Where, fixed to Carrier when given (it goes wherever that goes). */
	static ALandfallMarker* SpawnTagged(UWorld* World, FName Tag, const FVector& Where, USceneComponent* Carrier = nullptr);
};
