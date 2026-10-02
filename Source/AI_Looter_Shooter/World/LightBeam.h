#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UObject;
class UStaticMeshComponent;

namespace LightBeams
{
	/**
	 * Turns a static mesh component into a soft vertical light beam standing on its parent's origin (sky beacons, the
	 * pillar over loot): the stylized glow material on the engine's cylinder. No collision, no shadows; fades out toward
	 * the top. The beam gets a material instance of its own (made inside the component).
	 */
	AI_LOOTER_SHOOTER_API void Setup(UStaticMeshComponent* Beam, const FLinearColor& Color, float Glow, float Height, float Radius);

	/**
	 * The beam's material on its own, for beams that all look the same (every ammo drop's): make it once and hand it to
	 * the Setup below, so a field of them shares one instance rather than each making its own object and uniform buffer.
	 * Null if the glow material is missing.
	 */
	AI_LOOTER_SHOOTER_API UMaterialInstanceDynamic* CreateMaterial(UObject* Outer, const FLinearColor& Color, float Glow, float Height);

	/** Setup with a material from CreateMaterial, made for the same Height (the glow fades over it). */
	AI_LOOTER_SHOOTER_API void Setup(UStaticMeshComponent* Beam, UMaterialInterface* Material, float Height, float Radius);
}
