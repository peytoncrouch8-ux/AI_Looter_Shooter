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

	/**
	 * A beam guttering like a dying flame (a cursed gun's loot beam), at Time seconds: it burns low and wavers, and every
	 * few seconds sputters nearly out and catches again, shrinking and narrowing as it dims. Glow, Height and Radius are
	 * its steady numbers (as given to Setup, whose material instance of its own it changes); call it every frame it shows.
	 */
	AI_LOOTER_SHOOTER_API void Gutter(UStaticMeshComponent* Beam, float Time, float Glow, float Height, float Radius);

	/** How strongly a guttering beam burns at Time: about 0.1 (sputtering, nearly out) to 1. */
	AI_LOOTER_SHOOTER_API float GutterStrength(float Time);

	/** A beam's flare as its loot lands, as multiples of its steady numbers: all 1 once it's over (FlareSeconds). */
	struct FBeamFlare
	{
		float Lift = 1.f;
		float Flash = 1.f;
		float Width = 1.f;
	};

	/** How long a landing's flare lasts. */
	inline constexpr float FlareSeconds = 1.1f;

	/**
	 * The flare Time seconds after the loot landed: the beam shoots up from a third of its height past its full one
	 * (1.3x, a Grand one 1.5x) in a fifth of a second and settles back, flashing bright (3.5x, Grand 5x) and wide as it
	 * goes, the flash dying away quickly. bGrand: a Legendary's.
	 */
	AI_LOOTER_SHOOTER_API FBeamFlare FlareAt(float Time, bool bGrand);

	/**
	 * A beam flaring as its loot lands (a Rare or better gun dropped by a kill or a chest), at Time seconds: Glow, Height
	 * and Radius are its steady numbers (as given to Setup, whose material instance of its own it changes). Call it every
	 * frame until FlareSeconds, and once at or past it to leave the beam as it was.
	 */
	AI_LOOTER_SHOOTER_API void Flare(UStaticMeshComponent* Beam, float Time, bool bGrand, float Glow, float Height, float Radius);
}
