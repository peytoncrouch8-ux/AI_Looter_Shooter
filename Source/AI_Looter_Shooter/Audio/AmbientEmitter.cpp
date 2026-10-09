#include "Audio/AmbientEmitter.h"
#include "Audio/AmbientEmitterComponent.h"

const FName AAmbientEmitter::AmbienceTag(TEXT("Ambience"));

AAmbientEmitter::AAmbientEmitter()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	Emitter = CreateDefaultSubobject<UAmbientEmitterComponent>(TEXT("Emitter"));
	RootComponent = Emitter;
	Tags.Add(AmbienceTag);
}
