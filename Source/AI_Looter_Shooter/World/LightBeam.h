#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;

namespace LightBeams
{
	/**
	 * Turns a static mesh component into a soft vertical light beam standing on its parent's origin (sky beacons, the
	 * pillar over loot): the stylized glow material on the engine's cylinder. No collision, no shadows; fades out toward
	 * the top.
	 */
	AI_LOOTER_SHOOTER_API void Setup(UStaticMeshComponent* Beam, const FLinearColor& Color, float Glow, float Height, float Radius);
}
