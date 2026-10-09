#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmbientEmitter.generated.h"

class UAmbientEmitterComponent;

/**
 * A place's sound standing in the level on its own: a creek's line, a pond's shore, a waterfall, the Rim, the Sink,
 * Main Street. Tools/Unreal/build_area_sound.py places them from the layout (their cues, paths and reach); the actor is
 * only a holder for its UAmbientEmitterComponent, which does the work. It has no body, no collision and never ticks
 * itself. Tagged Ambience, so scripts find the ones they placed.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AAmbientEmitter : public AActor
{
	GENERATED_BODY()

public:
	/** The tag every placed emitter carries. */
	static const FName AmbienceTag;

	AAmbientEmitter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ambience")
	TObjectPtr<UAmbientEmitterComponent> Emitter;
};
